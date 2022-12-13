/*
 * Copyright (c) 2018-2020, Pelion and affiliates.
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

#include <string.h>
#include <stdint.h>
#include "common/bits.h"
#include "common/endian.h"
#include "common/string_extra.h"
#include "common/log_legacy.h"
#include "common/ns_list.h"
#include "stack/mac/mac_common_defines.h"
#include "stack/ws_management_api.h"

#include "6lowpan/mac/mac_ie_lib.h"
#include "6lowpan/ws/ws_common_defines.h"

#include "6lowpan/ws/ws_ie_lib.h"

// Figure 6-42 Node Role IE Format
#define WS_WH_NR_IE_NODE_ROLE_ID_MASK 0b00000111

// Figure 6-50 Unicast Schedule IE
// Figure 6-51 Broadcast Schedule IE
// Figure 6-66 LFN Channel Information Fields
#define WS_WP_SCHEDULE_IE_CHAN_PLAN_MASK     0b00000111
#define WS_WP_SCHEDULE_IE_CHAN_FUNC_MASK     0b00111000
#define WS_WP_SCHEDULE_IE_EXCL_CHAN_CTL_MASK 0b11000000

// Figure 6-58 PAN IE
#define WS_WP_PAN_IE_USE_PARENT_BS_IE_MASK 0b00000001
#define WS_WP_PAN_IE_ROUTING_METHOD_MASK   0b00000010
#define WS_WP_PAN_IE_LFN_WINDOW_STYLE_MASK 0b00000100
#define WS_WP_PAN_IE_FAN_TPS_VERSION_MASK  0b11100000

// Figure 6-62 Capability IE
#define WS_WP_POM_IE_PHY_OP_MODE_NUMBER_MASK 0b00001111
#define WS_WP_POM_IE_MDR_CAPABLE_MASK        0b00010000

// Figure 6-68 LFN GTK Hash IE
#define WS_WP_LGTKHASH_IE_INCLUDE_LGTK0_MASK 0b00000001
#define WS_WP_LGTKHASH_IE_INCLUDE_LGTK1_MASK 0b00000010
#define WS_WP_LGTKHASH_IE_INCLUDE_LGTK2_MASK 0b00000100
#define WS_WP_LGTKHASH_IE_ACTIVE_INDEX_MASK  0b00011000

static uint8_t *ws_wh_header_base_write(uint8_t *ptr, uint16_t length, uint8_t type)
{
    ptr = mac_ie_header_base_write(ptr, MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID, length + 1);
    *ptr++ = type;
    return ptr;
}

static uint16_t ws_channel_plan_length(uint8_t channel_plan)
{
    switch (channel_plan) {
        case 0:
            //Regulator domain and operationg class inline
            return 2;
        case 1:
            //CHo, Channel spasing and number of channel's inline
            return 6;
        case 2:
            //Regulator domain and channel plan ID inline
            return 2;

        default:
            return 0;
    }
}

static uint16_t ws_channel_function_length(uint8_t channel_function, uint16_t hop_channel_count)
{
    switch (channel_function) {
        case 0:
            //Fixed channel inline
            return 2;
        case 1:
        case 2:
            return 0;
        case 3:
            //Hop count + channel hop list
            return (1 + hop_channel_count);
        default:
            return 0;

    }
}

static uint16_t ws_excluded_channel_length(ws_generic_channel_info_t *generic_channel_info)
{
    uint16_t length;
    if (generic_channel_info->excluded_channel_ctrl == WS_EXC_CHAN_CTRL_RANGE) {
        length = (generic_channel_info->excluded_channels.range_out.excluded_range_length * 4) + 1;
    } else if (generic_channel_info->excluded_channel_ctrl == WS_EXC_CHAN_CTRL_BITMASK) {
        length = generic_channel_info->excluded_channels.mask_out.channel_mask_bytes_inline;
    } else {
        length = 0;
    }
    return length;
}

static void ws_generic_channel_info_init(struct ws_hopping_schedule *hopping_schedule, ws_generic_channel_info_t *generic_channel_info, bool unicast_schedule)
{
    generic_channel_info->channel_plan = hopping_schedule->channel_plan;
    if (unicast_schedule) {
        generic_channel_info->channel_function = hopping_schedule->uc_channel_function;
        generic_channel_info->excluded_channel_ctrl = hopping_schedule->uc_excluded_channels.excluded_channel_ctrl;
        if (generic_channel_info->excluded_channel_ctrl == WS_EXC_CHAN_CTRL_RANGE) {
            generic_channel_info->excluded_channels.range_out.excluded_range_length = hopping_schedule->uc_excluded_channels.excluded_range_length;
            generic_channel_info->excluded_channels.range_out.excluded_range = hopping_schedule->uc_excluded_channels.excluded_range;
        } else if (generic_channel_info->excluded_channel_ctrl == WS_EXC_CHAN_CTRL_BITMASK) {
            generic_channel_info->excluded_channels.mask_out.channel_mask_bytes_inline = hopping_schedule->uc_excluded_channels.channel_mask_bytes_inline;
            generic_channel_info->excluded_channels.mask_out.excluded_channel_count = hopping_schedule->uc_excluded_channels.excluded_channel_count;
            generic_channel_info->excluded_channels.mask_out.channel_mask = hopping_schedule->uc_excluded_channels.channel_mask;
        }
    } else {
        generic_channel_info->channel_function = hopping_schedule->bc_channel_function;
        generic_channel_info->excluded_channel_ctrl = hopping_schedule->bc_excluded_channels.excluded_channel_ctrl;
        if (generic_channel_info->excluded_channel_ctrl == WS_EXC_CHAN_CTRL_RANGE) {
            generic_channel_info->excluded_channels.range_out.excluded_range_length = hopping_schedule->bc_excluded_channels.excluded_range_length;
            generic_channel_info->excluded_channels.range_out.excluded_range = hopping_schedule->bc_excluded_channels.excluded_range;
        } else if (generic_channel_info->excluded_channel_ctrl == WS_EXC_CHAN_CTRL_BITMASK) {
            generic_channel_info->excluded_channels.mask_out.channel_mask_bytes_inline = hopping_schedule->bc_excluded_channels.channel_mask_bytes_inline;
            generic_channel_info->excluded_channels.mask_out.excluded_channel_count = hopping_schedule->bc_excluded_channels.excluded_channel_count;
            generic_channel_info->excluded_channels.mask_out.channel_mask = hopping_schedule->bc_excluded_channels.channel_mask;
        }
    }
}

static void ws_wp_channel_plan_set(ws_generic_channel_info_t *generic_channel_info, struct ws_hopping_schedule *hopping_schedule)
{
    switch (generic_channel_info->channel_plan) {
        case 0:
            //Regulator domain and operationg class inline
            generic_channel_info->plan.zero.regulatory_domain = hopping_schedule->regulatory_domain;
            generic_channel_info->plan.zero.operating_class = hopping_schedule->operating_class;
            break;
        case 1:
            //CHo, Channel spasing and number of channel's inline
            generic_channel_info->plan.one.ch0 = hopping_schedule->ch0_freq / 1000;
            generic_channel_info->plan.one.channel_spacing = hopping_schedule->channel_spacing;
            generic_channel_info->plan.one.number_of_channel = hopping_schedule->number_of_channels;
            break;
        case 2:
            generic_channel_info->plan.two.regulatory_domain = hopping_schedule->regulatory_domain;
            generic_channel_info->plan.two.channel_plan_id = hopping_schedule->channel_plan_id;
            break;
        default:
            break;
    }
}

static void ws_wp_channel_function_set(ws_generic_channel_info_t *generic_channel_info, struct ws_hopping_schedule *hopping_schedule, bool unicast_schedule)
{
    switch (generic_channel_info->channel_function) {
        case 0:
            //Fixed channel inline
            if (unicast_schedule) {
                generic_channel_info->function.zero.fixed_channel = hopping_schedule->uc_fixed_channel;
            } else {
                generic_channel_info->function.zero.fixed_channel = hopping_schedule->bc_fixed_channel;
            }
            break;
        case 1:
        case 2:
            //No Inline
            break;
        case 3:
            //TODO add list to possible to set
            //Force 1 channel 0
            generic_channel_info->function.three.channel_hop_count = 1;
            generic_channel_info->function.three.channel_list = NULL;
            break;
        default:
            break;

    }
}

static uint16_t ws_wp_generic_schedule_length_get(ws_generic_channel_info_t *generic_channel_info)
{
    uint16_t length = 1;

    length += ws_channel_plan_length(generic_channel_info->channel_plan);
    uint16_t number_of_channels = 1;
    if (generic_channel_info->channel_plan == 3) {
        number_of_channels = generic_channel_info->function.three.channel_hop_count;
    } else {
        number_of_channels = 1;
    }
    length += ws_channel_function_length(generic_channel_info->channel_function, number_of_channels);

    length += ws_excluded_channel_length(generic_channel_info);

    return length;
}

uint16_t ws_wp_nested_hopping_schedule_length(struct ws_hopping_schedule *hopping_schedule, bool unicast_schedule)
{
    ws_generic_channel_info_t generic_channel_info;

    ws_generic_channel_info_init(hopping_schedule, &generic_channel_info, unicast_schedule);
    ws_wp_channel_function_set(&generic_channel_info, hopping_schedule, unicast_schedule);
    uint16_t length;
    if (unicast_schedule) {
        length = 3;
    } else {
        length = 9;
    }
    length += ws_wp_generic_schedule_length_get(&generic_channel_info);
    return length;
}

uint8_t *ws_wh_utt_write(uint8_t *ptr, uint8_t message_type)
{
    ptr = ws_wh_header_base_write(ptr, 4, WH_IE_UTT_TYPE);
    *ptr++ = message_type;
    memset(ptr, 0, 3);
    ptr += 3;
    return ptr;
}

uint8_t *ws_wh_bt_write(uint8_t *ptr)
{
    ptr = ws_wh_header_base_write(ptr, 5, WH_IE_BT_TYPE);
    memset(ptr, 0, 5);
    ptr += 5;
    return ptr;
}


uint8_t *ws_wh_fc_write(uint8_t *ptr, ws_fc_ie_t *fc_ie)
{
    ptr = ws_wh_header_base_write(ptr, 2, WH_IE_FC_TYPE);
    *ptr++ = fc_ie->tx_flow_ctrl;
    *ptr++ = fc_ie->rx_flow_ctrl;
    return ptr;
}

uint8_t *ws_wh_rsl_write(uint8_t *ptr, uint8_t rsl)
{
    ptr = ws_wh_header_base_write(ptr, 1, WH_IE_RSL_TYPE);
    *ptr++ = rsl;
    return ptr;
}

uint8_t *ws_wh_ea_write(uint8_t *ptr, uint8_t *eui64)
{
    ptr = ws_wh_header_base_write(ptr, 8, WH_IE_EA_TYPE);
    memcpy(ptr, eui64, 8);
    ptr += 8;
    return ptr;
}

uint8_t *ws_wh_vh_write(uint8_t *ptr, uint8_t *vendor_header, uint8_t vendor_header_length)
{
    ptr = ws_wh_header_base_write(ptr, vendor_header_length, WH_IE_VH_TYPE);
    if (vendor_header_length) {
        memcpy(ptr, vendor_header, vendor_header_length);
        ptr += vendor_header_length;
    }
    return ptr;
}

uint8_t *ws_wh_lutt_write(uint8_t *ptr, uint8_t message_type)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_lutt_length(), WH_IE_LUTT_TYPE);
    *ptr++ = message_type;
    memset(ptr, 0, 2); /* Unicast Slot Number 2 bytes */
    ptr += 2;
    memset(ptr, 0, 3); /* UFSI 3 bytes */
    ptr += 3;
    return ptr;
}

