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
 * Portions Copyright (c) 1994, Jason Downs. All Rights Reserved.
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

/* getgrent name service switch (getgrent_ns) */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getgrent.c	8.2 (Berkeley) 3/21/94";
#else
__RCSID("$NetBSD: getgrent.c,v 1.48 2003/10/13 15:36:33 agc Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <grp.h>
#include <limits.h>
#include <nsswitch.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#ifdef HESIOD
#include <hesiod.h>
#endif

#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif

#include "gr_storage.h"

static int _grs_parse(int, gid_t, const char *, struct group *, struct group_storage *, char *, size_t);

static int _grs_bad_method(void *, void *, va_list);

static int _grs_local_start(void *, void *, va_list);
static int _grs_local_end(void *, void *, va_list);
static int _grs_local_scan(void *, void *, va_list);

#ifdef HESIOD
static int _grs_hesiod_start(void *, void *, va_list);
static int _grs_hesiod_end(void *, void *, va_list);
static int _grs_hesiod_scan(void *, void *, va_list);
#endif

#ifdef YP
static int _grs_yp_start(void *, void *, va_list);
static int _grs_yp_end(void *, void *, va_list);
static int _grs_yp_scan(void *, void *, va_list);
#endif

#ifdef _GROUP_COMPAT
static int _grs_ns_start(struct group_storage *);
static int _grs_ns_end(struct group_storage *);
static int _grs_ns_scan(int, gid_t, const char *, struct group *, struct group_storage *, char *, size_t);

static int _grs_compat_start(void *, void *, va_list);
static int _grs_compat_end(void *, void *, va_list);
static int _grs_compat_scan(void *, void *, va_list);
#endif

static int
_grs_parse(search, gid, name, group, state, buffer, buflen)
	int search;
	gid_t gid;
	const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;
{
	register char *cp, **m;
	char *bp;

	if (buflen < sizeof(buffer)) {
		return (0);
	}
	if (index(buffer, '+')) {
		return (0);
	}
#if defined(HESIOD) || defined(YP) || defined(_GROUP_COMPAT)
	bp = buffer;
#else
	if (!fgets(buffer, buflen, state->fp)) {
		return (0);
	}
	bp = buffer;
	/* skip lines that are too big */
	if (!index(buffer, '\n')) {
		int ch;

		while ((ch = getc(state->fp)) != '\n' && ch != EOF) {
			;
		}
		return (0);
	}
#endif /* !defined(HESIOD) || !defined(YP) || !defined(_GROUP_COMPAT) */
	group->gr_name = strsep(&bp, ":\n");
	if (search && name && strcmp(group->gr_name, name)) {
		return (0);
	}
	group->gr_passwd = strsep(&bp, ":\n");
	if (!(cp = strsep(&bp, ":\n"))) {
		return (0);
	}
	group->gr_gid = atoi(cp);
	if (search && name == NULL && group->gr_gid != gid) {
		return (0);
	}
	cp = NULL;
	for (m = group->gr_mem = state->mem;; bp++) {
		if (m == &state->mem[MAXGRP - 1]) {
			break;
		}
		if (*bp == ',') {
			if (cp) {
				*bp = '\0';
				*m++ = cp;
				cp = NULL;
			}
		} else if (*bp == '\0' || *bp == '\n' || *bp == ' ') {
			if (cp) {
				*bp = '\0';
				*m++ = cp;
			}
			break;
		} else if (cp == NULL) {
			cp = bp;
		}
	}
	*m = NULL;
	return (1);
}

/* nsdispatch: methods */
static int
_grs_bad_method(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	static int warned;

	if (!warned) {
		syslog(LOG_ERR, "nsswitch.conf group_compat database can't use '%s'", (char *)nscb);
	}
	warned = 1;
	return (NS_UNAVAIL);
}


/* Local/Files Methods */

static int
_grs_local_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct group_storage *state;

	state = va_arg(ap, struct group_storage *);

	int rval;

	if (state->fp == NULL) {
		state->fp = fopen(_PATH_GROUP, "r");
		rval = NS_UNAVAIL;
	} else {
		rewind(state->fp);
		rval = NS_SUCCESS;
	}
	return (rval);
}

