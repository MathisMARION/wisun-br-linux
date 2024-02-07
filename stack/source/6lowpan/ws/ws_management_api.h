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

/**
 * \file ws_management_api.h
 * \brief Wi-SUN management interface.
 *
 * This interface is used for configuring Wi-SUN devices.
 * After creating the Wi-SUN interface, you can use this interface to configure the Wi-SUN device
 * behaviour. When you are done with the configurations, you need to call interface up to enable a Wi-SUN node.
 *
 */
#ifndef WS_MANAGEMENT_API_H_
#define WS_MANAGEMENT_API_H_
#include <stdint.h>
#include "common/int24.h"

/*
 *  Network Size definitions are device amount in hundreds of devices.
 *  These definitions are meant to give some estimates of sizes. Any value can be given as parameter
 */

#define NETWORK_SIZE_CERTIFICATE    0x00    /**< Network configuration used in Wi-SUN certification */
#define NETWORK_SIZE_SMALL          0x01    /**< Small networks */
#define NETWORK_SIZE_MEDIUM         0x08    /**< 100 - 800 device networks are medium sized */
#define NETWORK_SIZE_LARGE          0x0F    /**< 800 - 1500 device networks are large */
#define NETWORK_SIZE_XLARGE         0x19    /**< 2500+ devices */
#define NETWORK_SIZE_AUTOMATIC      0xFF    /**< Automatic network size */

/**
 * Initialize Wi-SUN stack.
 *
 * Generates the default configuration for Wi-SUN operation
 *
 * \param interface_id Network interface ID.
 * \param regulatory_domain Mandatory regulatory domain value of the device.
 *
 * \return 0, Init OK.
 * \return <0 Init fail.
 */
int ws_management_node_init(
    int8_t interface_id,
    uint8_t regulatory_domain);


/**
 * Set timing parameters related to network size.
 *
 * timing parameters follows the specification example from Wi-SUN specification
 *
 * Default value: medium 100 - 800 device
 * small network size: less than 100 devices
 * Large network size: 800 - 1500 devices
 * automatic: when discovering the network network size is learned
 *            from advertisements and timings adjusted accordingly
 *
 * When network size is changed, it will override following configuration values:
 * - Timing settings set by ws_management_timing_parameters_set()
 *
 * If values should be other than defaults set by stack, they need to set using
 * above function calls after network size change.
 *
 * \param interface_id Network interface ID.
 * \param network_size Network size in hundreds of devices, certificate or automatic.
 *                     See NETWORK_SIZE_ definition.
 *
 * \return 0, Init OK.
 * \return <0 Init fail.
 */
int ws_management_network_size_set(
    int8_t interface_id,
    uint8_t network_size);

#endif
