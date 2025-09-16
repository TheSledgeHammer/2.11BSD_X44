/*	$NetBSD: citrus_iconv.c,v 1.11 2019/10/09 23:24:00 christos Exp $	*/

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

/*
 * File currently a Work in Progress (WIP).
 * Not ready for use.
 */
#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_iconv.c,v 1.11 2019/10/09 23:24:00 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include "citrus_iconv.h"

int
_citrus_iconv_init_shared(struct _citrus_iconv_shared *ci)
{
	struct _citrus_iconv_std_shared *is;
	int ret;

	is = malloc(sizeof(*is));
	return (citrus_io_init_shared(ci, is, curdir, src, dst, var, lenvar));
}

void
_citrus_iconv_uninit_shared(struct _citrus_iconv_shared *ci)
{
	struct _citrus_iconv_std_shared *is = ci->ci_closure;

	_citrus_io_uninit_shared(ci, is);
}

int
_citrus_iconv_convert(struct _citrus_iconv *cv, struct _citrus_iconv_shared *ci)
{
	const struct _citrus_iconv_std_shared *is = ci->ci_closure;
	struct _citrus_iconv_std_context *sc = cv->cv_closure;

	return (_citrus_io_convert(ci, is, sc, in, inbytes, out, outbytes, flags, invalids));
}

int
_citrus_iconv_init_context(struct _citrus_iconv *cv, struct _citrus_iconv_shared *ci)
{
	const struct _citrus_iconv_std_shared *is = ci->ci_closure;
	struct _citrus_iconv_std_context *sc = NULL;
	int ret;

	ret = _citrus_io_init_context(is, sc);
	if (ret != 0) {
		return (ret);
	}
	cv->cv_closure = (void *)sc;
	return (0);
}

void
_citrus_iconv_uninit_context(struct _citrus_iconv *cv, struct _citrus_iconv_shared *ci)
{
	struct _citrus_iconv_std_shared *is = cv->cv_closure;

	_citrus_io_uninit_context(ci, is);
	cv->cv_closure = is;
}

void
close_shared(struct _citrus_iconv_shared *ci)
{
	if (ci) {
//		if (ci->ci_module) {
			if (ci->ci_ops) {
				if (ci->ci_closure) {
					_citrus_iconv_uninit_shared(ci);
				}
				free(ci->ci_ops);
			}
		//	_citrus_unload_module(ci->ci_module);
		}
		free(ci);
	}
}

int
open_shared()
{
	struct _citrus_iconv_shared *ci;
	size_t len_convname;
	int ret;

	/* initialize iconv handle */
	len_convname = strlen(convname);
	ci = malloc(sizeof(*ci)+len_convname+1);

	if (ci->ci_ops->io_init_shared == NULL ||
	    ci->ci_ops->io_uninit_shared == NULL ||
	    ci->ci_ops->io_init_context == NULL ||
	    ci->ci_ops->io_uninit_context == NULL ||
	    ci->ci_ops->io_convert == NULL) {
		ret = EINVAL;
	}

	ret = _citrus_iconv_init_shared(ci, basedir, src, dst,
			(const void *) variable, strlen(variable) + 1);


	close_shared(ci);
	return ret;
}

void
release_shared(struct _citrus_iconv_shared * __restrict ci)
{

}

int
_citrus_iconv_open()
{

}

void
_citrus_iconv_close(struct _citrus_iconv *cv)
{
	if (cv ) {
		_citrus_iconv_uninit_context(cv, cv->cv_shared);
		release_shared(cv->cv_shared);
		free(cv);
	}
}