static int
_grs_local_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct group_storage *state;

	state = va_arg(ap, struct group_storage *);

	if (state->fp) {
		fclose(state->fp);
		state->fp = NULL;
	}
	return (NS_SUCCESS);
}

static int
_grs_local_scan(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	int search;
	gid_t gid;
	const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;

	search = va_arg(ap, int);
	gid = va_arg(ap, gid_t);
	name = va_arg(ap, const char *);
	group = va_arg(ap, struct group *);
	state = va_arg(ap, struct group_storage *);
	buffer = va_arg(ap, char *);
	buflen = va_arg(ap, size_t);

	int rval;

	rval = NS_NOTFOUND;
	for (;;) {
		rval = _grs_parse(search, gid, name, group, state, buffer, buflen);
		return (rval);
	}
	/* NOTREACHED */
}

/* Hesiod/DNS Methods */
#ifdef HESIOD

static int
_grs_hesiod_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct group_storage *state;

	state = va_arg(ap, struct group_storage*);

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
_grs_hesiod_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct group_storage *state;

	state = va_arg(ap, struct group_storage*);

	state->num = 0;
	if (state->context) {
		hesiod_end(state->context);
		state->context = NULL;
	}
	return (NS_SUCCESS);
}

static int
_grs_hesiod_scan(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	int search;
	gid_t gid;
	const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;

	search = va_arg(ap, int);
	gid = va_arg(ap, gid_t);
	name = va_arg(ap, const char *);
	group = va_arg(ap, struct group *);
	state = va_arg(ap, struct group_storage *);
	buffer = va_arg(ap, char *);
	buflen = va_arg(ap, size_t);

	const char **curzone;
	char **hp;
	int rval;

	static const char *zones_gid_group[] = {
			"gid",
			"group",
			NULL
	};

	static const char *zones_group[] = {
			"group",
			NULL
	};

	hp = NULL;
	rval = NS_NOTFOUND;
	for (;;) {
		if (search) {
			if (name) { /* find group name */
				snprintf(buffer, buflen, "%s", name);
				curzone = zones_group;
			} else { /* find gid */
				snprintf(buffer, buflen, "%u", (unsigned int) gid);
				curzone = zones_gid_group;
			}
		} else {
			if (state->num == -1) {
				return (NS_NOTFOUND);
			}
			/* find group-NNN */
			snprintf(buffer, buflen, "group-%u", state->num);
			state->num++;
			curzone = zones_group;
		}

		for (; *curzone; curzone++) { /* search zones */
			hp = hesiod_resolve(state->context, buffer, *curzone);
			if (hp != NULL) {
				break;
			} else {
				rval = NS_UNAVAIL;
				goto scan_out;
			}
		}
		if (*curzone == NULL) {
			if (!search) {
				state->num = -1;
			}
			goto scan_out;
		}

		strncpy(buffer, hp[0], buflen);
		buffer[buflen - 1] = '\0';
		rval = _grs_parse(search, gid, hp[0], group, state, buffer, buflen);
scan_out:
		if (hp) {
			hesiod_free_list(state->context, hp);
		}
		return (rval);
	}
	/* NOTREACHED */
}

#endif

/* YP/NIS Methods */
#ifdef YP

static int
_grs_yp_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct group_storage *state;

	state = va_arg(ap, struct group_storage*);

	int rval;

	rval = NS_NOTFOUND;
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
	return (rval);
}

static int
_grs_yp_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct group_storage *state;

	state = va_arg(ap, struct group_storage*);

	if (state->domain) {
		state->domain = NULL;
	}
	if (state->current) {
		free(state->current);
		state->current = NULL;
	}
	return (NS_SUCCESS);
}

