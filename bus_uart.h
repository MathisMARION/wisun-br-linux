/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2021, Silicon Labs
 * Main authors:
 *    - Jérôme Pouiller <jerome.pouiller@silabs.com>
 */
#ifndef BUS_UART_H
#define BUS_UART_H

#include <stdbool.h>

struct wsbr_ctxt;

int wsbr_uart_open(const char *device, int bitrate, bool hardflow);
int wsbr_uart_tx(struct wsbr_ctxt *ctxt, const void *buf, unsigned int len);
int wsbr_uart_rx(struct wsbr_ctxt *ctxt, void *buf, unsigned int len);

#endif

