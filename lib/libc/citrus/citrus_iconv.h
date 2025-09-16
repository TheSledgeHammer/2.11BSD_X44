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

struct _citrus_iconv_ops {
	int (*io_init_shared)(struct _citrus_iconv_std_shared *,
			const char *__restrict, const char *__restrict, const char *__restrict,
			const void *__restrict, size_t);
	void (*io_uninit_shared)(struct _citrus_iconv_std_shared *);
	int (*io_convert)(const struct _citrus_iconv_std_shared *,
			struct _citrus_iconv_std_context *, const char *__restrict *__restrict,
			size_t *__restrict, char *__restrict *__restrict, size_t *__restrict,
			u_int32_t, size_t *__restrict);
	int (*io_init_context)(struct _citrus_iconv_std_shared *, struct _citrus_iconv_std_context *);
	void (*io_uninit_context)(struct _citrus_iconv_std_shared *);
};

struct _citrus_iconv_shared {
	struct _citrus_iconv_ops *ci_ops;
	void					 *ci_closure;
};

struct _citrus_iconv {
	struct _citrus_iconv_shared *cv_shared;
	void					 *cv_closure;
};

#define _CITRUS_ICONV_F_HIDE_INVALID	0x0001

extern struct _citrus_iconv_ops iconv_std;

static __inline int
_citrus_io_init_shared(struct _citrus_iconv_shared *ci,
		struct _citrus_iconv_std_shared *is, const char *__restrict curdir,
		const char *__restrict src, const char *__restrict dst,
		const void *__restrict var, size_t lenvar)
{
	return ((*ci->ci_ops->io_init_shared)(is, curdir, src, dst, var, lenvar));
}

static __inline void
_citrus_io_uninit_shared(struct _citrus_iconv_shared *ci, struct _citrus_iconv_std_shared *is)
{
	(*ci->ci_ops->io_init_shared)(is);
}

static __inline int
_citrus_io_convert(struct _citrus_iconv_shared *ci,
		const struct _citrus_iconv_std_shared *is,
		struct _citrus_iconv_std_context *sc,
		const char *__restrict* __restrict in, size_t *__restrict inbytes,
		char *__restrict * __restrict out, size_t *__restrict outbytes,
		u_int32_t flags, size_t *__restrict invalids)
{
	return ((*ci->ci_ops->io_convert)(is, sc, in, inbytes, out, outbytes, flags, invalids));
}

static __inline int
_citrus_io_init_context(struct _citrus_iconv_shared *ci, struct _citrus_iconv_std_shared *is, , struct _citrus_iconv_std_context *sc)
{
	return ((*ci->ci_ops->io_init_context)(is, sc));
}

static __inline void
_citrus_io_uninit_context(struct _citrus_iconv_shared *ci, struct _citrus_iconv_std_shared *is)
{
	(*ci->ci_ops->io_uninit_context)(is);
}

#endif /* _CITRUS_ICONV_H_ */
