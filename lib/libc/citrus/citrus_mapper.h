/*	$NetBSD: citrus_mapper.h,v 1.3 2003/07/12 15:39:19 tshiozak Exp $	*/

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

#ifndef _CITRUS_MAPPER_H_
#define _CITRUS_MAPPER_H_

struct _citrus_mapper_area;

/*
 * ABI version change log
 *   0x00000001
 *     initial version
 */
#define _CITRUS_MAPPER_ABI_VERSION	0x00000001
struct _citrus_mapper_ops {
	uint32_t mo_abi_version;
	int 	(*mo_init)(struct _citrus_mapper_area *__restrict, struct _citrus_mapper *__restrict,
			const char *__restrict, const void *__restrict, size_t,
			struct _citrus_mapper_traits * __restrict, size_t);
	void 	(*mo_uninit)(struct _citrus_mapper *);
	int		(*mo_convert)(struct _citrus_mapper * __restrict, _citrus_index_t * __restrict,
			_citrus_index_t, void * __restrict);
	void	(*mo_init_state)(struct _citrus_mapper * __restrict, void * __restrict);
};

struct _citrus_mapper_traits {
	/* version 0x00000001 */
	size_t					 mt_state_size;
	size_t					 mt_src_max;
	size_t					 mt_dst_max;
};

struct _citrus_mapper {
	struct _citrus_mapper_ops		 *cm_ops;
	void					 *cm_closure;
	struct _citrus_mapper_traits		 *cm_traits;
	_CITRUS_HASH_ENTRY(_citrus_mapper)	 cm_entry;
	int					 cm_refcount;
	char					 *cm_key;
};

/* return values of _citrus_mapper_convert */
#define _CITRUS_MAPPER_CONVERT_SUCCESS		(0)
#define _CITRUS_MAPPER_CONVERT_NONIDENTICAL	(1)
#define _CITRUS_MAPPER_CONVERT_SRC_MORE		(2)
#define _CITRUS_MAPPER_CONVERT_DST_MORE		(3)
#define _CITRUS_MAPPER_CONVERT_ILSEQ		(4)
#define _CITRUS_MAPPER_CONVERT_FATAL		(5)

#endif /* _CITRUS_MAPPER_H_ */