uint8_t *ws_wh_lus_write(uint8_t *ptr, struct ws_lus_ie *lus_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_lus_length(), WH_IE_LUS_TYPE);
    ptr = write_le24(ptr, lus_ie->listen_interval);
    *ptr++ = lus_ie->channel_plan_tag;
    return ptr;
}

uint8_t *ws_wh_flus_write(uint8_t *ptr, struct ws_flus_ie *flus_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_flus_length(), WH_IE_FLUS_TYPE);
    *ptr++ = flus_ie->dwell_interval;
    *ptr++ = flus_ie->channel_plan_tag;
    return ptr;
}

uint8_t *ws_wh_lbt_write(uint8_t *ptr, struct ws_lbt_ie *lbt_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_lbt_length(), WH_IE_LBT_TYPE);
    memset(ptr, 0, 2); /* LFN Broadcast Slot Number 2 bytes */
    ptr += 2;
    memset(ptr, 0, 3); /* LFN Broadcast Interval Offset 3 bytes */
    ptr += 3;
    return ptr;

}

uint8_t *ws_wh_lbs_write(uint8_t *ptr, struct ws_lbs_ie *lbs_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_lbs_length(), WH_IE_LBS_TYPE);
    ptr = write_le24(ptr, lbs_ie->broadcast_interval);
    ptr = write_le16(ptr, lbs_ie->broadcast_scheduler_id);
    *ptr++ = lbs_ie->channel_plan_tag;
    *ptr++ = lbs_ie->broadcast_sync_period;
    return ptr;
}

uint8_t *ws_wh_lbc_write(uint8_t *ptr, struct ws_lbc_ie *lbc_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_lbc_length(), WH_IE_LBC_TYPE);
    ptr = write_le24(ptr, lbc_ie->lfn_broadcast_interval);
    *ptr++ = lbc_ie->broadcast_sync_period;
    return ptr;
}

uint16_t ws_wh_nr_length(struct ws_nr_ie *nr_ie)
{
    uint16_t length;
    if (nr_ie->node_role == WS_NR_ROLE_LFN) {
        length = 9;
    } else {
        length = 3;
    }
    return length;
}

