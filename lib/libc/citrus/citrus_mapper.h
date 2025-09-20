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
	int 	(*mo_init)(struct _citrus_mapper_area *__restrict, struct _citrus_mapper_std *,
			const char *__restrict, const void *__restrict, size_t,
			struct _citrus_mapper_traits * __restrict, size_t);
	void 	(*mo_uninit)(struct _citrus_mapper_std *);
	int		(*mo_convert)(struct _citrus_mapper_std *, _citrus_index_t * __restrict,
			_citrus_index_t, void * __restrict);
	void	(*mo_init_state)(struct _citrus_mapper_std *, void * __restrict);
};

struct _citrus_mapper_traits {
	/* version 0x00000001 */
	size_t					 mt_state_size;
	size_t					 mt_src_max;
	size_t					 mt_dst_max;
};

struct _citrus_mapper {
	struct _citrus_mapper_ops		 	*cm_ops;
	void					 			*cm_closure;
	struct _citrus_mapper_traits		*cm_traits;
	_CITRUS_HASH_ENTRY(_citrus_mapper)	cm_entry;
	int					 				cm_refcount;
	char					 			*cm_key;
};

/* return values of _citrus_mapper_convert */
#define _CITRUS_MAPPER_CONVERT_SUCCESS		(0)
#define _CITRUS_MAPPER_CONVERT_NONIDENTICAL	(1)
#define _CITRUS_MAPPER_CONVERT_SRC_MORE		(2)
#define _CITRUS_MAPPER_CONVERT_DST_MORE		(3)
#define _CITRUS_MAPPER_CONVERT_ILSEQ		(4)
#define _CITRUS_MAPPER_CONVERT_FATAL		(5)

/*
 * _citrus_mapper_convert:
 *	convert an index.
 *	- if the converter supports M:1 converter, the function may return
 *	  _CITRUS_MAPPER_CONVERT_SRC_MORE and the storage pointed by dst
 *	  may be unchanged in this case, although the internal status of
 *	  the mapper is affected.
 *	- if the converter supports 1:N converter, the function may return
 *	  _CITRUS_MAPPER_CONVERT_DST_MORE. In this case, the contiguous
 *	  call of this function ignores src and changes the storage pointed
 *	  by dst.
 *	- if the converter supports M:N converter, the function may behave
 *	  the combination of the above.
 *
 */
static __inline int
_citrus_io_mapper_convert(struct _citrus_mapper * __restrict cm,
			struct _citrus_mapper_std *ms,
		_citrus_index_t *__restrict dst, _citrus_index_t src,
		void *__restrict ps)
{
	_DIAGASSERT(cm && cm->cm_ops && cm->cm_ops->mo_convert && dst);

	return (*cm->cm_ops->mo_convert)(ms, dst, src, ps);
}

/*
 * _citrus_mapper_init_state:
 *	initialize the state.
 */
static __inline void
_citrus_io_mapper_init_state(struct _citrus_mapper * __restrict cm, struct _citrus_mapper_std *ms, void *__restrict ps)
{
	_DIAGASSERT(cm && cm->cm_ops && cm->cm_ops->mo_init_state);

	(*cm->cm_ops->mo_init_state)(ms, ps);
}

static __inline int
_citrus_io_mapper_init(struct _citrus_mapper_area *__restrict ma,
		struct _citrus_mapper *__restrict cm, struct _citrus_mapper_std *ms,
		const char *__restrict curdir, const void *__restrict var,
		size_t lenvar, struct _citrus_mapper_traits *__restrict mt,
		size_t lenmt)
{
	_DIAGASSERT(cm && cm->cm_ops && cm->cm_ops->mo_init);

	return ((*cm->cm_ops->mo_init)(ma, ms, curdir, var, lenvar, mt, lenmt));
}

static __inline void
_citrus_io_mapper_uninit(struct _citrus_mapper *cm, struct _citrus_mapper_std *ms)
{
	_DIAGASSERT(cm && cm->cm_ops && cm->cm_ops->mo_uninit);

	(*cm->cm_ops->mo_uninit)(ms);
}

#endif /* _CITRUS_MAPPER_H_ */
