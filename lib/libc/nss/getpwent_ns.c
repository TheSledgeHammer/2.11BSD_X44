/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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

/*
 * Portions Copyright (c) 1994, 1995, Jason Downs.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getpwent.c	8.2 (Berkeley) 4/27/95";
#else
__RCSID("$NetBSD: getpwent.c,v 1.56 2003/11/26 00:48:59 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <nsswitch.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
#include <ndbm.h>
#include "pw_ndbm.h"
#else
#include <db.h>
#include "pw_db.h"
#endif

#ifdef HESIOD
#include <hesiod.h>
#endif

#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif

#include "pw_private.h"
#include "pw_storage.h"

const char __yp_token[] = "__YP!";	/* Let pwd_mkdb pull this in. */

#ifdef YP
static const char __nis_pw_n_1[] = "master.passwd.byname";
static const char __nis_pw_n_2[] = "passwd.byname";
static const char __nis_pw_u_1[] = "master.passwd.byuid";
static const char __nis_pw_u_2[] = "passwd.byuid";

static const char *__nis_pw_n_map[4] = { __nis_pw_n_2, __nis_pw_n_2, __nis_pw_n_2, __nis_pw_n_1 };
static const char *__nis_pw_u_map[4] = { __nis_pw_u_2, __nis_pw_u_2, __nis_pw_u_2, __nis_pw_u_1 };

/* macros for deciding which NIS maps to use. */
#define	PASSWD_BYNAME(x)	((x)->maptype == YPMAP_MASTER ? __nis_pw_n_1 : __nis_pw_n_2)
#define	PASSWD_BYUID(x)		((x)->maptype == YPMAP_MASTER ? __nis_pw_u_1 : __nis_pw_u_2)
#endif

static int _pws_parse(const char *, struct passwd *, char *, size_t, int);
static int _pws_bad_method(void *, void *, va_list);

static int _pws_local_start(void *, void *, va_list);
static int _pws_local_end(void *, void *, va_list);
static int _pws_local_search(void *, void *, va_list);

#ifdef HESIOD
static int _pws_hesiod_start(void *, void *, va_list);
static int _pws_hesiod_end(void *, void *, va_list);
static int _pws_hesiod_search(void *, void *, va_list);
#endif

#ifdef YP
static int _pws_yp_maptype(struct passwd_storage *);
static int _pws_yp_start(void *, void *, va_list);
static int _pws_yp_end(void *, void *, va_list);
static int _pws_yp_parse(const char *, struct passwd *, char *, size_t, struct passwd_storage *);
static int _pws_yp_scan(struct passwd *, struct passwd_storage *, char *, size_t, const char *);
static int _pws_yp_search(void *, void *, va_list);
#endif

#ifdef _GROUP_COMPAT
static int _pws_ns_start(struct passwd_storage *);
static int _pws_ns_end(struct passwd_storage *);
static int _pws_ns_search(struct passwd *, char *, size_t, struct passwd_storage *, int, const char *, uid_t);

static void _pws_compat_proto_set(struct passwd *, struct passwd_storage *, int);
static int _pws_compat_add_exclude(struct passwd_storage *, const char *);
static int _pws_compat_is_excluded(struct passwd_storage *, const char *);

static int _pws_compat_start(void *, void *, va_list);
static int _pws_compat_end(void *, void *, va_list);
static int _pws_compat_search(void *, void *, va_list);
#endif

static int
_pws_parse(s, pw, buffer, buflen, old)
	const char *s;
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	int old;
{
	int flags;

	if (strlcpy(buffer, s, buflen) >= buflen) {
		return (0);
	}
	flags = _PASSWORD_NOWARN;
	if (old) {
		flags |= _PASSWORD_OLDFMT;
	}
	return (__pw_scan(buffer, pw, &flags));
}

/* nsdispatch: methods */
static int
_pws_bad_method(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	static int warned;

	if (!warned) {
		syslog(LOG_ERR, "nsswitch.conf passwd_compat database can't use '%s'", (char *)nscb);
	}
	warned = 1;
	return (NS_UNAVAIL);
}

