/*	$NetBSD: citrus_mapper_std_local.h,v 1.2 2003/07/12 15:39:21 tshiozak Exp $	*/

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

#ifndef _CITRUS_MAPPER_STD_H_
#define _CITRUS_MAPPER_STD_H_

#define _CITRUS_MAPPER_STD_MAGIC		"MAPPER\0\0"

#define _CITRUS_MAPPER_STD_SYM_TYPE		"type"
#define _CITRUS_MAPPER_STD_SYM_INFO		"info"
#define _CITRUS_MAPPER_STD_SYM_TABLE	"table"


#define _CITRUS_MAPPER_STD_TYPE_ROWCOL	"rowcol"

struct _citrus_mapper_std_rowcol {
	struct _citrus_region	rc_table;
	int					rc_src_col_bits;
	_citrus_index_t		rc_dst_invalid;
	_citrus_index_t		rc_src_row_begin;
	_citrus_index_t		rc_src_row_end;
	_citrus_index_t		rc_src_col_begin;
	_citrus_index_t		rc_src_col_end;
	_citrus_index_t		rc_src_col_width;
	_citrus_index_t		rc_dst_unit_bits;
	int					rc_oob_mode;
	_citrus_index_t		rc_dst_ilseq;
};

struct _citrus_mapper_std {
	struct _citrus_region	ms_file;
	struct _citrus_db		*ms_db;
	int (*ms_convert)(struct _citrus_mapper_std *__restrict,
			_index_t *__restrict, _index_t, void *__restrict);
	void (*ms_uninit)(struct _citrus_mapper_std *);
	union {
		struct _citrus_mapper_std_rowcol rowcol;
	} u;
#define ms_rowcol			u.rowcol
};

/* prototypes */
__BEGIN_DECLS
int _citrus_mapper_std_mapper_getops(struct _citrus_mapper_ops *, size_t, u_int32_t);
__END_DECLS
#endif /* _CITRUS_MAPPER_STD_H_ */
