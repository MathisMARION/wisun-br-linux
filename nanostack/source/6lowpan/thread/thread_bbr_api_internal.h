/*
 * Copyright (c) 2016-2019, Pelion and affiliates.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \brief Nanostack BBR internal instance and corresponding methods.
 */

#ifndef THREAD_BBR_API_INTERNAL_H_
#define THREAD_BBR_API_INTERNAL_H_

#include "net_interface.h"
#define thread_bbr_init(interface_id, external_commisssioner_port)
#define thread_bbr_delete(interface_id)
#define thread_bbr_seconds_timer(interface_id, tics)
#define thread_bbr_commissioner_proxy_service_update(interface_id)

#ifdef HAVE_THREAD_BORDER_ROUTER
/**
 * \brief nd proxy and relay service state change
 *
 * \param interface_id current interface id
 */
int thread_bbr_proxy_state_update(int8_t caller_interface_id, int8_t handler_interface_id, bool status);

/**
 * \brief Test if border router routing is enabled
 *
 * \param cur Protocol interface
 */
bool thread_bbr_routing_enabled(protocol_interface_info_entry_t *cur);

/**
 * \brief Inform BBR for network data change
 *
 * \param cur Protocol interface
 */
void thread_bbr_network_data_update_notify(protocol_interface_info_entry_t *cur);

/**
 * \brief Add new nd entry to bbr
 *
 * \param interface_id addr_data_ptr lifetime info
 */
int thread_bbr_nd_entry_add(int8_t interface_id, const uint8_t *addr_data_ptr, uint32_t lifetime, void *info);

/**
 * \brief Add new dua entry to bbr
 *
 * \param interface_id addr_data_ptr lifetime info mleid_ptr
 */
int thread_bbr_dua_entry_add(int8_t interface_id, const uint8_t *addr_data_ptr, uint32_t lifetime, const uint8_t *mleid_ptr);

/**
 * \brief Send na
 *
 * \param interface_id addr_data_ptr lifetime info mleid_ptr
 */
int thread_bbr_na_send(int8_t interface_id, const uint8_t target[static 16]);


#else
#define thread_bbr_proxy_state_update(caller_interface_id , handler_interface_id, status) (NULL)
#define thread_bbr_routing_enabled(cur) false
#define thread_bbr_network_data_update_notify(cur)
#define thread_bbr_nd_entry_add(interface_id, addr_data_ptr, lifetime, info) (0)
#define thread_bbr_dua_entry_add(interface_id, addr_data_ptr, lifetime, mleid_ptr) (0)
#define thread_bbr_na_send(interface_id, target) (0)
#endif //HAVE_THREAD_BORDER_ROUTER


#endif /* THREAD_BORDER_ROUTER_API_INTERNAL_H_ */

