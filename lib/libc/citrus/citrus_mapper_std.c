/*	$NetBSD: citrus_mapper_std.c,v 1.3 2003/07/12 15:39:20 tshiozak Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_mapper_std.c,v 1.3 2003/07/12 15:39:20 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/endian.h>
#include <sys/queue.h>

#include "citrus_types.h"
#include "citrus_region.h"
#include "citrus_memstream.h"
#include "citrus_bcs.h"
#include "citrus_frune.h"
#include "citrus_mmap.h"
#include "citrus_hash.h"
#include "citrus_mapper.h"
#include "citrus_mapper_std.h"

static int rowcol_convert(struct _citrus_mapper_std * __restrict, _index_t * __restrict, _index_t, void * __restrict);
static int rowcol_init(struct _citrus_mapper_std *);
static int _citrus_mapper_std_mapper_init(struct _citrus_mapper_area *__restrict, struct _citrus_mapper_std *,
			       const char *__restrict, const void *__restrict, size_t, struct _citrus_mapper_traits *__restrict, size_t);
static void _citrus_mapper_std_mapper_uninit(struct _citrus_mapper_std *);
static void _citrus_mapper_std_mapper_init_state(struct _citrus_mapper * __restrict, void * __restrict);
static int _citrus_mapper_std_mapper_convert(struct _citrus_mapper_std *, _index_t * __restrict, _index_t, void * __restrict);

struct _citrus_mapper_ops _citrus_mapper_std_mapper_ops = {
		.mo_abi_version = _CITRUS_MAPPER_ABI_VERSION,
		.mo_init = _citrus_mapper_std_mapper_init,
		.mo_uninit = _citrus_mapper_std_mapper_uninit,
		.mo_convert = _citrus_mapper_std_mapper_convert,
		.mo_init_state = _citrus_mapper_std_mapper_init_state
};

int
_citrus_mapper_std_mapper_getops(struct _citrus_mapper_ops *ops, size_t lenops, u_int32_t expected_version)
{
	return (_citrus_getops(ops, &_citrus_mapper_std_mapper_ops, lenops, _CITRUS_MAPPER_ABI_VERSION, expected_version));
}

/*ARGSUSED*/
static int
rowcol_convert(struct _citrus_mapper_std * __restrict ms,
	       _index_t * __restrict dst, _index_t src,
	       void * __restrict ps)
{
	struct _citrus_mapper_std_rowcol *rc = &ms->ms_rowcol;
	_index_t row, col, idx;
	u_int32_t conv;

	if (rc->rc_src_col_bits == 32) {
		row = 0;
		col = src;
	} else {
		row = src >> rc->rc_src_col_bits;
		col = src & ((1U<<rc->rc_src_col_bits)-1);
	}
	if (row < rc->rc_src_row_begin || row > rc->rc_src_row_end ||
	    col < rc->rc_src_col_begin || col > rc->rc_src_col_end) {
		switch (rc->rc_oob_mode) {
		case _CITRUS_MAPPER_STD_OOB_NONIDENTICAL:
			*dst = rc->rc_dst_invalid;
			return _CITRUS_MAPPER_CONVERT_NONIDENTICAL;
		case _CITRUS_MAPPER_STD_OOB_ILSEQ:
			return _CITRUS_MAPPER_CONVERT_ILSEQ;
		default:
			return _CITRUS_MAPPER_CONVERT_FATAL;
		}
	}

	idx  =
	    (row - rc->rc_src_row_begin)*rc->rc_src_col_width +
	    (col - rc->rc_src_col_begin);

	switch (rc->rc_dst_unit_bits) {
	case 8:
		conv = _citrus_region_peek8(&rc->rc_table, idx);
		break;
	case 16:
		conv = be16toh(_citrus_region_peek16(&rc->rc_table, idx*2));
		break;
	case 32:
		conv = be32toh(_citrus_region_peek32(&rc->rc_table, idx*4));
		break;
	}

	if (conv == rc->rc_dst_invalid) {
		*dst = rc->rc_dst_invalid;
		return _CITRUS_MAPPER_CONVERT_NONIDENTICAL;
	}
	if (conv == rc->rc_dst_ilseq)
		return _CITRUS_MAPPER_CONVERT_ILSEQ;

	*dst = conv;

	return _CITRUS_MAPPER_CONVERT_SUCCESS;
}

