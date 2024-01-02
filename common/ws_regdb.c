/*
 * Copyright (c) 2021-2023 Silicon Laboratories Inc. (www.silabs.com)
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
#include <stddef.h>

#include "common/utils.h"

#include "ws_regdb.h"

const struct phy_params phy_params_table[] = {
    /*                                                                                     ,- oqpsk_chip_rate
       ,- rail_phy_mode_id                                                                 |  ,- oqpsk_rate_mode
       |   ,- phy_mode_id                                     ofdm_mcs -.  ,- ofdm_option  |  |  ,- oqpsk_spreading_mode
       |   |    modulation      datarate  mode  fsk_modulation_index    |  |   fec         |  |  |  */
    {  1, 0x01, MODULATION_2FSK,   50000, 0x1a, MODULATION_INDEX_0_5,   0, 0, false,       0, 0, 0 },
    {  2, 0x02, MODULATION_2FSK,   50000, 0x1b, MODULATION_INDEX_1_0,   0, 0, false,       0, 0, 0 },
    {  3, 0x03, MODULATION_2FSK,  100000, 0x2a, MODULATION_INDEX_0_5,   0, 0, false,       0, 0, 0 },
    {  4, 0x04, MODULATION_2FSK,  100000, 0x2b, MODULATION_INDEX_1_0,   0, 0, false,       0, 0, 0 },
    {  5, 0x05, MODULATION_2FSK,  150000, 0x03, MODULATION_INDEX_0_5,   0, 0, false,       0, 0, 0 },
    {  6, 0x06, MODULATION_2FSK,  200000, 0x4a, MODULATION_INDEX_0_5,   0, 0, false,       0, 0, 0 },
    {  7, 0x07, MODULATION_2FSK,  200000, 0x4b, MODULATION_INDEX_1_0,   0, 0, false,       0, 0, 0 },
    {  8, 0x08, MODULATION_2FSK,  300000, 0x05, MODULATION_INDEX_0_5,   0, 0, false,       0, 0, 0 },
    { 17, 0x11, MODULATION_2FSK,   50000,    0, MODULATION_INDEX_0_5,   0, 0,  true,       0, 0, 0 },
    { 18, 0x12, MODULATION_2FSK,   50000,    0, MODULATION_INDEX_1_0,   0, 0,  true,       0, 0, 0 },
    { 19, 0x13, MODULATION_2FSK,  100000,    0, MODULATION_INDEX_0_5,   0, 0,  true,       0, 0, 0 },
    { 20, 0x14, MODULATION_2FSK,  100000,    0, MODULATION_INDEX_1_0,   0, 0,  true,       0, 0, 0 },
    { 21, 0x15, MODULATION_2FSK,  150000,    0, MODULATION_INDEX_0_5,   0, 0,  true,       0, 0, 0 },
    { 22, 0x16, MODULATION_2FSK,  200000,    0, MODULATION_INDEX_0_5,   0, 0,  true,       0, 0, 0 },
    { 23, 0x17, MODULATION_2FSK,  200000,    0, MODULATION_INDEX_1_0,   0, 0,  true,       0, 0, 0 },
    { 24, 0x18, MODULATION_2FSK,  300000,    0, MODULATION_INDEX_0_5,   0, 0,  true,       0, 0, 0 },
    { 32, 0x20, MODULATION_OFDM,  100000,    0, MODULATION_INDEX_UNDEF, 0, 1, false,       0, 0, 0 }, // not Wi-SUN standard
    { 32, 0x21, MODULATION_OFDM,  200000,    0, MODULATION_INDEX_UNDEF, 1, 1, false,       0, 0, 0 }, // not Wi-SUN standard
    { 32, 0x22, MODULATION_OFDM,  400000,    0, MODULATION_INDEX_UNDEF, 2, 1, false,       0, 0, 0 },
    { 32, 0x23, MODULATION_OFDM,  800000,    0, MODULATION_INDEX_UNDEF, 3, 1, false,       0, 0, 0 },
    { 32, 0x24, MODULATION_OFDM, 1200000,    0, MODULATION_INDEX_UNDEF, 4, 1, false,       0, 0, 0 },
    { 32, 0x25, MODULATION_OFDM, 1600000,    0, MODULATION_INDEX_UNDEF, 5, 1, false,       0, 0, 0 },
    { 32, 0x26, MODULATION_OFDM, 2400000,    0, MODULATION_INDEX_UNDEF, 6, 1, false,       0, 0, 0 },
    { 32, 0x27, MODULATION_OFDM, 3600000,    0, MODULATION_INDEX_UNDEF, 7, 1, false,       0, 0, 0 }, // not IEEE 802.15.4 standard
    { 48, 0x30, MODULATION_OFDM,   50000,    0, MODULATION_INDEX_UNDEF, 0, 2, false,       0, 0, 0 }, // not Wi-SUN standard
    { 48, 0x31, MODULATION_OFDM,  100000,    0, MODULATION_INDEX_UNDEF, 1, 2, false,       0, 0, 0 }, // not Wi-SUN standard
    { 48, 0x32, MODULATION_OFDM,  200000,    0, MODULATION_INDEX_UNDEF, 2, 2, false,       0, 0, 0 }, // not Wi-SUN standard
    { 48, 0x33, MODULATION_OFDM,  400000,    0, MODULATION_INDEX_UNDEF, 3, 2, false,       0, 0, 0 },
    { 48, 0x34, MODULATION_OFDM,  600000,    0, MODULATION_INDEX_UNDEF, 4, 2, false,       0, 0, 0 },
    { 48, 0x35, MODULATION_OFDM,  800000,    0, MODULATION_INDEX_UNDEF, 5, 2, false,       0, 0, 0 },
    { 48, 0x36, MODULATION_OFDM, 1200000,    0, MODULATION_INDEX_UNDEF, 6, 2, false,       0, 0, 0 },
    { 48, 0x37, MODULATION_OFDM, 1800000,    0, MODULATION_INDEX_UNDEF, 7, 2, false,       0, 0, 0 }, // not IEEE 802.15.4 standard
    { 64, 0x40, MODULATION_OFDM,   25000,    0, MODULATION_INDEX_UNDEF, 0, 3, false,       0, 0, 0 }, // not Wi-SUN standard
    { 64, 0x41, MODULATION_OFDM,   50000,    0, MODULATION_INDEX_UNDEF, 1, 3, false,       0, 0, 0 }, // not Wi-SUN standard
    { 64, 0x42, MODULATION_OFDM,  100000,    0, MODULATION_INDEX_UNDEF, 2, 3, false,       0, 0, 0 }, // not Wi-SUN standard
    { 64, 0x43, MODULATION_OFDM,  200000,    0, MODULATION_INDEX_UNDEF, 3, 3, false,       0, 0, 0 }, // not Wi-SUN standard
    { 64, 0x44, MODULATION_OFDM,  300000,    0, MODULATION_INDEX_UNDEF, 4, 3, false,       0, 0, 0 },
    { 64, 0x45, MODULATION_OFDM,  400000,    0, MODULATION_INDEX_UNDEF, 5, 3, false,       0, 0, 0 },
    { 64, 0x46, MODULATION_OFDM,  600000,    0, MODULATION_INDEX_UNDEF, 6, 3, false,       0, 0, 0 },
    { 64, 0x47, MODULATION_OFDM,  900000,    0, MODULATION_INDEX_UNDEF, 7, 3, false,       0, 0, 0 }, // not IEEE 802.15.4 standard
    { 80, 0x50, MODULATION_OFDM,   12500,    0, MODULATION_INDEX_UNDEF, 0, 4, false,       0, 0, 0 }, // not Wi-SUN standard
    { 80, 0x51, MODULATION_OFDM,   25000,    0, MODULATION_INDEX_UNDEF, 1, 4, false,       0, 0, 0 }, // not Wi-SUN standard
    { 80, 0x52, MODULATION_OFDM,   50000,    0, MODULATION_INDEX_UNDEF, 2, 4, false,       0, 0, 0 }, // not Wi-SUN standard
    { 80, 0x53, MODULATION_OFDM,  100000,    0, MODULATION_INDEX_UNDEF, 3, 4, false,       0, 0, 0 }, // not Wi-SUN standard
    { 80, 0x54, MODULATION_OFDM,  150000,    0, MODULATION_INDEX_UNDEF, 4, 4, false,       0, 0, 0 },
    { 80, 0x55, MODULATION_OFDM,  200000,    0, MODULATION_INDEX_UNDEF, 5, 4, false,       0, 0, 0 },
    { 80, 0x56, MODULATION_OFDM,  300000,    0, MODULATION_INDEX_UNDEF, 6, 4, false,       0, 0, 0 },
    { 80, 0x57, MODULATION_OFDM,  450000,    0, MODULATION_INDEX_UNDEF, 7, 4, false,       0, 0, 0 }, // not IEEE 802.15.4 standard
    { 96, 0x60, MODULATION_OQPSK,   6250,    0, MODULATION_INDEX_UNDEF, 0, 0, false,  100000, 0, 0 }, // not Wi-SUN standard
    { 96, 0x61, MODULATION_OQPSK,  12500,    0, MODULATION_INDEX_UNDEF, 0, 0, false,  100000, 1, 0 }, // not Wi-SUN standard
    { 96, 0x62, MODULATION_OQPSK,  25000,    0, MODULATION_INDEX_UNDEF, 0, 0, false,  100000, 2, 0 }, // not Wi-SUN standard
    { 96, 0x63, MODULATION_OQPSK,  50000,    0, MODULATION_INDEX_UNDEF, 0, 0, false,  100000, 3, 0 }, // not Wi-SUN standard
    {112, 0x70, MODULATION_OQPSK,  31250,    0, MODULATION_INDEX_UNDEF, 0, 0, false, 1000000, 0, 0 }, // not Wi-SUN standard
    {112, 0x71, MODULATION_OQPSK, 125000,    0, MODULATION_INDEX_UNDEF, 0, 0, false, 1000000, 1, 0 }, // not Wi-SUN standard
    {112, 0x72, MODULATION_OQPSK, 250000,    0, MODULATION_INDEX_UNDEF, 0, 0, false, 1000000, 2, 0 }, // not Wi-SUN standard
    {112, 0x73, MODULATION_OQPSK, 500000,    0, MODULATION_INDEX_UNDEF, 0, 0, false, 1000000, 3, 0 }, // not Wi-SUN standard
    {  0,    0, MODULATION_UNDEFINED,  0,    0, MODULATION_INDEX_UNDEF, 0, 0, false,       0, 0, 0 },
};

