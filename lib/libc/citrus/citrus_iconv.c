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

void save_encoding_state(struct _citrus_iconv_std_encoding *);
void restore_encoding_state(struct _citrus_iconv_std_encoding *);
void init_encoding_state(struct _citrus_iconv_std_encoding *);
int mbtocsx(struct _citrus_iconv_std_encoding *, _csid_t *, _index_t *, const char **, size_t ,	size_t *);
int cstombx(struct _citrus_iconv_std_encoding *, char *, size_t, _csid_t, _index_t, size_t *);
int wctombx(struct _citrus_iconv_std_encoding *, char *, size_t, _wc_t, size_t *);
int put_state_resetx(struct _citrus_iconv_std_encoding *, char *, size_t, size_t *);
int get_state_desc_gen(struct _citrus_iconv_std_encoding *, int *);

/*
 * convenience routines for frune.
 */
void
save_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	_citrus_frune_save_encoding_state(se->se_handle, se->se_ps, se->se_pssaved);
}

void
restore_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	_citrus_frune_restore_encoding_state(se->se_handle, se->se_ps, se->se_pssaved);
}

void
init_encoding_state(struct _citrus_iconv_std_encoding *se)
{
	_citrus_frune_init_encoding_state(se->se_handle, se->se_ps, se->se_pssaved);
}

int
mbtocsx(struct _citrus_iconv_std_encoding *se, _csid_t *csid, _index_t *idx, const char **s, size_t n,	size_t *nresult)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_mbtocsx(fe, csid, idx, s, n, nresult));
}

int
cstombx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, _csid_t csid, _index_t idx, size_t *nresult)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_cstombx(fe, s, n, csid, idx, nresult));
}

int
wctombx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, _wc_t wc, size_t *nresult)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_wctombx(fe, s, n, wc, nresult));
}

int
put_state_resetx(struct _citrus_iconv_std_encoding *se, char *s, size_t n, size_t *nresult)
{
	struct _citrus_frune_encoding *fe;

	fe = se->se_handle;
	fe->fe_state = TO_STATE(se->se_ps);
	return (_citrus_frune_put_state_resetx(fe, s, n, nresult));
}

int
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
int
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

#ifdef notyet
static int
open_csmapper(struct _citrus_csmapper **rcm, const char *src, const char *dst, unsigned long *rnorm)
{
	int ret;
	struct _citrus_csmapper *cm;

	ret = _citrus_csmapper_open(&cm, src, dst, 0, rnorm);
	if (ret)
		return ret;
	if ( _citrus_csmapper_get_src_max(cm) != 1 ||  _citrus_csmapper_get_dst_max(cm) != 1 || _citrus_csmapper_get_state_size(cm) != 0) {
		 _citrus_citrus_csmapper_close(cm);
		return EINVAL;
	}

	*rcm = cm;

	return 0;
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
				return ret;
			}
		} else if (ret != ENOENT) {
			close_dsts(dl);
			free(sd);
			return ret;
		}
	}
	free(sd);
	return 0;
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
		return errno;

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
	return ret;
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
				ret = _citrus_csmapper_convert(sd->sd_mapper, &tmpidx, *idx, NULL);
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
_citrus_iconv_std_iconv_init_shared(struct _citrus_iconv_shared *ci, const char * __restrict curdir, const char * __restrict src, const char * __restrict dst, const void * __restrict var, size_t lenvar)
{
	int ret;
	struct _citrus_iconv_std_shared *is;
	struct _citrus_esdb esdbsrc, esdbdst;

	is = malloc(sizeof(*is));
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
	ci->ci_closure = is;

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
#endif
