/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
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
static char sccsid[] = "@(#)rune.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <ctype.h>
#include <limits.h>
#include <wctype.h>
#include <rune.h>
#include <stdio.h>
#include <stdlib.h>

#include "runefile.h"
#include "setlocale.h"

extern int			_none_init (_RuneLocale *);
extern int			_UTF2_init (_RuneLocale *);
extern int			_UTF8_init (_RuneLocale *);
extern int			_EUC_init (_RuneLocale *);
static _RuneLocale	*_Read_RuneMagi (FILE *);

static char *PathLocale = 0;

/* wctype_init: */
void
wctype_init(_RuneLocale *rl)
{
	memcpy(&rl->wctype, &_DefaultRuneLocale.wctype, sizeof(rl->wctype));
}

/* _wctrans_init: */
void
wctrans_init(_RuneLocale *rl)
{
	rl->wctrans[_WCTRANS_INDEX_LOWER].name = "tolower";
	rl->wctrans[_WCTRANS_INDEX_LOWER].cached = rl->maplower;
	rl->wctrans[_WCTRANS_INDEX_LOWER].extmap = &rl->maplower_ext;
	rl->wctrans[_WCTRANS_INDEX_UPPER].name = "toupper";
	rl->wctrans[_WCTRANS_INDEX_UPPER].cached = rl->mapupper;
	rl->wctrans[_WCTRANS_INDEX_UPPER].extmap = &rl->mapupper_ext;
}

int
setrunelocale(encoding)
	char *encoding;
{
	FILE *fp;
	char name[PATH_MAX];
	_RuneLocale *rl;

	if (!encoding)
		return (EFAULT);

	/*
	 * The "C" and "POSIX" locale are always here.
	 */
	if (!strcmp(encoding, "C") || !strcmp(encoding, "POSIX")) {
		_CurrentRuneLocale = &_DefaultRuneLocale;
		return (0);
	}

	if (!PathLocale && !(PathLocale = getenv("PATH_LOCALE")))
		PathLocale = _PATH_LOCALE;

	sprintf(name, "%s/%s/LC_CTYPE", PathLocale, encoding);

	if ((fp = fopen(name, "r")) == NULL)
		return (ENOENT);

	if ((rl = _Read_RuneMagi(fp)) == 0) {
		fclose(fp);
		return (EFTYPE);
	}

	wctype_init(rl);
	wctrans_init(rl);

	if (!rl->encoding[0] || !strcmp(rl->encoding, "UTF-8")) {
		return (_UTF8_init(rl));
	} else if (!strcmp(rl->encoding, "UTF-2")) {
		return (_UTF2_init(rl));
	} else if (!strcmp(rl->encoding, "NONE")) {
		return (_none_init(rl));
	} else if (!strcmp(rl->encoding, "EUC")) {
		return (_EUC_init(rl));
	} else {
		return (EINVAL);
	}
}

void
setinvalidrune(ir)
	rune_t ir;
{
	_INVALID_RUNE = ir;
}