const struct chan_params chan_params_table[] = {
    /*                                                     chan_count -.
                              chan_plan_id -.     chan_spacing -.     |    ,- chan_count_valid
         domain    class   regional_reg     |   chan0_freq      |     |    |      valid_phy_modes */
    { REG_DOMAIN_AZ, 1, REG_REGIONAL_NONE,  48,  915200000,  200000,  64,  64, {  2,  3, 18, 19              }, }, // REG_DOMAIN_AZ and REG_DOMAIN_NZ share the same ID
    { REG_DOMAIN_AZ, 2, REG_REGIONAL_NONE,  49,  915400000,  400000,  32,  32, {  5,  6,  8, 21, 22, 24      }, },
    { REG_DOMAIN_BZ, 1, REG_REGIONAL_NONE,   1,  902200000,  200000, 129,  90, {  2,  3, 18, 19, 84, 85, 86, }, .chan_allowed = "0-25,65-255", },
    { REG_DOMAIN_BZ, 2, REG_REGIONAL_NONE,   2,  902400000,  400000,  64,  43, {  5,  6, 21, 22, 68, 69, 70, }, .chan_allowed = "0-11,33-255", },
    { REG_DOMAIN_BZ, 3, REG_REGIONAL_NONE,   3,  902600000,  600000,  42,  28, {  8, 24,                     }, .chan_allowed = "0-7,22-255", },
    { REG_DOMAIN_BZ, 0, REG_REGIONAL_NONE,   4,  902800000,  800000,  32,  22, { 51, 52, 53, 54,             }, .chan_allowed = "0-5,16-255", },
    { REG_DOMAIN_BZ, 0, REG_REGIONAL_NONE,   5,  903200000, 1200000,  21,  13, { 34, 35, 36, 37, 38,         }, .chan_allowed = "0-2,11-255", },
    { REG_DOMAIN_CN, 1, REG_REGIONAL_NONE, 160,  470200000,  200000, 199, 199, {  2,  3,  5, 18, 19, 21      }, },
    { REG_DOMAIN_CN, 2, REG_REGIONAL_NONE,   0,  779200000,  200000,  39,  39, {  2,  3,                     }, },
    { REG_DOMAIN_CN, 3, REG_REGIONAL_NONE,   0,  779400000,  400000,  19,  19, {  5,  6,  8,                 }, },
    { REG_DOMAIN_CN, 4, REG_REGIONAL_NONE, 128,  920625000,  250000,  16,  16, {  2,  3,  5, 18, 19, 21      }, },
    { REG_DOMAIN_EU, 1, REG_REGIONAL_NONE,   0,  863100000,  100000,  69,  69, {  1,                         }, },
    { REG_DOMAIN_EU, 2, REG_REGIONAL_NONE,   0,  863100000,  200000,  35,  35, {  3,  5,                     }, },
    { REG_DOMAIN_EU, 0, REG_REGIONAL_NONE,  32,  863100000,  100000,  69,  62, {  1, 17,                     }, .chan_allowed = "0-54,57-60,64,67-255", },
    { REG_DOMAIN_EU, 0, REG_REGIONAL_NONE,  33,  863100000,  200000,  35,  29, {  3,  5, 19, 21, 84, 85, 86, }, .chan_allowed = "0-26,29,34-255", },
    { REG_DOMAIN_EU, 3, REG_REGIONAL_NONE,  34,  870100000,  100000,  55,  55, {  1, 17,                     }, },
    { REG_DOMAIN_EU, 4, REG_REGIONAL_NONE,  35,  870200000,  200000,  27,  27, {  3,  5, 19, 21, 84, 85, 86, }, },
    { REG_DOMAIN_EU, 0, REG_REGIONAL_NONE,  36,  863100000,  100000, 125, 118, {  1, 17,                     }, .chan_allowed = "0-54,57-60,64,67-255", },
    { REG_DOMAIN_EU, 0, REG_REGIONAL_NONE,  37,  863100000,  200000,  62,  56, {  3,  5, 19, 21, 84, 85, 86, }, .chan_allowed = "0-26,29,34-255", },
    { REG_DOMAIN_HK, 1, REG_REGIONAL_NONE,  64,  920200000,  200000,  24,  24, {  2,  3, 18, 19              }, },
    { REG_DOMAIN_HK, 2, REG_REGIONAL_NONE,  65,  920400000,  400000,  12,  12, {  5,  6,  8, 21, 22, 24      }, },
    { REG_DOMAIN_IN, 1, REG_REGIONAL_NONE,  39,  865100000,  100000,  29,  29, {  1, 17                      }, },
    { REG_DOMAIN_IN, 2, REG_REGIONAL_NONE,  40,  865100000,  200000,  15,  15, {  3,  5, 19, 21, 84, 85, 86  }, },
    { REG_DOMAIN_JP, 1, REG_REGIONAL_ARIB,   0,  920600000,  200000,  38,  38, {  2,                         }, },
    { REG_DOMAIN_JP, 2, REG_REGIONAL_ARIB,   0,  920900000,  400000,  18,  18, {  4,  5,                     }, },
    { REG_DOMAIN_JP, 3, REG_REGIONAL_ARIB,   0,  920800000,  600000,  12,  12, {  7,  8,                     }, },
    { REG_DOMAIN_JP, 0, REG_REGIONAL_ARIB,  21,  920600000,  200000,  38,  29, {  2, 18, 84, 85, 86,         }, .chan_allowed = "9-255", },
    { REG_DOMAIN_JP, 0, REG_REGIONAL_ARIB,  22,  920900000,  400000,  18,  14, {  4,  5, 20, 21, 68, 69, 70, }, .chan_allowed = "4-255", },
    { REG_DOMAIN_JP, 0, REG_REGIONAL_ARIB,  23,  920800000,  600000,  12,   9, {  7,  8, 23, 24,             }, .chan_allowed = "3-255", },
    { REG_DOMAIN_JP, 0, REG_REGIONAL_ARIB,  24,  921100000,  800000,   9,   7, { 51, 52, 53, 54,             }, .chan_allowed = "2-255", },
    { REG_DOMAIN_KR, 1, REG_REGIONAL_NONE,  96,  917100000,  200000,  32,  32, {  2,  3, 18, 19              }, },
    { REG_DOMAIN_KR, 2, REG_REGIONAL_NONE,  97,  917300000,  400000,  16,  16, {  5,  6,  8, 21, 22, 24      }, },
    { REG_DOMAIN_MX, 1, REG_REGIONAL_NONE,   1,  902200000,  200000, 129, 129, {  2,  3, 18, 19              }, },
    { REG_DOMAIN_MX, 2, REG_REGIONAL_NONE,   2,  902400000,  400000,  64,  64, {  5,  6,  8, 21, 22, 24      }, },
    { REG_DOMAIN_MY, 1, REG_REGIONAL_NONE,  80,  919200000,  200000,  19,  19, {  2,  3, 18, 19              }, },
    { REG_DOMAIN_MY, 2, REG_REGIONAL_NONE,  81,  919200000,  400000,  10,  10, {  5,  6,  8, 21, 22, 24      }, },
    { REG_DOMAIN_NA, 1, REG_REGIONAL_NONE,   1,  902200000,  200000, 129, 129, {  2,  3, 18, 19, 84, 85, 86, }, },
    { REG_DOMAIN_NA, 2, REG_REGIONAL_NONE,   2,  902400000,  400000,  64,  64, {  5,  6, 21, 22, 68, 69, 70, }, },
    { REG_DOMAIN_NA, 3, REG_REGIONAL_NONE,   3,  902600000,  600000,  42,  42, {  8, 24,                     }, },
    { REG_DOMAIN_NA, 0, REG_REGIONAL_NONE,   4,  902800000,  800000,  32,  32, { 51, 52, 53, 54,             }, },
    { REG_DOMAIN_NA, 0, REG_REGIONAL_NONE,   5,  903200000, 1200000,  21,  21, { 34, 35, 36, 37, 38,         }, },
    { REG_DOMAIN_PH, 1, REG_REGIONAL_NONE,   0,  915200000,  200000,  14,  14, {  2,  3,                     }, },
    { REG_DOMAIN_PH, 2, REG_REGIONAL_NONE,   0,  915400000,  400000,   7,   7, {  5,  6,  8,                 }, },
    { REG_DOMAIN_PH, 0, REG_REGIONAL_NONE,  48,  915200000,  200000,  64,  14, {  2,  3,  18, 19             }, .chan_allowed = "0-13", },
    { REG_DOMAIN_PH, 0, REG_REGIONAL_NONE,  49,  915400000,  400000,  32,   7, {  5,  6,  8,  21, 22, 24     }, .chan_allowed = "0-6", },
    { REG_DOMAIN_SG, 1, REG_REGIONAL_NONE,   0,  866100000,  100000,  29,  29, {  1,                         }, },
    { REG_DOMAIN_SG, 2, REG_REGIONAL_NONE,   0,  866100000,  200000,  15,  15, {  3,  5,                     }, },
    { REG_DOMAIN_SG, 3, REG_REGIONAL_NONE,   0,  866300000,  400000,   7,   7, {  6,  8,                     }, },
    { REG_DOMAIN_SG, 0, REG_REGIONAL_NONE,  32,  863100000,  100000,  69,  29, {  1, 17                      }, .chan_allowed = "30-58", },
    { REG_DOMAIN_SG, 0, REG_REGIONAL_NONE,  33,  863100000,  200000,  35,  15, {  3,  5, 18, 21              }, .chan_allowed = "15-29", },
    { REG_DOMAIN_SG, 0, REG_REGIONAL_NONE,  38,  863500000,  400000,  16,   7, {  6,  8, 22, 24              }, .chan_allowed = "7-13", },
    { REG_DOMAIN_SG, 4, REG_REGIONAL_NONE,  64,  920200000,  200000,  24,  24, {  2,  3,                     }, },
    { REG_DOMAIN_SG, 5, REG_REGIONAL_NONE,  65,  920400000,  400000,  12,  12, {  5,  6,  8,                 }, },
    { REG_DOMAIN_TH, 1, REG_REGIONAL_NONE,  64,  920200000,  200000,  24,  24, {  2,  3, 18, 19              }, },
    { REG_DOMAIN_TH, 2, REG_REGIONAL_NONE,  65,  920400000,  400000,  12,  12, {  5,  6,  8, 21, 22, 24      }, },
    { REG_DOMAIN_VN, 1, REG_REGIONAL_NONE,  64,  920200000,  200000,  24,  24, {  2,  3, 18, 19              }, },
    { REG_DOMAIN_VN, 2, REG_REGIONAL_NONE,  65,  920400000,  400000,  12,  12, {  5,  6,  8, 21, 22, 24      }, },
    { REG_DOMAIN_WW, 1, REG_REGIONAL_NONE, 112, 2400200000,  200000, 416, 416, {  2,  3, 18, 19              }, },
    { REG_DOMAIN_WW, 2, REG_REGIONAL_NONE, 113, 2400400000,  400000, 207, 207, {  5,  6,  8, 21, 22, 24      }, },
    { REG_DOMAIN_UNDEF, 0, REG_REGIONAL_NONE, 0,         0,       0,   0,   0, {                             }, },
};

