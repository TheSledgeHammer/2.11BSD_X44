/*-
 * Copyright (c)2025 Martin Kelly,
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

#ifndef _CITRUS_RUNE_H_
#define _CITRUS_RUNE_H_

#include <assert.h>
#include <rune.h>
#include <runetype.h>
#include <encoding.h>

static __inline rune_t
_citrus_rune_sgetrune(_RuneLocale *rl, const char *s, size_t n, char const **r)
{
    _DIAGASSERT(rl && rl->ops && rl->ops->ro_sgetrune);
    return ((*rl->ops->ro_sgetrune)(s, n, r));
}

static __inline int
_citrus_rune_sputrune(_RuneLocale *rl, rune_t c, char *s, size_t n, char **r)
{
    _DIAGASSERT(rl && rl->ops && rl->ops->ro_sputrune);
    return ((*rl->ops->ro_sputrune)(c, s, n, r));
}

static __inline int
_citrus_rune_sgetmbrune(_RuneLocale *rl, _ENCODING_INFO *ei, wchar_t *wc, const char **s, size_t n, _ENCODING_STATE *es, size_t *r)
{
    _DIAGASSERT(rl && rl->ops && rl->ops->ro_sgetmbrune);
    return ((*rl->ops->ro_sgetmbrune)(ei, wc, s, n, es, r));
}

static __inline int
_citrus_rune_sputmbrune(_RuneLocale *rl, _ENCODING_INFO *ei, char *s, size_t n, wchar_t wc, _ENCODING_STATE *es, size_t *r)
{
    _DIAGASSERT(rl && rl->ops && rl->ops->ro_sputmbrune);
    return ((*rl->ops->ro_sputmbrune)(ei, s, n, wc, es, r));
}

static __inline int
_citrus_rune_sgetcsrune(_RuneLocale *rl, _ENCODING_INFO *ei, wchar_t *wc, _csid_t csid, _index_t idx)
{
    _DIAGASSERT(rl && rl->ops && rl->ops->ro_sgetcsrune);
    return ((*rl->ops->ro_sgetcsrune)(ei, wc, csid, idx));
}

static __inline int
_citrus_rune_sputcsrune(_RuneLocale *rl, _ENCODING_INFO *ei, _csid_t *csid, _index_t *idx, wchar_t wc)
{
    _DIAGASSERT(rl && rl->ops && rl->ops->ro_sputcsrune);
    return ((*rl->ops->ro_sputcsrune)(ei, csid, idx, wc));
}

static __inline int
_citrus_rune_module_init(_RuneLocale *rl, _ENCODING_INFO *ei, const void *var, size_t lenvar)
{
	_DIAGASSERT(rl && rl->ops && rl->ops->ro_module_init);
	return ((*rl->ops->ro_module_init)(ei, var, lenvar));
}

static __inline void
_citrus_rune_module_uninit(_RuneLocale *rl, _ENCODING_INFO *ei)
{
	_DIAGASSERT(rl && rl->ops && rl->ops->ro_module_uninit);
	(*rl->ops->ro_module_uninit)(ei);
}

#endif
