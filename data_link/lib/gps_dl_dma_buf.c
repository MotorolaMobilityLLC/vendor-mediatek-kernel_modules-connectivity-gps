/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include "gps_dl_config.h"
#include "gps_dl_context.h"

#include "gps_dl_dma_buf.h"

#define GDL_COUNT_FREE(r, w, l)\
	((w >= r) ? (l + r - w) : (r - w))

#define GDL_COUNT_DATA(r, w, l)\
	((w >= r) ? (w - r) : (l + w - r))

void gps_dma_buf_reset(struct gps_dl_dma_buf *p_dma)
{
	GDL_LOGXD(p_dma->dev_index, "dir = %d", p_dma->dir);
	p_dma->read_index = 0;
	p_dma->reader_working = 0;
	p_dma->write_index = 0;
	p_dma->writer_working = 0;
	p_dma->dma_working_entry.is_valid = false;
}

void gps_dma_buf_show(struct gps_dl_dma_buf *p_dma)
{
	GDL_LOGXD(p_dma->dev_index, "dir = %d, l = %d, r = %d(%d), w = %d(%d)",
		p_dma->dir, p_dma->len,
		p_dma->read_index, p_dma->reader_working,
		p_dma->write_index, p_dma->writer_working);
}

void gps_dma_buf_align_as_byte_mode(struct gps_dl_dma_buf *p_dma)
{
	unsigned int ri, wi;

	ri = p_dma->read_index;
	wi = p_dma->write_index;

	if (!gps_dl_is_1byte_mode()) {
		p_dma->read_index = ((p_dma->read_index + 3) / 4) * 4;
		if (p_dma->read_index >= p_dma->len)
			p_dma->read_index -= p_dma->len;
		p_dma->reader_working = 0;

		p_dma->write_index = ((p_dma->write_index + 3) / 4) * 4;
		if (p_dma->write_index >= p_dma->len)
			p_dma->write_index -= p_dma->len;
		p_dma->writer_working = 0;
		p_dma->dma_working_entry.is_valid = false;
	}

	GDL_LOGD("dma link index = %d, is_1byte = %d, ri:%u -> %u, wi:%u -> %u",
		p_dma->dev_index, gps_dl_is_1byte_mode(),
		ri, p_dma->read_index,
		wi, p_dma->write_index);

	/* clear it anyway */
	p_dma->read_index = p_dma->write_index;
}

#if 0
enum GDL_RET_STATUS gdl_dma_buf_init(struct gps_dl_dma_buf *p_dma_buf)
{
	return GDL_OKAY;
}

enum GDL_RET_STATUS gdl_dma_buf_deinit(struct gps_dl_dma_buf *p_dma_buf)
{
	return GDL_OKAY;
}
#endif

bool gps_dma_buf_is_empty(struct gps_dl_dma_buf *p_dma)
{
	return (p_dma->read_index == p_dma->write_index);
}

