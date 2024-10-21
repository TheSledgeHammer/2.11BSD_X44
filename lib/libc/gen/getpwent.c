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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getpwent.c	5.9.1 (2.11BSD) 1996/1/12";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>

#if defined(USE_NDBM) && (USE_NDBM == 0)
#include <ndbm.h>
#else
#include <db.h>
#endif

#include <fcntl.h>
#include <syslog.h>
#include <utmp.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#ifdef __weak_alias
__weak_alias(getpwent,_getpwent)
__weak_alias(getpwnam,_getpwnam)
__weak_alias(getpwuid,_getpwuid)
__weak_alias(setpwent,_setpwent)
__weak_alias(setpassent,_setpassent)
__weak_alias(endpwent,_endpwent)
__weak_alias(setpwfile,_setpwfile)
#endif

#define	MAXLINELENGTH	256
#if defined(USE_NDBM) && (USE_NDBM == 0)
static DBM *_pw_db;			/* password database */
static const char *_pw_file = _PATH_PASSWD;	/* password path */
static FILE *_pw_fp;			/* password file */
static int _pw_rewind = 1;
#else
static DB *_pw_db;			/* password database */
static const char *_pw_file = _PATH_MP_DB;	/* password path */
#endif
static struct passwd _pw_passwd;	/* password structure */
static int _pw_keynum;			/* key counter */
static int _pw_stayopen;		/* keep fd's open */
static char _pw_flag;
static char *line;

static int start_pw(void);
static int hashpw(char *, char *);
#if defined(USE_NDBM) && (USE_NDBM == 0)
static int scanpw(void);
static int init_ndbm(void);
static int fetch_pw(datum);
static void getpw(void);
#else
static int init_db(void);
static int fetch_pw(DBT *);
#endif

#if defined(USE_NDBM) && (USE_NDBM == 0)
struct passwd *
getpwent(void)
{
	datum key;
	int rval;
	char bf[sizeof(_pw_keynum) + 1];

	if (!_pw_db && !_pw_fp && !start_pw()) {
		return ((struct passwd *) NULL);
	}
    rval = 0;
	do {
		if (_pw_db) {
			++_pw_keynum;
			bf[0] = _PW_KEYBYNUM;
			bcopy((char *)&_pw_keynum, bf + 1, sizeof(_pw_keynum));
			key.dptr = (u_char *)bf;
			key.dsize = sizeof(_pw_keynum) + 1;
			rval = fetch_pw(key);
		} else { /* _pw_fp */
			rval = scanpw();
		}
	} while (rval && _pw_flag != _PW_KEYBYNUM);
	if (rval) {
		getpw();
	}
	return (rval ? &_pw_passwd : (struct passwd *) NULL);
}

struct passwd *
getpwnam(nam)
	char *nam;
{
	datum key;
	int len, rval;
	char bf[UT_NAMESIZE + 1];

	if (!_pw_db && !start_pw()) {
		return ((struct passwd *) NULL);
	}
    rval = 0;
	if (_pw_db) {
		bf[0] = _PW_KEYBYNAME;
		len = strlen(nam);
		bcopy(nam, bf + 1, MIN(len, UT_NAMESIZE));
		key.dptr = nam;
		key.dsize = strlen(nam);
		rval = fetch_pw(key);
	} else {	/* _pw_fp */
		for (rval = 0; scanpw();) {
			if (!strcmp(nam, _pw_passwd.pw_name)) {
				rval = 1;
				break;
			}
		}
	}
	if (!_pw_stayopen) {
		endpwent();
	}
	if (rval) {
		getpw();
	}
	return (rval ? &_pw_passwd : (struct passwd *) NULL);
}

struct passwd *
getpwuid(uid)
	uid_t uid;
{
	datum key;
	int keyuid, rval;
	char bf[sizeof(keyuid) + 1];

	if (!_pw_db && !start_pw()) {
		return ((struct passwd *) NULL);
	}
    rval = 0;
	if (_pw_db) {
		bf[0] = _PW_KEYBYUID;
		keyuid = uid;
		bcopy(&keyuid, bf + 1, sizeof(keyuid));
		key.dptr = (u_char *)bf;
		key.dsize =  sizeof(keyuid) + 1;
		rval = fetch_pw(key);
	} else {	/* _pw_fp */
		for (rval = 0; scanpw();) {
			if (_pw_passwd.pw_uid == uid) {
				rval = 1;
				break;
			}
		}
	}
	if (!_pw_stayopen) {
		endpwent();
	}
	if (rval) {
		getpw();
	}
	return (rval ? &_pw_passwd : (struct passwd *) NULL);
}