static _RuneLocale *
_Read_RuneMagi(fp)
	FILE *fp;
{
	char *fdata, *data;
	void *lastp;
	_FileRuneLocale *frl;
	_FileRuneEntry *frr;
	_RuneLocale *rl;
	_RuneEntry *rr;
	struct stat sb;
	int x, saverr;
	void *variable;
	_FileRuneEntry *runetype_ext_ranges;
	_FileRuneEntry *maplower_ext_ranges;
	_FileRuneEntry *mapupper_ext_ranges;
	int runetype_ext_len = 0;

	if (fstat(fileno(fp), &sb) < 0) {
		return (NULL);
	}

	if (sb.st_size < sizeof(_FileRuneLocale)) {
		return (NULL);
	}

	fdata = malloc(sb.st_size);
	if(fdata == NULL) {
		return (NULL);
	}

	/* Someone might have read the magic number once already */
	rewind(fp);

	if (fread(fdata, sb.st_size, 1, fp) != 1) {
		free(fdata);
		return (NULL);
	}

	frl = (_FileRuneLocale *)(void *)fdata;
	lastp = fdata + sb.st_size;

	variable = frl + 1;

	if (memcmp(frl->magic, _FILE_RUNE_MAGIC_1, sizeof(frl->magic))) {
		goto invalid;
	}

	runetype_ext_ranges = (_FileRuneEntry*) variable;
	variable = runetype_ext_ranges + frl->runetype_ext_nranges;
	if (variable > lastp) {
		goto invalid;
	}

	maplower_ext_ranges = (_FileRuneEntry*) variable;
	variable = maplower_ext_ranges + frl->maplower_ext_nranges;
	if (variable > lastp) {
		goto invalid;
	}

	mapupper_ext_ranges = (_FileRuneEntry*) variable;
	variable = mapupper_ext_ranges + frl->mapupper_ext_nranges;
	if (variable > lastp) {
		goto invalid;
	}

	frr = runetype_ext_ranges;
	for (x = 0; x < frl->runetype_ext_nranges; ++x) {
		uint32_t *stypes;
		if (frr[x].map == 0) {
			int len = frr[x].max - frr[x].min + 1;
			stypes = variable;
			variable = stypes + len;
			runetype_ext_len += len;
			if (variable > lastp) {
				goto invalid;
			}
		}
	}

	if ((char*) variable + frl->variable_len > (char*) lastp) {
		goto invalid;
	}

	/*
	 * Convert from disk format to host format.
	 */
	data = malloc(sizeof(_RuneLocale) +
			(frl->runetype_ext_nranges + frl->maplower_ext_nranges +
			frl->mapupper_ext_nranges) * sizeof(_RuneEntry)+
			runetype_ext_len * sizeof(*rr->types) + frl->variable_len);
	if (data == NULL) {
		saverr = errno;
		free(fdata);
		errno = saverr;
		return (NULL);
	}

	rl = (_RuneLocale*) data;
	rl->variable = rl + 1;

	memcpy(rl->magic, _RUNE_MAGIC_1, sizeof(rl->magic));
	memcpy(rl->encoding, frl->encoding, sizeof(rl->encoding));

	rl->invalid_rune = frl->invalid_rune;
	rl->variable_len = frl->variable_len;
	rl->runetype_ext.nranges = frl->runetype_ext_nranges;
	rl->maplower_ext.nranges = frl->maplower_ext_nranges;
	rl->mapupper_ext.nranges = frl->mapupper_ext_nranges;

	for (x = 0; x < _CACHED_RUNES; ++x) {
		rl->runetype[x] = frl->runetype[x];
		rl->maplower[x] = frl->maplower[x];
		rl->mapupper[x] = frl->mapupper[x];
	}

	rl->runetype_ext.ranges = (_RuneEntry*) rl->variable;
	rl->variable = rl->runetype_ext.ranges + rl->runetype_ext.nranges;

	rl->maplower_ext.ranges = (_RuneEntry*) rl->variable;
	rl->variable = rl->maplower_ext.ranges + rl->maplower_ext.nranges;

	rl->mapupper_ext.ranges = (_RuneEntry*) rl->variable;
	rl->variable = rl->mapupper_ext.ranges + rl->mapupper_ext.nranges;

	variable = mapupper_ext_ranges + frl->mapupper_ext_nranges;
	frr = runetype_ext_ranges;
	rr = rl->runetype_ext.ranges;
	for (x = 0; x < rl->runetype_ext.nranges; ++x) {
		uint32_t *stypes;

		rr[x].min = frr[x].min;
		rr[x].max = frr[x].max;
		rr[x].map = frr[x].map;
		if (rr[x].map == 0) {
			int len = rr[x].max - rr[x].min + 1;
			stypes = variable;
			variable = stypes + len;
			rr[x].types = rl->variable;
			rl->variable = rr[x].types + len;
			while (len-- > 0)
				rr[x].types[len] = stypes[len];
		} else
			rr[x].types = NULL;
	}

	frr = maplower_ext_ranges;
	rr = rl->maplower_ext.ranges;
	for (x = 0; x < rl->maplower_ext.nranges; ++x) {
		rr[x].min = frr[x].min;
		rr[x].max = frr[x].max;
		rr[x].map = frr[x].map;
	}

	frr = mapupper_ext_ranges;
	rr = rl->mapupper_ext.ranges;
	for (x = 0; x < rl->mapupper_ext.nranges; ++x) {
		rr[x].min = frr[x].min;
		rr[x].max = frr[x].max;
		rr[x].map = frr[x].map;
	}

	memcpy(rl->variable, variable, rl->variable_len);
	free(fdata);

	/*
	 * Go out and zero pointers that should be zero.
	 */
	if (!rl->variable_len)
		rl->variable = NULL;

	if (!rl->runetype_ext.nranges)
		rl->runetype_ext.ranges = NULL;

	if (!rl->maplower_ext.nranges)
		rl->maplower_ext.ranges = NULL;

	if (!rl->mapupper_ext.nranges)
		rl->mapupper_ext.ranges = NULL;

	return (rl);

invalid:
	free(fdata);
	errno = EINVAL;
	return (NULL);
}