enum GDL_RET_STATUS gdl_dma_buf_put(struct gps_dl_dma_buf *p_dma,
	const unsigned char *p_buf, unsigned int buf_len)
{
	struct gdl_dma_buf_entry entry;
	struct gdl_dma_buf_entry *p_entry = &entry;

	/* unsigned int free_len; */
	/* unsigned int wrap_len; */
	enum GDL_RET_STATUS gdl_ret;

	ASSERT_NOT_NULL(p_dma, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_buf, GDL_FAIL_ASSERT);

	gdl_ret = gdl_dma_buf_get_free_entry(p_dma, p_entry);

	if (GDL_OKAY != gdl_ret)
		return gdl_ret;

#if 0
	free_len = GDL_COUNT_FREE(p_entry->read_index,
		p_entry->write_index, p_entry->buf_length);
	GDL_LOGD("r=%u, w=%u, l=%u, f=%u", p_entry->read_index,
		p_entry->write_index, p_entry->buf_length, free_len);

	if (free_len < buf_len) {
		gdl_dma_buf_set_free_entry(p_dma, NULL);
		return GDL_FAIL_NOSPACE;
	}

	wrap_len = p_entry->buf_length - p_entry->write_index;
	if (wrap_len >= buf_len) {
		memcpy(((unsigned char *)p_entry->vir_addr) + p_entry->write_index,
			p_buf, buf_len);

		p_entry->write_index += buf_len;
		if (p_entry->write_index >= p_entry->buf_length)
			p_entry->write_index = 0;
	} else {
		memcpy(((unsigned char *)p_entry->vir_addr) + p_entry->write_index,
			p_buf, wrap_len);

		memcpy(((unsigned char *)p_entry->vir_addr) + 0,
			p_buf + wrap_len, buf_len - wrap_len);

		p_entry->write_index = buf_len - wrap_len;
	}
#endif
	gdl_ret = gdl_dma_buf_buf_to_entry(p_entry, p_buf, buf_len,
		&p_entry->write_index);

	if (GDL_OKAY != gdl_ret)
		return gdl_ret;

	/* TODO: make a data entry */

	GDL_LOGD("new_w=%u", p_entry->write_index);
	gdl_dma_buf_set_free_entry(p_dma, p_entry);

	return GDL_OKAY;
}

enum GDL_RET_STATUS gdl_dma_buf_get(struct gps_dl_dma_buf *p_dma,
	unsigned char *p_buf, unsigned int buf_len, unsigned int *p_data_len,
	bool *p_is_nodata)
{
	struct gdl_dma_buf_entry entry;
	struct gdl_dma_buf_entry *p_entry = &entry;

	/* unsigned int data_len; */
	/* unsigned int wrap_len; */
	enum GDL_RET_STATUS gdl_ret;

	ASSERT_NOT_NULL(p_entry, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_buf, GDL_FAIL_ASSERT);
	ASSERT_NOT_ZERO(buf_len, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_data_len, GDL_FAIL_ASSERT);

	gdl_ret = gdl_dma_buf_get_data_entry(p_dma, p_entry);

	if (GDL_OKAY != gdl_ret)
		return gdl_ret;

#if 0
	data_len = GDL_COUNT_DATA(p_entry->read_index,
		p_entry->write_index, p_entry->buf_length);
	GDL_LOGD("r=%u, w=%u, l=%u, d=%u", p_entry->read_index,
			p_entry->write_index, p_entry->buf_length, data_len);

	/* assert data_len > 0 */

	if (data_len > buf_len) {
		/* TODO: improve it */
		gdl_dma_buf_set_data_entry(p_dma, p_entry);
		return GDL_FAIL_NOSPACE;
	}

	if (p_entry->write_index > p_entry->read_index) {
		memcpy(p_buf, ((unsigned char *)p_entry->vir_addr) + p_entry->read_index,
			data_len);
	} else {
		wrap_len = p_entry->buf_length - p_entry->read_index;

		memcpy(p_buf, ((unsigned char *)p_entry->vir_addr) + p_entry->read_index,
			wrap_len);

		memcpy(p_buf + wrap_len, ((unsigned char *)p_entry->vir_addr) + 0,
			data_len - wrap_len);
	}
#endif
	gdl_ret = gdl_dma_buf_entry_to_buf(p_entry, p_buf, buf_len, p_data_len);

	if (GDL_OKAY != gdl_ret)
		return gdl_ret;

	/* Todo: Case1: buf < data in entry */
	/* Note: we can limit the rx transfer max to 512, then case1 should not be happened */

	/* Todo: Case2: buf > data in entry, need to combine multiple entry until no data entry? */

	if (p_is_nodata)
		*p_is_nodata = p_entry->is_nodata;

	/* *p_data_len = data_len; */
	p_entry->read_index = p_entry->write_index;
	gdl_dma_buf_set_data_entry(p_dma, p_entry);

	return GDL_OKAY;
}