uint8_t *ws_wh_nr_write(uint8_t *ptr, struct ws_nr_ie *nr_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_nr_length(nr_ie), WH_IE_NR_TYPE);
    *ptr++ = FIELD_PREP(WS_WH_NR_IE_NODE_ROLE_ID_MASK, nr_ie->node_role);
    *ptr++ = nr_ie->clock_drift;
    *ptr++ = nr_ie->timing_accuracy;
    if (nr_ie->node_role == WS_NR_ROLE_LFN) {
        ptr = write_le24(ptr, nr_ie->listen_interval_min);
        ptr = write_le24(ptr, nr_ie->listen_interval_max);
    }
    return ptr;
}

uint8_t *ws_wh_lnd_write(uint8_t *ptr, struct ws_lnd_ie *lnd_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_lnd_length(), WH_IE_LND_TYPE);
    *ptr++ = lnd_ie->response_threshold;
    memset(ptr, 0, 3);  /* Response Delay 3 bytes */
    ptr += 3;
    *ptr++ = lnd_ie->discovery_slot_time;
    *ptr++ = lnd_ie->discovery_slots;
    memset(ptr, 0, 2);  /* Discovery First Slot 2 bytes */
    ptr += 2;
    return ptr;
}

uint8_t *ws_wh_lto_write(uint8_t *ptr, struct ws_lto_ie *lto_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_lto_length(), WH_IE_LTO_TYPE);
    ptr = write_le24(ptr, lto_ie->offset);
    ptr = write_le24(ptr, lto_ie->adjusted_listening_interval);
    return ptr;
}

uint8_t *ws_wh_panid_write(uint8_t *ptr, struct ws_panid_ie *panid_ie)
{
    ptr = ws_wh_header_base_write(ptr, ws_wh_panid_length(), WH_IE_PANID_TYPE);
    ptr = write_le16(ptr, panid_ie->panid);
    return ptr;
}

uint8_t *ws_wp_base_write(uint8_t *ptr, uint16_t length)
{
    return mac_ie_payload_base_write(ptr, WS_WP_NESTED_IE, length);
}

static uint8_t ws_wp_channel_info_base_get(ws_generic_channel_info_t *generic_channel_info)
{
    uint8_t channel_info_base = 0;
    channel_info_base = generic_channel_info->channel_plan;
    channel_info_base |= (generic_channel_info->channel_function << 3);
    //Set Excluded Channel control part
    channel_info_base |= (generic_channel_info->excluded_channel_ctrl << 6);

    return channel_info_base;
}

static uint8_t *ws_wp_channel_plan_write(uint8_t *ptr, ws_generic_channel_info_t *generic_channel_info)
{
    switch (generic_channel_info->channel_plan) {
        case 0:
            //Regulator domain and operationg class inline
            *ptr++ = generic_channel_info->plan.zero.regulatory_domain;
            *ptr++ = generic_channel_info->plan.zero.operating_class;
            break;
        case 1:
            //CHo, Channel spasing and number of channel's inline
            ptr = write_le24(ptr, generic_channel_info->plan.one.ch0);
            *ptr++ = generic_channel_info->plan.one.channel_spacing;
            ptr = write_le16(ptr, generic_channel_info->plan.one.number_of_channel);
            break;
        case 2:
            *ptr++ = generic_channel_info->plan.two.regulatory_domain;
            *ptr++ = generic_channel_info->plan.two.channel_plan_id;
            break;
        default:
            break;
    }
    return ptr;
}

static uint8_t *ws_wp_channel_function_write(uint8_t *ptr, ws_generic_channel_info_t *generic_channel_info)
{
    switch (generic_channel_info->channel_function) {
        case 0:
            //Fixed channel inline
            ptr = write_le16(ptr, generic_channel_info->function.zero.fixed_channel);
            break;
        case 1:
        case 2:
            //No Inline
            break;
        case 3:
            //TODO do this properly
            //Hop count + channel hop list
            if (generic_channel_info->function.three.channel_list && generic_channel_info->function.three.channel_hop_count) {
                *ptr++ = generic_channel_info->function.three.channel_hop_count;
                memcpy(ptr, generic_channel_info->function.three.channel_list, generic_channel_info->function.three.channel_hop_count);
                ptr += generic_channel_info->function.three.channel_hop_count;
            } else {
                *ptr++ = 1;
                *ptr++ = 0;
            }

            break;
        default:
            break;

    }
    return ptr;
}

static uint8_t *ws_wp_nested_excluded_channel_write(uint8_t *ptr, ws_generic_channel_info_t *generic_channel_info)
{
    if (generic_channel_info->excluded_channel_ctrl == WS_EXC_CHAN_CTRL_RANGE) {
        uint8_t range_length = generic_channel_info->excluded_channels.range_out.excluded_range_length;
        ws_excluded_channel_range_data_t *range_ptr = generic_channel_info->excluded_channels.range_out.excluded_range;
        *ptr++ = range_length;
        while (range_length) {
            ptr = write_le16(ptr, range_ptr->range_start);
            ptr = write_le16(ptr, range_ptr->range_end);
            range_length--;
            range_ptr++;
        }
    } else if (generic_channel_info->excluded_channel_ctrl == WS_EXC_CHAN_CTRL_BITMASK) {
        memcpy(ptr, generic_channel_info->excluded_channels.mask_out.channel_mask,
               generic_channel_info->excluded_channels.mask_out.channel_mask_bytes_inline);
        ptr += generic_channel_info->excluded_channels.mask_out.channel_mask_bytes_inline;
    }
    return ptr;
}

uint8_t *ws_wp_nested_hopping_schedule_write(uint8_t *ptr, struct ws_hopping_schedule *hopping_schedule, bool unicast_schedule)
{
    //Calculate length
    ws_generic_channel_info_t generic_channel_info;

    ws_generic_channel_info_init(hopping_schedule, &generic_channel_info, unicast_schedule);
    ws_wp_channel_plan_set(&generic_channel_info, hopping_schedule);
    ws_wp_channel_function_set(&generic_channel_info, hopping_schedule, unicast_schedule);

    uint16_t length;
    if (unicast_schedule) {
        length = 3;
    } else {
        length = 9;
    }
    length += ws_wp_generic_schedule_length_get(&generic_channel_info);

    if (!unicast_schedule) {
        ptr = mac_ie_nested_ie_long_base_write(ptr, WP_PAYLOAD_IE_BS_TYPE, length);
        ptr = write_le32(ptr, hopping_schedule->fhss_broadcast_interval);
        ptr = write_le16(ptr, hopping_schedule->fhss_bsi);
        *ptr++ = hopping_schedule->fhss_bc_dwell_interval;
    } else {
        ptr = mac_ie_nested_ie_long_base_write(ptr, WP_PAYLOAD_IE_US_TYPE, length);
        *ptr++ =  hopping_schedule->fhss_uc_dwell_interval;
    }

    *ptr++ =  hopping_schedule->clock_drift;
    *ptr++ =  hopping_schedule->timing_accuracy;

    // Write a generic part of shedule
    *ptr++ = ws_wp_channel_info_base_get(&generic_channel_info);
    ptr = ws_wp_channel_plan_write(ptr, &generic_channel_info);
    ptr = ws_wp_channel_function_write(ptr, &generic_channel_info);
    ptr = ws_wp_nested_excluded_channel_write(ptr, &generic_channel_info);

    return ptr;
}

