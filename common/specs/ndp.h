/*
 * SPDX-License-Identifier: LicenseRef-MSLA
 * Copyright (c) 2024 Silicon Laboratories Inc. (www.silabs.com)
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at [1].  This software is distributed to you in
 * Object Code format and/or Source Code format and is governed by the sections
 * of the MSLA applicable to Object Code, Source Code and Modified Open Source
 * Code. By using this software, you agree to the terms of the MSLA.
 *
 * [1]: https://www.silabs.com/about-us/legal/master-software-license-agreement
 *
 */
#ifndef SPECS_NDP_H
#define SPECS_NDP_H

#include "common/endian.h"

// IPv6 Neighbor Discovery Option Formats
// https://www.iana.org/assignments/icmpv6-parameters/icmpv6-parameters.xhtml#icmpv6-parameters-5
enum {
    NDP_OPT_SLLAO =  1, // Source Link-Layer Address Option
    NDP_OPT_TLLAO =  2, // Target Link-Layer Address Option
    // ...
    NDP_OPT_ARO   = 33, // Address Registration Option
};

// Address Registration Option Status Values
// https://www.iana.org/assignments/icmpv6-parameters/icmpv6-parameters.xhtml#address-registration
enum {
    NDP_ARO_STATUS_SUCCESS   =  0, // Success
    NDP_ARO_STATUS_DUP       =  1, // Duplicate Address
    NDP_ARO_STATUS_NOMEM     =  2, // Neighbor Cache Full
    NDP_ARO_STATUS_MOVED     =  3, // Moved
    NDP_ARO_STATUS_REMOVED   =  4, // Removed
    NDP_ARO_STATUS_VALIDREQ  =  5, // Validation Requested
    NDP_ARO_STATUS_DUPSRC    =  6, // Duplicate Source Address
    NDP_ARO_STATUS_INVALSRC  =  7, // Invalid Source Address
    NDP_ARO_STATUS_INVALTOPO =  8, // Registered Address Topologically Incorrect
    NDP_ARO_STATUS_NOMEMBR   =  9, // 6LBR Registry Saturated
    NDP_ARO_STATUS_VALIDFAIL = 10, // Validation Failed
    NDP_ARO_STATUS_REFRESH   = 11, // Registration Refresh Request
    NDP_ARO_STATUS_INVALREG  = 12, // Invalid Registration
};

// RFC 6778 4.1. Address Registration Option
// RFC 8505 4.1. Extended Address Registration Option (EARO)
struct ndp_opt_earo {
    uint8_t status;
    uint8_t opaque;
    uint8_t flags;
// https://www.iana.org/assignments/icmpv6-parameters/icmpv6-parameters.xhtml#icmpv6-adress-registration-option-flags
#define NDP_MASK_ARO_T 0x01
#define NDP_MASK_ARO_R 0x02
#define NDP_MASK_ARO_I 0x0c
#define NDP_MASK_ARO_P 0x30
    uint8_t tid;
    be16_t  lifetime_minutes;
    be64_t  eui64; // ROVR (only EUI-64 supported)
} __attribute__((packed));

// P-Field Values
// https://www.iana.org/assignments/icmpv6-parameters/icmpv6-parameters.xhtml#p-field-values
enum {
    NDP_ADDR_TYPE_UNICAST   = 0,
    NDP_ADDR_TYPE_MULTICAST = 1,
    NDP_ADDR_TYPE_ANYCAST   = 2,
};

#endif