/* Local/Files Methods */
static int
_pws_local_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd_storage *state;

	state = va_arg(ap, struct passwd_storage *);

	int rval;

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	rval = _pw_start(&state->db, &state->fp, &state->keynum, &state->version);
#else
	rval = _pw_start(&state->db, &state->keynum, &state->version);
#endif
	return (rval);
}

static int
_pws_local_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd_storage *state;

	state = va_arg(ap, struct passwd_storage *);

	int rval;

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	rval = _pw_end(state->db, &state->fp, &state->rewind, &state->keynum);
#else
	rval = _pw_end(state->db, &state->keynum);
#endif
	return (rval);
}

static int
_pws_local_search(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	int search;
	const char *name;
	uid_t uid;

	pw = va_arg(ap, struct passwd *);
	buffer = va_arg(ap, char *);
	buflen = va_arg(ap, size_t);
	state = va_arg(ap, struct passwd_storage *);
	search = va_arg(ap, int);
	name = va_arg(ap, const char *);
	uid = va_arg(ap, uid_t);

	const void *from;
	size_t	fromlen;
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	DBM		key;
#else
	DBT 	key;
#endif
	int 	rval;

	switch (search) {
	case _PW_KEYBYNUM:
		if (state->keynum == -1) {
			return (NS_NOTFOUND); /* no more records */
		}
		state->keynum++;
		from = &state->keynum;
		fromlen = sizeof(state->keynum);
		break;
	case _PW_KEYBYNAME:
		from = name;
		fromlen = strlen(name);
		break;
	case _PW_KEYBYUID:
		from = &uid;
		fromlen = sizeof(uid);
		break;
	default:
		abort();
	}

	if (buflen <= fromlen) {
		errno = ERANGE;
		return (NS_UNAVAIL);
	}
	buffer[0] = search;
	bcopy(from, buffer + 1, fromlen);
	_pw_setkey(&key, buffer, fromlen + 1);
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	rval = _pw_getkey(state->db, &key, pw, buffer, buflen, NULL, &state->rewind, state->version);
#else
	rval = _pw_getkey(state->db, &key, pw, buffer, buflen, NULL, state->version);
#endif
	return (rval);
}

/* Hesiod/DNS Methods */
#ifdef HESIOD

static int
_pws_hesiod_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd_storage *state;

	state = va_arg(ap, struct passwd_storage *);

	int rval;

	state->num = 0;
	if (state->context == NULL) {			/* setup Hesiod */
		rval = hesiod_init(&state->context);
		if (rval == -1) {
			rval = NS_UNAVAIL;
		}
	} else {
		rval = NS_SUCCESS;
	}
	return (rval);
}

static int
_pws_hesiod_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd_storage *state;

	state = va_arg(ap, struct passwd_storage *);

	state->num = 0;
	if (state->context) {
		hesiod_end(state->context);
		state->context = NULL;
	}
	return (NS_SUCCESS);
}

static int
_pws_hesiod_search(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	int search;
	const char *name;
	uid_t uid;

	pw = va_arg(ap, struct passwd *);
	buffer = va_arg(ap, char *);
	buflen = va_arg(ap, size_t);
	state = va_arg(ap, struct passwd_storage *);
	search = va_arg(ap, int);
	name = va_arg(ap, const char *);
	uid = va_arg(ap, uid_t);

	const void *from;
	size_t	fromlen;
	const char	**curzone, **zones;
	char **hp;
	int  rval;

	static const char *_dns_uid_zones[] = {
			"uid",
			"passwd",
			NULL
	};

	static const char *_dns_nam_zones[] = {
			"passwd",
			NULL
	};

	switch (search) {
	case _PW_KEYBYNUM:
		if (state->num == -1) {
			return (NS_NOTFOUND);	/* no more hesiod records */
		}
		state->num++;
		from = &state->num;
		fromlen = sizeof(state->num);
		zones = _dns_nam_zones;
		break;
	case _PW_KEYBYNAME:
		from = name;
		fromlen = strlen(name);
		zones = _dns_nam_zones;
		break;
	case _PW_KEYBYUID:
		from = &uid;
		fromlen = sizeof(uid);
		zones = _dns_uid_zones;
		break;
	default:
		abort();
	}

	for (curzone = zones; *curzone; curzone++) {	/* search zones */
		hp = hesiod_resolve(state->context, buffer, *curzone);
		if (hp != NULL) {
			break;
		} else {
			rval = NS_UNAVAIL;
			goto scan_out;
		}
	}
	if (*curzone == NULL) {
		goto scan_out;
	}

	if (buflen <= fromlen) {
		return (NS_UNAVAIL);
	}
	buffer[0] = search;
	bcopy(from, buffer + 1, fromlen);
	rval = _pws_parse(hp[0], buffer, buflen, 1);
	if (rval) {
		rval = NS_SUCCESS;
	} else {
		rval = NS_UNAVAIL;
	}

scan_out:
	if (hp) {
		hesiod_free_list(state->context, hp);
	}
	return (rval);
}

