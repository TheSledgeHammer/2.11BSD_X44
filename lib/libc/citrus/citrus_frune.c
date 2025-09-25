/*-
 * Copyright (c) 2024
 *	Martin Kelly. All rights reserved.
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
#include <sys/types.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "locale/setlocale.h"

#include "citrus_ctype.h"
#include "citrus_types.h"
#include "citrus_stdenc.h"
#include "citrus_frune.h"

/*
 * open frune
 */
int
_citrus_frune_open(struct _citrus_frune_encoding **rfe, char *encoding, void *variable, size_t lenvar)
{
	struct _citrus_frune_encoding *fe;
	int ret;

	fe = malloc(sizeof(*fe));
	if (fe == NULL) {
		_citrus_frune_close(fe);
		return (errno);
	}

	fe->fe_info = NULL;
	fe->fe_state = NULL;
	fe->fe_runelocale = NULL;

	fe->fe_runelocale = findrunelocale(encoding);
	if (fe->fe_runelocale == NULL) {
		ret = errno;
		goto bad;
	}

	ret = validrunelocale(fe->fe_runelocale, encoding, variable, lenvar);
	if (ret != 0) {
		goto bad;
	}

	fe->fe_info = malloc(sizeof(*fe->fe_info));
	if (fe->fe_info == NULL) {
		ret = errno;
		goto bad;
	}

	ret = _citrus_stdenc_init((void **)&fe->fe_info, variable, lenvar);
	if (ret != 0) {
		goto bad;
	}

	fe->fe_state = malloc(sizeof(*fe->fe_state));
	if (fe->fe_state == NULL) {
		ret = errno;
		goto bad;
	}

	*rfe = fe;
	return (0);

bad:
	_citrus_frune_close(fe);
	return (ret);
}

/*
 * close frune
 */
void
_citrus_frune_close(struct _citrus_frune_encoding *fe)
{
	if (fe->fe_runelocale != NULL) {
		if (fe->fe_info != NULL) {
			_citrus_stdenc_uninit(fe->fe_info);
			if (fe->fe_state != NULL) {
				free(fe->fe_state);
			}
			free(fe->fe_info);
		}
		free(fe->fe_runelocale);
	}
	free(fe);
}

/*
 * convenience routines for translating frunes to stdenc.
 */
void
_citrus_frune_save_encoding_state(struct _citrus_frune_encoding *fe, void *ps, void *pssaved)
{
	if (ps) {
		memcpy(pssaved, ps, _citrus_stdenc_get_state_size(fe->fe_info));
	}
}

void
_citrus_frune_restore_encoding_state(struct _citrus_frune_encoding *fe, void *ps, void *pssaved)
{
	if (ps) {
		memcpy(ps, pssaved, _citrus_stdenc_get_state_size(fe->fe_info));
	}
}

void
_citrus_frune_init_encoding_state(struct _citrus_frune_encoding *fe, void *ps)
{
	if (ps) {
		_citrus_stdenc_init_state(fe->fe_info, fe->fe_state);
	}
}

int
_citrus_frune_mbtocsx(struct _citrus_frune_encoding *fe, _csid_t *csid, _index_t *idx, const char **s, size_t n, size_t *nresult)
{
	return (_citrus_stdenc_mbtocs(fe->fe_info, csid, idx, s, n, fe->fe_state, nresult));
}

int
_citrus_frune_cstombx(struct _citrus_frune_encoding *fe, char *s, size_t n, _csid_t csid, _index_t idx, size_t *nresult)
{
	return (_citrus_stdenc_cstomb(fe->fe_info, s, n, csid, idx, fe->fe_state, nresult));
}

int
_citrus_frune_wctombx(struct _citrus_frune_encoding *fe, char *s, size_t n, _wc_t wc, size_t *nresult)
{
	return (_citrus_stdenc_wctomb(fe->fe_info, s, n, wc, fe->fe_state, nresult));
}

size_t
_citrus_frune_get_state_size(struct _citrus_frune_encoding *fe)
{
    return (_citrus_stdenc_get_state_size(fe->fe_info));
}

size_t
_citrus_frune_get_mb_cur_max(struct _citrus_frune_encoding *fe)
{
    return (_citrus_stdenc_get_mb_cur_max(fe->fe_info));
}

int
_citrus_frune_put_state_resetx(struct _citrus_frune_encoding *fe, char *s, size_t n, size_t *nresult)
{
	return (_citrus_stdenc_put_state_reset(fe->fe_info, s, n, fe->fe_state, nresult));
}

int
_citrus_frune_get_state_desc_gen(struct _citrus_frune_encoding *fe, int *rstate)
{
	int ret, state;

	ret = _citrus_stdenc_get_state_desc(fe->fe_info, fe->fe_state, _CITRUS_STDENC_SDID_GENERIC, &state);
	if (!ret) {
		rstate = &state;
	}
	return (ret);
}