uint8_t *ws_wp_nested_vp_write(uint8_t *ptr, uint8_t *vendor_payload, uint16_t vendor_payload_length)
{
    if (vendor_payload_length) {
        ptr = mac_ie_nested_ie_long_base_write(ptr, WP_PAYLOAD_IE_VP_TYPE, vendor_payload_length);
        memcpy(ptr, vendor_payload, vendor_payload_length);
        ptr += vendor_payload_length;
    }
    return ptr;
}

uint8_t *ws_wp_nested_pan_info_write(uint8_t *ptr, struct ws_pan_information *pan_configuration)
{
    bool lfn_window_style = false;
    uint8_t tmp8;

    if (!pan_configuration)
        return mac_ie_nested_ie_short_base_write(ptr, WP_PAYLOAD_IE_PAN_TYPE, 0);
    ptr = mac_ie_nested_ie_short_base_write(ptr, WP_PAYLOAD_IE_PAN_TYPE, 5);
    ptr = write_le16(ptr, pan_configuration->pan_size);
    ptr = write_le16(ptr, pan_configuration->routing_cost);
    if (pan_configuration->version > WS_FAN_VERSION_1_0)
        lfn_window_style = pan_configuration->lfn_window_style;
    tmp8 = 0;
    tmp8 |= FIELD_PREP(WS_WP_PAN_IE_USE_PARENT_BS_IE_MASK, pan_configuration->use_parent_bs);
    tmp8 |= FIELD_PREP(WS_WP_PAN_IE_ROUTING_METHOD_MASK,   pan_configuration->rpl_routing_method);
    tmp8 |= FIELD_PREP(WS_WP_PAN_IE_LFN_WINDOW_STYLE_MASK, lfn_window_style);
    tmp8 |= FIELD_PREP(WS_WP_PAN_IE_FAN_TPS_VERSION_MASK,  pan_configuration->version);
    *ptr++ = tmp8;
    return ptr;
}

uint8_t *ws_wp_nested_netname_write(uint8_t *ptr, uint8_t *network_name, uint8_t network_name_length)
{
    ptr = mac_ie_nested_ie_short_base_write(ptr, WP_PAYLOAD_IE_NETNAME_TYPE, network_name_length);
    if (network_name_length) {
        memcpy(ptr, network_name, network_name_length);
        ptr += network_name_length;
    }
    return ptr;
}

uint8_t *ws_wp_nested_pan_ver_write(uint8_t *ptr, struct ws_pan_information *pan_configuration)
{
    if (!pan_configuration) {
        return ptr;
    }
    ptr = mac_ie_nested_ie_short_base_write(ptr, WP_PAYLOAD_IE_PAN_VER_TYPE, 2);
    ptr = write_le16(ptr, pan_configuration->pan_version);
    return ptr;
}

uint8_t *ws_wp_nested_gtkhash_write(uint8_t *ptr, gtkhash_t *gtkhash, uint8_t gtkhash_length)
{
    ptr = mac_ie_nested_ie_short_base_write(ptr, WP_PAYLOAD_IE_GTKHASH_TYPE, gtkhash_length);
    if (gtkhash_length) {
        memcpy(ptr, gtkhash, 32);
        ptr += 32;
    }
    return ptr;
}

uint16_t ws_wp_nested_pom_length(uint8_t phy_op_mode_number)
{
    return 1 + phy_op_mode_number;
}

uint8_t *ws_wp_nested_pom_write(uint8_t *ptr, uint8_t phy_op_mode_number, uint8_t *phy_operating_modes, uint8_t mdr_command_capable)
{
    uint8_t tmp8;

    if (!phy_op_mode_number)
        return ptr;
    ptr = mac_ie_nested_ie_short_base_write(ptr, WP_PAYLOAD_IE_POM_TYPE,  ws_wp_nested_pom_length(phy_op_mode_number));
    tmp8 = 0;
    tmp8 |= FIELD_PREP(WS_WP_POM_IE_PHY_OP_MODE_NUMBER_MASK, phy_op_mode_number);
    tmp8 |= FIELD_PREP(WS_WP_POM_IE_MDR_CAPABLE_MASK,        mdr_command_capable);
    *ptr++ = tmp8;
    memcpy(ptr, phy_operating_modes, phy_op_mode_number);
    ptr += phy_op_mode_number;
    return ptr;
}

uint8_t *ws_wp_nested_lfn_version_write(uint8_t *ptr, struct ws_lfnver_ie *lfnver_ie)
{
    ptr = mac_ie_nested_ie_short_base_write(ptr, WP_PAYLOAD_IE_LFN_VER_TYPE, ws_wp_nested_lfn_version_length());
    ptr = write_le16(ptr, lfnver_ie->lfn_version);

    return ptr;
}

uint16_t ws_wp_nested_lgtkhash_length(gtkhash_t *lgtkhash)
{
    uint16_t length = 1;
    int i;

    for (i = 0; i < 3; i++)
        if (memzcmp(lgtkhash[i], sizeof(lgtkhash[i])))
            length += 8;
    return length;
}

uint8_t *ws_wp_nested_lgtkhash_write(uint8_t *ptr, gtkhash_t *lgtkhash, unsigned active_lgtk_index)
{
    uint16_t length = ws_wp_nested_lgtkhash_length(lgtkhash);
    uint8_t tmp8;
    int i;

    ptr = mac_ie_nested_ie_short_base_write(ptr, WP_PAYLOAD_IE_LGTKHASH_TYPE, length);
    tmp8 = 0;
    tmp8 |= FIELD_PREP(WS_WP_LGTKHASH_IE_INCLUDE_LGTK0_MASK, (bool)memzcmp(lgtkhash[0], 8));
    tmp8 |= FIELD_PREP(WS_WP_LGTKHASH_IE_INCLUDE_LGTK1_MASK, (bool)memzcmp(lgtkhash[1], 8));
    tmp8 |= FIELD_PREP(WS_WP_LGTKHASH_IE_INCLUDE_LGTK2_MASK, (bool)memzcmp(lgtkhash[2], 8));
    tmp8 |= FIELD_PREP(WS_WP_LGTKHASH_IE_ACTIVE_INDEX_MASK,  active_lgtk_index);
    *ptr++ = tmp8;
    for (i = 0; i < 3; i++) {
        if (memzcmp(lgtkhash[i], sizeof(lgtkhash[i]))) {
            memcpy(ptr, lgtkhash[i], 8);
            ptr += 8;
        }
    }
    return ptr;
}

uint16_t ws_wp_nested_lfn_channel_plan_length(struct ws_lcp_ie *ws_lcp)
{
    uint16_t length = 1; //Channel Plan Tag

    length += ws_wp_generic_schedule_length_get(&ws_lcp->chan_plan);
    return length;
}