#endif

/* YP/NIS Methods */
#ifdef YP

static int
_pws_yp_maptype(state)
	struct passwd_storage *state;
{
	int order, rval;

	/* determine where to get pw_passwd from */
	if (state->maptype == YPMAP_UNKNOWN) {
		state->maptype = YPMAP_NONE;	/* default to no adjunct */
		if (geteuid() != 0) {			/* non-root can't use adjunct */
			return (NS_SUCCESS);
		}

		/* look for "master.passwd.*" */
		rval = yp_order(state->domain, "master.passwd.byname", &order);
		if (rval == 0) {
			state->maptype = YPMAP_MASTER;
			return (NS_SUCCESS);
		}

		/*
		 * NIS+ in YP compat mode doesn't support
		 * YPPROC_ORDER -- no point in continuing.
		 */
		if (rval == YPERR_YPERR) {
			return (NS_UNAVAIL);
		}

		/* master.passwd doesn't exist -- try passwd.adjunct */
		if (rval == YPERR_MAP) {
			rval = yp_order(state->domain, "passwd.adjunct.byname", &order);
			if (rval == 0) {
				state->maptype = YPMAP_ADJUNCT;
			}
			return (NS_SUCCESS);
		}
	}
	return (NS_SUCCESS);
}

static int
_pws_yp_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd_storage *state;

	state = va_arg(ap, struct passwd_storage *);

	int rval;

	state->done = 0;
	if (state->current) {
		free(state->current);
		state->current = NULL;
	}
	if (state->domain == NULL) { /* setup NIS */
		rval = yp_get_default_domain(&state->domain);
		switch (rval) {
		case 0:
			rval = NS_SUCCESS;
			break;
		case YPERR_RESRC:
			rval = NS_TRYAGAIN;
			break;
		default:
			rval = NS_UNAVAIL;
			break;
		}
	}
	if (rval == NS_SUCCESS) {
		rval = _pws_yp_maptype(state);
	}
	return (rval);
}

static int
_pws_yp_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd_storage *state;

	state = va_arg(ap, struct passwd_storage *);

	if (state->domain) {
		state->domain = NULL;
	}
	state->done = 0;
	if (state->current) {
		free(state->current);
	}
	state->current = NULL;
	state->maptype = YPMAP_UNKNOWN;
	return (NS_SUCCESS);
}

static int
_pws_yp_parse(s, pw, buffer, buflen, state)
	const char *s;
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
{
	size_t	elen;
	int rval;

	elen = strlen(s) + 1;
	if (elen >= buflen) {
		return (0);
	}
	rval = _pws_parse(s, pw, buffer, buflen, !(state->maptype == YPMAP_MASTER));
	if (!rval) {
		return (0);
	}
	if ((state->maptype == YPMAP_ADJUNCT) &&
		    (strstr(pw->pw_passwd, "##") != NULL)) {
		char *data, *bp;
		int	datalen;

		if (yp_match(state->domain, "passwd.adjunct.byname", pw->pw_name,
				(int) strlen(pw->pw_name), &data, &datalen) == 0) {
			/* skip name to get password */
			if (strsep(&data, ":") != NULL && (bp = strsep(&data, ":")) != NULL) {
				/* store new pw_passwd after entry */
				if (strlcpy(buf + elen, bp, buflen - elen) >= buflen - elen) {
					free(data);
					return (0);
				}
				pw->pw_passwd = &buffer[elen];
			}
			free(data);
		}
	}
	return (1);
}