static int
rowcol_init(struct _citrus_mapper_std *ms)
{
	int ret;
	struct _citrus_mapper_std_rowcol *rc = &ms->ms_rowcol;
	const struct _citrus_mapper_std_rowcol_info_x *rcx;
	const struct _citrus_mapper_std_rowcol_ext_ilseq_info_x *eix;
	struct _citrus_region r;
	u_int64_t table_size;

	ms->ms_convert = &rowcol_convert;
	ms->ms_uninit = NULL;

	/* get table region */
	ret = _citrus_db_lookup_by_string(ms->ms_db, _CITRUS_MAPPER_STD_SYM_TABLE,
			      &rc->rc_table, NULL);
	if (ret) {
		if (ret==ENOENT)
			ret = EFTYPE;
		return ret;
	}

	/* get table information */
	ret = _citrus_db_lookup_by_string(ms->ms_db, _CITRUS_MAPPER_STD_SYM_INFO, &r, NULL);
	if (ret) {
		if (ret==ENOENT)
			ret = EFTYPE;
		return ret;
	}
	if (_citrus_region_size(&r) < sizeof(*rcx))
		return EFTYPE;
	rcx = _citrus_region_head(&r);

	/* convert */
#define CONV_ROWCOL(rc, rcx, elem)			\
do {							\
	(rc)->rc_##elem = be32toh((rcx)->rcx_##elem);	\
} while (/*CONSTCOND*/0)
	CONV_ROWCOL(rc, rcx, src_col_bits);
	CONV_ROWCOL(rc, rcx, dst_invalid);
	CONV_ROWCOL(rc, rcx, src_row_begin);
	CONV_ROWCOL(rc, rcx, src_row_end);
	CONV_ROWCOL(rc, rcx, src_col_begin);
	CONV_ROWCOL(rc, rcx, src_col_end);
	CONV_ROWCOL(rc, rcx, dst_unit_bits);

	/* ilseq extension */
	rc->rc_oob_mode = _CITRUS_MAPPER_STD_OOB_NONIDENTICAL;
	rc->rc_dst_ilseq = rc->rc_dst_invalid;
	ret = _citrus_db_lookup_by_string(ms->ms_db,
			      _CITRUS_MAPPER_STD_SYM_ROWCOL_EXT_ILSEQ,
			      &r, NULL);
	if (ret && ret != ENOENT)
		return ret;
	if (_citrus_region_size(&r) < sizeof(*eix))
		return EFTYPE;
	if (ret == 0) {
		eix = _citrus_region_head(&r);
		rc->rc_oob_mode = be32toh(eix->eix_oob_mode);
		rc->rc_dst_ilseq = be32toh(eix->eix_dst_ilseq);
	}
	rc->rc_src_col_width = rc->rc_src_col_end - rc->rc_src_col_begin +1;

	/* validation checks */
	if (rc->rc_src_col_end < rc->rc_src_col_begin ||
	    rc->rc_src_row_end < rc->rc_src_row_begin ||
	    !(rc->rc_dst_unit_bits==8 || rc->rc_dst_unit_bits==16 ||
	      rc->rc_dst_unit_bits==32) ||
	    !(rc->rc_src_col_bits >= 0 && rc->rc_src_col_bits <= 32))
		return EFTYPE;

	/* calcurate expected table size */
	table_size  = rc->rc_src_row_end - rc->rc_src_row_begin + 1;
	table_size *= rc->rc_src_col_width;
	table_size *= rc->rc_dst_unit_bits/8;

	if (table_size > UINT32_MAX ||
	    _citrus_region_size(&rc->rc_table) < table_size)
		return EFTYPE;

	return 0;
}

typedef int (*initfunc_t)(struct _citrus_mapper_std *);
static struct {
	const char			*t_name;
	initfunc_t			t_init;
} types[] = {
	{
			.t_name = _CITRUS_MAPPER_STD_TYPE_ROWCOL,
			.t_init = &rowcol_init
	},
};
#define NUM_OF_TYPES ((int)(sizeof(types)/sizeof(types[0])))

/*ARGSUSED*/
static int
_citrus_mapper_std_mapper_init(struct _citrus_mapper_area *__restrict ma,
			       struct _citrus_mapper_std * ms,
			       const char * __restrict curdir,
			       const void * __restrict var, size_t lenvar,
			       struct _citrus_mapper_traits * __restrict mt,
			       size_t lenmt)
{
	char path[PATH_MAX];
	const char *type;
	int ret, id;

	/* set traits */
	if (lenmt < sizeof(*mt)) {
		ret = EINVAL;
		goto err0;
	}
	mt->mt_src_max = mt->mt_dst_max = 1;	/* 1:1 converter */
	mt->mt_state_size = 0;			/* stateless */

	/* alloc mapper std structure */
//	ms = malloc(sizeof(*ms));
	if (ms == NULL) {
		ret = errno;
		goto err0;
	}

	/* open mapper file */
	snprintf(path, sizeof(path),
		 "%s/%.*s", curdir, (int)lenvar, (const char *)var);
	ret = _citrus_map_file(&ms->ms_file, path);
	if (ret)
		goto err1;

	ret = _citrus_db_open(&ms->ms_db, &ms->ms_file, _CITRUS_MAPPER_STD_MAGIC, &_citrus_db_hash_std, NULL);
	if (ret)
		goto err2;

	/* get mapper type */
	ret = _citrus_db_lookup_string_by_string(ms->ms_db, _CITRUS_MAPPER_STD_SYM_TYPE, &type, NULL);
	if (ret) {
		if (ret == ENOENT)
			ret = EFTYPE;
		goto err3;
	}
	for (id = 0; id < NUM_OF_TYPES; id++)
		if (_bcs_strcasecmp(type, types[id].t_name) == 0)
			break;

	if (id == NUM_OF_TYPES)
		goto err3;

	/* init the per-type structure */
	ret = (*types[id].t_init)(ms);
	if (ret)
		goto err3;

	//cm->cm_closure = ms;

	return 0;

err3:
	_citrus_db_close(ms->ms_db);
err2:
	_citrus_unmap_file(&ms->ms_file);
err1:
	free(ms);
err0:
	return ret;
}

/*ARGSUSED*/
static void
_citrus_mapper_std_mapper_uninit(struct _citrus_mapper_std *ms)
{
	if (ms->ms_uninit) {
		(*ms->ms_uninit)(ms);
	}
	_citrus_db_close(ms->ms_db);
	_citrus_unmap_file(&ms->ms_file);
	free(ms);
}

/*ARGSUSED*/
static void
_citrus_mapper_std_mapper_init_state(struct _citrus_mapper_std *ms, void * __restrict ps)
{

}

/*ARGSUSED*/
static int
_citrus_mapper_std_mapper_convert(struct _citrus_mapper_std *ms,
				  _index_t * __restrict dst, _index_t src,
				  void * __restrict ps)
{
	_DIAGASSERT(ms->ms_convert != NULL);

	return ((*ms->ms_convert)(ms, dst, src, ps));
}