unsigned long
___runetype(c)
	rune_t c;
{
	int x;
	_RuneRange *rr = &_CurrentRuneLocale->runetype_ext;
	_RuneEntry *re = rr->ranges;

	if (c == EOF)
		return (0);
	for (x = 0; x < rr->nranges; ++x, ++re) {
		if (c < re->min)
			return (0L);
		if (c <= re->max) {
			if (re->types)
				return (re->types[c - re->min]);
			else
				return (re->map);
		}
	}
	return (0L);
}

_RuneType
___runetype_mb(c)
	wint_t c;
{
	uint32_t x;
	_RuneRange *rr = &_CurrentRuneLocale->runetype_ext;
	_RuneEntry *re = rr->ranges;

	if (c == WEOF)
		return (0U);

	for (x = 0; x < rr->nranges; ++x, ++re) {
		/* XXX assumes wchar_t = int */
		if ((rune_t) c < re->min)
			return (0U);
		if ((rune_t) c <= re->max) {
			if (re->types)
				return (re->types[(rune_t) c - re->min]);
			else
				return (re->map);
		}
	}
	return (0U);
}

rune_t
___toupper(c)
	rune_t c;
{
	int x;
	_RuneRange *rr = &_CurrentRuneLocale->mapupper_ext;
	_RuneEntry *re = rr->ranges;

	if (c == EOF)
		return (EOF);
	for (x = 0; x < rr->nranges; ++x, ++re) {
		if (c < re->min)
			return (c);
		if (c <= re->max)
			return (re->map + c - re->min);
	}
	return (c);
}

wint_t
___toupper_mb(c)
	wint_t c;
{
	int x;
	_RuneRange *rr = &_CurrentRuneLocale->mapupper_ext;
	_RuneEntry *re = rr->ranges;

	if (c < 0 || c == EOF)
		return(c);

	for (x = 0; x < rr->nranges; ++x, ++re) {
		if (c < re->min)
			return(c);
		if (c <= re->max)
			return(re->map + c - re->min);
	}
	return(c);
}

rune_t
___tolower(c)
	rune_t c;
{
	int x;
	_RuneRange *rr = &_CurrentRuneLocale->maplower_ext;
	_RuneEntry *re = rr->ranges;

	if (c == EOF)
		return (EOF);
	for (x = 0; x < rr->nranges; ++x, ++re) {
		if (c < re->min)
			return (c);
		if (c <= re->max)
			return (re->map + c - re->min);
	}
	return (c);
}

wint_t
___tolower_mb(c)
	wint_t c;
{
	int x;
	_RuneRange *rr = &_CurrentRuneLocale->maplower_ext;
	_RuneEntry *re = rr->ranges;

	if (c < 0 || c == EOF)
		return(c);

	for (x = 0; x < rr->nranges; ++x, ++re) {
		if (c < re->min)
			return(c);
		if (c <= re->max)
			return(re->map + c - re->min);
	}
	return(c);
}

#if !defined(_USE_CTYPE_INLINE_) && !defined(_USE_CTYPE_MACROS_)
/*
 * See comments in <machine/ansi.h>
 */
int
__istype(c, f)
	rune_t c;
	unsigned long f;
{
	return (((
			(_RUNE_ISCACHED(c)) ?
					___runetype(c) : _CurrentRuneLocale->runetype[c]) & f) ?
			1 : 0);
}

int
__isctype(c, f)
	rune_t c;
	unsigned long f;
{
	return ((((_RUNE_ISCACHED(c)) ? 0 : _DefaultRuneLocale.runetype[c]) & f) ?
			1 : 0);
}

rune_t
toupper(c)
	rune_t c;
{
	return ((_RUNE_ISCACHED(c)) ?
			___toupper(c) : _CurrentRuneLocale->mapupper[c]);
}

rune_t
tolower(c)
	rune_t c;
{
	return ((_RUNE_ISCACHED(c)) ?
			___tolower(c) : _CurrentRuneLocale->maplower[c]);
}

wint_t
toupper_mb(c)
	wint_t c;
{
	return ((_RUNE_ISCACHED(c)) ?
			___toupper_mb(c) : _CurrentRuneLocale->mapupper[c]);
}

wint_t
tolower_mb(c)
	wint_t c;
{
	return ((_RUNE_ISCACHED(c)) ?
			___tolower_mb(c) : _CurrentRuneLocale->maplower[c]);
}
#endif