static int
_pws_yp_scan(pw, state, buffer, buflen, map_arr)
	struct passwd *pw;
	struct passwd_storage *state;
	char *buffer;
	size_t buflen;
	const char *map_arr;
{
	char *key, *data;
	int keylen, datalen, rval, nisr;

next_entry:
	key = NULL;
	data = NULL;
	rval = NS_NOTFOUND;

	if (state->current) {
		nisr = yp_next(state->domain, map_arr, state->current,
				state->currentlen, &key, &keylen, &data, &datalen);
		free(state->current);
		state->current = NULL;
		switch (nisr) {
		case 0:
			state->current = key;
			state->currentlen = keylen;
			key = NULL;
			break;
		case YPERR_NOMORE:
			state->done = 1;
			goto scan_out;
		default:
			rval = NS_UNAVAIL;
			goto scan_out;
		}
	} else {
		nisr = yp_first(state->domain, map_arr, state->current,
				state->currentlen, &data, &datalen);
		if (nisr) {
			rval = NS_UNAVAIL;
			goto scan_out;
		}
	}

	data[datalen] = '\0';				/* clear trailing \n */
	rval = _pws_yp_parse(data, pw, buffer, buflen, state);
	if (rval) {
		rval = NS_SUCCESS;
	} else {
		if (key) {
			free(key);
		}
		goto next_entry;
	}

scan_out:
	if (key) {
		free(key);
	}
	if (data) {
		free(data);
	}
	return (rval);
}

static int
_pws_yp_search(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	int search;
	const char *name;
	uid_t uid;

	pw = va_arg(ap, struct passwd *);
	buffer = va_arg(ap, char *);
	buflen = va_arg(ap, size_t);
	state = va_arg(ap, struct passwd_storage *);
	search = va_arg(ap, int);
	name = va_arg(ap, const char *);
	uid = va_arg(ap, uid_t);

	const void *from;
	const char *map_arr;
	size_t	fromlen, nmaps;
	char *data;
	int rval, nisr, datalen;

	switch (search) {
	case _PW_KEYBYNUM:
		break;
	case _PW_KEYBYNAME:
		from = name;
		fromlen = strlen(name);
		map_arr = __nis_pw_n_map;
		nmaps = (sizeof(__nis_pw_n_map)/sizeof(__nis_pw_n_map[0]));
		break;
	case _PW_KEYBYUID:
		from = &uid;
		fromlen = sizeof(uid);
		map_arr = __nis_pw_u_map;
		nmaps = (sizeof(__nis_pw_u_map)/sizeof(__nis_pw_u_map[0]));
		break;
	default:
		abort();
	}

	if (buflen <= fromlen) {
		return (NS_UNAVAIL);
	}

	buffer[0] = search;
	bcopy(from, buffer + 1, fromlen);
	rval = NS_NOTFOUND | NS_UNAVAIL;
	if (search != _PW_KEYBYNUM) {
		data = NULL;
		nisr = yp_match(state->domain, map_arr[state->maptype], buffer,
				(int) strlen(buffer), &data, &datalen);
		switch (nisr) {
		case 0:
			data[datalen] = '\0';
			rval = _pws_yp_parse(data, pw, buffer, buflen, state);
			if (rval) {
				rval = NS_SUCCESS; /* validate line */
			} else {
				rval = NS_UNAVAIL;
			}
			break;
		case YPERR_KEY:
			rval = NS_NOTFOUND;
			break;
		default:
			rval = NS_UNAVAIL;
			break;
		}
	}

	if (state->done) {
		return (NS_NOTFOUND);
	}

	rval = _pws_yp_scan(pw, state, buffer, buflen, map_arr);
	if (rval == NS_UNAVAIL) {
		return (rval);
	}
	if (data) {
		free(data);
	}
	return (rval);
}