bool ws_regdb_check_phy_chan_compat(const struct phy_params *phy_params, const struct chan_params *chan_params)
{
    int i;

    if (!phy_params || !chan_params)
        return false;

    for (i = 0; chan_params->valid_phy_modes[i]; i++)
        if (chan_params->valid_phy_modes[i] == phy_params->phy_mode_id)
            return true;
    return false;
}

const struct phy_params *ws_regdb_phy_params_from_mode(int operating_mode)
{
    int i;

    for (i = 0; phy_params_table[i].phy_mode_id; i++)
        if (phy_params_table[i].op_mode == operating_mode)
            return &phy_params_table[i];
    return NULL;
}

const struct phy_params *ws_regdb_phy_params_from_id(int phy_mode_id)
{
    int i;

    for (i = 0; phy_params_table[i].phy_mode_id; i++)
        if (phy_params_table[i].phy_mode_id == phy_mode_id)
            return &phy_params_table[i];
    return NULL;
}

const struct phy_params *ws_regdb_phy_params(int phy_mode_id, int operating_mode)
{
    const struct phy_params* phy_params = NULL;
    phy_params = ws_regdb_phy_params_from_id(phy_mode_id);
    if (!phy_params)
        phy_params = ws_regdb_phy_params_from_mode(operating_mode);

    return phy_params;
}

