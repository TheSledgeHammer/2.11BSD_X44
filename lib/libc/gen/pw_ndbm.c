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

/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * Work in Progress
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

#include <ndbm.h>

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

#include "pw_ndbm.h"

static int _pw_getdb(DBM *, datum *, int *);
static int _pw_version(DBM **, datum *, datum *, int *, int *);
static int _pw_opendb(DBM **, FILE *, int *, int *);
static void _pw_closedb(DBM *, FILE *);
static int _pw_hashdb(datum *, struct passwd *, char *, char *, char *, int);
static int _pw_fetch(DBM *, datum *, struct passwd *, char *, size_t, int *, int *, int);

int
_pw_start(db, fp, rewind, keynum, version)
	DBM **db;
	FILE *fp;
	int *rewind, *keynum, *version;
{
	int rv;

	*rewind = 1;
	*keynum = 0;
	rv = _pw_opendb(db, fp, rewind, version);
	if (rv != NS_SUCCESS) {
		return (rv);
	}
	return (NS_SUCCESS);
}

int
_pw_end(db, fp, keynum)
	DBM *db;
	FILE *fp;
	int *keynum;
{
	*keynum = 0;
	if (db) {
		_pw_closedb(db, fp);
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
_pw_getkey(db, key, pw, buffer, buflen, pwflags, rewind, version)
	DBM *db;
	datum *key;
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	int *pwflags, *rewind, version;
{
	if (db == NULL) {			/* this shouldn't happen */
		return (NS_UNAVAIL);
	}
	return (_pw_fetch(db, key, pw, buffer, buflen, pwflags, rewind, version));
}

void
_pw_setkey(key, data, size)
	datum *key;
	char *data;
	size_t size;
{
	key->dsize = size;
	key->dptr = (u_char *)data;
}

static int
_pw_version(db, key, value, rewind, version)
	DBM **db;
	datum *key, *value;
	int *rewind, *version;
{
	key->dptr = __UNCONST("VERSION");
	key->dsize = strlen((char *)key->dptr) + 1;
	switch (_pw_getdb(*db, key, rewind)) {
	case 0:
		if (sizeof(*version) != value->dsize) {
			return (NS_UNAVAIL);
		}
		(void)bcopy(value->dptr, version, value->dsize);
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
_pw_opendb(db, fp, rewind, version)
	DBM **db;
	FILE *fp;
	int *rewind;
	int *version;
{
	const char *dbfile = NULL;
	datum key;
	datum value;

	_DIAGASSERT(db != NULL);
	_DIAGASSERT(version != NULL);

	if (*db != NULL) {
		*rewind = 1;
		return (NS_SUCCESS);
	}
	if (fp != NULL) {
		rewind(fp);
		return (NS_SUCCESS);
	}

	if (geteuid() == 0) {
		dbfile = _PATH_MASTERPASSWD;
		*db = dbm_open(dbfile, O_RDONLY, 0);
		*rewind = 1;
	}

	if (*db == NULL) {
		dbfile = _PATH_PASSWD;
		*db = dbm_open(dbfile, O_RDONLY, 0);
		*rewind = 1;
	}

	if (fp == fopen(dbfile, "r")) {
		return (NS_SUCCESS);
	}

	if (*db == NULL) {
		return (NS_UNAVAIL);
	}

	return (_pw_version(db, &key, &value, version));
}

int
_pw_getdb(db, key, rewind)
	DBM *db;
	datum *key;
	int *rewind;
{
	int ret;

	if (flock(dbm_dirfno(db), LOCK_SH)) {
		ret = -1;
		goto out;
	}

	if (!key->dptr) {
		if (*rewind) {
			*rewind = 0;
			*key = dbm_firstkey(db);
		} else {
			*key = dbm_nextkey(db);
		}
	}

	if (key->dptr) {
		*key = dbm_fetch(db, *key);
		if (*key != NULL) {
			ret = 0;
		} else if (*key == NULL) {
			ret = 1;
		} else {
			ret = -1;
		}
	}
out:
	(void)flock(dbm_dirfno(db), LOCK_UN);
	return (ret);
}

void
_pw_closedb(db, fp)
	DBM *db;
	FILE *fp;
{
	if (db) {
		dbm_close(db);
		db = (DBM *)NULL;
	} else if (fp) {
		(void)fclose(fp);
		fp = (FILE *)NULL;
	}
}

int
_pw_storedb(db, key, data, flag)
	DB *db;
	datum key;
	datum data;
	int flag;
{
	return (dbm_store(db, key, data, flag));
}

static int
_pw_hashdb(data, pw, p, t, pwflags, version)
	datum *data;
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
		if (data->dsize > (size_t)(p - (char *) data->dptr)) {
			SCALAR(*pwflags);
		} else {
			pwflags = p;
		}
	}
	return (NS_SUCCESS);
}

static int
_pw_fetch(db, key, pw, buffer, buflen, pwflags, rewind, version)
	DBM *db;
	datum *key;
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	int *pwflags, *rewind, version;
{
	register char *p, *t;
	datum data;

	switch (_pw_getdb(db, key, rewind)) {
	case 0:
		break; 					/* found */
	case 1:
		return (NS_NOTFOUND); 	/* not found */
	case -1:
		return (NS_UNAVAIL); 	/* error in db routines */
	default:
		abort();
	}
	p = (char *)key->dptr;
	if ((data.dsize > buflen) && !(buffer = realloc(buffer, buflen += 1024))) {
		errno = ERANGE;
		return (NS_UNAVAIL);
	}
	t = buffer;
	return (_pw_hashdb(&data, pw, p, t, (char *)pwflags, version));
}

int
_pw_scanfp(fp, pw, buffer)
	FILE *fp;
	struct passwd *pw;
	char *buffer;
{
	register char *cp;
	register int ch;
	char *bp;

	bp = buffer;
	for (;;) {
		if (!(fgets(buffer, sizeof(buffer), fp))) {
			return (0);
		}
		/* skip lines that are too big */
		if (!(cp = index(buffer, '\n'))) {
			while ((ch = fgetc(fp)) != '\n' && ch != EOF) {
				;
			}
			continue;
		}
		*cp = '\0';
		pw->pw_name = strsep(&bp, ":");
		pw->pw_passwd = strsep(&bp, ":");
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		pw->pw_uid = atoi(cp);
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		pw->pw_gid = atoi(cp);
		pw->pw_class = strsep(&bp, ":");
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		pw->pw_change = atol(cp);
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		pw->pw_expire = atol(cp);
		pw->pw_gecos = strsep(&bp, ":");
		pw->pw_dir = strsep(&bp, ":");
		pw->pw_shell = strsep(&bp, ":");
		if (!pw->pw_shell) {
			continue;
		}
		return (1);
	}
	/* NOTREACHED */
	return (1);
}

void
_pw_readfp(db, pw, buffer)
	DBM *db;
	struct passwd *pw;
	char *buffer;
{
	char pwbuf[50];
	const char *dbfile;
	long pos;
	int fd, n;
	register char *p;

	pwbuf = buffer;
	if (geteuid() == 0) {
		dbfile = _PATH_MASTERPASSWD;
		fd = open(dbfile, O_RDONLY, 0);
		if (fd < 0) {
			return;
		}
	}

	if (*db != NULL) {
		dbfile = _PATH_PASSWD;
		fd = open(dbfile, O_RDONLY, 0);
		if (fd < 0) {
			return;
		}
	}
	pos = atol(pw->pw_passwd);
	if (lseek(fd, pos, SEEK_SET) != pos) {
		goto bad;
	}
	n = read(fd, pwbuf, sizeof(pwbuf) -1);
	if (n < 0) {
		goto bad;
	}
	pwbuf[n] = '\0';
	for (p = pwbuf; *p; ++p) {
		if (*p == ':') {
			*p = '\0';
			pw->pw_passwd = pwbuf;
			break;
		}
	}
bad:
	(void)close(fd);
}