#endif

/* Group Compat Methods */
#ifdef _GROUP_COMPAT

static int
_pws_ns_start(state)
	struct passwd_storage *state;
{
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_pws_bad_method, "files")
			NS_DNS_CB(_pws_hesiod_start, NULL)
			NS_NIS_CB(_pws_yp_start, NULL)
			NS_COMPAT_CB(_pws_bad_method, "compat")
			NS_NULL_CB
	};

	return (nsdispatch(NULL, dtab, NSDB_PASSWD_COMPAT, "_pws_ns_start", __nsdefaultnis, state));
}

static int
_pws_ns_end(state)
	struct passwd_storage *state;
{
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_pws_bad_method, "files")
			NS_DNS_CB(_pws_hesiod_end, NULL)
			NS_NIS_CB(_pws_yp_end, NULL)
			NS_COMPAT_CB(_pws_bad_method, "compat")
			NS_NULL_CB
	};

	return (nsdispatch(NULL, dtab, NSDB_PASSWD_COMPAT, "_pws_ns_end", __nsdefaultnis, state));
}

static int
_pws_ns_search(pw, buffer, buflen, state, search, name, uid)
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	int search;
	const char *name;
	uid_t uid;
{
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_pws_bad_method, "files")
			NS_DNS_CB(_pws_hesiod_search, NULL)
			NS_NIS_CB(_pws_yp_search, NULL)
			NS_COMPAT_CB(_pws_bad_method, "compat")
			NS_NULL_CB
	};

	return (nsdispatch(NULL, dtab, NSDB_PASSWD_COMPAT, "_pws_ns_search", __nsdefaultnis, pw, buffer, buflen, state, search, name, uid));
}

static int
_pws_compat_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd_storage *state;

	state = va_arg(ap, struct passwd_storage *);

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	datum *key;
	datum *pkey;
#else
	DBT key, data;
	DBT pkey, pdata;
#endif
	char bf[MAXLOGNAME];
	int rval, ret;

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	rval = _pw_start(&state->db, &state->fp, &state->keynum, &state->version);
#else
	rval = _pw_start(&state->db, &state->keynum, &state->version);
#endif

	if (rval == NS_SUCCESS) {
		state->mode = COMPAT_NOTOKEN;

		/*
		 *	Determine if the "compat" token is present in pwd.db;
		 *	either "__YP!" or PW_KEYBYNAME+"+".
		 *	Only works if pwd_mkdb installs the token.
		 */
		_pw_setkey(&key, (unsigned char*) __UNCONST(__yp_token),
				strlen(__yp_token));

		bf[0] = _PW_KEYBYNAME; /* Pre-token database support. */
		bf[1] = '+';
		_pw_setkey(&pkey, bf, 2);
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
		ret = (_pw_getdb(state->db, &key, &state->rewind) == 0 || _pw_getdb(state->db, &pkey, &state->rewind) == 0);
#else
		ret = (_pw_getdb(state->db, &key, &data, 0) == 0
				|| _pw_getdb(state->db, &pkey, &pdata, 0) == 0);
#endif
		if (ret) {
			state->mode = COMPAT_NONE;
		}
		rval = _pws_ns_start(state);
	}
	return (rval);
}

static int
_pws_compat_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd_storage *state;

	state = va_arg(ap, struct passwd_storage *);

	int rval;

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	rval = _pw_end(state->db, &state->fp, &state->rewind, &state->keynum);
#else
	rval = _pw_end(state->db, &state->keynum);
#endif

	if (rval == NS_SUCCESS) {
		state->mode = COMPAT_NOTOKEN;
		if (state->user) {
			free(state->user);
		}
		state->user = NULL;
		if (state->exclude != NULL) {
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
			_pw_closedb(state->exclude, state->fp);
#else
			_pw_closedb(state->exclude);
#endif
		}
		state->exclude = NULL;
		state->proto.pw_name = NULL;
		state->protoflags = 0;
		rval = _pws_ns_end(state);
	}
	return (rval);
}

