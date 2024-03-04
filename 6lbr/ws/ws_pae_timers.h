/*
 * Copyright (c) 2019-2020, Pelion and affiliates.
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

#ifndef WS_PAE_TIMERS_H_
#define WS_PAE_TIMERS_H_
#include <stdint.h>
#include <stdbool.h>

#include "security/protocols/sec_prot_cfg.h"

/**
 * ws_pae_timers_settings_init initializes timer settings structure
 *
 * \param timer_settings timer settings
 * \param new_timer_settings new timer settings
 *
 */
void ws_pae_timers_settings_init(sec_timer_cfg_t *timer_settings, const struct sec_timer_cfg *new_timer_settings);

#endif
