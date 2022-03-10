/*
 * Copyright (c) 2018-2019, Pelion and affiliates.
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
#include "mbed-client-libservice/common_functions.h"

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define final(a,b,c) \
{ \
    c ^= b; c -= rot(b, 14); \
    a ^= c; a -= rot(c, 11); \
    b ^= a; b -= rot(a, 25); \
    c ^= b; c -= rot(b, 16); \
    a ^= c; a -= rot(c, 4); \
    b ^= a; b -= rot(a, 14); \
    c ^= b; c -= rot(b, 24); \
}

#define mix(a,b,c) \
{ \
    a -= c; a ^= rot(c, 4); c += b; \
    b -= a; b ^= rot(a, 6); a += c; \
    c -= b; c ^= rot(b, 8); b += a; \
    a -= c; a ^= rot(c, 16); c += b; \
    b -= a; b ^= rot(a, 19); a += c; \
    c -= b; c ^= rot(b, 4); b += a; \
}

static uint32_t global_seed = 1;


uint16_t tr51_calc_nearest_prime_number(uint16_t start_value)
{
    if (start_value < 2) {
        return 0;
    }
    uint16_t divider = start_value - 1;
    while (start_value) {
        if (start_value % divider--) {
            if (divider == 1) {
                break;
            }
        } else {
            divider = ++start_value - 1;
        }
    }
    return start_value;
}

static void tr51_seed_rand(uint32_t seed)
{
    if (!seed) {
        seed = 1;
    }
    global_seed = seed;
}

static int32_t tr51_get_rand(void)
{
    uint32_t random_val = ((global_seed * 1103515245) + 12345) & 0x7fffffff;
    global_seed = random_val;
    return random_val;
}

/**
 * @brief Calculate channel table based on TR51 channel function.
 * @param number_of_channels Number of channels in table.
 * @param nearest_prime Nearest prime number. Must be equal to or larger than number_of_channels.
 * @param channel_table Output channel table. Has to be at least nearest_prime in length.
 */
static void tr51_calculate_channel_table(uint16_t number_of_channels, uint16_t nearest_prime, int16_t *channel_table)
{
    int32_t i, j, k;
    tr51_seed_rand(1);
    for (i = 0; i < nearest_prime; i++) {
        channel_table[i] = -1;
    }
    for (i = 0; i < number_of_channels; i++) {
        j = tr51_get_rand() % number_of_channels;
        k = 0;
        while (k <= i) {
            if (j == channel_table[k]) {
                j = tr51_get_rand() % number_of_channels;
                k = 0;
            } else {
                k = k + 1;
            }
        }
        channel_table[i] = j;
    }
}

static void tr51_compute_cfd(uint8_t *mac, uint8_t *first_element, uint8_t *step_size, uint16_t channel_table_length)
{
    *first_element = (mac[5] ^ mac[6] ^ mac[7]) % channel_table_length;
    *step_size = (mac[7] % (channel_table_length - 1)) + 1;
}

static uint8_t tr51_find_excluded(int32_t channel, uint32_t *excluded_channels)
{
    if (excluded_channels != NULL) {
        uint8_t index = channel / 32;
        channel %= 32;
        if (excluded_channels[index] & ((uint32_t)1 << channel)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Calculate hopping sequence for a specific peer using tr51 channel function.
 * @param channel_table Used channel table.
 * @param channel_table_length Length of the used channel table.
 * @param first_element Start generated by CFD function.
 * @param step_size Step size generated by CFD function.
 * @param output_table Output hopping sequence table.
 * @param excluded_channels Bit mask where excluded channels are set to 1.
 * @return Number of channels in sequence.
 */
static uint16_t tr51_calculate_hopping_sequence(int16_t *channel_table, uint16_t channel_table_length, uint8_t first_element, uint8_t step_size, uint8_t *output_table, uint32_t *excluded_channels)
{
    uint16_t cntr = channel_table_length;
    uint8_t index = first_element;
    uint8_t slot = 0;
    while (cntr--) {
        if (channel_table[index] != -1) {
            if (tr51_find_excluded(channel_table[index], excluded_channels) == false) {
                output_table[slot] = channel_table[index];
                slot++;
            }
        }
        index += step_size;
        index %= channel_table_length;
    }
    return slot;
}

static uint32_t dh1cf_hashword(const uint32_t *key, size_t key_length, uint32_t init_value)
{
    uint32_t a, b, c;
    a = b = c = 0xdeadbeef + (((uint32_t)key_length) << 2) + init_value;
    while (key_length > 3) {
        a += key[0];
        b += key[1];
        c += key[2];
        mix(a, b, c);
        key_length -= 3;
        key += 3;
    }
    switch (key_length) {
        case 3:
            c += key[2];
        /* fall through */
        case 2:
            b += key[1];
        /* fall through */
        case 1:
            a += key[0];
            final(a, b, c);
        /* fall through */
        case 0:
            break;
    }
    return c;
}

int32_t dh1cf_get_uc_channel_index(uint16_t slot_number, uint8_t *mac, int16_t number_of_channels)
{
    int32_t channel_number;
    uint32_t key[3];
    key[0] = (uint32_t) slot_number;
    key[1] = common_read_32_bit(&mac[4]);
    key[2] = common_read_32_bit(&mac[0]);
    channel_number = dh1cf_hashword(key, 3, 0) % number_of_channels;
    return channel_number;
}

int32_t dh1cf_get_bc_channel_index(uint16_t slot_number, uint16_t bsi, int16_t number_of_channels)
{
    int32_t channel_number;
    uint32_t key[3];
    key[0] = (uint32_t) slot_number;
    key[1] = (uint32_t) bsi << 16;
    key[2] = 0;
    channel_number = dh1cf_hashword(key, 3, 0) % number_of_channels;
    return channel_number;
}

int tr51_init_channel_table(int16_t *channel_table, int16_t number_of_channels)
{
    uint16_t nearest_prime = tr51_calc_nearest_prime_number(number_of_channels);
    tr51_calculate_channel_table(number_of_channels, nearest_prime, channel_table);
    return 0;
}

int32_t tr51_get_uc_channel_index(int16_t *channel_table, uint8_t *output_table, uint16_t slot_number, uint8_t *mac, int16_t number_of_channels, uint32_t *excluded_channels)
{
    uint16_t nearest_prime = tr51_calc_nearest_prime_number(number_of_channels);
    uint8_t first_element;
    uint8_t step_size;
    tr51_compute_cfd(mac, &first_element, &step_size, nearest_prime);
    tr51_calculate_hopping_sequence(channel_table, nearest_prime, first_element, step_size, output_table, excluded_channels);
    return output_table[slot_number];
}

int32_t tr51_get_bc_channel_index(int16_t *channel_table, uint8_t *output_table, uint16_t slot_number, uint16_t bsi, int16_t number_of_channels, uint32_t *excluded_channels)
{
    uint16_t nearest_prime = tr51_calc_nearest_prime_number(number_of_channels);
    uint8_t mac[8] = {0, 0, 0, 0, 0, 0, (uint8_t)(bsi >> 8), (uint8_t)bsi};
    uint8_t first_element;
    uint8_t step_size;
    tr51_compute_cfd(mac, &first_element, &step_size, nearest_prime);
    tr51_calculate_hopping_sequence(channel_table, nearest_prime, first_element, step_size, output_table, excluded_channels);
    return output_table[slot_number];
}
