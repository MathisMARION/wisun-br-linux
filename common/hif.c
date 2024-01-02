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
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "common/iobuf.h"
#include "common/log.h"
#include "common/utils.h"

#include "hif.h"

void hif_push_bool(struct iobuf_write *buf, bool val)
{
    iobuf_push_u8(buf, val);
    TRACE(TR_HIF_EXTRA, "hif tx:     bool: %s", val ? "true" : "false");
}

void hif_push_uint(struct iobuf_write *buf, unsigned int val)
{
    TRACE(TR_HIF_EXTRA, "hif tx:     uint: %u", val);
    do {
        iobuf_push_u8(buf, (val & 0x7F) | 0x80);
        val >>= 7;
    } while (val);
    buf->data[buf->len - 1] &= ~0x80;
}

void hif_push_u8(struct iobuf_write *buf, uint8_t val)
{
    iobuf_push_u8(buf, val);
    TRACE(TR_HIF_EXTRA, "hif tx:       u8: %u", val);
}

void hif_push_i8(struct iobuf_write *buf, int8_t val)
{
    iobuf_push_u8(buf, (uint8_t)val);
    TRACE(TR_HIF_EXTRA, "hif tx:       i8: %i", val);
}

void hif_push_fixed_u8_array(struct iobuf_write *buf, const uint8_t *val, int num)
{
    iobuf_push_data(buf, val, num);
    TRACE(TR_HIF_EXTRA, "hif tx:   u8[%2d]: %s", num,
        tr_bytes(buf->data + buf->len - num, num, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR));
}

void hif_push_u16(struct iobuf_write *buf, uint16_t val)
{
    iobuf_push_le16(buf, val);
    TRACE(TR_HIF_EXTRA, "hif tx:      u16: %u", val);
}

void hif_push_i16(struct iobuf_write *buf, int16_t val)
{
    iobuf_push_le16(buf, (uint16_t)val);
    TRACE(TR_HIF_EXTRA, "hif tx:      i16: %i", val);
}

void hif_push_fixed_u16_array(struct iobuf_write *buf, const uint16_t *val, int num)
{
    for (int i = 0; i < num; i++)
        iobuf_push_le16(buf, val[i]);
    TRACE(TR_HIF_EXTRA, "hif tx:  u16[%2d]: %s", num,
        tr_bytes(buf->data + buf->len - 2 * num, 2 * num, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR));
}

void hif_push_u32(struct iobuf_write *buf, uint32_t val)
{
    iobuf_push_le32(buf, val);
    TRACE(TR_HIF_EXTRA, "hif tx:      u32: %u", val);
}

void hif_push_i32(struct iobuf_write *buf, int32_t val)
{
    iobuf_push_le32(buf, (uint32_t)val);
    TRACE(TR_HIF_EXTRA, "hif tx:      i32: %i", val);
}

void hif_push_fixed_u32_array(struct iobuf_write *buf, const uint32_t *val, int num)
{
    for (int i = 0; i < num; i++)
        iobuf_push_le32(buf, val[i]);
    TRACE(TR_HIF_EXTRA, "hif tx:  u32[%2d]: %s", num,
        tr_bytes(buf->data + buf->len - 4 * num, 4 * num, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR));
}

void hif_push_u64(struct iobuf_write *buf, uint64_t val)
{
    iobuf_push_le64(buf, val);
    TRACE(TR_HIF_EXTRA, "hif tx:      u64: %"PRIu64, val);
}

void hif_push_str(struct iobuf_write *buf, const char *val)
{
    iobuf_push_data(buf, (const uint8_t *)val, strlen(val) + 1);
    TRACE(TR_HIF_EXTRA, "hif tx:   string: %s", val);
}

void hif_push_data(struct iobuf_write *buf, const uint8_t *val, size_t size)
{
    iobuf_push_le16(buf, size);
    iobuf_push_data(buf, val, size);
    TRACE(TR_HIF_EXTRA, "hif tx:     data: %s (%zu bytes)",
        tr_bytes(val, size, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR), size);
}

void hif_push_raw(struct iobuf_write *buf, const uint8_t *val, size_t size)
{
    iobuf_push_data(buf, val, size);
    TRACE(TR_HIF_EXTRA, "hif tx:      raw: %s (%zu bytes)",
        tr_bytes(val, size, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR), size);
}

bool hif_pop_bool(struct iobuf_read *buf)
{
    uint8_t val;

    if (buf->err)
        return false;
    val = iobuf_pop_u8(buf);
    WARN_ON(val != 1 && val != 0);
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:     bool: %s", val ? "true" : "false");
    return val;
}

unsigned int __hif_pop_uint(struct iobuf_read *buf)
{
    unsigned int val = 0;
    uint8_t cur;
    int i = 0;

    do {
        cur = iobuf_pop_u8(buf);
        val |= (cur & 0x7F) << i;
        i += 7;
        if (i > 32) {
            buf->err = true;
            return 0;
        }
    } while (cur & 0x80);
    return val;
}

