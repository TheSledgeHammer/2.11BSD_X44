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

/*
 * File currently a Work in Progress (WIP).
 * Not ready for use.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "citrus_ctype.h"
#include "citrus_types.h"
#include "citrus_stdenc.h"
#include "citrus_frune.h"
#include "citrus_iconv.h"
#include "citrus_iconv_std.h"
#include "citrus_mapper.h"

static void save_encoding_state(struct _citrus_iconv_std_encoding *);
static void restore_encoding_state(struct _citrus_iconv_std_encoding *);
static void init_encoding_state(struct _citrus_iconv_std_encoding *);
static int mbtocsx(struct _citrus_iconv_std_encoding *, _csid_t *, _index_t *, const char **, size_t ,	size_t *);
static int cstombx(struct _citrus_iconv_std_encoding *, char *, size_t, _csid_t, _index_t, size_t *);
static int wctombx(struct _citrus_iconv_std_encoding *, char *, size_t, _wc_t, size_t *);
static int put_state_resetx(struct _citrus_iconv_std_encoding *, char *, size_t, size_t *);
static int get_state_desc_gen(struct _citrus_iconv_std_encoding *, int *);
static int init_encoding(struct _citrus_iconv_std_encoding *, struct _citrus_frune_encoding *, void *, void *);


static int open_csmapper(struct _citrus_csmapper **, const char *, const char *, unsigned long *);
static void close_dsts(struct _citrus_iconv_std_dst_list *);
static int open_dsts(struct _citrus_iconv_std_dst_list *, const struct _citrus_esdb_charset *, const struct _citrus_esdb *);
static void close_srcs(struct _citrus_iconv_std_src_list *);
static int open_srcs(struct _citrus_iconv_std_src_list *, const struct _citrus_esdb *, const struct _citrus_esdb *);
static int do_conv(const struct _citrus_iconv_std_shared *, struct _citrus_iconv_std_context *, _csid_t *, _index_t *);
static int _citrus_iconv_std_iconv_init_shared(struct _citrus_iconv_std_shared *,
		const char *__restrict, const char *__restrict, const char *__restrict,
		const void *__restrict, size_t);
static void _citrus_iconv_std_iconv_uninit_shared(struct _citrus_iconv_std_shared *);
static int _citrus_iconv_std_iconv_convert(
		const struct _citrus_iconv_std_shared *,
		struct _citrus_iconv_std_context *, const char*__restrict* __restrict,
		size_t *__restrict, char *__restrict * __restrict, size_t *__restrict,
		u_int32_t, size_t *__restrict);
static int _citrus_iconv_std_iconv_init_context(struct _citrus_iconv_std_shared *, struct _citrus_iconv_std_context *);
static void _citrus_iconv_std_iconv_uninit_context(struct _citrus_iconv_std_shared *);


struct _citrus_iconv_ops _citrus_iconv_std_iconv_ops = {
		.io_abi_version = _CITRUS_ICONV_ABI_VERSION,
		.io_init_shared = _citrus_iconv_std_iconv_init_shared,
		.io_uninit_shared = _citrus_iconv_std_iconv_uninit_shared,
		.io_convert = _citrus_iconv_std_iconv_convert,
		.io_init_context = _citrus_iconv_std_iconv_init_context,
		.io_uninit_context = _citrus_iconv_std_iconv_uninit_context
};

int
_citrus_iconv_std_iconv_getops(struct _citrus_iconv_ops *ops, size_t lenops, u_int32_t expected_version)
{
	return (_citrus_getops(op, &_citrus_iconv_std_iconv_ops, lenops, _CITRUS_ICONV_ABI_VERSION, expected_version));
}

/*
 * convenience routines for frune.
 */
static __inline void
save_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	_citrus_frune_save_encoding_state(se->se_handle, se->se_ps, se->se_pssaved);
}

static __inline void
restore_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	_citrus_frune_restore_encoding_state(se->se_handle, se->se_ps, se->se_pssaved);
}

static __inline void
init_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	_citrus_frune_init_encoding_state(se->se_handle, se->se_ps, se->se_pssaved);
}

