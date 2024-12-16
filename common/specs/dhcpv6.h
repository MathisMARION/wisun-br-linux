/*
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
 */

#ifndef DHCPV6_H
#define DHCPV6_H

// RFC3315 - Section 5.2
// https://datatracker.ietf.org/doc/html/rfc3315#section-5.2
#define DHCPV6_CLIENT_UDP_PORT 546
#define DHCPV6_SERVER_UDP_PORT 547

// RFC3315 - Section 24.2
// https://datatracker.ietf.org/doc/html/rfc3315#section-24.2
#define DHCPV6_MSG_SOLICIT      1
#define DHCPV6_MSG_ADVERT       2  /* Unused */
#define DHCPV6_MSG_REQUEST      3  /* Unused */
#define DHCPV6_MSG_CONFIRM      4  /* Unused */
#define DHCPV6_MSG_RENEW        5  /* Unused */
#define DHCPV6_MSG_REBIND       6  /* Unused */
#define DHCPV6_MSG_REPLY        7
#define DHCPV6_MSG_RELEASE      8  /* Unused */
#define DHCPV6_MSG_DECLINE      9  /* Unused */
#define DHCPV6_MSG_RECONFIGURE  10 /* Unused */
#define DHCPV6_MSG_INFO_REQUEST 11 /* Unused */
#define DHCPV6_MSG_RELAY_FWD    12
#define DHCPV6_MSG_RELAY_REPLY  13

// RFC3315 - Section 24.3
// https://datatracker.ietf.org/doc/html/rfc3315#section-24.3
#define DHCPV6_OPT_CLIENT_ID                  0x0001
#define DHCPV6_OPT_SERVER_ID                  0x0002
#define DHCPV6_OPT_IA_NA                      0x0003
#define DHCPV6_OPT_IA_TA                      0x0004 /* Unused */
#define DHCPV6_OPT_IA_ADDRESS                 0x0005
#define DHCPV6_OPT_ORO                        0x0006 /* Unused */
#define DHCPV6_OPT_PREFERENCE                 0x0007 /* Unused */
#define DHCPV6_OPT_ELAPSED_TIME               0x0008
#define DHCPV6_OPT_RELAY                      0x0009
#define DHCPV6_OPT_RESERVED1                  0x000a /* Unused */
#define DHCPV6_OPT_AUTH                       0x000b /* Unused */
#define DHCPV6_OPT_UNICAST                    0x000c /* Unused */
#define DHCPV6_OPT_STATUS_CODE                0x000d
#define DHCPV6_OPT_RAPID_COMMIT               0x000e
#define DHCPV6_OPT_USER_CLASS                 0x000f /* Unused */
#define DHCPV6_OPT_VENDOR_CLASS               0x0010 /* Unused */
#define DHCPV6_OPT_VENDOR_SPECIFIC            0x0011 /* Unused */
#define DHCPV6_OPT_INTERFACE_ID               0x0012
#define DHCPV6_OPT_RECONF_MSG                 0x0013 /* Unused */
#define DHCPV6_OPT_RECONF_ACCEPT              0x0014 /* Unused */

// RFC3315 - Section 24.5
// https://datatracker.ietf.org/doc/html/rfc3315#section-24.5
#define DHCPV6_DUID_TYPE_LINK_LAYER_PLUS_TIME 0x0001 /* Unused */
#define DHCPV6_DUID_TYPE_EN                   0x0002 /* Unused */
#define DHCPV6_DUID_TYPE_LINK_LAYER           0x0003
#define DHCPV6_DUID_TYPE_UUID                 0x0004 /* Unused */

// Address Resolution Protocol (ARP) Parameters
// https://www.iana.org/assignments/arp-parameters/arp-parameters.xhtml
#define DHCPV6_DUID_HW_TYPE_IEEE802           0x0006
#define DHCPV6_DUID_HW_TYPE_EUI64             0x001b

#endif