static void
_pws_compat_proto_set(pw, state, pwflags)
	struct passwd *pw;
	struct passwd_storage *state;
	int pwflags;
{
	struct passwd *proto;
	char *ptr;

	proto = &state->proto;
	ptr = state->protobuf;

	/* name */
	if (pw->pw_name && (pw->pw_name)[0]) {
		bcopy(pw->pw_name, ptr, (strlen(pw->pw_name) + 1));
		proto->pw_name = ptr;
		ptr += (strlen(pw->pw_name) + 1);
	} else {
		proto->pw_name = (char*) NULL;
	}

	/* password */
	if (pw->pw_passwd && (pw->pw_passwd)[0]) {
		bcopy(pw->pw_passwd, ptr, (strlen(pw->pw_passwd) + 1));
		proto->pw_passwd = ptr;
		ptr += (strlen(pw->pw_passwd) + 1);
	} else {
		proto->pw_passwd = (char*) NULL;
	}

	/* uid */
	proto->pw_uid = pw->pw_uid;

	/* gid */
	proto->pw_gid = pw->pw_gid;

	/* change (ignored anyway) */
	proto->pw_change = pw->pw_change;

	/* class (ignored anyway) */
	proto->pw_class = "";

	/* gecos */
	if (pw->pw_gecos && (pw->pw_gecos)[0]) {
		bcopy(pw->pw_gecos, ptr, (strlen(pw->pw_gecos) + 1));
		proto->pw_gecos = ptr;
		ptr += (strlen(pw->pw_gecos) + 1);
	} else {
		proto->pw_gecos = (char*) NULL;
	}

	/* dir */
	if (pw->pw_dir && (pw->pw_dir)[0]) {
		bcopy(pw->pw_dir, ptr, (strlen(pw->pw_dir) + 1));
		proto->pw_dir = ptr;
		ptr += (strlen(pw->pw_dir) + 1);
	} else {
		proto->pw_dir = (char*) NULL;
	}

	/* shell */
	if (pw->pw_shell && (pw->pw_shell)[0]) {
		bcopy(pw->pw_shell, ptr, (strlen(pw->pw_shell) + 1));
		proto->pw_shell = ptr;
		ptr += (strlen(pw->pw_shell) + 1);
	} else {
		proto->pw_shell = (char*) NULL;
	}

	/* expire (ignored anyway) */
	proto->pw_expire = pw->pw_expire;

	/* flags */
	state->protoflags = pwflags;
}

static int
_pws_compat_add_exclude(state, name)
	struct passwd_storage *state;
	const char *name;
{
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	datum key, data;
#else
	DBT	key, data;
#endif
	int ret;

	if (state->exclude == NULL) {
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
		state->exclude = dbm_open(NULL, O_RDWR, 600);
#else
		state->exclude = dbopen(NULL, O_RDWR, 600, DB_HASH, NULL);
#endif
		if (state->exclude == NULL) {
			return (0);
		}
	}
	/* set up the key */
	_pw_setkey(&key, (unsigned char *)__UNCONST(name), strlen(name));
	/* data is nothing */
	_pw_setkey(&data, (unsigned char *)__UNCONST(name), strlen(name));

	/* store it */
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	ret = _pw_storedb(state->exclude, key, data, 0);
#else
	ret = _pw_storedb(state->exclude, &key, &data, 0);
#endif
	if (ret == -1 ) {
		return (0);
	}
	return (1);
}

static int
_pws_compat_is_excluded(state, name)
	struct passwd_storage *state;
	const char *name;
{
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	datum key;
#else
	DBT	key, data;
#endif
	int ret;
	if (state->exclude == NULL) {
		return (0);			/* nothing excluded */
	}
	/* set up the key */
	_pw_setkey(&key, (unsigned char*) __UNCONST(name), strlen(name));

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	ret = _pw_getdb(state->exclude, &key, 0);
#else
	ret = _pw_getdb(state->exclude, &key, &data, 0);
#endif
	if (ret == 0) {
		return (1); /* is excluded */
	}
	return (0);
}