static __inline int
mbtocsx(struct _citrus_iconv_std_encoding *se, _csid_t *csid, _index_t *idx, const char **s, size_t n,	size_t *nresult)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_mbtocsx(fe, csid, idx, s, n, nresult));
}

static __inline int
cstombx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, _csid_t csid, _index_t idx, size_t *nresult)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_cstombx(fe, s, n, csid, idx, nresult));
}

static __inline int
wctombx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, _wc_t wc, size_t *nresult)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_wctombx(fe, s, n, wc, nresult));
}

static __inline int
put_state_resetx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, size_t *nresult)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_put_state_resetx(fe, s, n, nresult));
}

static __inline int
get_state_desc_gen(struct _citrus_iconv_std_encoding *se, int *rstate)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_get_state_desc_gen(fe, rstate));
}

/*
 * init encoding context
 */
static int
init_encoding(struct _citrus_iconv_std_encoding *se, struct _citrus_frune_encoding *fe, void *ps1, void *ps2)
{
	int ret;

	se->se_handle = fe;
	se->se_ps = ps1;
	se->se_pssaved = ps2;
	if (se->se_ps) {
		fe->fe_state = TO_STATE(se->se_ps);
		goto out;
	}
	if (!ret && se->se_pssaved) {
		fe->fe_state = TO_STATE(se->se_pssaved);
		goto out;
	}
	ret = -1;
	return (ret);

out:
	ret = _citrus_stdenc_init_state(fe->fe_info, fe->fe_state);
	return (ret);
}

static int
open_csmapper(struct _citrus_csmapper **rcm, const char *src, const char *dst, unsigned long *rnorm)
{
	int ret;
	struct _citrus_csmapper *cm;

	ret = _citrus_csmapper_open(&cm, src, dst, 0, rnorm);
	if (ret)
		return (ret);
	if ( _citrus_csmapper_get_src_max(cm) != 1 ||  _citrus_csmapper_get_dst_max(cm) != 1 || _citrus_csmapper_get_state_size(cm) != 0) {
		 _citrus_citrus_csmapper_close(cm);
		return (EINVAL);
	}

	*rcm = cm;

	return (0);
}

static void
close_dsts(struct _citrus_iconv_std_dst_list *dl)
{
	struct _citrus_iconv_std_dst *sd;

	while ((sd = TAILQ_FIRST(dl)) != NULL) {
		TAILQ_REMOVE(dl, sd, sd_entry);
		_citrus_csmapper_close(sd->sd_mapper);
		free(sd);
	}
}

static int
open_dsts(struct _citrus_iconv_std_dst_list *dl, const struct _citrus_esdb_charset *ec, const struct _citrus_esdb *dbdst)
{
	int i, ret;
	struct _citrus_iconv_std_dst *sd, *sdtmp;
	unsigned long norm;

	sd = malloc(sizeof(*sd));
	if (sd == NULL) {
		return errno;
	}
	for (i = 0; i < dbdst->db_num_charsets; i++) {
		ret = open_csmapper(&sd->sd_mapper, ec->ec_csname, dbdst->db_charsets[i].ec_csname, &norm);
		if (ret == 0) {
			sd->sd_csid = dbdst->db_charsets[i].ec_csid;
			sd->sd_norm = norm;
			/* insert this mapper by sorted order. */
			TAILQ_FOREACH(sdtmp, dl, sd_entry) {
				if (sdtmp->sd_norm > norm) {
					TAILQ_INSERT_BEFORE(sdtmp, sd, sd_entry);
					sd = NULL;
					break;
				}
			}
			if (sd) {
				TAILQ_INSERT_TAIL(dl, sd, sd_entry);
			}
			sd = malloc(sizeof(*sd));
			if (sd == NULL) {
				ret = errno;
				close_dsts(dl);
				return (ret);
			}
		} else if (ret != ENOENT) {
			close_dsts(dl);
			free(sd);
			return (ret);
		}
	}
	free(sd);
	return (0);
}

