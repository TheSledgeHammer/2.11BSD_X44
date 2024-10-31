/*-
 * Copyright (c) 2003-2004 Tim J. Robbins
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
__FBSDID("$FreeBSD$");
#endif

#include <limits.h>
#include <rune.h>
#include <string.h>
#include <wchar.h>

/*
 * Emulate the deprecated 4.4BSD sgetrune() function in terms of
 * the ISO C mbrtowc() function.
 */
rune_t
emulated_sgetrune(string, n, result)
	const char *string;
	size_t n;
	char const **result;
{
	rune_t rune;
	size_t nconv;

	nconv = mbrtowc(&rune, string, n, NULL);
    if (nconv == (size_t)-2) {
        if (result != NULL) {
            *result = string;
        }
        return (_INVALID_RUNE);
    }
    if (nconv == (size_t)-1) {
        if (result != NULL) {
            *result = string + 1;
        }
        return (_INVALID_RUNE);
    }
    if (nconv == 0) {
        nconv = 1;
    }
    if (result != NULL) {
        *result = string + nconv;
    }
    return (0);
}

/*
 * Emulate the deprecated 4.4BSD sputrune() function in terms of
 * the ISO C wcrtomb() function.
 */
int
emulated_sputrune(c, string, n, result)
	rune_t c;
    char *string, **result;
    size_t n;
{
    static const mbstate_t initial;
    mbstate_t mbs;
    char buf[MB_LEN_MAX];
    size_t nconv;

    mbs = initial;
    nconv = wcrtomb(buf, (wchar_t)c, &mbs);
    if (nconv == (size_t)-1) {
        if (result != NULL) {
            *result = NULL;
        }
        return (0);
    }
    if (string == NULL) {
        if (result != NULL) {
            *result = (char *) 0 + nconv;
        }
    } else if (n >= nconv) {
        memcpy(string, buf, nconv);
        if (result != NULL) {
            *result = string + nconv;
        }
    } else {
        if (result != NULL) {
            *result = NULL;
        }
    }
    return ((int) nconv);
}
