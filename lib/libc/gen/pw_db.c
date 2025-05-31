/*	$NetBSD: getpwent.c,v 1.66.2.3 2006/07/13 09:29:40 ghen Exp $	*/

/*-
 * Copyright (c) 1997-2000, 2004-2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
__RCSID("$NetBSD: getpwent.c,v 1.66.2.3 2006/07/13 09:29:40 ghen Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>

#include <db.h>

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

#include "pw_db.h"

static int _pw_version(DB **, DBT *, DBT *, int *);
static int _pw_opendb(DB **, int *);
static int _pw_hashdb(DBT *, struct passwd *, char *, char *, char *, int);
static int _pw_fetch(DB *, DBT *, struct passwd *, char *, size_t, int *, int);

int
_pw_start(db, keynum, version)
	DB **db;
	int *keynum, *version;
{
	int rval;

	*keynum = 0;
	rval = _pw_opendb(db, version);
	if (rval != NS_SUCCESS) {
		return (rval);
	}
	return (NS_SUCCESS);
}

int
_pw_end(db, keynum)
	DB *db;
	int *keynum;
{
	*keynum = 0;
	if (db) {
		_pw_closedb(db);
	}
	return (NS_SUCCESS);
}

/*
 * _pw_getkey
 *	Lookup key in *db, filling in pw
 *	with the result, allocating memory from buffer (size buflen).
 *	(The caller may point key.data to buffer on entry; the contents
 *	of key.data will be invalid on exit.)
 */
int
_pw_getkey(db, key, pw, buffer, buflen, pwflags, version)
	DB *db;
	DBT *key;
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	int *pwflags, version;
{
	if (db == NULL) {			/* this shouldn't happen */
		return (NS_UNAVAIL);
	}
	return (_pw_fetch(db, key, pw, buffer, buflen, pwflags, version));
}

void
_pw_setkey(key, data, size)
	DBT *key;
	char *data;
	size_t size;
{
	key->size = size;
	key->data = (u_char *)data;
}

static int
_pw_version(db, key, value, version)
	DB **db;
	DBT *key, *value;
	int *version;
{
	key->data = __UNCONST("VERSION");
	key->size = strlen((char *)key->data) + 1;
	switch (_pw_getdb(*db, key, value, 0)) {
	//switch ((*(*db)->get)(*db, key, value, 0)) {
	case 0:
		if (sizeof(*version) != value->size) {
			return (NS_UNAVAIL);
		}
		(void)bcopy(value->data, version, value->size);
		break;
	case 1:
		*version = 0;		/* not found */
		break;
	case -1:
		return (NS_UNAVAIL);	/* error in db routines */
	default:
		abort();
	}
	return (NS_SUCCESS);
}

/*
 * _pw_opendb
 *	if *db is NULL, dbopen(3) /etc/spwd.db or /etc/pwd.db (depending
 *	upon permissions, etc)
 */
static int
_pw_opendb(db, version)
	DB **db;
	int *version;
{
	static int	warned;
	const char *dbfile = NULL;
	DBT key;
	DBT value;

	if (*db != NULL) { /* open *db */
		return (NS_SUCCESS);
	}

	if (geteuid() == 0) {
		dbfile = _PATH_SMP_DB;
		*db = dbopen(dbfile, O_RDONLY, 0, DB_HASH, NULL);
	}

	if (*db == NULL) {
		dbfile = _PATH_MP_DB;
		*db = dbopen(dbfile, O_RDONLY, 0, DB_HASH, NULL);
	}

	if (*db == NULL) {
		if (!warned) {

		}
		warned = 1;
		return (NS_UNAVAIL);
	}
	return (_pw_version(db, &key, &value, version));
}

int
_pw_getdb(db, key, value, flag)
	DB *db;
	DBT *key;
	DBT *value;
	unsigned int flag;
{
	return ((db->get)(db, key, value, flag));
}

void
_pw_closedb(db)
	DB *db;
{
	if (db) {
		(void)(db->close)(db);
		db = (DB *)NULL;
	}
}

int
_pw_storedb(db, key, value, flag)
	DB *db;
	DBT *key;
	DBT *value;
	unsigned int flag;
{
	return ((db->put)(db, key, value, flag));
}

static int
_pw_hashdb(data, pw, p, t, pwflags, version)
	DBT *data;
	struct passwd *pw;
	char *p, *t, *pwflags;
	int version;
{
#define MACRO(a)	do { a } while (0)
#define	EXPAND(e)	MACRO(e = t; while ((*t++ = *p++));)
#define	SCALAR(v)	MACRO(memmove(&(v), p, sizeof v); p += sizeof v;)

	EXPAND(pw->pw_name);
	EXPAND(pw->pw_passwd);
	SCALAR(pw->pw_uid);
	SCALAR(pw->pw_gid);
	if (version == 0) {
		int32_t tmp;
		SCALAR(tmp);
		pw->pw_change = tmp;
	} else {
		SCALAR(pw->pw_change);
	}
	EXPAND(pw->pw_class);
	EXPAND(pw->pw_gecos);
	EXPAND(pw->pw_dir);
	EXPAND(pw->pw_shell);
	if (version == 0) {
		int32_t tmp;
		SCALAR(tmp);
		pw->pw_expire = tmp;
	} else {
		SCALAR(pw->pw_expire);
	}
	if (pwflags) {
		if (data->size > (size_t)(p - (char *)data->data)) {
			SCALAR(*pwflags);
		} else {
			pwflags = p;
		}
	}
	return (NS_SUCCESS);
}

static int
_pw_fetch(db, key, pw, buffer, buflen, pwflags, version)
	DB *db;
	DBT *key;
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	int *pwflags, version;
{
	register char *p, *t;
	DBT data;

	switch ((db->get)(db, key, &data, 0)) {
	case 0:
		break; 					/* found */
	case 1:
		return (NS_NOTFOUND); 	/* not found */
	case -1:
		return (NS_UNAVAIL); 	/* error in db routines */
	default:
		abort();
	}
	p = (char *)data.data;
	if ((data.size > buflen) && !(buffer = realloc(buffer, buflen += 1024))) {
		errno = ERANGE;
		return (NS_UNAVAIL);
	}
	t = buffer;
	return (_pw_hashdb(&data, pw, p, t, (char *)pwflags, version));
}