static int
_pws_compat_search(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	int search;
	const char *name;
	uid_t uid;

	pw = va_arg(ap, struct passwd *);
	buffer = va_arg(ap, char *);
	buflen = va_arg(ap, size_t);
	state = va_arg(ap, struct passwd_storage *);
	search = va_arg(ap, int);
	name = va_arg(ap, const char *);
	uid = va_arg(ap, uid_t);

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	datum	key;
#else
	DBT 	key;
#endif
	const char	*user, *host, *dom;
	const void	*from;
	size_t fromlen;
	int rval, pwflags;

	if (buflen <= 1) {			/* buffer too small */
		return (NS_UNAVAIL);
	}

	for (;;) { /* loop over pwd.db */
		if (state->mode != COMPAT_NOTOKEN && state->mode != COMPAT_NONE) {
			/* doing a compat lookup */
			switch (state->mode) {
			case COMPAT_FULL:
				rval = _pws_ns_search(pw, buffer, buflen, state, search, name,
						uid);
				if (rval != NS_SUCCESS) {
					state->mode = COMPAT_NONE;
				}
				break;
			case COMPAT_NETGROUP:
				/* get next user from netgroup */
				rval = getnetgrent(&host, &user, &dom);
				if (rval == 0) {
					endnetgrent();
					state->mode = COMPAT_NONE;
					break;
				}
				if (!user || !*user) {
					break;
				}
				rval = _pws_ns_search(pw, buffer, buflen, state, _PW_KEYBYNAME, user, 0);
				break;
			case COMPAT_USER:
				/* get specific user */
				if (state->user == NULL) {
					state->mode = COMPAT_NONE;
					break;
				}
				rval = _pws_ns_search(pw, buffer, buflen, state, _PW_KEYBYNAME, state->user, 0);
				free(state->user);
				state->user = NULL;
				state->mode = COMPAT_NONE;
				break;
			case COMPAT_NOTOKEN:
			case COMPAT_NONE:
				abort();
			}

			if (rval != NS_SUCCESS) {	/* if not matched, next loop */
				continue;
			}

			_pws_compat_proto_set(pw, state, state->protoflags);

			if (_pws_compat_is_excluded(state, pw->pw_name)) {
				continue;				/* excluded; next loop */
			}

			if ((search == _PW_KEYBYNAME && strcmp(pw->pw_name, name) != 0)
					|| (search == _PW_KEYBYUID && pw->pw_uid != uid)) {
				continue; 				/* not specific; next loop */
			}
			break;						/* exit loop if found */
		} else {
			state->proto.pw_name = NULL;
		}

		if (state->mode == COMPAT_NOTOKEN) {
			/* no compat token; do direct lookup */
			switch (search) {
			case _PW_KEYBYNUM:
				if (state->keynum == -1) {/* no more records */
					return (NS_NOTFOUND);
				}
				state->keynum++;
				from = &state->keynum;
				fromlen = sizeof(state->keynum);
				break;
			case _PW_KEYBYNAME:
				from = name;
				fromlen = strlen(name);
				break;
			case _PW_KEYBYUID:
				from = &uid;
				fromlen = sizeof(uid);
				break;
			default:
				abort();
			}
			buffer[0] = search;
		} else {
			/* compat token; do line by line */
			if (state->keynum == -1) { /* no more records */
				return (NS_NOTFOUND);
			}
			state->keynum++;
			from = &state->keynum;
			fromlen = sizeof(state->keynum);
			buffer[0] = _PW_KEYBYNUM;
		}

		if (buflen <= fromlen) {
			return (NS_UNAVAIL);
		}

		bcopy(from, buffer + 1, fromlen);
		_pw_setkey(&key, buffer, fromlen + 1);
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
		rval = _pw_getkey(state->db, &key, pw, buffer, buflen, &pwflags,
				&state->rewind, state->version);
#else
		rval = _pw_getkey(state->db, &key, pw, buffer, buflen, &pwflags,
				state->version);
#endif
		if (rval != NS_SUCCESS) {
			break;
		}

		if (state->mode == COMPAT_NOTOKEN) {
			break;
		}

		if (pw->pw_name[0] == '+') {
			/* compat inclusion */
			switch (pw->pw_name[1]) {
			case '\0':
				state->mode = COMPAT_FULL;
				break;
			case '@':
				state->mode = COMPAT_NETGROUP;
				setnetgrent(pw->pw_name + 2);
				break;
			default:
				state->mode = COMPAT_USER;
				if (state->user) {
					free(state->user);
				}
				state->user = strdup(pw->pw_name + 1);
				break;
			}
			_pws_compat_proto_set(pw, state, pwflags);
			continue;
		} else if (pw->pw_name[0] == '-') {
			/* compat exclusion */
			rval = NS_SUCCESS;
			switch (pw->pw_name[1]) {
			case '\0':
				break;
			case '@':
				setnetgrent(pw->pw_name + 2);
				while (getnetgrent(&host, &user, &dom)) {
					if (!user || !*user) {
						continue;
					}
					if (!_compat_add_exclude(state, user)) {
						rval = NS_UNAVAIL;
						break;
					}
				}
				endnetgrent();
				break;
			default:
				if (!_compat_add_exclude(state, pw->pw_name + 1)) {
					rval = NS_UNAVAIL;
				}
				break;
			}
			if (rval != NS_SUCCESS) {
				break;
			}
			continue;
		}
		if (search == _PW_KEYBYNUM
				|| (search == _PW_KEYBYUID && pw->pw_uid == uid)
				|| (search == _PW_KEYBYNAME && strcmp(pw->pw_name, name) == 0)) {
			break; /* token mode match found */
		}
	}
	if (rval == NS_NOTFOUND
			&& (search == _PW_KEYBYNUM || state->mode != COMPAT_NOTOKEN)) {
		state->keynum = -1; /* flag `no more records' */
	}
	if (rval == NS_SUCCESS) {
		if ((search == _PW_KEYBYNAME && strcmp(pw->pw_name, name) != 0)
				|| (search == _PW_KEYBYUID && pw->pw_uid != uid)) {
			rval = NS_NOTFOUND;
		}
	}
	return (rval);
}