unsigned int hif_pop_uint(struct iobuf_read *buf)
{
    unsigned int val = __hif_pop_uint(buf);

    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:     uint: %u", val);
    return val;
}

uint8_t hif_pop_u8(struct iobuf_read *buf) {
    uint8_t val = iobuf_pop_u8(buf);

    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:       u8: %u", val);
    return val;
}

int8_t hif_pop_i8(struct iobuf_read *buf)
{
    int8_t val = iobuf_pop_u8(buf);

    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:       i8: %i", val);
    return val;
}

void hif_pop_fixed_u8_array(struct iobuf_read *buf, uint8_t *val, int num)
{
    iobuf_pop_data(buf, val, num);
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:   u8[%2d]: %s", num,
            tr_bytes(iobuf_ptr(buf) - num, num, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR));
}

uint16_t hif_pop_u16(struct iobuf_read *buf)
{
    uint16_t val = iobuf_pop_le16(buf);

    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:      u16: %u", val);
    return val;
}

int16_t hif_pop_i16(struct iobuf_read *buf)
{
    int16_t val = iobuf_pop_le16(buf);

    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:      i16: %i", val);
    return val;
}

void hif_pop_fixed_u16_array(struct iobuf_read *buf, uint16_t *val, int num)
{
    for (int i = 0; i < num; i++)
        val[i] = iobuf_pop_le16(buf);
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:  u16[%2d]: %s", num,
            tr_bytes(iobuf_ptr(buf) - 2 * num, 2 * num, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR));
}

uint32_t hif_pop_u32(struct iobuf_read *buf)
{
    uint32_t val = iobuf_pop_le32(buf);

    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:      u32: %u", val);
    return val;
}

int32_t hif_pop_i32(struct iobuf_read *buf)
{
    int32_t val = iobuf_pop_le32(buf);

    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:      i32: %i", val);
    return val;
}

void hif_pop_fixed_u32_array(struct iobuf_read *buf, uint32_t *val, int num)
{
    for (int i = 0; i < num; i++)
        val[i] = iobuf_pop_le32(buf);
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:  u32[%2d]: %s", num,
            tr_bytes(iobuf_ptr(buf) - 4 * num, 4 * num, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR));
}

uint64_t hif_pop_u64(struct iobuf_read *buf)
{
    uint64_t val = iobuf_pop_le64(buf);

    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:      u64: %"PRIu64, val);
    return val;
}

const char *hif_pop_str(struct iobuf_read *buf)
{
    const char *val = (char *)iobuf_ptr(buf);
    int len = strnlen(val, iobuf_remaining_size(buf));

    if (len == iobuf_remaining_size(buf) && val[len - 1]) {
        buf->err = true;
        return NULL;
    }
    buf->cnt += len + 1;
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:   string: %s", val);
    return val;
}

unsigned int hif_pop_data_ptr(struct iobuf_read *buf, const uint8_t **val)
{
    const uint8_t *ptr;
    unsigned int size = iobuf_pop_le16(buf);

    ptr = iobuf_pop_data_ptr(buf, size);
    if (val)
        *val = ptr;
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:     data: %s (%u bytes)",
            size ? tr_bytes(ptr, size, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR) : "-", size);
    return size;
}

unsigned int hif_pop_data(struct iobuf_read *buf, uint8_t *val, unsigned int val_size)
{
    const uint8_t *ptr;
    unsigned int size;

    size = iobuf_pop_le16(buf);
    WARN_ON(size > val_size, "hif rx: data bigger than buffer");
    ptr = iobuf_pop_data_ptr(buf, size);
    if (val)
        memcpy(val, ptr, MIN(size, val_size));
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:     data: %s (%u bytes)",
            size ? tr_bytes(ptr, size, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR) : "-", size);
    return size;
}

unsigned int hif_pop_raw_ptr(struct iobuf_read *buf, const uint8_t **val)
{
    unsigned int size = iobuf_remaining_size(buf);

    *val = iobuf_pop_data_ptr(buf, size);
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:      raw: %s (%u bytes)",
            size ? tr_bytes(*val, size, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR) : "-", size);
    return size;
}

unsigned int hif_pop_raw(struct iobuf_read *buf, uint8_t *val, unsigned int val_size)
{
    unsigned int size = MIN(iobuf_remaining_size(buf), val_size);

    iobuf_pop_data(buf, val, size);
    if (!buf->err)
        TRACE(TR_HIF_EXTRA, "hif rx:      raw: %s (%u bytes)",
            size ? tr_bytes(val, size, NULL, 128, DELIM_SPACE | ELLIPSIS_STAR) : "-", size);
    return size;
}