enum GDL_RET_STATUS gdl_dma_buf_get_data_entry(struct gps_dl_dma_buf *p_dma,
	struct gdl_dma_buf_entry *p_entry)
{
	ASSERT_NOT_NULL(p_dma, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_entry, GDL_FAIL_ASSERT);

	if (p_dma->reader_working)
		return GDL_FAIL_BUSY;

	p_dma->reader_working = true;

	if (p_dma->read_index != p_dma->write_index) {
		if ((p_dma->transfer_max > 0) && (p_dma->dir == GDL_DMA_A2D)) {
			p_entry->write_index = p_dma->read_index + p_dma->transfer_max;
			if (p_entry->write_index >= p_dma->len)
				p_entry->write_index -= p_dma->len;
			if (p_entry->write_index > p_dma->write_index)
				p_entry->write_index = p_dma->write_index;
		} else
			p_entry->write_index = p_dma->write_index;
		p_entry->read_index = p_dma->read_index;
		p_entry->buf_length = p_dma->len;
		p_entry->phy_addr = p_dma->phy_addr;
		p_entry->vir_addr = p_dma->vir_addr;
		p_entry->is_nodata = p_dma->is_nodata;
		p_entry->is_valid = true;
		return GDL_OKAY;
	}

	p_dma->reader_working = false;
	return GDL_FAIL_NODATA;
}

enum GDL_RET_STATUS gdl_dma_buf_set_data_entry(struct gps_dl_dma_buf *p_dma,
	struct gdl_dma_buf_entry *p_entry)
{
	ASSERT_NOT_NULL(p_dma, GDL_FAIL_ASSERT);

	if (!p_dma->reader_working)
		return GDL_FAIL_STATE_MISMATCH;

	if (NULL == p_entry) {
		p_dma->reader_working = false;
		return GDL_OKAY;
	}

	p_dma->read_index = p_entry->write_index;
	p_dma->reader_working = false;
	return GDL_OKAY;
}

enum GDL_RET_STATUS gdl_dma_buf_get_free_entry(struct gps_dl_dma_buf *p_dma,
	struct gdl_dma_buf_entry *p_entry)
{
	ASSERT_NOT_NULL(p_dma, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_entry, GDL_FAIL_ASSERT);

	if (p_dma->writer_working)
		return GDL_FAIL_BUSY;

	p_dma->writer_working = true;

	if (p_dma->read_index == p_dma->write_index) {
		if ((p_dma->transfer_max > 0) && (p_dma->dir == GDL_DMA_D2A)) {
			p_entry->read_index = p_dma->read_index + p_dma->transfer_max;
			if (p_entry->read_index >= p_dma->len)
				p_entry->read_index -= p_dma->len;
		} else
			p_entry->read_index = p_dma->read_index;
		p_entry->write_index = p_dma->write_index;
		p_entry->buf_length = p_dma->len;
		p_entry->phy_addr = p_dma->phy_addr;
		p_entry->vir_addr = p_dma->vir_addr;
		p_entry->is_valid = true;

		/* This field not used for free entry, just make static analysis tool happy. */
		p_entry->is_nodata = false;
		return GDL_OKAY;
	}

	p_dma->writer_working = false;

	return GDL_FAIL_NOSPACE;
}

enum GDL_RET_STATUS gdl_dma_buf_set_free_entry(struct gps_dl_dma_buf *p_dma,
	struct gdl_dma_buf_entry *p_entry)
{
	ASSERT_NOT_NULL(p_dma, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_entry, GDL_FAIL_ASSERT);

	if (!p_dma->writer_working)
		return GDL_FAIL_STATE_MISMATCH;

	p_dma->write_index = p_entry->write_index;
	p_dma->writer_working = false;
	p_dma->is_nodata = p_entry->is_nodata;
	return GDL_OKAY;
}