static void
close_srcs(struct _citrus_iconv_std_src_list *sl)
{
	struct _citrus_iconv_std_src *ss;

	while ((ss = TAILQ_FIRST(sl)) != NULL) {
		TAILQ_REMOVE(sl, ss, ss_entry);
		close_dsts(&ss->ss_dsts);
		free(ss);
	}
}

static int
open_srcs(struct _citrus_iconv_std_src_list *sl, const struct _citrus_esdb *dbsrc, const struct _citrus_esdb *dbdst)
{
	int i, ret, count = 0;
	struct _citrus_iconv_std_src *ss;

	ss = malloc(sizeof(*ss));
	if (ss == NULL)
		return (errno);

	TAILQ_INIT(&ss->ss_dsts);

	for (i = 0; i < dbsrc->db_num_charsets; i++) {
		ret = open_dsts(&ss->ss_dsts, &dbsrc->db_charsets[i], dbdst);
		if (ret) {
			goto err;
		}
		if (!TAILQ_EMPTY(&ss->ss_dsts)) {
			ss->ss_csid = dbsrc->db_charsets[i].ec_csid;
			TAILQ_INSERT_TAIL(sl, ss, ss_entry);
			ss = malloc(sizeof(*ss));
			if (ss == NULL) {
				ret = errno;
				goto err;
			}
			count++;
			TAILQ_INIT(&ss->ss_dsts);
		}
	}
	free(ss);

	return (count ? 0 : ENOENT);

err:
	free(ss);
	close_srcs(sl);
	return (ret);
}

/* do convert a character */
#define E_NO_CORRESPONDING_CHAR ENOENT /* XXX */

/*ARGSUSED*/
static int
do_conv(const struct _citrus_iconv_std_shared *is, struct _citrus_iconv_std_context *sc, _csid_t *csid, _index_t *idx)
{
	_index_t tmpidx;
	int ret;
	struct _citrus_iconv_std_src *ss;
	struct _citrus_iconv_std_dst *sd;

	TAILQ_FOREACH(ss, &is->is_srcs, ss_entry) {
		if (ss->ss_csid == *csid) {
			TAILQ_FOREACH(sd, &ss->ss_dsts, sd_entry) {
				ret = _citrus_csmapper_convert(sd->sd_mapper, &tmpidx, *idx, NULL);
				switch (ret) {
				case _CITRUS_MAPPER_CONVERT_SUCCESS:
					*csid = sd->sd_csid;
					*idx = tmpidx;
					return (0);
				case _CITRUS_MAPPER_CONVERT_NONIDENTICAL:
					break;
				case _CITRUS_MAPPER_CONVERT_SRC_MORE:
					/*FALLTHROUGH*/
				case _CITRUS_MAPPER_CONVERT_DST_MORE:
					/*FALLTHROUGH*/
				case _CITRUS_MAPPER_CONVERT_FATAL:
					return (EINVAL);
				case _CITRUS_MAPPER_CONVERT_ILSEQ:
					return (EILSEQ);
				}
			}
			break;
		}
	}

	return (E_NO_CORRESPONDING_CHAR);
}

static int
_citrus_iconv_std_iconv_init_shared(
		struct _citrus_iconv_std_shared *is, const char *__restrict curdir,
		const char *__restrict src, const char *__restrict dst,
		const void *__restrict var, size_t lenvar)
{
	int ret;
	struct _citrus_esdb esdbsrc, esdbdst;

	if (is == NULL) {
		ret = errno;
		goto err0;
	}
	ret = _citrus_esdb_open(&esdbsrc, src);
	if (ret) {
		goto err1;
	}
	ret = _citrus_esdb_open(&esdbdst, dst);
	if (ret) {
		goto err2;
	}

	ret = _citrus_frune_open(&is->is_src_encoding, esdbsrc.db_encname, esdbsrc.db_variable, esdbsrc.db_len_variable);
	if (ret) {
		goto err3;
	}
	ret = _citrus_frune_open(&is->is_dst_encoding, esdbdst.db_encname, esdbdst.db_variable, esdbdst.db_len_variable);
	if (ret) {
		goto err4;
	}
	is->is_use_invalid = esdbdst.db_use_invalid;
	is->is_invalid = esdbdst.db_invalid;

	TAILQ_INIT(&is->is_srcs);
	ret = open_srcs(&is->is_srcs, &esdbsrc, &esdbdst);
	if (ret) {
		goto err5;
	}
	_citrus_esdb_close(&esdbsrc);
	_citrus_esdb_close(&esdbdst);

	return (0);

err5:
	_citrus_frune_close(is->is_dst_encoding);
err4:
	_citrus_frune_close(is->is_src_encoding);
err3:
	_citrus_esdb_close(&esdbdst);
err2:
	_citrus_esdb_close(&esdbsrc);
err1:
	free(is);
err0:
	return (ret);
}

