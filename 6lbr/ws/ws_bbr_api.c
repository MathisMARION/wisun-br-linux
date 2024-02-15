/*
 * Copyright (c) 2018-2021, Pelion and affiliates.
 * Copyright (c) 2021-2023 Silicon Laboratories Inc. (www.silabs.com)
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <fnmatch.h>
#include "app/version.h"
#include "app/wsbr.h"
#include "common/rand.h"
#include "common/bits.h"
#include "common/key_value_storage.h"
#include "common/log_legacy.h"
#include "common/endian.h"
#include "common/events_scheduler.h"
#include "common/sys_queue_extra.h"
#include "common/specs/ip.h"

#include "net/timers.h"
#include "net/protocol.h"
#include "net/ns_buffer.h"
#include "net/ns_address_internal.h"
#include "6lowpan/bootstraps/protocol_6lowpan.h"
#include "6lowpan/lowpan_adaptation_interface.h"
#include "rpl/rpl.h"

#include "6lowpan/mac/mpx_api.h"
#include "ws/ws_bbr_api.h"
#include "ws/ws_llc.h"
#include "ws/ws_config.h"
#include "ws/ws_common.h"
#include "ws/ws_bootstrap.h"
#include "ws/ws_pae_key_storage.h"
#include "ws/ws_pae_controller.h"
#include "ws/ws_bootstrap_6lbr.h"

#include "ws/ws_bbr_api.h"

#define TRACE_GROUP "BBRw"

#define RPL_INSTANCE_ID 1

#define WS_ULA_LIFETIME 24*3600
#define WS_ROUTE_LIFETIME WS_ULA_LIFETIME
#define BBR_CHECK_INTERVAL 60
#define BBR_BACKUP_ULA_DELAY 300

void ws_bbr_nvm_info_read(uint16_t *bsi, uint16_t *pan_id, uint16_t *pan_version, uint16_t *lfn_version)
{
    struct storage_parse_info *info = storage_open_prefix("br-info", "r");
    int ret;

    if (!info)
        return;
    for (;;) {
        ret = storage_parse_line(info);
        if (ret == EOF)
            break;
        if (ret) {
            WARN("%s:%d: invalid line: '%s'", info->filename, info->linenr, info->line);
        } else if (!fnmatch("bsi", info->key, 0)) {
            *bsi = strtoul(info->value, NULL, 0);
        } else if (!fnmatch("pan_id", info->key, 0)) {
            *pan_id = strtoul(info->value, NULL, 0);
        } else if (!fnmatch("pan_version", info->key, 0)) {
            *pan_version = strtoul(info->value, NULL, 0) + PAN_VERSION_STORAGE_READ_INCREMENT;
        } else if (!fnmatch("lfn_version", info->key, 0)) {
            *lfn_version = strtoul(info->value, NULL, 0);
        } else if (!fnmatch("api_version", info->key, 0)) {
            // Ignore for now
        } else {
            WARN("%s:%d: invalid key: '%s'", info->filename, info->linenr, info->line);
        }
    }
    storage_close(info);
}

void ws_bbr_nvm_info_write(uint16_t bsi, uint16_t pan_id, uint16_t pan_version, uint16_t lfn_version)
{
    struct storage_parse_info *info = storage_open_prefix("br-info", "w");

    if (!info)
        return;
    fprintf(info->file, "api_version = %#08x\n", version_daemon_api);
    fprintf(info->file, "# Broadcast Schedule Identifier\n");
    fprintf(info->file, "bsi = %d\n", bsi);
    fprintf(info->file, "pan_id = %#04x\n", pan_id);
    fprintf(info->file, "pan_version = %d\n", pan_version);
    fprintf(info->file, "lfn_version = %d\n", lfn_version);
    storage_close(info);
}

bool ws_bbr_backbone_address_get(struct net_if *cur, uint8_t *address)
{
    const uint8_t *addr;

    addr = addr_select_with_prefix(cur, NULL, 0, SOCKET_IPV6_PREFER_SRC_PUBLIC | SOCKET_IPV6_PREFER_SRC_6LOWPAN_SHORT);
    if (!addr)
        return false;
    memcpy(address, addr, 16);
    return true;
}

void ws_bbr_pan_version_increase(struct net_if *cur)
{
    if (!cur) {
        return;
    }
    tr_debug("Border router version number update");
    // Version number is not periodically increased forcing nodes to check Border router availability using DAO
    cur->ws_info.pan_information.pan_version++;
    // Inconsistent for border router to make information distribute faster
    ws_mngt_async_trickle_reset_pc(cur);
    ws_bbr_nvm_info_write(cur->ws_info.fhss_conf.bsi, cur->ws_info.pan_information.pan_id,
                          cur->ws_info.pan_information.pan_version, cur->ws_info.pan_information.lfn_version);
}

void ws_bbr_lfn_version_increase(struct net_if *cur)
{
    if (!cur) {
        return;
    }
    tr_debug("Border router LFN version number update");
    cur->ws_info.pan_information.lfn_version++;
    // Inconsistent for border router to make information distribute faster
    ws_mngt_async_trickle_reset_pc(cur);

    ws_bbr_nvm_info_write(cur->ws_info.fhss_conf.bsi, cur->ws_info.pan_information.pan_id,
                          cur->ws_info.pan_information.pan_version, cur->ws_info.pan_information.lfn_version);
    //   Wi-SUN FAN 1.1v06 6.3.4.6.3 FFN Discovery / Join
    // A Border Router MUST increment PAN Version (PANVER-IE) [...] when [...]
    // the following occurs:
    // d. A change in LFN Version.
    ws_bbr_pan_version_increase(cur);
}

uint16_t test_pan_size_override = 0xffff;

uint16_t ws_bbr_pan_size(struct net_if *cur)
{
    if (!cur) {
        return 0;
    }

    if (test_pan_size_override != 0xffff) {
        return test_pan_size_override;
    }

    return SLIST_SIZE(&g_ctxt.rpl_root.targets, link);
}

static void ws_bbr_forwarding_cb(struct net_if *interface, buffer_t *buf)
{
    uint8_t traffic_class = buf->options.traffic_class >> IP_TCLASS_DSCP_SHIFT;

    if (traffic_class == IP_DSCP_EF) {
        //indicate EF forwarding to adaptation
        lowpan_adaptation_expedite_forward_enable(interface);
    }
}

void ws_bbr_init(struct net_if *interface)
{
    interface->if_common_forwarding_out_cb = &ws_bbr_forwarding_cb;
}

int ws_bbr_routing_table_get(int8_t interface_id, bbr_route_info_t *table_ptr, uint16_t table_len)
{
    struct rpl_root *root = &g_ctxt.rpl_root;
    struct rpl_transit *transit;
    struct rpl_target *target;
    int cnt = 0;

    SLIST_FOREACH(target, &root->targets, link) {
        transit = rpl_transit_preferred(root, target);
        if (!transit)
            continue;
        memcpy(table_ptr[cnt].target, target->prefix + 8, 8);
        memcpy(table_ptr[cnt].parent, transit->parent + 8, 8);
        cnt++;
        if (cnt >= table_len)
            break;
    }
    return cnt;
}

int ws_bbr_set_mode_switch(int8_t interface_id, int mode, uint8_t phy_mode_id, uint8_t *neighbor_mac_address)
{
    struct net_if *interface = protocol_stack_interface_info_get_by_id(interface_id);
    if (interface == NULL)
        return -1;

    uint8_t all_nodes[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //only for wsbrd-v1.5-rc1

    if (neighbor_mac_address)
        return ws_llc_set_mode_switch(interface, mode, phy_mode_id, neighbor_mac_address);
    return ws_llc_set_mode_switch(interface, mode, phy_mode_id, all_nodes);

}