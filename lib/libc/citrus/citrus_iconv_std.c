/*	$NetBSD: citrus_iconv_std.c,v 1.16 2012/02/12 13:51:29 wiz Exp $	*/

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

#include <sys/cdefs.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <machine/endian.h>
#include <sys/queue.h>

#include "citrus_ctype.h"
#include "citrus_types.h"
#include "citrus_stdenc.h"
#include "citrus_iconv.h"
#include "citrus_iconv_std.h"

/*
 * convenience routines for stdenc.
 */
static __inline void
save_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	if (se->se_ps) {
		memcpy(se->se_pssaved, se->se_ps, _citrus_stdenc_get_state_size(se->se_handle));
	}
}

static __inline void
restore_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	if (se->se_ps) {
		memcpy(se->se_ps, se->se_pssaved, _citrus_stdenc_get_state_size(se->se_handle));
	}
}

static __inline void
init_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	if (se->se_ps) {
		_citrus_stdenc_init_state(se->se_handle, se->se_ps);
	}
}

static __inline int
mbtocsx(struct _citrus_iconv_std_encoding *se, _csid_t *csid, _index_t *idx, const char **s, size_t n,	size_t *nresult)
{
	return _citrus_stdenc_mbtocs(se->se_handle, csid, idx, s, n, TO_STATE(se->se_ps), nresult);
}

static __inline int
cstombx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, _csid_t csid, _index_t idx, size_t *nresult)
{
	return _citrus_stdenc_cstomb(se->se_handle, s, n, csid, idx, TO_STATE(se->se_ps), nresult);
}

static __inline int
wctombx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, _wc_t wc, size_t *nresult)
{
	return _citrus_stdenc_wctomb(se->se_handle, s, n, wc, TO_STATE(se->se_ps), nresult);
}

static __inline int
put_state_resetx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, size_t *nresult)
{
	return _citrus_stdenc_put_state_reset(se->se_handle, s, n, TO_STATE(se->se_ps), nresult);
}

static __inline int
get_state_desc_gen(struct _citrus_iconv_std_encoding *se, int *rstate)
{
	int ret, state;

	ret = _citrus_stdenc_get_state_desc(se->se_handle, TO_STATE(se->se_ps), _CITRUS_STDENC_SDID_GENERIC, &state);
	if (!ret) {
		*rstate = &state;
	}

	return ret;
}

/*
 * init encoding context
 */
static int
init_encoding(struct _citrus_iconv_std_encoding *se, _ENCODING_INFO *cs, void *ps1, void *ps2)
{
	int ret = -1;

	se->se_handle = cs;
	se->se_ps = ps1;
	se->se_pssaved = ps2;

	if (se->se_ps) {
		ret = _citrus_stdenc_init_state(cs, TO_STATE(se->se_ps));
	}
	if (!ret && se->se_pssaved) {
		ret = _citrus_stdenc_init_state(cs, TO_STATE(se->se_pssaved));
	}

	return ret;
}

static int
open_csmapper(struct _csmapper **rcm, const char *src, const char *dst, unsigned long *rnorm)
{
	int ret;
	struct _csmapper *cm;

	ret = _csmapper_open(&cm, src, dst, 0, rnorm);
	if (ret)
		return ret;
	if (_csmapper_get_src_max(cm) != 1 || _csmapper_get_dst_max(cm) != 1 || _csmapper_get_state_size(cm) != 0) {
		_csmapper_close(cm);
		return EINVAL;
	}

	*rcm = cm;

	return 0;
}

static void
close_dsts(struct _citrus_iconv_std_dst_list *dl)
{
	struct _citrus_iconv_std_dst *sd;

	while ((sd=TAILQ_FIRST(dl)) != NULL) {
		TAILQ_REMOVE(dl, sd, sd_entry);
		_csmapper_close(sd->sd_mapper);
		free(sd);
	}
}

static int
open_dsts(struct _citrus_iconv_std_dst_list *dl, const struct _esdb_charset *ec, const struct _esdb *dbdst)
{

}

static void
close_srcs(struct _citrus_iconv_std_src_list *sl)
{
	struct _citrus_iconv_std_src *ss;

	while ((ss=TAILQ_FIRST(sl)) != NULL) {
		TAILQ_REMOVE(sl, ss, ss_entry);
		close_dsts(&ss->ss_dsts);
		free(ss);
	}
}

static int
open_srcs(struct _citrus_iconv_std_src_list *sl, const struct _esdb *dbsrc, const struct _esdb *dbdst)
{

}