static void
_citrus_iconv_std_iconv_uninit_shared(struct _citrus_iconv_std_shared *is)
{
	if (is == NULL) {
		return;
	}

	_citrus_stdenc_uninit(is->is_src_encoding);
	_citrus_stdenc_uninit(is->is_dst_encoding);
	close_srcs(&is->is_srcs);
	free(is);
}

static int
_citrus_iconv_std_iconv_init_context(struct _citrus_iconv_std_shared *is, struct _citrus_iconv_std_context *sc)
{
	int ret;
	size_t szpssrc, szpsdst, sz;
	char *ptr;

	szpssrc = _citrus_stdenc_get_state_size(is->is_src_encoding);
	szpsdst = _citrus_stdenc_get_state_size(is->is_dst_encoding);
	sz = (szpssrc + szpsdst) * 2 + sizeof(struct _citrus_iconv_std_context);
	sc = malloc(sz);
	if (sc == NULL) {
		return (errno);
	}

	ptr = (char *)&sc[1];
	if (szpssrc) {
		init_encoding(&sc->sc_src_encoding, is->is_src_encoding, ptr,
				ptr + szpssrc);
	} else {
		init_encoding(&sc->sc_src_encoding, is->is_src_encoding, NULL, NULL);
	}
	ptr += szpssrc * 2;
	if (szpsdst) {
		init_encoding(&sc->sc_dst_encoding, is->is_dst_encoding, ptr,
				ptr + szpsdst);
	} else {
		init_encoding(&sc->sc_dst_encoding, is->is_dst_encoding, NULL, NULL);
	}

	return (0);
}

static void
_citrus_iconv_std_iconv_uninit_context(struct _citrus_iconv_std_shared *is)
{
	free(is);
}

static int
_citrus_iconv_std_iconv_convert(
		const struct _citrus_iconv_std_shared *is, struct _citrus_iconv_std_context *sc,
		const char *__restrict* __restrict in, size_t *__restrict inbytes,
		char *__restrict* __restrict out, size_t *__restrict outbytes,
		u_int32_t flags, size_t *__restrict invalids)
{
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

			ret = put_state_resetx(&sc->sc_dst_encoding,
					       *out, *outbytes,
					       &szrout);
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
		return (0);
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
		ret = mbtocsx(&sc->sc_src_encoding, &csid, &idx,
			      &tmpin, *inbytes, &szrin);
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
				if ((flags & _CITRUS_ICONV_F_HIDE_INVALID) == 0
						&& is->is_use_invalid) {
					ret = wctombx(&sc->sc_dst_encoding, *out, *outbytes,
							is->is_invalid, &szrout);
					if (ret)
						goto err;
				}
				goto next;
			} else {
				goto err;
			}
		}
		/* csid/index -> mb */
		ret = cstombx(&sc->sc_dst_encoding, *out, *outbytes, csid, idx,
				&szrout);
		if (ret)
			goto err;
next:
		_DIAGASSERT(*inbytes >= szrin && *outbytes >= szrout);
		*inbytes -= tmpin - *in; /* szrin is insufficient on \0. */
		*in = tmpin;
		*outbytes -= szrout;
		*out += szrout;
	}
	*invalids = inval;

	return (0);

err:
	restore_encoding_state(&sc->sc_src_encoding);
	restore_encoding_state(&sc->sc_dst_encoding);
err_norestore:
	*invalids = inval;
	return (ret);
}