static const struct chan_params *ws_regdb_chan_params_fan1_0(int reg_domain, int operating_class)
{
    int i;

    if (!operating_class)
        return NULL;
    for (i = 0; chan_params_table[i].chan0_freq; i++)
        if (chan_params_table[i].reg_domain == reg_domain &&
            chan_params_table[i].op_class == operating_class)
            return &chan_params_table[i];
    return NULL;
}

static const struct chan_params *ws_regdb_chan_params_fan1_1(int reg_domain, int chan_plan_id)
{
    int i;

    if (!chan_plan_id)
        return NULL;
    for (i = 0; chan_params_table[i].chan0_freq; i++)
        if (chan_params_table[i].reg_domain == reg_domain &&
            chan_params_table[i].chan_plan_id == chan_plan_id)
            return &chan_params_table[i];
    return NULL;
}

const struct chan_params * ws_regdb_chan_params(int reg_domain, int chan_plan_id, int operating_class)
{
    const struct chan_params * chan_params = NULL;

    chan_params = ws_regdb_chan_params_fan1_1(reg_domain, chan_plan_id);
    if (!chan_params)
        chan_params = ws_regdb_chan_params_fan1_0(reg_domain, operating_class);

    return chan_params;
}

const struct chan_params *ws_regdb_chan_params_universal(int chan0_freq, int chan_spacing, int chan_count_valid)
{
    int i;

    for (i = 0; chan_params_table[i].chan0_freq; i++)
        if (chan_params_table[i].chan0_freq == chan0_freq &&
            chan_params_table[i].chan_spacing == chan_spacing &&
            chan_params_table[i].chan_count_valid == chan_count_valid)
            return &chan_params_table[i];
    return NULL;
}

static const struct {
    int val;
    int id;
} chan_spacing_table[] = {
    {  100000, CHANNEL_SPACING_100  },
    {  200000, CHANNEL_SPACING_200  },
    {  250000, CHANNEL_SPACING_250  },
    {  400000, CHANNEL_SPACING_400  },
    {  600000, CHANNEL_SPACING_600  },
    {  800000, CHANNEL_SPACING_800  },
    { 1200000, CHANNEL_SPACING_1200 },
};

int ws_regdb_chan_spacing_id(int val)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(chan_spacing_table); i++)
        if (val == chan_spacing_table[i].val)
            return chan_spacing_table[i].id;
    return CHANNEL_SPACING_UNDEF;
}

bool ws_regdb_is_std(uint8_t reg_domain, uint8_t phy_mode_id)
{
    int i, j;

    for (i = 0; chan_params_table[i].chan0_freq; i++) {
        if (chan_params_table[i].reg_domain == reg_domain) {
            for (j = 0; chan_params_table[i].valid_phy_modes[j]; j++)
                if (chan_params_table[i].valid_phy_modes[j] == phy_mode_id)
                    return true;
        }
    }
    return false;
}
