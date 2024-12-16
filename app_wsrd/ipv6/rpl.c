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
#define _GNU_SOURCE
#include <sys/socket.h>
#include <netinet/icmp6.h>
#include <netinet/in.h>

#include "common/bits.h"
#include "common/iobuf.h"
#include "common/log.h"
#include "common/named_values.h"
#include "common/seqno.h"
#include "common/string_extra.h"
#include "common/time_extra.h"
#include "common/memutils.h"
#include "common/specs/icmpv6.h"
#include "common/specs/rpl.h"
#include "app_wsrd/ipv6/rpl_pkt.h"
#include "app_wsrd/ipv6/ipv6.h"
#include "app_wsrd/ipv6/ipv6_addr.h"
#include "rpl.h"

static const struct name_value rpl_codes[] = {
    { "dis",     RPL_CODE_DIS },
    { "dio",     RPL_CODE_DIO },
    { "dao",     RPL_CODE_DAO },
    { "dao-ack", RPL_CODE_DAO_ACK },
    { 0 }
};

static const char *tr_icmp_rpl(uint8_t code)
{
    return val_to_str(code, rpl_codes, "unknown");
}

void rpl_neigh_add(struct ipv6_ctx *ipv6, struct ipv6_neigh *nce)
{
    BUG_ON(nce->rpl_neigh);
    nce->rpl_neigh = zalloc(sizeof(struct rpl_neigh));
    TRACE(TR_RPL, "rpl: neigh add %s", tr_ipv6(nce->ipv6_addr.s6_addr));
}

void rpl_neigh_del(struct ipv6_ctx *ipv6, struct ipv6_neigh *nce)
{
    TRACE(TR_RPL, "rpl: neigh del %s", tr_ipv6(nce->ipv6_addr.s6_addr));
    free(nce->rpl_neigh);
    nce->rpl_neigh = NULL;
}

struct ipv6_neigh *rpl_neigh_pref_parent(struct ipv6_ctx *ipv6)
{
    struct ipv6_neigh *nce;

    // TODO: proper parent selection, currently this returns the 1st in cache
    SLIST_FOREACH(nce, &ipv6->neigh_cache, link)
        if (nce->rpl_neigh)
            return nce;
    return NULL;
}