#endif

int
_pws_start(state)
	struct passwd_storage *state;
{
	int rval;
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_pws_local_start, NULL)
			NS_DNS_CB(_pws_hesiod_start, NULL)
			NS_NIS_CB(_pws_yp_start, NULL)
			NS_COMPAT_CB(_pws_compat_start, NULL)
			NS_NULL_CB
	};

	rval = nsdispatch(NULL, dtab, NSDB_PASSWD, "_pws_start", __nsdefaultcompat, state);
	return ((rval == NS_SUCCESS) ? 1 : 0);
}

int
_pws_end(state)
	struct passwd_storage *state;
{
	int rval;
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_pws_local_end, NULL)
			NS_DNS_CB(_pws_hesiod_end, NULL)
			NS_NIS_CB(_pws_yp_end, NULL)
			NS_COMPAT_CB(_pws_compat_end, NULL)
			NS_NULL_CB
	};

	rval = nsdispatch(NULL, dtab, NSDB_PASSWD, "_pws_end", __nsdefaultcompat, state);
	return ((rval == NS_SUCCESS) ? 1 : 0);
}

int
_pws_search(pw, buffer, buflen, state, search, name, uid)
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	int search;
	const char *name;
	uid_t uid;
{
	int rval;
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_pws_local_search, NULL)
			NS_DNS_CB(_pws_hesiod_search, NULL)
			NS_NIS_CB(_pws_yp_search, NULL)
			NS_COMPAT_CB(_pws_compat_search, NULL)
			NS_NULL_CB
	};

	rval = nsdispatch(NULL, dtab, NSDB_PASSWD, "_pws_search", __nsdefaultcompat, state);
	return ((rval == NS_SUCCESS) ? 1 : 0);
}