enum GDL_RET_STATUS gdl_dma_buf_entry_to_buf(const struct gdl_dma_buf_entry *p_entry,
	unsigned char *p_buf, unsigned int buf_len, unsigned int *p_data_len)
{
	unsigned int data_len;
	unsigned int wrap_len;

	ASSERT_NOT_NULL(p_entry, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_buf, GDL_FAIL_ASSERT);
	ASSERT_NOT_ZERO(buf_len, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_data_len, GDL_FAIL_ASSERT);

	data_len = GDL_COUNT_DATA(p_entry->read_index,
		p_entry->write_index, p_entry->buf_length);
	GDL_LOGD("r=%u, w=%u, l=%u, d=%u", p_entry->read_index,
			p_entry->write_index, p_entry->buf_length, data_len);

	if (data_len > buf_len) {
		*p_data_len = 0;
		return GDL_FAIL_NOSPACE;
	}

	if (p_entry->write_index > p_entry->read_index) {
		memcpy(p_buf, ((unsigned char *)p_entry->vir_addr) + p_entry->read_index,
			data_len);
	} else {
		wrap_len = p_entry->buf_length - p_entry->read_index;

		memcpy(p_buf, ((unsigned char *)p_entry->vir_addr) + p_entry->read_index,
			wrap_len);

		memcpy(p_buf + wrap_len, ((unsigned char *)p_entry->vir_addr) + 0,
			data_len - wrap_len);
	}

	*p_data_len = data_len;
	return GDL_OKAY;
}

enum GDL_RET_STATUS gdl_dma_buf_buf_to_entry(const struct gdl_dma_buf_entry *p_entry,
	const unsigned char *p_buf, unsigned int data_len, unsigned int *p_write_index)
{
	unsigned int free_len;
	unsigned int wrap_len;
	unsigned int write_index;
	unsigned int alligned_data_len;
	unsigned int fill_zero_len;

	if (gps_dl_is_1byte_mode()) {
		alligned_data_len = data_len;
		fill_zero_len = 0;
	} else {
		alligned_data_len = ((data_len + 3) / 4) * 4;
		fill_zero_len = alligned_data_len - data_len;
		GDL_LOGD("data_len = %u, alligned = %u", data_len, alligned_data_len);
	}

	ASSERT_NOT_NULL(p_entry, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_buf, GDL_FAIL_ASSERT);
	ASSERT_NOT_ZERO(data_len, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_write_index, GDL_FAIL_ASSERT);

	/* TODO: make dma done event */

	free_len = GDL_COUNT_FREE(p_entry->read_index,
		p_entry->write_index, p_entry->buf_length);

	GDL_LOGD("r=%u, w=%u, l=%u, f=%u", p_entry->read_index,
		p_entry->write_index, p_entry->buf_length, free_len);

	if (free_len < alligned_data_len)
		return GDL_FAIL_NOSPACE;

	wrap_len = p_entry->buf_length - p_entry->write_index;
	if (wrap_len >= data_len) {
		memcpy(((unsigned char *)p_entry->vir_addr) + p_entry->write_index,
			p_buf, data_len);

		write_index = p_entry->write_index + data_len;
		if (write_index >= p_entry->buf_length)
			write_index = 0;
	} else {
		memcpy(((unsigned char *)p_entry->vir_addr) + p_entry->write_index,
			p_buf, wrap_len);

		memcpy(((unsigned char *)p_entry->vir_addr) + 0,
			p_buf + wrap_len, data_len - wrap_len);

		write_index = data_len - wrap_len;
	}

	/* fill it to allignment */
	if (fill_zero_len >= 0) {
		wrap_len = p_entry->buf_length - write_index;
		if (wrap_len >= fill_zero_len) {
			memset(((unsigned char *)p_entry->vir_addr) + write_index,
				0, fill_zero_len);

			write_index += fill_zero_len;
			if (write_index >= p_entry->buf_length)
				write_index = 0;
		} else {
			memset(((unsigned char *)p_entry->vir_addr) + write_index,
				0, wrap_len);
			memset(((unsigned char *)p_entry->vir_addr) + 0,
				0, fill_zero_len - wrap_len);

			write_index = fill_zero_len - wrap_len;
		}
	}

	GDL_LOGD("new_w=%u", write_index);
	*p_write_index = write_index;

	return GDL_OKAY;
}