/* do convert a character */
#define E_NO_CORRESPONDING_CHAR ENOENT /* XXX */
static int
/*ARGSUSED*/
do_conv(const struct _citrus_iconv_std_shared *is, struct _citrus_iconv_std_context *sc, _csid_t *csid, _index_t *idx)
{
	_index_t tmpidx;
	int ret;
	struct _citrus_iconv_std_src *ss;
	struct _citrus_iconv_std_dst *sd;

	TAILQ_FOREACH(ss, &is->is_srcs, ss_entry) {
		if (ss->ss_csid == *csid) {
			TAILQ_FOREACH(sd, &ss->ss_dsts, sd_entry) {
				ret = _csmapper_convert(sd->sd_mapper, &tmpidx, *idx, NULL);
				switch (ret) {
				case _MAPPER_CONVERT_SUCCESS:
					*csid = sd->sd_csid;
					*idx = tmpidx;
					return 0;
				case _MAPPER_CONVERT_NONIDENTICAL:
					break;
				case _MAPPER_CONVERT_SRC_MORE:
					/*FALLTHROUGH*/
				case _MAPPER_CONVERT_DST_MORE:
					/*FALLTHROUGH*/
				case _MAPPER_CONVERT_FATAL:
					return EINVAL;
				case _MAPPER_CONVERT_ILSEQ:
					return EILSEQ;
				}
			}
			break;
		}
	}

	return E_NO_CORRESPONDING_CHAR;
}

static int
_citrus_iconv_std_iconv_convert(struct _citrus_iconv * __restrict cv, const char * __restrict * __restrict in, size_t * __restrict inbytes, char * __restrict * __restrict out, size_t * __restrict outbytes, u_int32_t flags, size_t * __restrict invalids)
{
	const struct _citrus_iconv_std_shared *is = cv->cv_shared->ci_closure;
	struct _citrus_iconv_std_context *sc = cv->cv_closure;
	_index_t idx;
	_csid_t csid;
	int ret, state;
	size_t szrin, szrout;
	size_t inval;
	const char *tmpin;

    inval = 0;
	if (in==NULL || *in==NULL) {
		/* special cases */
		if (out!=NULL && *out!=NULL) {
			/* init output state and store the shift sequence */
			save_encoding_state(&sc->sc_src_encoding);
			save_encoding_state(&sc->sc_dst_encoding);
			szrout = 0;

			ret = put_state_resetx(&sc->sc_dst_encoding, *out, *outbytes, &szrout);
			if (ret)
				goto err;

			if (szrout == (size_t)-2) {
				/* too small to store the character */
				ret = EINVAL;
				goto err;
			}
			*out += szrout;
			*outbytes -= szrout;
		} else
			/* otherwise, discard the shift sequence */
			init_encoding_state(&sc->sc_dst_encoding);
		init_encoding_state(&sc->sc_src_encoding);
		*invalids = 0;
		return 0;
	}
    /* normal case */
	for (;;) {
		if (*inbytes==0) {
			ret = get_state_desc_gen(&sc->sc_src_encoding, &state);
			if (state == _CITRUS_STDENC_SDGEN_INITIAL ||
			    state == _CITRUS_STDENC_SDGEN_STABLE)
				break;
		}

		/* save the encoding states for the error recovery */
		save_encoding_state(&sc->sc_src_encoding);
		save_encoding_state(&sc->sc_dst_encoding);

		/* mb -> csid/index */
		tmpin = *in;
		szrin = szrout = 0;
		ret = mbtocsx(&sc->sc_src_encoding, &csid, &idx, &tmpin, *inbytes, &szrin);
		if (ret)
			goto err;

		if (szrin == (size_t)-2) {
			/* incompleted character */
			ret = get_state_desc_gen(&sc->sc_src_encoding, &state);
			if (ret) {
				ret = EINVAL;
				goto err;
			}
			switch (state) {
			case _CITRUS_STDENC_SDGEN_INITIAL:
			case _CITRUS_STDENC_SDGEN_STABLE:
				/* fetch shift sequences only. */
				goto next;
			}
			ret = EINVAL;
			goto err;
		}
		/* convert the character */
		ret = do_conv(is, sc, &csid, &idx);
		if (ret) {
			if (ret == E_NO_CORRESPONDING_CHAR) {
				inval++;
				szrout = 0;
				if ((flags&_CITRUS_ICONV_F_HIDE_INVALID)==0 &&
				    is->is_use_invalid) {
					ret = wctombx(&sc->sc_dst_encoding,
						      *out, *outbytes,
						      is->is_invalid,
						      &szrout);
					if (ret)
						goto err;
				}
				goto next;
			} else {
				goto err;
			}
		}
		/* csid/index -> mb */
		ret = cstombx(&sc->sc_dst_encoding, *out, *outbytes, csid, idx, &szrout);
		if (ret)
			goto err;
next:
		_DIAGASSERT(*inbytes>=szrin && *outbytes>=szrout);
		*inbytes -= tmpin-*in; /* szrin is insufficient on \0. */
		*in = tmpin;
		*outbytes -= szrout;
		*out += szrout;
	}

    *invalids = inval;

	return 0;

err:
	restore_encoding_state(&sc->sc_src_encoding);
	restore_encoding_state(&sc->sc_dst_encoding);
err_norestore:
	*invalids = inval;

	return ret;
}
