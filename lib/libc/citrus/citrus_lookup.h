/*	$NetBSD: citrus_lookup.h,v 1.2 2004/07/21 14:16:34 tshiozak Exp $	*/

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

#ifndef _CITRUS_LOOKUP_H_
#define _CITRUS_LOOKUP_H_

#define _CITRUS_LOOKUP_CASE_SENSITIVE	0
#define _CITRUS_LOOKUP_CASE_IGNORE		1

/*
 * Temporary hold over
 */
static __inline char *
_citrus_lookup_simple(const char *name, const char *key, char *linebuf, size_t linebufsize, int ignore_case)
{
	switch (ignore_case) {
	case _CITRUS_LOOKUP_CASE_SENSITIVE:
		ignore_case = 0;
		break;
	case _CITRUS_LOOKUP_CASE_IGNORE:
		ignore_case = 1;
		break;
	}
	return (__unaliasname(name, key, linebuf, linebufsize));
}

#endif /* _CITRUS_LOOKUP_H_ */