enum GDL_RET_STATUS gdl_dma_buf_entry_to_transfer(
	const struct gdl_dma_buf_entry *p_entry,
	struct gdl_hw_dma_transfer *p_transfer, bool is_tx)
{
	ASSERT_NOT_NULL(p_entry, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_transfer, GDL_FAIL_ASSERT);

	p_transfer->buf_start_addr = (unsigned int)p_entry->phy_addr;
	if (is_tx) {
		p_transfer->transfer_start_addr =
			(unsigned int)p_entry->phy_addr + p_entry->read_index;
		p_transfer->len_to_wrap = p_entry->buf_length - p_entry->read_index;
		p_transfer->transfer_max_len = GDL_COUNT_DATA(
			p_entry->read_index, p_entry->write_index, p_entry->buf_length);

		if (!gps_dl_is_1byte_mode()) {
			p_transfer->len_to_wrap /= 4;
			p_transfer->transfer_max_len /= 4;
		}
	} else {
		p_transfer->transfer_start_addr =
			(unsigned int)p_entry->phy_addr + p_entry->write_index;
		p_transfer->len_to_wrap = p_entry->buf_length - p_entry->write_index;
		p_transfer->transfer_max_len = GDL_COUNT_FREE(
			p_entry->read_index, p_entry->write_index, p_entry->buf_length);

		if (!gps_dl_is_1byte_mode()) {
			p_transfer->len_to_wrap /= 4;
			p_transfer->transfer_max_len /= 4;
		}
	}

	GDL_LOGD("r=%u, w=%u, l=%u, is_tx=%d, transfer: ba=0x%08x, ta=0x%08x, wl=%d, tl=%d",
		p_entry->read_index, p_entry->write_index, p_entry->buf_length, is_tx,
		p_transfer->buf_start_addr, p_transfer->buf_start_addr,
		p_transfer->len_to_wrap, p_transfer->transfer_max_len);

	ASSERT_NOT_ZERO(p_transfer->buf_start_addr, GDL_FAIL_ASSERT);
	ASSERT_NOT_ZERO(p_transfer->transfer_start_addr, GDL_FAIL_ASSERT);
	ASSERT_NOT_ZERO(p_transfer->len_to_wrap, GDL_FAIL_ASSERT);
	ASSERT_NOT_ZERO(p_transfer->transfer_max_len, GDL_FAIL_ASSERT);

	return GDL_OKAY;
}

enum GDL_RET_STATUS gdl_dma_buf_entry_transfer_left_to_write_index(
	const struct gdl_dma_buf_entry *p_entry,
	unsigned int left_len, unsigned int *p_write_index)
{
	unsigned int free_len;
	unsigned int new_write_index;

	ASSERT_NOT_NULL(p_entry, GDL_FAIL_ASSERT);
	ASSERT_NOT_NULL(p_write_index, GDL_FAIL_ASSERT);

	free_len = GDL_COUNT_FREE(p_entry->read_index,
		p_entry->write_index, p_entry->buf_length);

	GDL_ASSERT(free_len > left_len, GDL_FAIL_ASSERT, "");

	new_write_index = p_entry->write_index + free_len - left_len;
	if (new_write_index >= p_entry->buf_length)
		new_write_index -= p_entry->buf_length;

	GDL_LOGD("r=%u, w=%u, l=%u, left=%d, new_w=%d",
		p_entry->read_index, p_entry->write_index, p_entry->buf_length,
		left_len, new_write_index);
	GDL_ASSERT(new_write_index < p_entry->buf_length, GDL_FAIL_ASSERT, "");

	*p_write_index = new_write_index;

	return GDL_OKAY;
}