static void rpl_recv_dio(struct ipv6_ctx *ipv6, const uint8_t *buf, size_t buf_len,
                         const struct in6_addr *src)
{
    const struct rpl_opt_config *config = NULL;
    const struct rpl_opt_prefix *prefix = NULL;
    const struct rpl_dio_base *dio_base;
    const struct rpl_opt *opt;
    bool pref_parent_change;
    struct ipv6_neigh *nce;
    struct in6_addr addr;
    uint8_t eui64[8];
    struct iobuf_read iobuf = {
        .data_size = buf_len,
        .data = buf,
    };

    if (!IN6_IS_ADDR_LINKLOCAL(src)) {
        TRACE(TR_DROP, "drop %-9s: invalid source address", "rpl-dio");
        return;
    }

    dio_base = (const struct rpl_dio_base *)iobuf_pop_data_ptr(&iobuf, sizeof(*dio_base));
    if (!dio_base)
        goto malformed;

    if (FIELD_GET(RPL_MASK_INSTANCE_ID_TYPE, dio_base->instance_id) == RPL_INSTANCE_ID_TYPE_LOCAL) {
        TRACE(TR_DROP, "drop %-9s: unsupported local RPL instance", "rpl-dio");
        goto drop_neigh;
    }
    if (!FIELD_GET(RPL_MASK_DIO_G, dio_base->g_mop_prf)) {
        TRACE(TR_DROP, "drop %-9s: unsupported floating DODAG", "rpl-dio");
        goto drop_neigh;
    }
    if (FIELD_GET(RPL_MASK_DIO_MOP, dio_base->g_mop_prf) != RPL_MOP_NON_STORING) {
        TRACE(TR_DROP, "drop %-9s: unsupported mode of operation", "rpl-dio");
        goto drop_neigh;
    }

    while (iobuf_remaining_size(&iobuf)) {
        opt = (const struct rpl_opt *)iobuf_ptr(&iobuf);
        if (opt->type == RPL_OPT_PAD1) {
            iobuf_pop_u8(&iobuf);
            continue;
        }
        if (!iobuf_pop_data_ptr(&iobuf, sizeof(*opt)))
            goto malformed;
        if (!iobuf_pop_data_ptr(&iobuf, opt->len))
            goto malformed;
        switch (opt->type) {
        case RPL_OPT_PADN:
            continue;
        case RPL_OPT_CONFIG:
            if (opt->len < sizeof(*config))
                goto malformed;
            config = (const struct rpl_opt_config *)(opt + 1);
            break;
        case RPL_OPT_PREFIX:
            if (opt->len < sizeof(*prefix))
                goto malformed;
            if (prefix)
                TRACE(TR_IGNORE, "ignore: rpl-dio multiple prefix options");
            prefix = (const struct rpl_opt_prefix *)(opt + 1);
            if (prefix->prefix_len > 128)
                goto malformed;
            if (!FIELD_GET(RPL_MASK_OPT_PREFIX_R, prefix->flags)) {
                TRACE(TR_DROP, "drop %-9s: unsupported prefix w/o router address", "rpl-dio");
                goto drop_neigh;
            }
            break;
        default:
            TRACE(TR_IGNORE, "ignore: rpl-dio unsupported option %u", opt->type);
            break;
        }
    }
    if (iobuf.err) {
        TRACE(TR_DROP, "drop %-9s: malformed packet", "rpl-dio");
        goto drop_neigh;
    }
    if (!config) {
        TRACE(TR_DROP, "drop %-9s: missing DODAG configuration option", "rpl-dio");
        goto drop_neigh;
    }
    if (ntohs(config->ocp) != RPL_OCP_MRHOF) {
        TRACE(TR_DROP, "drop %-9s: unsupported objective function", "rpl-dio");
        goto drop_neigh;
    }
    if (!prefix) {
        TRACE(TR_DROP, "drop %-9s: missing prefix information option", "rpl-dio");
        goto drop_neigh;
    }

    /*
     *   Wi-SUN FAN 1.1v08 6.2.3.1.4.1 FFN Neighbor Discovery
     * Router Solicitation/Router Advertisement is not used. Router discovery
     * is performed using DIO and DIS messaging.
     *
     * NOTE: Since a NCE is normally created on receipt of an RA packet, it is
     * being created here instead.
     */
    addr = prefix->prefix; // Prevent GCC warning -Waddress-of-packed-member
    nce = ipv6_neigh_get(ipv6, &addr);
    if (!nce) {
        ipv6_addr_conv_iid_eui64(eui64, src->s6_addr + 8);
        nce = ipv6_neigh_add(ipv6, &addr, eui64);
    }

    pref_parent_change = false;
    if (!nce->rpl_neigh) {
        // TODO: parent selection
        if (rpl_neigh_pref_parent(ipv6)) {
            TRACE(TR_DROP, "drop %-9s: parent already selected", "rpl-dio");
            goto drop_neigh;
        }
        rpl_neigh_add(ipv6, nce);
        pref_parent_change = true;
    }

    nce->rpl_neigh->dio_base = *dio_base;
    nce->rpl_neigh->config = *config;
    // TODO: timer for prefix lifetime
    TRACE(TR_RPL, "rpl: neigh set %s rank=%u ",
          tr_ipv6(nce->ipv6_addr.s6_addr), ntohs(dio_base->rank));

    TRACE(TR_RPL, "rpl: select inst-id=%u dodag-ver=%u dodag-id=%s",
          dio_base->instance_id, dio_base->dodag_verno,
          tr_ipv6(dio_base->dodag_id.s6_addr));

    if (pref_parent_change && ipv6->rpl.on_pref_parent_change)
        ipv6->rpl.on_pref_parent_change(ipv6, nce);

    // TODO: filter candidate neighbors according to
    // Wi-SUN FAN 1.1v08 6.2.3.1.6.3 Upward Route Formation

    return;

malformed:
    TRACE(TR_DROP, "drop %-9s: malformed packet", "rpl-dio");
drop_neigh:
    addr = prefix->prefix; // Prevent GCC warning -Waddress-of-packed-member
    nce = ipv6_neigh_get(ipv6, &addr);
    if (nce && nce->rpl_neigh)
        rpl_neigh_del(ipv6, nce);
}

