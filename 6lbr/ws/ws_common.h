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

#ifndef WS_COMMON_H_
#define WS_COMMON_H_
#include <stdint.h>
#include <stdbool.h>

#include "common/specs/ws.h"
#include "common/ns_list.h"

#include "ws/ws_config.h"
#include "ws/ws_neigh.h"
#include "ws/ws_mngt.h"
#include "ws/ws_common_defines.h"
#include "ws/ws_ie_custom.h"

#define NO_PENDING_PROCESS 0
#define PENDING_KEY_INDEX_ADVERTISMENT 1
#define PENDING_KEY_INDEX_ACTIVATE 2

typedef struct ws_pending_key_index {
    unsigned state: 2;
    uint8_t index;
} ws_pending_key_index_t;

/**
 * \brief Struct fhss_ws_configuration defines configuration of WS FHSS.
 */
typedef struct fhss_ws_configuration {
    /** WS unicast channel function. */
    enum ws_channel_functions ws_uc_channel_function;
    /** WS broadcast channel function. */
    enum ws_channel_functions ws_bc_channel_function;
    /** Broadcast schedule identifier. */
    int bsi;
    /** Unicast dwell interval. Range: 15-250 milliseconds. */
    uint8_t fhss_uc_dwell_interval;
    /** Broadcast interval. Duration between broadcast dwell intervals. Range: 0-16777216 milliseconds. */
    uint32_t fhss_broadcast_interval;
    uint32_t lfn_bc_interval;
    uint8_t lfn_bc_sync_period;
    /** Broadcast dwell interval. Range: 15-250 milliseconds. */
    uint8_t fhss_bc_dwell_interval;
    /** Domain channel mask, Wi-SUN uses it to exclure channels on US-IE and BS-IE. */
    uint8_t domain_channel_mask[32];
    /** Wi-SUN specific unicast channel mask */
    uint8_t unicast_channel_mask[32];
    /** Wi-SUN specific broadcast channel mask */
    uint8_t broadcast_channel_mask[32];
    uint32_t async_tx_duration_ms;
    uint8_t regulatory_domain;
    uint8_t operating_class;
    uint8_t channel_plan_id;
    uint8_t number_of_channels;
    uint32_t channel_spacing;
    uint24_t ch0_freq;
    uint8_t channel_plan;
    int regulation;
} fhss_ws_configuration_t;


typedef struct ws_info {
    char network_name[33];
    struct ws_mngt mngt;
    struct ws_ie_custom_list ie_custom_list;
    bool enable_lfn;
    bool enable_ffn10;
    unsigned int key_index_mask;  // Bitmask of installed key indices
    ws_pending_key_index_t pending_key_index_info;
    struct ws_pan_information pan_information;
    ws_hopping_schedule_t hopping_schedule;
    struct ws_neigh_table neighbor_storage;
    // FIXME: fhss_conf is redundant with hopping_schedule
    struct fhss_ws_configuration fhss_conf;
} ws_info_t;


int8_t ws_common_generate_channel_list(const struct fhss_ws_configuration *fhss_config, uint8_t *channel_mask, uint16_t number_of_channels, uint8_t regulatory_domain, uint8_t operating_class, uint8_t channel_plan_id);

void ws_common_calc_chan_excl(ws_excluded_channel_data_t *chan_excl, const uint8_t chan_mask_custom[],
                              const uint8_t chan_mask_reg[], uint16_t chan_count);

void ws_common_seconds_timer(int seconds);

uint8_t ws_common_allow_child_registration(struct net_if *cur, const uint8_t *eui64, uint16_t aro_timeout);

bool ws_common_negative_aro_mark(struct net_if *interface, const uint8_t *eui64);

uint32_t ws_common_version_timeout_get(uint8_t config);

uint32_t ws_common_latency_estimate_get(struct net_if *cur);

uint32_t ws_common_datarate_get_from_phy_mode(uint8_t phy_mode_id, uint8_t operating_mode);

uint32_t ws_common_datarate_get(struct net_if *cur);

bool ws_common_is_valid_nr(uint8_t node_role);

float ws_common_rsl_calc(float rsl_dbm, int rx_power_dbm);

int ws_common_get_fixed_channel(const uint8_t bitmask[32]);

#endif //WS_COMMON_H_
