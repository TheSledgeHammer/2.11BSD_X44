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


#include "citrus_iconv.h"

#define _CITRUS_ICONV_DIR		"iconv.dir"
#define _CITRUS_ICONV_ALIAS		"iconv.alias"

#define CI_HASH_SIZE 			101
#define CI_INITIAL_MAX_REUSE	5
#define CI_ENV_MAX_REUSE		"ICONV_MAX_REUSE"

/*
 * lookup_iconv_entry:
 *	lookup iconv.dir entry in the specified directory.
 *
 * line format of iconv.dir file:
 *	key  module  arg
 * key    : lookup key.
 * module : iconv module name.
 * arg    : argument for the module (generally, description file name)
 *
 */
static __inline int
lookup_iconv_entry(const char *curdir, const char *key, char *linebuf, size_t linebufsize, const char **module, const char **variable)
{
	return 0;
}

static __inline void
close_shared(struct _citrus_iconv_shared *ci)
{

}

static __inline int
open_shared(struct _citrus_iconv_shared * __restrict * __restrict rci, const char * __restrict basedir, const char * __restrict convname, const char * __restrict src, const char * __restrict dst)
{
	return 0;
}

static __inline int
hash_func(const char *key)
{
	return _string_hash_func(key, CI_HASH_SIZE);
}

static __inline int
match_func(struct _citrus_iconv_shared * __restrict ci,
	   const char * __restrict key)
{
	return strcmp(ci->ci_convname, key);
}

static int
get_shared(struct _citrus_iconv_shared * __restrict * __restrict rci, const char *basedir, const char *src, const char *dst)
{
	return 0;
}

static void
release_shared(struct _citrus_iconv_shared * __restrict ci)
{

}

/*
 * _citrus_iconv_open:
 *	open a converter for the specified in/out codes.
 */
int
_citrus_iconv_open(struct _citrus_iconv * __restrict * __restrict rcv, const char * __restrict basedir, const char * __restrict src, const char * __restrict dst)
{
	int ret;
	struct _citrus_iconv_shared *ci = NULL;
	struct _citrus_iconv *cv;
	char realsrc[PATH_MAX], realdst[PATH_MAX];
	char buf[PATH_MAX], path[PATH_MAX];

	return 0;
}

/*
 * _citrus_iconv_close:
 *	close the specified converter.
 */
void
_citrus_iconv_close(struct _citrus_iconv *cv)
{
	if (cv) {
		//uninit_context(cv);
		release_shared(cv->cv_shared);
		free(cv);
	}
}