static int
_grs_yp_scan(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	int search;
	gid_t gid;
	const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;

	search = va_arg(ap, int);
	gid = va_arg(ap, gid_t);
	name = va_arg(ap, const char *);
	group = va_arg(ap, struct group *);
	state = va_arg(ap, struct group_storage *);
	buffer = va_arg(ap, char *);
	buflen = va_arg(ap, size_t);

	const char *map;
	char *key, *data;
	int	nisr, rval, keylen, datalen;

	key = NULL;
	data = NULL;
	rval = NS_SUCCESS;
	for (;;) {
		if (search) {
			if (name) { /* find group name */
				snprintf(buffer, buflen, "%s", name);
				map = "group.byname";
			} else { /* find gid */
				snprintf(buffer, buflen, "%u", (unsigned int) gid);
				map = "group.bygid";
			}
			nisr = yp_match(state->domain, map, buffer, (int) strlen(buffer),
					&data, &datalen);
			switch (nisr) {
			case 0:
				break;
			case YPERR_KEY:
				rval = NS_NOTFOUND;
				break;
			default:
				rval = NS_UNAVAIL;
				break;
			}
		} else {
			if (state->done) { /* exhausted search */
				return (NS_NOTFOUND);
			}
			map = "group.byname";
			if (state->current) { /* already searching */
				nisr = yp_next(state->domain, map, state->current,
						state->currentlen, &key, &keylen, &data, &datalen);
				switch (nisr) {
				case 0:
					state->current = key;
					state->currentlen = keylen;
					key = NULL;
					break;
				case YPERR_NOMORE:
					rval = NS_NOTFOUND;
					state->done = 1;
					break;
				default:
					rval = NS_UNAVAIL;
					break;
				}
			} else { /* new search */
				nisr = yp_first(state->domain, map, &state->current,
						&state->currentlen, &data, &datalen);
				if (nisr) {
					rval = NS_UNAVAIL;
				}
			}
		}
		if (rval == NS_SUCCESS) {
			data[datalen] = '\0'; /* clear trailing \n */
			strncpy(buffer, data, buflen);
			buffer[buflen - 1] = '\0';
			rval = _grs_parse(search, gid, data, group, state, buffer, buflen);
		}
		if (key) {
			free(key);
		}
		if (data) {
			free(data);
		}
		return (rval);
	}
	/* NOTREACHED */
}

#endif

/* Group Compat Methods */
#ifdef _GROUP_COMPAT

static int
_grs_ns_start(state)
	struct group_storage *state;
{
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_grs_bad_method, "files")
			NS_DNS_CB(_grs_hesiod_start, NULL)
			NS_NIS_CB(_grs_yp_start, NULL)
			NS_COMPAT_CB(_grs_bad_method, "compat")
			NS_NULL_CB
	};

	return (nsdispatch(NULL, dtab, NSDB_GROUP_COMPAT, "_grs_ns_start",
			__nsdefaultnis, state));
}

static int
_grs_ns_end(state)
	struct group_storage *state;
{
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_grs_bad_method, "files")
			NS_DNS_CB(_grs_hesiod_end, NULL)
			NS_NIS_CB(_grs_yp_end, NULL)
			NS_COMPAT_CB(_grs_bad_method, "compat")
			NS_NULL_CB
	};

	return (nsdispatch(NULL, dtab, NSDB_GROUP_COMPAT, "_grs_ns_end",
			__nsdefaultnis, state));
}

static int
_grs_ns_scan(search, gid, name, group, state, buffer, buflen)
	int search;
	gid_t gid;
	const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;
{
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_grs_bad_method, "files")
			NS_DNS_CB(_grs_hesiod_scan, NULL)
			NS_NIS_CB(_grs_yp_scan, NULL)
			NS_COMPAT_CB(_grs_bad_method, "compat")
			NS_NULL_CB
	};

	return (nsdispatch(NULL, dtab, NSDB_GROUP_COMPAT, "_grs_ns_scan",
			__nsdefaultnis, search, gid, name, group, state, buffer, buflen));
}

static int
_grs_compat_start(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct group_storage *state;

	state = va_arg(ap, struct group_storage*);

	int rval;

	if (state->fp == NULL) {
		state->fp = fopen(_PATH_GROUP, "r");
		rval = NS_UNAVAIL;
	} else {
		rewind(state->fp);
		rval = NS_SUCCESS;
	}
	if (rval == NS_SUCCESS) {
		rval = _grs_ns_start(state);
	}
	return (rval);
}