uint8_t *ws_wp_nested_lfn_channel_plan_write(uint8_t *ptr, struct ws_lcp_ie *ws_lcp)
{
    uint16_t length = ws_wp_nested_lfn_channel_plan_length(ws_lcp);

    ptr = mac_ie_nested_ie_long_base_write(ptr, WP_PAYLOAD_IE_LFN_CHANNEL_PLAN_TYPE, length);
    *ptr++ = ws_lcp->lfn_channel_plan_tag;
    *ptr++ = ws_wp_channel_info_base_get(&ws_lcp->chan_plan);
    ptr = ws_wp_channel_plan_write(ptr, &ws_lcp->chan_plan);
    ptr = ws_wp_channel_function_write(ptr, &ws_lcp->chan_plan);
    ptr = ws_wp_nested_excluded_channel_write(ptr, &ws_lcp->chan_plan);
    return ptr;
}

uint8_t *ws_wp_nested_lbats_write(uint8_t *ptr, struct ws_lbats_ie *lbats_ie)
{
    ptr = mac_ie_nested_ie_long_base_write(ptr, WP_PAYLOAD_IE_LBATS_TYPE, ws_wp_nested_lbats_length());
    *ptr++ = lbats_ie->additional_transmissions;
    ptr = write_le16(ptr, lbats_ie->next_transmit_delay);
    return ptr;
}

bool ws_wh_utt_read(const uint8_t *data, uint16_t length, struct ws_utt_ie *utt_ie)
{
    mac_header_IE_t utt_ie_data;

    utt_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (4 > mac_ie_header_sub_id_discover(data, length, &utt_ie_data, WH_IE_UTT_TYPE)) {
        // NO UTT header
        return false;
    }
    data = utt_ie_data.content_ptr;
    utt_ie->message_type = *data++;
    utt_ie->ufsi = read_le24(data);
    return true;
}

bool ws_wh_bt_read(const uint8_t *data, uint16_t length, struct ws_bt_ie *bt_ie)
{
    mac_header_IE_t btt_ie_data;

    btt_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (5 > mac_ie_header_sub_id_discover(data, length, &btt_ie_data, WH_IE_BT_TYPE)) {
        return false;
    }
    data = btt_ie_data.content_ptr;
    bt_ie->broadcast_slot_number = read_le16(data);
    bt_ie->broadcast_interval_offset = read_le24(data + 2);
    return true;
}

bool ws_wh_fc_read(const uint8_t *data, uint16_t length, struct ws_fc_ie *fc_ie)
{
    mac_header_IE_t fc_ie_data;

    fc_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (2 > mac_ie_header_sub_id_discover(data, length, &fc_ie_data, WH_IE_FC_TYPE)) {
        return false;
    }
    data = fc_ie_data.content_ptr;
    fc_ie->tx_flow_ctrl = *data++;
    fc_ie->rx_flow_ctrl = *data;
    return true;
}

bool ws_wh_rsl_read(const uint8_t *data, uint16_t length, int8_t *rsl)
{
    mac_header_IE_t rsl_ie_data;

    rsl_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (1 > mac_ie_header_sub_id_discover(data, length, &rsl_ie_data, WH_IE_RSL_TYPE)) {
        return false;
    }
    *rsl = *rsl_ie_data.content_ptr;

    return true;
}

bool ws_wh_ea_read(const uint8_t *data, uint16_t length, uint8_t *eui64)
{
    mac_header_IE_t rsl_ie_data;

    rsl_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (8 > mac_ie_header_sub_id_discover(data, length, &rsl_ie_data, WH_IE_EA_TYPE)) {
        return false;
    }
    memcpy(eui64, rsl_ie_data.content_ptr, 8);

    return true;
}

bool ws_wh_lutt_read(const uint8_t *data, uint16_t length, struct ws_lutt_ie *lutt_ie)
{
    mac_header_IE_t lutt_ie_data;

    lutt_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_lutt_length() > mac_ie_header_sub_id_discover(data, length, &lutt_ie_data, WH_IE_LUTT_TYPE)) {
        return false;
    }
    data = lutt_ie_data.content_ptr;
    lutt_ie->message_type = *data++;
    lutt_ie->slot_number = read_le16(data);
    lutt_ie->interval_offset = read_le24(data + 2);

    return true;
}

bool ws_wh_lus_read(const uint8_t *data, uint16_t length, struct ws_lus_ie *lus_ie)
{
    mac_header_IE_t lus_ie_data;

    lus_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_lus_length() > mac_ie_header_sub_id_discover(data, length, &lus_ie_data, WH_IE_LUS_TYPE)) {
        return false;
    }
    data = lus_ie_data.content_ptr;
    lus_ie->listen_interval = read_le24(data);
    data += 3;
    lus_ie->channel_plan_tag = *data;

    return true;
}

bool ws_wh_flus_read(const uint8_t *data, uint16_t length, struct ws_flus_ie *flus_ie)
{
    mac_header_IE_t flus_ie_data;

    flus_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_flus_length() > mac_ie_header_sub_id_discover(data, length, &flus_ie_data, WH_IE_FLUS_TYPE)) {
        return false;
    }
    data = flus_ie_data.content_ptr;
    flus_ie->dwell_interval = *data++;
    flus_ie->channel_plan_tag = *data;

    return true;
}

bool ws_wh_lbt_read(const uint8_t *data, uint16_t length, struct ws_lbt_ie *lbt_ie)
{
    mac_header_IE_t lbt_ie_data;

    lbt_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_lbt_length() > mac_ie_header_sub_id_discover(data, length, &lbt_ie_data, WH_IE_LBT_TYPE)) {
        return false;
    }
    data = lbt_ie_data.content_ptr;
    lbt_ie->slot_number = read_le16(data);
    lbt_ie->interval_offset = read_le24(data + 2);

    return true;
}

bool ws_wh_lbs_read(const uint8_t *data, uint16_t length, struct ws_lbs_ie *lbs_ie)
{
    mac_header_IE_t lbs_ie_data;

    lbs_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_lbs_length() > mac_ie_header_sub_id_discover(data, length, &lbs_ie_data, WH_IE_LBS_TYPE)) {
        return false;
    }
    data = lbs_ie_data.content_ptr;
    lbs_ie->broadcast_interval = read_le24(data);
    data += 3;
    lbs_ie->broadcast_scheduler_id = read_le16(data);
    data += 2;
    lbs_ie->channel_plan_tag = *data++;
    lbs_ie->broadcast_sync_period = *data;

    return true;
}

bool ws_wh_nr_read(const uint8_t *data, uint16_t length, struct ws_nr_ie *nr_ie)
{
    mac_header_IE_t nr_ie_data;

    nr_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (3 > mac_ie_header_sub_id_discover(data, length, &nr_ie_data, WH_IE_NR_TYPE)) {
        return false;
    }
    data = nr_ie_data.content_ptr;
    nr_ie->node_role = FIELD_GET(WS_WH_NR_IE_NODE_ROLE_ID_MASK, *data++);
    nr_ie->clock_drift = *data++;
    nr_ie->timing_accuracy = *data++;
    switch (nr_ie->node_role) {
        case WS_NR_ROLE_BR:
            break;
        case WS_NR_ROLE_ROUTER:
            break;
        case WS_NR_ROLE_LFN:
            if (9 > nr_ie_data.length) {
                return false;
            }
            nr_ie->listen_interval_min = read_le24(data);
            nr_ie->listen_interval_max = read_le24(data + 3);
            break;
        default:
            return false;
    }

    return true;
}

