/*
 * Copyright (c) 2015-2021, Pelion and affiliates.
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

/*
 * \file protocol_6lowpan_bootstrap.c
 *
 */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "common/rand.h"
#include "common/log_legacy.h"
#include "common/endian.h"
#include "service_libs/mac_neighbor_table/mac_neighbor_table.h"
#include "service_libs/blacklist/blacklist.h"
#include "service_libs/etx/etx.h"
#include "common/events_scheduler.h"

#include "nwk_interface/protocol.h"
#include "common_protocols/icmpv6.h"
#include "core/net_interface.h"
#include "6lowpan/lowpan_adaptation_interface.h"
#include "6lowpan/mac/mac_helper.h"
#include "6lowpan/nd/nd_router_object.h"
#include "6lowpan/bootstraps/protocol_6lowpan.h"
#include "6lowpan/bootstraps/protocol_6lowpan_interface.h"

#include "6lowpan/bootstraps/protocol_6lowpan_bootstrap.h"

/* Fixed-point randomisation limits for randlib_randomise_base() - RFC 3315
 * says RAND is uniformly distributed between -0.1 and +0.1
 */
#define LOWPAN_RAND_LOW   0x7333 // 1 - 0.1; minimum for "1+RAND"
#define LOWPAN_RAND_HIGH  0x8CCD // 1 + 0.1; maximum for "1+RAND"

#define TRACE_GROUP_LOWPAN_BOOT "6Bo"
#define TRACE_GROUP "6Bo"

#define MAX_MC_DIS_COUNT 3

uint8_t *protocol_6lowpan_nd_border_router_address_get()
{
    return 0;
}

uint8_t protocol_6lowpan_rf_link_scalability_from_lqi(uint8_t lqi)
{
    uint8_t i = 16;
    if (lqi >= 240) {
        i = 1;
    } else {
        lqi /= 16;
        if (lqi) {
            i = (16 - lqi);
        }
    }
    return i;
}

bool lowpan_neighbour_data_clean(int8_t interface_id, const uint8_t *link_local_address)
{

    struct net_if *cur = protocol_stack_interface_info_get_by_id(interface_id);
    if (!cur) {
        return false;
    }
    mac_neighbor_table_entry_t *neigh_entry = mac_neighbor_entry_get_by_ll64(cur->mac_parameters.mac_neighbor_table, link_local_address, false, NULL);
    if (neigh_entry)
        mac_neighbor_table_neighbor_remove(cur->mac_parameters.mac_neighbor_table, neigh_entry);
    return false;
}

