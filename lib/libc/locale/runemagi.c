/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright 2014 Garrett D'Amore <garrett@damore.org>
 * Copyright 2010 Nexenta Systems, Inc.  All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
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
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "runefile.h"

/* add encoding to RuneLocale file? */
void
citrus_encoding_init(_ENCODING_INFO *ei, _ENCODING_STATE *es)
{
	if (ei == NULL) {
		ei = malloc(sizeof(_ENCODING_INFO *));
	}
	if (es == NULL) {
		es = malloc(sizeof(_ENCODING_STATE *));
	}
}

int
_citrus_ctype_mbrtowc_priv(_ENCODING_INFO * __restrict ei, wchar_t * __restrict pwc, const char ** __restrict s, size_t n, _ENCODING_STATE * __restrict psenc, size_t * __restrict nresult)
{
	return (sgetrune_mb(ei, pwc, s, n, psenc, nresult));
}

int
_citrus_ctype_wcrtomb_priv(_ENCODING_INFO * __restrict ei, char * __restrict s, size_t n, wchar_t wc, _ENCODING_STATE * __restrict psenc, size_t * __restrict nresult)
{
	return (sputrune_mb(ei, s, n, wc, psenc, nresult));
}

/* rune.c 
 * Todo: 
 * - update mklocale to use _FileRuneLocale 
 * - fix up other locale files that use _FileRuneLocale
 */
_RuneLocale *
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
	variable = runetype_ext_ranges + frl->runetype_ext.nranges;
	if (variable > lastp) {
		goto invalid;
	}

	maplower_ext_ranges = (_FileRuneEntry*) variable;
	variable = maplower_ext_ranges + frl->maplower_ext.nranges;
	if (variable > lastp) {
		goto invalid;
	}

	mapupper_ext_ranges = (_FileRuneEntry*) variable;
	variable = mapupper_ext_ranges + frl->mapupper_ext.nranges;
	if (variable > lastp) {
		goto invalid;
	}

	frr = runetype_ext_ranges;
	for (x = 0; x < frl->runetype_ext.nranges; ++x) {
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

	data = malloc(sizeof(_RuneLocale) +
			(frl->runetype_ext.nranges + frl->maplower_ext.nranges +
			frl->mapupper_ext.nranges) * sizeof(_RuneEntry)+
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

	rl->variable_len = frl->variable_len;
	rl->runetype_ext.nranges = frl->runetype_ext.nranges;
	rl->maplower_ext.nranges = frl->maplower_ext.nranges;
	rl->mapupper_ext.nranges = frl->mapupper_ext.nranges;

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

	variable = mapupper_ext_ranges + frl->mapupper_ext.nranges;
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