bool ws_wh_lnd_read(const uint8_t *data, uint16_t length, struct ws_lnd_ie *lnd_ie)
{
    mac_header_IE_t lnd_ie_data;

    lnd_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_lnd_length() > mac_ie_header_sub_id_discover(data, length, &lnd_ie_data, WH_IE_LND_TYPE)) {
        return false;
    }
    data = lnd_ie_data.content_ptr;
    lnd_ie->response_threshold = *data++;
    lnd_ie->response_delay = read_le24(data);
    data += 3;
    lnd_ie->discovery_slot_time = *data++;
    lnd_ie->discovery_slots = *data++;
    lnd_ie->discovery_first_slot = read_le16(data);

    return true;
}

bool ws_wh_lto_read(const uint8_t *data, uint16_t length, struct ws_lto_ie *lto_ie)
{
    mac_header_IE_t lto_ie_data;

    lto_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_lto_length() > mac_ie_header_sub_id_discover(data, length, &lto_ie_data, WH_IE_LTO_TYPE)) {
        return false;
    }
    data = lto_ie_data.content_ptr;
    lto_ie->offset = read_le24(data);
    lto_ie->adjusted_listening_interval = read_le24(data + 3);

    return true;
}

bool ws_wh_panid_read(const uint8_t *data, uint16_t length, struct ws_panid_ie *panid_ie)
{
    mac_header_IE_t panid_ie_data;

    panid_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_panid_length() > mac_ie_header_sub_id_discover(data, length, &panid_ie_data, WH_IE_PANID_TYPE)) {
        return false;
    }
    panid_ie->panid = read_le16(panid_ie_data.content_ptr);

    return true;
}

bool ws_wh_lbc_read(const uint8_t *data, uint16_t length, struct ws_lbc_ie *lbc_ie)
{
    mac_header_IE_t lbc_ie_data;

    lbc_ie_data.id = MAC_HEADER_ASSIGNED_EXTERNAL_ORG_IE_ID;
    if (ws_wh_lbc_length() > mac_ie_header_sub_id_discover(data, length, &lbc_ie_data, WH_IE_LBC_TYPE)) {
        return false;
    }
    data = lbc_ie_data.content_ptr;
    lbc_ie->lfn_broadcast_interval = read_le24(data);
    data += 3;
    lbc_ie->broadcast_sync_period = *data;
    return true;
}

static const uint8_t *ws_channel_plan_zero_read(const uint8_t *ptr, ws_channel_plan_zero_t *plan)
{
    plan->regulatory_domain = *ptr++;
    plan->operating_class = *ptr++;
    return ptr;
}

static const uint8_t *ws_channel_plan_one_read(const uint8_t *ptr, ws_channel_plan_one_t *plan)
{
    plan->ch0 = read_le24(ptr);
    ptr += 3;
    plan->channel_spacing = *ptr++;
    plan->number_of_channel = read_le16(ptr);
    ptr += 2;
    return ptr;
}

static const uint8_t *ws_channel_plan_two_read(const uint8_t *ptr, ws_channel_plan_two_t *plan)
{
    plan->regulatory_domain = *ptr++;
    plan->channel_plan_id = *ptr++;
    return ptr;
}

static const uint8_t *ws_channel_function_zero_read(const uint8_t *ptr, ws_channel_function_zero_t *plan)
{
    plan->fixed_channel = read_le16(ptr);
    return ptr + 2;
}

static const uint8_t *ws_channel_function_three_read(const uint8_t *ptr, ws_channel_function_three_t *plan)
{
    plan->channel_hop_count = *ptr++;
    plan->channel_list = ptr;
    return ptr;
}

bool ws_wp_nested_us_read(const uint8_t *data, uint16_t length, struct ws_us_ie *us_ie)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_US_TYPE;
    nested_payload_ie.type_long = true;
    if (4 > mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    }

    data = nested_payload_ie.content_ptr;
    us_ie->dwell_interval = *data++;
    us_ie->clock_drift = *data++;
    us_ie->timing_accuracy = *data++;
    us_ie->chan_plan.channel_plan          = FIELD_GET(WS_WP_SCHEDULE_IE_CHAN_PLAN_MASK,     *data);
    us_ie->chan_plan.channel_function      = FIELD_GET(WS_WP_SCHEDULE_IE_CHAN_FUNC_MASK,     *data);
    us_ie->chan_plan.excluded_channel_ctrl = FIELD_GET(WS_WP_SCHEDULE_IE_EXCL_CHAN_CTL_MASK, *data);
    data++;
    uint16_t info_length = 0;
    nested_payload_ie.length -= 4;
    info_length = ws_channel_plan_length(us_ie->chan_plan.channel_plan);
    if (nested_payload_ie.length < info_length) {
        return false;
    }

    nested_payload_ie.length -= info_length;
    switch (us_ie->chan_plan.channel_plan) {
        case 0:
            data = ws_channel_plan_zero_read(data, &us_ie->chan_plan.plan.zero);
            break;

        case 1:
            data = ws_channel_plan_one_read(data, &us_ie->chan_plan.plan.one);
            break;
        case 2:
            data = ws_channel_plan_two_read(data, &us_ie->chan_plan.plan.two);
            break;
        default:
            return false;

    }

    info_length = ws_channel_function_length(us_ie->chan_plan.channel_function, 0);

    if (nested_payload_ie.length < info_length) {
        return false;
    }
    nested_payload_ie.length -= info_length;


    switch (us_ie->chan_plan.channel_function) {
        case 0:
            data = ws_channel_function_zero_read(data, &us_ie->chan_plan.function.zero);
            break;

        case 1:
        case 2:
            break;

        case 3:

            data = ws_channel_function_three_read(data, &us_ie->chan_plan.function.three);
            info_length = us_ie->chan_plan.function.three.channel_hop_count;
            if (nested_payload_ie.length < info_length) {
                return false;
            }
            nested_payload_ie.length -= info_length;
            data += info_length;
            break;
        default:
            return false;

    }

    switch (us_ie->chan_plan.excluded_channel_ctrl) {
        case WS_EXC_CHAN_CTRL_NONE:

            break;
        case WS_EXC_CHAN_CTRL_RANGE:
            us_ie->chan_plan.excluded_channels.range.number_of_range = *data;
            if (nested_payload_ie.length < (us_ie->chan_plan.excluded_channels.range.number_of_range * 4) + 1) {
                return false;
            }
            //Set Range start after validation
            us_ie->chan_plan.excluded_channels.range.range_start = data + 1;
            break;

        case WS_EXC_CHAN_CTRL_BITMASK:
            if (us_ie->chan_plan.channel_plan == 1) {
                us_ie->chan_plan.excluded_channels.mask.mask_len_inline = ((us_ie->chan_plan.plan.one.number_of_channel + 7) / 8);
                if (us_ie->chan_plan.excluded_channels.mask.mask_len_inline != nested_payload_ie.length) {
                    //Channel mask length is not correct
                    return false;
                }
            } else {
                us_ie->chan_plan.excluded_channels.mask.mask_len_inline = nested_payload_ie.length;
            }

            us_ie->chan_plan.excluded_channels.mask.channel_mask = data;
            break;
        default:
            return false;
    }

    return true;
}