static int
_grs_compat_end(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	struct group_storage *state;

	state = va_arg(ap, struct group_storage *);

	if (state->name) {
		free(state->name);
		state->name = NULL;
	}
	if (state->fp) {
		fclose(state->fp);
		state->fp = NULL;
	}
	return (_grs_ns_end(state));
}

static int
_grs_compat_scan(nsrv, nscb, ap)
	void *nsrv, *nscb;
	va_list ap;
{
	int search;
	gid_t gid;
	const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;

	search = va_arg(ap, int);
	gid = va_arg(ap, gid_t);
	name = va_arg(ap, const char *);
	group = va_arg(ap, struct group *);
	state = va_arg(ap, struct group_storage *);
	buffer = va_arg(ap, char*);
	buflen = va_arg(ap, size_t);

	int rval;

	for (;;) {
		if (state->name != NULL) {
			rval = _grs_ns_scan(search, gid, state->name, group, state, buffer, buflen);
			free(state->name);
			state->name = NULL;
		} else {
			rval = _grs_ns_scan(search, gid, name, group, state, buffer, buflen);
		}
		if (rval != NS_SUCCESS) {
			return (rval);
		}
		if (!search) {
			return (NS_SUCCESS);
		}
		if (name) {
			if (!strcmp(group->gr_name, name)) {
				return (NS_SUCCESS);
			}
		} else {
			if (group->gr_gid == gid) {
				return (NS_SUCCESS);
			}
		}

		if (!fgets(buffer, buflen, state->fp)) {
			return (NS_NOTFOUND);
		}

		/* skip lines that are too big */
		if (!index(buffer, '\n')) {
			int ch;

			while ((ch = getc(state->fp)) != '\n' && ch != EOF) {
				;
			}
			continue;
		}

		if (index(buffer, '+')) {
			char *tptr, *bp;

			if (state->name) {
				free(state->name);
				state->name = NULL;
			}
			switch (buffer[1]) {
			case ':':
			case '\0':
				state->name = strdup("");
				break;
			case '\n':
			default:
				bp = buffer;
				tptr = strsep(&bp, ":\n");
				state->name = strdup(tptr + 1);
				break;
			}
			continue;
		}
		rval = _grs_parse(search, gid, name, group, state, buffer, buflen);
		return (rval);
	}
	/* NOTREACHED */
}

#endif

int
_grs_start(state)
	struct group_storage *state;
{
	int rval;
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_grs_local_start, NULL)
			NS_DNS_CB(_grs_hesiod_start, NULL)
			NS_NIS_CB(_grs_yp_start, NULL)
			NS_COMPAT_CB(_grs_compat_start, NULL)
			NS_NULL_CB
	};

	rval = nsdispatch(NULL, dtab, NSDB_GROUP, "_grs_start", __nsdefaultcompat, state);
	return ((rval == NS_SUCCESS) ? 1 : 0);
}

int
_grs_end(state)
	struct group_storage *state;
{
	int rval;
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_grs_local_end, NULL)
			NS_DNS_CB(_grs_hesiod_end, NULL)
			NS_NIS_CB(_grs_yp_end, NULL)
			NS_COMPAT_CB(_grs_compat_end, NULL)
			NS_NULL_CB
	};

	rval = nsdispatch(NULL, dtab, NSDB_GROUP, "_grs_end", __nsdefaultcompat, state);
	return ((rval == NS_SUCCESS) ? 1 : 0);
}

int
_grs_grscan(search, gid, name, group, state, buffer, buflen)
	int search;
	gid_t gid;
	const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;
{
	int rval;
	static const ns_dtab dtab[] = {
			NS_FILES_CB(_grs_local_scan, NULL)
			NS_DNS_CB(_grs_hesiod_scan, NULL)
			NS_NIS_CB(_grs_yp_scan, NULL)
			NS_COMPAT_CB(_grs_compat_scan, NULL)
			NS_NULL_CB
	};

	rval = nsdispatch(NULL, dtab, NSDB_GROUP, "_grs_grscan", __nsdefaultcompat, search, gid, name, group, state, buffer, buflen);
	return ((rval == NS_SUCCESS) ? 1 : 0);
}