static void rpl_recv_dispatch(struct ipv6_ctx *ipv6, const uint8_t *pkt, size_t size,
                              const struct in6_addr *src, const struct in6_addr *dst)
{
    struct iobuf_read buf = {
        .data_size = size,
        .data      = pkt,
    };
    uint8_t type, code;

    type = iobuf_pop_u8(&buf);
    code = iobuf_pop_u8(&buf);
    iobuf_pop_be16(&buf); // Checksum verified by kernel
    BUG_ON(buf.err);
    BUG_ON(type != ICMPV6_TYPE_RPL);

    TRACE(TR_ICMP, "rx-icmp rpl-%-9s src=%s", tr_icmp_rpl(code), tr_ipv6(src->s6_addr));

    switch (code) {
    case RPL_CODE_DIO:
        rpl_recv_dio(ipv6, iobuf_ptr(&buf), iobuf_remaining_size(&buf), src);
        break;
    default:
        TRACE(TR_DROP, "drop %-9s: unsupported code %u", "rpl", code);
        break;
    }
}

void rpl_recv(struct ipv6_ctx *ipv6)
{
    uint8_t cmsgbuf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
    struct sockaddr_in6 src;
    uint8_t buf[1280];
    struct iovec iov = {
        .iov_base = buf,
        .iov_len  = sizeof(buf),
    };
    struct msghdr msg = {
        .msg_iov        = &iov,
        .msg_iovlen     = 1,
        .msg_name       = &src,
        .msg_namelen    = sizeof(src),
        .msg_control    = cmsgbuf,
        .msg_controllen = sizeof(cmsgbuf),
    };
    struct in6_pktinfo *pktinfo;
    struct cmsghdr *cmsg;
    ssize_t size;

    size = recvmsg(ipv6->rpl.fd, &msg, 0);
    FATAL_ON(size < 0, 2, "%s: recvmsg: %m", __func__);
    if (msg.msg_namelen != sizeof(src) || src.sin6_family != AF_INET6) {
        TRACE(TR_DROP, "drop %-9s: source address not IPv6", "rpl");
        return;
    }
    cmsg = CMSG_FIRSTHDR(&msg);
    BUG_ON(!cmsg);
    BUG_ON(cmsg->cmsg_level != IPPROTO_IPV6);
    BUG_ON(cmsg->cmsg_type  != IPV6_PKTINFO);
    BUG_ON(cmsg->cmsg_len < sizeof(struct in6_pktinfo));
    pktinfo = (struct in6_pktinfo *)CMSG_DATA(cmsg);
    rpl_recv_dispatch(ipv6, iov.iov_base, size,
                      &src.sin6_addr, &pktinfo->ipi6_addr);
}

void rpl_start(struct ipv6_ctx *ipv6)
{
    struct icmp6_filter filter;
    int err;

    ipv6->rpl.fd = socket(PF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    FATAL_ON(ipv6->rpl.fd < 0, 2, "%s: socket: %m", __func__);
    err = setsockopt(ipv6->rpl.fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, (int[1]){ true }, sizeof(int));
    FATAL_ON(err < 0, 2, "%s: setsockopt IPV6_RECVPKTINFO: %m", __func__);
    err = setsockopt(ipv6->rpl.fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (int[1]){ false }, sizeof(int));
    FATAL_ON(err < 0, 2, "%s: setsockopt IPV6_MULTICAST_LOOP: %m", __func__);
    err = setsockopt(ipv6->rpl.fd, SOL_SOCKET, SO_BINDTODEVICE, ipv6->tun.ifname, IF_NAMESIZE);
    FATAL_ON(err < 0, 2, "%s: setsockopt SO_BINDTODEVICE %s: %m", __func__, ipv6->tun.ifname);
    ICMP6_FILTER_SETBLOCKALL(&filter);
    ICMP6_FILTER_SETPASS(ICMPV6_TYPE_RPL, &filter);
    err = setsockopt(ipv6->rpl.fd, IPPROTO_ICMPV6, ICMP6_FILTER, &filter, sizeof(filter));
    FATAL_ON(err < 0, 2, "%s: setsockopt ICMP6_FILTER: %m", __func__);
}