bool ws_wp_nested_bs_read(const uint8_t *data, uint16_t length, struct ws_bs_ie *bs_ie)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_BS_TYPE;
    nested_payload_ie.type_long = true;
    if (10 > mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    }
    data = nested_payload_ie.content_ptr;
    bs_ie->broadcast_interval = read_le32(data);
    bs_ie->broadcast_schedule_identifier = read_le16(data + 4);
    data += 6;
    bs_ie->dwell_interval = *data++;
    bs_ie->clock_drift = *data++;
    bs_ie->timing_accuracy = *data++;

    bs_ie->chan_plan.channel_plan          = FIELD_GET(WS_WP_SCHEDULE_IE_CHAN_PLAN_MASK,     *data);
    bs_ie->chan_plan.channel_function      = FIELD_GET(WS_WP_SCHEDULE_IE_CHAN_FUNC_MASK,     *data);
    bs_ie->chan_plan.excluded_channel_ctrl = FIELD_GET(WS_WP_SCHEDULE_IE_EXCL_CHAN_CTL_MASK, *data);
    data++;
    nested_payload_ie.length -= 10;
    uint16_t info_length = 0;

    info_length = ws_channel_plan_length(bs_ie->chan_plan.channel_plan);
    if (nested_payload_ie.length < info_length) {
        return false;
    }
    nested_payload_ie.length -= info_length;
    switch (bs_ie->chan_plan.channel_plan) {
        case 0:
            data = ws_channel_plan_zero_read(data, &bs_ie->chan_plan.plan.zero);
            break;

        case 1:
            data = ws_channel_plan_one_read(data, &bs_ie->chan_plan.plan.one);
            break;
        case 2:
            data = ws_channel_plan_two_read(data, &bs_ie->chan_plan.plan.two);
            break;
        default:
            return false;

    }

    info_length = ws_channel_function_length(bs_ie->chan_plan.channel_function, 0);
    if (nested_payload_ie.length < info_length) {
        return false;
    }
    nested_payload_ie.length -= info_length;

    switch (bs_ie->chan_plan.channel_function) {
        case 0:
            data = ws_channel_function_zero_read(data, &bs_ie->chan_plan.function.zero);
            break;

        case 1:
        case 2:
            break;

        case 3:
            data = ws_channel_function_three_read(data, &bs_ie->chan_plan.function.three);
            info_length = bs_ie->chan_plan.function.three.channel_hop_count;
            if (nested_payload_ie.length < info_length) {
                return false;
            }
            nested_payload_ie.length -= info_length;
            data += info_length;
            break;
        default:
            return false;

    }

    switch (bs_ie->chan_plan.excluded_channel_ctrl) {
        case WS_EXC_CHAN_CTRL_NONE:

            break;
        case WS_EXC_CHAN_CTRL_RANGE:
            bs_ie->chan_plan.excluded_channels.range.number_of_range = *data;
            if (nested_payload_ie.length < (bs_ie->chan_plan.excluded_channels.range.number_of_range * 4) + 1) {
                return false;
            }
            //Set Range start after validation
            bs_ie->chan_plan.excluded_channels.range.range_start = data + 1;
            break;

        case WS_EXC_CHAN_CTRL_BITMASK:
            if (bs_ie->chan_plan.channel_plan == 1) {
                bs_ie->chan_plan.excluded_channels.mask.mask_len_inline = ((bs_ie->chan_plan.plan.one.number_of_channel + 7) / 8);
                if (bs_ie->chan_plan.excluded_channels.mask.mask_len_inline != nested_payload_ie.length) {
                    //Channel mask length is not correct
                    return false;
                }
            } else {
                bs_ie->chan_plan.excluded_channels.mask.mask_len_inline = nested_payload_ie.length;
            }

            bs_ie->chan_plan.excluded_channels.mask.channel_mask = data;
            break;
        default:
            return false;
    }

    return true;
}

bool ws_wp_nested_pan_read(const uint8_t *data, uint16_t length, struct ws_pan_information *pan_configuration)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_PAN_TYPE;
    nested_payload_ie.type_long = false;
    if (5 > mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    }

    pan_configuration->pan_size = read_le16(nested_payload_ie.content_ptr);
    pan_configuration->routing_cost = read_le16(nested_payload_ie.content_ptr + 2);
    pan_configuration->use_parent_bs      = FIELD_GET(WS_WP_PAN_IE_USE_PARENT_BS_IE_MASK, nested_payload_ie.content_ptr[4]);
    pan_configuration->rpl_routing_method = FIELD_GET(WS_WP_PAN_IE_ROUTING_METHOD_MASK,   nested_payload_ie.content_ptr[4]);
    pan_configuration->version            = FIELD_GET(WS_WP_PAN_IE_FAN_TPS_VERSION_MASK,  nested_payload_ie.content_ptr[4]);
    if (pan_configuration->version > WS_FAN_VERSION_1_0)
        pan_configuration->lfn_window_style = FIELD_GET(WS_WP_PAN_IE_LFN_WINDOW_STYLE_MASK, nested_payload_ie.content_ptr[4]);
    else
        pan_configuration->lfn_window_style = false;
    return true;
}

bool ws_wp_nested_pan_version_read(const uint8_t *data, uint16_t length, uint16_t *pan_version)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_PAN_VER_TYPE;
    nested_payload_ie.type_long = false;
    if (2 > mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    }
    *pan_version = read_le16(nested_payload_ie.content_ptr);

    return true;
}

gtkhash_t *ws_wp_nested_gtkhash_read(const uint8_t *data, uint16_t length)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_GTKHASH_TYPE;
    nested_payload_ie.type_long = false;
    if (mac_ie_nested_discover(data, length, &nested_payload_ie) !=  32) {
        return NULL;
    }

    return (gtkhash_t *)nested_payload_ie.content_ptr;
}

bool ws_wp_nested_network_name_read(const uint8_t *data, uint16_t length, ws_wp_network_name_t *network_name)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_NETNAME_TYPE;
    nested_payload_ie.type_long = false;
    if (0 == mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    } else if (nested_payload_ie.length > 32) {
        //Too long name
        return false;
    }
    network_name->network_name = nested_payload_ie.content_ptr;
    network_name->network_name_length = nested_payload_ie.length;
    return true;
}