#else

struct passwd *
getpwent(void)
{
	DBT key;
	int rval;
	char bf[sizeof(_pw_keynum) + 1];

	if (!_pw_db && !start_pw()) {
		return ((struct passwd *) NULL);
	}
	++_pw_keynum;
	bf[0] = _PW_KEYBYNUM;
	bcopy((char *)&_pw_keynum, bf + 1, sizeof(_pw_keynum));
	key.data = (u_char *)bf;
	key.size = sizeof(_pw_keynum) + 1;
	rval = fetch_pw(&key);
	return (rval ? &_pw_passwd : (struct passwd *) NULL);
}

struct passwd *
getpwnam(nam)
	char *nam;
{
	DBT key;
	int len, rval;
	char bf[UT_NAMESIZE + 1];

	if (!_pw_db && !start_pw()) {
		return ((struct passwd *) NULL);
	}
	bf[0] = _PW_KEYBYNAME;
	len = strlen(nam);
	bcopy(nam, bf + 1, MIN(len, UT_NAMESIZE));
	key.data = (u_char *)bf;
	key.size = len + 1;
	rval = fetch_pw(&key);
	if (!_pw_stayopen) {
		endpwent();
	}
	return (rval ? &_pw_passwd : (struct passwd *) NULL);
}

struct passwd *
getpwuid(uid)
	uid_t uid;
{
	DBT key;
	int keyuid, rval;
	char bf[sizeof(keyuid) + 1];

	if (!_pw_db && !start_pw()) {
		return ((struct passwd *) NULL);
	}
	bf[0] = _PW_KEYBYUID;
	keyuid = uid;
	bcopy(&keyuid, bf + 1, sizeof(keyuid));
	key.data = (u_char *)bf;
	key.size = sizeof(keyuid) + 1;
	rval = fetch_pw(&key);
	if (!_pw_stayopen) {
		endpwent();
	}
	return (rval ? &_pw_passwd : (struct passwd *) NULL);
}

#endif

static int
start_pw(void)
{
	int ret;

#if defined(USE_NDBM) && (USE_NDBM == 0)
	ret = init_ndbm();
#else
	ret = init_db();
#endif
	return (ret);
}

void
setpwent(void)
{
	(void)setpassent(0);
}

int
setpassent(stayopen)
	int stayopen;
{
	if (!start_pw()) {
		return (0);
	}
	_pw_keynum = 0;
	_pw_stayopen = stayopen;
	return (1);
}

void
endpwent(void)
{
	_pw_keynum = 0;
#if defined(USE_NDBM) && (USE_NDBM == 0)
	if (_pw_db) {
		dbm_close(_pw_db);
		_pw_db = (DBM *)NULL;
	} else if (_pw_fp) {
		(void)fclose(_pw_fp);
		_pw_fp = (FILE *)NULL;
	}
#else
	if (_pw_db) {
		(void)(_pw_db->close)(_pw_db);
		_pw_db = (DB *)NULL;
	}
#endif
}

void
setpwfile(file)
	const char *file;
{
	_pw_file = file;
}

static int
hashpw(p, t)
	char *p, *t;
{
#define MACRO(a)	do { a } while (0)
#define	EXPAND(e)	MACRO(e = t; while ((*t++ = *p++));)
#define	SCALAR(v)	MACRO(memmove(&(v), p, sizeof v); p += sizeof v;)
	EXPAND(_pw_passwd.pw_name);
	EXPAND(_pw_passwd.pw_passwd);
	SCALAR(_pw_passwd.pw_uid);
	SCALAR(_pw_passwd.pw_gid);
	SCALAR(_pw_passwd.pw_change);
	EXPAND(_pw_passwd.pw_class);
	EXPAND(_pw_passwd.pw_gecos);
	EXPAND(_pw_passwd.pw_dir);
	EXPAND(_pw_passwd.pw_shell);
	SCALAR(_pw_passwd.pw_expire);
	_pw_flag = *p;
	return (1);
}

#if defined(USE_NDBM) && (USE_NDBM == 0)

