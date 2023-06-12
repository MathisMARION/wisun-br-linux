/*
 * Copyright (c) 2023 Silicon Laboratories Inc. (www.silabs.com)
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at [1].  This software is distributed to you in
 * Object Code format and/or Source Code format and is governed by the sections
 * of the MSLA applicable to Object Code, Source Code and Modified Open Source
 * Code. By using this software, you agree to the terms of the MSLA.
 *
 * [1]: https://www.silabs.com/about-us/legal/master-software-license-agreement
 */
#ifndef RCP_API_H
#define RCP_API_H
#include <stdint.h>
#include "stack/mac/fhss_ws_extension.h"
#include "stack/mac/platform/arm_hal_phy.h"

struct fhss_ws_configuration;
struct phy_rf_channel_configuration;

void rcp_noop(void);
void rcp_reset(void);
void rcp_allocate_fhss(const struct fhss_ws_configuration *timing_info);
void rcp_register_fhss(void);
void rcp_unregister_fhss(void);
void rcp_release_fhss(void);
void rcp_get_rx_sensitivity(void);
void rcp_set_rf_config(const struct phy_rf_channel_configuration *config);
void rcp_set_rx_on_idle(bool enable);
void rcp_set_802154_mode(phy_802_15_4_mode_e val);
void rcp_set_ack_wait_duration(uint16_t val);
void rcp_set_cca_threshold(uint8_t number_of_channels, uint8_t default_dbm,
                           uint8_t high_limit, uint8_t low_limit);
void rcp_set_max_rf_retry(uint8_t max_cca_failure, uint8_t max_tx_failure,
                          uint16_t blacklist_min_ms, uint16_t blacklist_max_ms);
void rcp_set_max_mac_retry(uint8_t val);
void rcp_set_max_csma_backoffs(uint8_t val);
void rcp_set_min_be(uint8_t val);
void rcp_set_max_be(uint8_t val);
void rcp_set_max_async_duration(uint32_t val);
void rcp_set_tx_power(int8_t val);
void rcp_set_fhss_timings(const struct fhss_ws_configuration *timing_info);
void rcp_set_fhss_parent(const uint8_t parent[8],
                         const struct broadcast_timing_info *timing_info,
                         bool force_synch);
void rcp_set_fhss_neighbor(const uint8_t neigh[8],
                           const struct fhss_ws_neighbor_timing_info *timing_info);
void rcp_drop_fhss_neighbor(const uint8_t eui64[8]);
void rcp_set_fhss_hop_count(int hop_count);
void rcp_set_coordinator_mac64(uint8_t val[8]);
void rcp_set_coordinator_mac16(uint16_t val);
void rcp_set_tx_allowance_level(fhss_ws_tx_allow_level_e normal,
                                fhss_ws_tx_allow_level_e expedited_forwarding);
void rcp_set_security(bool enable);
void rcp_set_frame_counter_per_key(bool enable);
void rcp_set_frame_counter(int slot, uint32_t val);
void rcp_set_key(uint8_t slot, const uint8_t *lookup_data, const uint8_t *key);
void rcp_set_default_key_source(const uint8_t lookup_data[8]);
void rcp_set_neighbor(uint8_t slot, uint16_t panid, uint16_t mac16,
                      uint8_t *mac64, uint32_t frame_counter);

#endif