bool ws_wp_nested_pom_read(const uint8_t *data, uint16_t length, struct ws_pom_ie *pom_ie)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_POM_TYPE;
    nested_payload_ie.type_long = false;
    if (mac_ie_nested_discover(data, length, &nested_payload_ie) <= 1) {
        // Too short
        return false;
    }

    pom_ie->phy_op_mode_number  = FIELD_GET(WS_WP_POM_IE_PHY_OP_MODE_NUMBER_MASK, nested_payload_ie.content_ptr[0]);
    pom_ie->mdr_command_capable = FIELD_GET(WS_WP_POM_IE_MDR_CAPABLE_MASK,        nested_payload_ie.content_ptr[0]);
    if (pom_ie->phy_op_mode_number != 0) {
        pom_ie->phy_op_mode_id = nested_payload_ie.content_ptr + 1; //FIXME: this is nasty
    } else {
        pom_ie->phy_op_mode_id = NULL;
    }

    return true;
}

bool ws_wp_nested_lfn_version_read(const uint8_t *data, uint16_t length, struct ws_lfnver_ie *ws_lfnver)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_LFN_VER_TYPE;
    nested_payload_ie.type_long = false;
    if (ws_wp_nested_lfn_version_length() > mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    }

    ws_lfnver->lfn_version = read_le16(nested_payload_ie.content_ptr);
    return true;
}

bool ws_wp_nested_lgtkhash_read(const uint8_t *data, uint16_t length, gtkhash_t *lgtkhash, unsigned *active_lgtk_index)
{
    mac_nested_payload_IE_t nested_payload_ie;
    unsigned valid_hashs;
    int i;

    nested_payload_ie.id = WP_PAYLOAD_IE_LGTKHASH_TYPE;
    nested_payload_ie.type_long = false;
    if (1 > mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    }
    data = nested_payload_ie.content_ptr;

    valid_hashs = FIELD_GET(WS_WP_LGTKHASH_IE_INCLUDE_LGTK0_MASK |
                            WS_WP_LGTKHASH_IE_INCLUDE_LGTK1_MASK |
                            WS_WP_LGTKHASH_IE_INCLUDE_LGTK2_MASK, *data);
    *active_lgtk_index = FIELD_GET(WS_WP_LGTKHASH_IE_ACTIVE_INDEX_MASK, *data);

    if (__builtin_popcount(valid_hashs) * 8 > nested_payload_ie.length) {
        return false;
    }

    data++;
    for (i = 0; i < 3; i++) {
        if (valid_hashs & (1 << i)) {
            memcpy(lgtkhash[i], data, 8);
            data += 8;
        } else {
            memset(lgtkhash[i], 0, 8);
        }
    }

    return true;
}

bool ws_wp_nested_lbats_read(const uint8_t *data, uint16_t length, struct ws_lbats_ie *lbats_ie)
{
    mac_nested_payload_IE_t nested_payload_ie;

    nested_payload_ie.id = WP_PAYLOAD_IE_LBATS_TYPE;
    nested_payload_ie.type_long = true;
    if (3 > mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    }
    lbats_ie->additional_transmissions = *nested_payload_ie.content_ptr++;
    lbats_ie->next_transmit_delay = read_le16(nested_payload_ie.content_ptr);

    return true;
}

bool ws_wp_nested_lfn_channel_plan_read(const uint8_t *data, uint16_t length, struct ws_lcp_ie *ws_lcp)
{
    mac_nested_payload_IE_t nested_payload_ie;
    uint16_t info_length;
    uint8_t plan_tag_id;

    nested_payload_ie.id = WP_PAYLOAD_IE_LFN_CHANNEL_PLAN_TYPE;
    nested_payload_ie.type_long = true;
    if (2 > mac_ie_nested_discover(data, length, &nested_payload_ie)) {
        return false;
    }
    // FIXME is mac_ie_nested_tagged_discover() needed?
    plan_tag_id = *nested_payload_ie.content_ptr;
    if (1 < mac_ie_nested_tagged_discover(data, length, &nested_payload_ie, plan_tag_id)) {
        return false;
    }
    //Parse Channel Plan, function and excluded channel
    data = nested_payload_ie.content_ptr;
    ws_lcp->lfn_channel_plan_tag = *data++;
    ws_lcp->chan_plan.channel_plan          = FIELD_GET(WS_WP_SCHEDULE_IE_CHAN_PLAN_MASK,     *data);
    ws_lcp->chan_plan.channel_function      = FIELD_GET(WS_WP_SCHEDULE_IE_CHAN_FUNC_MASK,     *data);
    ws_lcp->chan_plan.excluded_channel_ctrl = FIELD_GET(WS_WP_SCHEDULE_IE_EXCL_CHAN_CTL_MASK, *data);
    data++;
    nested_payload_ie.length--;

    info_length = ws_channel_plan_length(ws_lcp->chan_plan.channel_plan);
    if (nested_payload_ie.length < info_length) {
        return false;
    }
    nested_payload_ie.length -= info_length;

    switch (ws_lcp->chan_plan.channel_plan) {
        case 0:
            data = ws_channel_plan_zero_read(data, &ws_lcp->chan_plan.plan.zero);
            break;
        case 1:
            data = ws_channel_plan_one_read(data, &ws_lcp->chan_plan.plan.one);
            break;
        case 2:
            data = ws_channel_plan_two_read(data, &ws_lcp->chan_plan.plan.two);
            break;
        default:
            return false;
    }

    info_length = ws_channel_function_length(ws_lcp->chan_plan.channel_function, 0);
    if (nested_payload_ie.length < info_length) {
        return false;
    }
    nested_payload_ie.length -= info_length;

    switch (ws_lcp->chan_plan.channel_function) {
        case 0:
            data = ws_channel_function_zero_read(data, &ws_lcp->chan_plan.function.zero);
            break;
        case 1:
        case 2:
            break;
        case 3:
            data = ws_channel_function_three_read(data, &ws_lcp->chan_plan.function.three);
            info_length = ws_lcp->chan_plan.function.three.channel_hop_count;
            if (nested_payload_ie.length < info_length) {
                return false;
            }
            nested_payload_ie.length -= info_length;
            data += info_length;
            break;
        default:
            return false;
    }

    switch (ws_lcp->chan_plan.excluded_channel_ctrl) {
        case WS_EXC_CHAN_CTRL_NONE:
            break;
        case WS_EXC_CHAN_CTRL_RANGE:
            ws_lcp->chan_plan.excluded_channels.range.number_of_range = *data;
            if (nested_payload_ie.length < (ws_lcp->chan_plan.excluded_channels.range.number_of_range * 4) + 1) {
                return false;
            }
            //Set Range start after validation
            ws_lcp->chan_plan.excluded_channels.range.range_start = data + 1;
            break;
        case WS_EXC_CHAN_CTRL_BITMASK:
            if (ws_lcp->chan_plan.channel_plan == 1 &&
                (ws_lcp->chan_plan.plan.one.number_of_channel + 7) / 8 != nested_payload_ie.length) {
                //Channel mask length is not correct
                return false;
            }
            ws_lcp->chan_plan.excluded_channels.mask.mask_len_inline = nested_payload_ie.length;
            ws_lcp->chan_plan.excluded_channels.mask.channel_mask = data;
            break;
        default:
            return false;
    }

    return true;

}