static int
scanpw(void)
{
	register char *cp;
	char *bp = line;
	register int ch;

	for (;;) {
		if (!(fgets(line, sizeof(line), _pw_fp))) {
			return (0);
		}
		/* skip lines that are too big */
		if (!(cp = index(line, '\n'))) {
			while ((ch = fgetc(_pw_fp)) != '\n' && ch != EOF) {
				;
			}
			continue;
		}
		*cp = '\0';
		_pw_passwd.pw_name = strsep(&bp, ":");
		_pw_passwd.pw_passwd = strsep(&bp, ":");
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		_pw_passwd.pw_uid = atoi(cp);
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		_pw_passwd.pw_gid = atoi(cp);
		_pw_passwd.pw_class = strsep(&bp, ":");
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		_pw_passwd.pw_change = atol(cp);
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		_pw_passwd.pw_expire = atol(cp);
		_pw_passwd.pw_gecos = strsep(&bp, ":");
		_pw_passwd.pw_dir = strsep(&bp, ":");
		_pw_passwd.pw_shell = strsep(&bp, ":");
		if (!_pw_passwd.pw_shell) {
			continue;
		}
		return (1);
	}
	/* NOTREACHED */
	return (1);
}

static int
init_ndbm(void)
{
	register char *p;

	if (_pw_db) {
		_pw_rewind = 1;
		return (1);
	}
	if (_pw_fp) {
		rewind(_pw_fp);
		return (1);
	}
	_pw_db = dbm_open(_pw_file, O_RDONLY, 0);
	if (_pw_db) {
		_pw_rewind = 1;
		return (1);
	}
	/*
	 * special case; if it's the official password file, look in
	 * the master password file, otherwise, look in the file itself.
	 */
	p = strcmp(_pw_file, _PATH_PASSWD) ? _pw_file : _PATH_MASTERPASSWD;
	if (_pw_fp == fopen(p, "r")) {
		return (1);
	}
	return (0);
}

static int
fetch_pw(key)
    datum key;
{
    register char *p, *t;

    if (flock(dbm_dirfno(_pw_db), LOCK_SH)) {
        return (0);
    }
    if (!key.dptr) {
        if (_pw_rewind) {
			_pw_rewind = 0;
			key = dbm_firstkey(_pw_db);
		} else {
			key = dbm_nextkey(_pw_db);
		}
    }
    if (key.dptr) {
		key = dbm_fetch(_pw_db, key);
	}
	(void) flock(dbm_dirfno(_pw_db), LOCK_UN);
    p = (char *)key.dptr;
    if (!p) {
        return (0);
    }
    t = line;
    return (hashpw(p, t));
}

static void
getpw(void)
{
	static char pwbuf[50];
	long pos;
	int fd, n;
	register char *p;

	if (geteuid()) {
		return;
	}

	/*
	 * special case; if it's the official password file, look in
	 * the master password file, otherwise, look in the file itself.
	 */
	p = strcmp(_pw_file, _PATH_PASSWD) ? _pw_file : _PATH_MASTERPASSWD;
	if ((fd = open(p, O_RDONLY, 0)) < 0) {
		return;
	}
	pos = atol(_pw_passwd.pw_passwd);
	if (lseek(fd, pos, L_SET) != pos) {
		goto bad;
	}
	if ((n = read(fd, pwbuf, sizeof(pwbuf) - 1)) < 0) {
		goto bad;
	}
	pwbuf[n] = '\0';
	for (p = pwbuf; *p; ++p) {
		if (*p == ':') {
			*p = '\0';
			_pw_passwd.pw_passwd = pwbuf;
			break;
		}
	}
bad:
	(void)close(fd);
}

#else

static int
init_db(void)
{
	register const char *p;
	
	if (geteuid()) {
		return (1);
	}
	_pw_db = dbopen(_pw_file, O_RDONLY, 0, DB_HASH, NULL);
	if (_pw_db) {
		return (1);
	}
	p = strcmp(_pw_file, _PATH_MP_DB) ? _pw_file : _PATH_SMP_DB;
	_pw_db = dbopen(p, O_RDONLY, 0, DB_HASH, NULL);
	if (_pw_db) {
		return (1);
	}
	return (0);
}

static int
fetch_pw(key)
	DBT *key;
{
	register char *p, *t;
	static u_int max;
	DBT data;

	if ((_pw_db->get)(_pw_db, key, &data, 0)) {
		return (0);
	}
	p = (char *)data.data;
	if (data.size > max && !(line = realloc(line, max += 1024))) {
		return (0);
	}
    t = line;
    return (hashpw(p, t));
}

#endif
