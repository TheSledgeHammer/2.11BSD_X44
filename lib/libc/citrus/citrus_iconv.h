/*	$NetBSD: citrus_iconv_local.h,v 1.3 2008/02/09 14:56:20 junyoung Exp $	*/

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

#ifndef _CITRUS_ICONV_H_
#define _CITRUS_ICONV_H_

struct _citrus_iconv_shared;
struct _citrus_iconv_ops;
struct _citrus_iconv;

#define _CITRUS_ICONV_GETOPS_FUNC_BASE(_n_)				            \
int _n_(struct _citrus_iconv_ops *, size_t, uint32_t)

#define _CITRUS_ICONV_GETOPS_FUNC(_n_)					            \
_CITRUS_ICONV_GETOPS_FUNC_BASE(_citrus_##_n_##_iconv_getops)

#define _CITRUS_ICONV_DECLS(_m_)					                \
static int	_citrus_##_m_##_iconv_init_shared			            \
	(struct _citrus_iconv_shared * __restrict,			            \
	 const char * __restrict,					                    \
	 const char * __restrict, const char * __restrict,		        \
	 const void * __restrict, size_t);				                \
static void	_citrus_##_m_##_iconv_uninit_shared			            \
	(struct _citrus_iconv_shared *);				                \
static int	_citrus_##_m_##_iconv_convert				            \
	(struct _citrus_iconv * __restrict,				                \
	 const char * __restrict * __restrict, size_t * __restrict,	    \
	 char * __restrict * __restrict, size_t * __restrict outbytes,	\
	 uint32_t, size_t * __restrict);				                \
static int	_citrus_##_m_##_iconv_init_context			            \
	(struct _citrus_iconv *);					                    \
static void	_citrus_##_m_##_iconv_uninit_context			        \
	(struct _citrus_iconv *)

#define _CITRUS_ICONV_DEF_OPS(_m_)					                \
struct _citrus_iconv_ops _citrus_##_m_##_iconv_ops = {			    \
	/* io_abi_version */	_CITRUS_ICONV_ABI_VERSION,		        \
	/* io_init_shared */	&_citrus_##_m_##_iconv_init_shared,	    \
	/* io_uninit_shared */	&_citrus_##_m_##_iconv_uninit_shared,	\
	/* io_init_context */	&_citrus_##_m_##_iconv_init_context,	\
	/* io_uninit_context */	&_citrus_##_m_##_iconv_uninit_context,	\
	/* io_convert */	    &_citrus_##_m_##_iconv_convert		    \
}

typedef _CITRUS_ICONV_GETOPS_FUNC_BASE((*_citrus_iconv_getops_t));
typedef	int	(*_citrus_iconv_init_shared_t)
	(struct _citrus_iconv_shared * __restrict,
	 const char * __restrict, const char * __restrict,
	 const char * __restrict, const void * __restrict, size_t);
typedef void	(*_citrus_iconv_uninit_shared_t)
	(struct _citrus_iconv_shared *);
typedef int	(*_citrus_iconv_convert_t)
	(struct _citrus_iconv * __restrict,
	 const char *__restrict* __restrict, size_t * __restrict,
	 char * __restrict * __restrict, size_t * __restrict, uint32_t,
	 size_t * __restrict);
typedef int	(*_citrus_iconv_init_context_t)(struct _citrus_iconv *);
typedef void	(*_citrus_iconv_uninit_context_t)(struct _citrus_iconv *);

__BEGIN_DECLS
int	_citrus_iconv_open(struct _citrus_iconv * __restrict * __restrict,
			   const char * __restrict,
			   const char * __restrict, const char * __restrict);
void _citrus_iconv_close(struct _citrus_iconv *);
__END_DECLS

struct _citrus_iconv_ops {
	uint32_t			            io_abi_version;
	_citrus_iconv_init_shared_t	    io_init_shared;
	_citrus_iconv_uninit_shared_t	io_uninit_shared;
	_citrus_iconv_init_context_t	io_init_context;
	_citrus_iconv_uninit_context_t	io_uninit_context;
	_citrus_iconv_convert_t		    io_convert;
};

#define _CITRUS_ICONV_ABI_VERSION	2
struct _citrus_iconv_shared {
	struct _citrus_iconv_ops 					*ci_ops;
	void					 					*ci_closure;
	/* private */
	_CITRUS_HASH_ENTRY(_citrus_iconv_shared)	ci_hash_entry;
	TAILQ_ENTRY(_citrus_iconv_shared)			ci_tailq_entry;
	unsigned int								ci_used_count;
	char										*ci_convname;
};

struct _citrus_iconv {
	struct _citrus_iconv_shared 				*cv_shared;
	void					 					*cv_closure;
};

#define _CITRUS_ICONV_F_HIDE_INVALID	0x0001

static __inline int
_citrus_iconv_init_shared(struct _citrus_iconv_shared *__restrict ci,
		const char *__restrict curdir,
		const char *__restrict src, const char *__restrict dst,
		const void *__restrict var, size_t lenvar)
{
	_DIAGASSERT(ci && ci->ci_ops && ci->ci_ops->io_init_shared);

	return ((*ci->ci_ops->io_init_shared)(ci, curdir, src, dst, var, lenvar));
}

static __inline void
_citrus_iconv_uninit_shared(struct _citrus_iconv_shared *ci)
{
	_DIAGASSERT(ci && ci->ci_ops && ci->ci_ops->io_uninit_shared);

	(*ci->ci_ops->io_uninit_shared)(ci);
}

static __inline int
_citrus_iconv_convert(struct _citrus_iconv * __restrict cv,
		const char *__restrict* __restrict in, size_t *__restrict inbytes,
		char *__restrict * __restrict out, size_t *__restrict outbytes,
		u_int32_t flags, size_t *__restrict invalids)
{
	_DIAGASSERT(
			cv && cv->cv_shared && cv->cv_shared->ci_ops
					&& cv->cv_shared->ci_ops->io_convert);
	_DIAGASSERT(out || outbytes == 0);

	return ((*cv->cv_shared->ci_ops->io_convert)(cv, in, inbytes, out, outbytes, flags, invalids));
}

static __inline int
_citrus_iconv_init_context(struct _citrus_iconv *cv)
{
	_DIAGASSERT(
			cv && cv->cv_shared && cv->cv_shared->ci_ops
					&& cv->cv_shared->ci_ops->io_init_context);

	return ((*cv->cv_shared->ci_ops->io_init_context)(cv));
}

static __inline void
_citrus_iconv_uninit_context(struct _citrus_iconv *cv)
{
	_DIAGASSERT(
			cv && cv->cv_shared && cv->cv_shared->ci_ops
					&& cv->cv_shared->ci_ops->io_uninit_context);

	(*cv->cv_shared->ci_ops->io_uninit_context)(cv);
}

#endif /* _CITRUS_ICONV_H_ */
