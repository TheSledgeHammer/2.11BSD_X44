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

/*
 * Work in Progress
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

#ifdef YP
static const char __nis_pw_n_1[] = "master.passwd.byname";
static const char __nis_pw_n_2[] = "passwd.byname";
static const char __nis_pw_u_1[] = "master.passwd.byuid";
static const char __nis_pw_u_2[] = "passwd.byuid";

static const char * const __nis_pw_n_map[4] = { __nis_pw_n_2, __nis_pw_n_2, __nis_pw_n_2, __nis_pw_n_1 };
static const char * const __nis_pw_u_map[4] = { __nis_pw_u_2, __nis_pw_u_2, __nis_pw_u_2, __nis_pw_u_1 };

/* macros for deciding which NIS maps to use. */
#define	PASSWD_BYNAME(x)	((x)->maptype == NISMAP_MASTER ? __nis_pw_n_1 : __nis_pw_n_2)
#define	PASSWD_BYUID(x)		((x)->maptype == NISMAP_MASTER ? __nis_pw_u_1 : __nis_pw_u_2)
#endif

static int _pws_parse(const char *, struct passwd *, char *, size_t, int);

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

static int _pws_compat_start(void *, void *, va_list);
static int _pws_compat_end(void *, void *, va_list);
static int _pws_compat_search(void *, void *, va_list);

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
	if (state->maptype == NISMAP_UNKNOWN) {
		state->maptype = NISMAP_NONE;	/* default to no adjunct */
		if (geteuid() != 0) {			/* non-root can't use adjunct */
			return (NS_SUCCESS);
		}

		/* look for "master.passwd.*" */
		rval = yp_order(state->domain, "master.passwd.byname", &order);
		if (rval == 0) {
			state->maptype = NISMAP_MASTER;
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
				state->maptype = NISMAP_ADJUNCT;
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
	state->maptype = NISMAP_UNKNOWN;
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
	rval = _pws_parse(s, pw, buffer, buflen, !(state->maptype == NISMAP_MASTER));
	if (!rval) {
		return (0);
	}
	if ((state->maptype == NISMAP_ADJUNCT) &&
		    (strstr(pw->pw_passwd, "##") != NULL)) {
		char *data;
		int	datalen;

		if (yp_match(state->domain, "passwd.adjunct.byname", pw->pw_name,
				(int) strlen(pw->pw_name), &data, &datalen) == 0) {
			char *bp, *ep;
			/* skip name to get password */
			ep = data;
			if (strsep(&ep, ":") != NULL && (bp = strsep(&ep, ":")) != NULL) {
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
	size_t	fromlen;
	const char *map_arr;
	char *data;
	int rval, nisr, datalen;

	switch (search) {
	case _PW_KEYBYNUM:
		break;
	case _PW_KEYBYNAME:
		from = name;
		fromlen = strlen(name);
		map_arr = PASSWD_BYNAME(state);
		break;
	case _PW_KEYBYUID:
		from = &uid;
		fromlen = sizeof(uid);
		map_arr = PASSWD_BYUID(state);
		break;
	default:
		abort();
	}

	rval = NS_UNAVAIL;
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
	/* Belongs inside if (search != _PW_KEYBYNUM) ??
	if (buflen <= fromlen) {
		return (NS_UNAVAIL);
	}

	buffer[0] = search;
	bcopy(from, buffer + 1, fromlen);
	*/
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
_pws_compat_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{

}

static int
_pws_compat_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{

}

static int
_pws_compat_search(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{

}
#endif

int
_pws_start(state)
	struct passwd_storage *state;
{

}

int
_pws_end(state)
	struct passwd_storage *state;
{

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

}
