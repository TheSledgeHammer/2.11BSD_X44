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

#include "pw_storage.h"

#ifdef __weak_alias
__weak_alias(getpwent,_getpwent)
__weak_alias(getpwnam,_getpwnam)
__weak_alias(getpwuid,_getpwuid)
__weak_alias(setpwent,_setpwent)
__weak_alias(setpassent,_setpassent)
__weak_alias(endpwent,_endpwent)
__weak_alias(setpwfile,_setpwfile)
#endif

#ifdef _REENTRANT
static 	mutex_t			_pwmutex = MUTEX_INITIALIZER;
#endif

static struct passwd_storage	_pws_storage;
static struct passwd 		_pws_passwd;
static char			_pws_passwdbuf[_GETPW_R_SIZE_MAX];

static int _pws_keybynum(struct passwd *, char *, size_t, struct passwd_storage *, struct passwd **);
static int _pws_keybyname(const char *, struct passwd *, char *, size_t, struct passwd_storage *, struct passwd **);
static int _pws_keybyuid(uid_t uid, struct passwd *, char *, size_t, struct passwd_storage *, struct passwd **);
static int _pws_setpwent(struct passwd_storage *);
static int _pws_setpassent(struct passwd_storage *, int, int *);
static int _pws_endpwent(struct passwd_storage *);
static int _pws_setpwfile(struct passwd_storage *, const char *, int *);

static int
_pws_keybynum(pw, buffer, buflen, state, result)
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	struct passwd **result;
{
	int rval;

	if (state->db == NULL) {
		rval = _pws_start(state);
		if (rval != NS_SUCCESS) {
			return (rval);
		}
	}

	rval = _pws_search(pw, buffer, buflen, state, _PW_KEYBYNUM, NULL, 0);
	if (rval == NS_NOTFOUND) {
		state->keynum = -1; /* flag `no more records' */
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	} else {
		if (state->fp != NULL) {
			rval = _pw_scanfp(state->fp, pw, buffer);
		}
#endif
	}

	if (!state->stayopen) {
		_pws_endpwent(state);
	}

	if (rval == NS_SUCCESS) {
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
		_pw_readfp(state->db, pw, buffer);
#endif
		result = &pw;
	}
	return (rval);
}

static int
_pws_keybyname(name, pw, buffer, buflen, state, result)
	const char *name;
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	struct passwd **result;
{
	int rval;

	if (state->db == NULL) {
		rval = _pws_start(state);
		if (rval != NS_SUCCESS) {
			return (rval);
		}
	}

	rval = _pws_search(pw, buffer, buflen, state, _PW_KEYBYNAME, name, 0);
	if (strcmp(pw->pw_name, name) != 0) {
		rval = NS_NOTFOUND;
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	} else {
		int ret;

		ret = _pw_scanfp(state->fp, pw, buffer);
		for (rval = 0; ret;) {
			if (strcmp(name, pw->pw_name) == 0) {
				rval = NS_SUCCESS;
				break;
			}
		}
#endif
	}

	if (!state->stayopen) {
		_pws_endpwent(state);
	}

	if (rval == NS_SUCCESS) {
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
		_pw_readfp(state->db, pw, buffer);
#endif
		result = &pw;
	}
	return (rval);
}

static int
_pws_keybyuid(uid, pw, buffer, buflen, state, result)
	uid_t uid;
	struct passwd *pw;
	char *buffer;
	size_t buflen;
	struct passwd_storage *state;
	struct passwd **result;
{
	int rval;

	if (state->db == NULL) {
		rval = _pws_start(state);
		if (rval != NS_SUCCESS) {
			return (rval);
		}
	}

	rval = _pws_search(pw, buffer, buflen, state, _PW_KEYBYUID, NULL, uid);
	if (pw->pw_uid != uid) {
		rval = NS_NOTFOUND;
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	} else {
		int ret;

		ret = _pw_scanfp(state->fp, pw, buffer);
		for (rval = 0; ret;) {
			if (pw->pw_uid == uid) {
				rval = NS_SUCCESS;
				break;
			}
		}
#endif
	}
	if (!state->stayopen) {
		_pws_endpwent(state);
	}
	if (rval == NS_SUCCESS) {
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
		_pw_readfp(state->db, pw, buffer);
#endif
		result = &pw;
	}
	return (rval);
}

static int
_pws_setpwent(state)
	struct passwd_storage *state;
{
	int result;

	return (_pws_setpassent(state, 0, &result));
}

static int
_pws_setpassent(state, stayopen, result)
	struct passwd_storage *state;
	int stayopen, *result;
{
	int rval;

	state->keynum = 0;
	state->stayopen = stayopen;
	rval = _pws_start(state);
	*result = (rval == NS_SUCCESS);
	return (rval);
}

static int
_pws_endpwent(state)
	struct passwd_storage *state;
{
	state->stayopen = 0;
	return (_pws_end(state));
}

static int
_pws_setpwfile(state, file, result)
	struct passwd_storage *state;
	const char *file;
	int *result;
{
	int rval;

#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	state->file = file;
	rval = _pws_start(state);
	*result = (rval == NS_SUCCESS);
#else
	/* Not supported for DB */
	file = NULL;
	*result = EINVAL;
	rval = NS_NOTFOUND;
#endif
	return (rval);
}

/*
 *	public functions
 */
int
getpwent_r(pwd, buffer, buflen, result)
	struct passwd *pwd;
	char *buffer;
	size_t buflen;
	struct passwd **result;
{
	int rval;

	mutex_lock(&_pwmutex);
	rval = _pws_keybynum(pwd, buffer, buflen, &_pws_storage, result);
	mutex_unlock(&_pwmutex);
	return (rval);
}

struct passwd *
getpwent(void)
{
	struct passwd *result;
	int rval;

	rval = getpwent_r(&_pws_passwd, _pws_passwdbuf, sizeof(_pws_passwdbuf), &result);
	return ((rval == NS_SUCCESS) ? result : NULL);
}

int
getpwnam_r(name, pwd, buffer, buflen, result)
	const char *name;
	struct passwd *pwd;
	char *buffer;
	size_t buflen;
	struct passwd **result;
{
	int rval;

	mutex_lock(&_pwmutex);
	rval = _pws_keybyname(name, pwd, buffer, buflen, &_pws_storage, result);
	mutex_unlock(&_pwmutex);
	return (rval);
}

struct passwd *
getpwnam(name)
	const char *name;
{
	struct passwd *result;
	int rval;

	rval = getpwnam_r(name, &_pws_passwd, _pws_passwdbuf, sizeof(_pws_passwdbuf), &result);
	return ((rval == NS_SUCCESS) ? result : NULL);
}

int
getpwuid_r(uid, pwd, buffer, buflen, result)
	uid_t uid;
	struct passwd *pwd;
	char *buffer;
	size_t buflen;
	struct passwd **result;
{
	int rval;

	mutex_lock(&_pwmutex);
	rval = _pws_keybyuid(uid, pwd, buffer, buflen, &_pws_storage, result);
	mutex_unlock(&_pwmutex);
	return (rval);
}

struct passwd *
getpwuid(uid)
	uid_t uid;
{
	struct passwd *result;
	int rval;

	rval = getpwuid_r(uid, &_pws_passwd, _pws_passwdbuf, sizeof(_pws_passwdbuf), &result);
	return ((rval == NS_SUCCESS) ? result : NULL);
}

void
setpwent(void)
{
	mutex_lock(&_pwmutex);
	(void)_pws_setpwent(&_pws_storage);
	mutex_unlock(&_pwmutex);
}

int
setpassent(stayopen)
	int stayopen;
{
	int rval, result;

	mutex_lock(&_pwmutex);
	rval = _pws_setpassent(&_pws_storage, stayopen, &result);
	mutex_unlock(&_pwmutex);
	return ((rval == NS_SUCCESS) ? result : 0);
}

void
endpwent(void)
{
	mutex_lock(&_pwmutex);
	(void)_pws_endpwent(&_pws_storage);
	mutex_unlock(&_pwmutex);
}

void
setpwfile(file)
	const char *file;
{
	int result;

	mutex_lock(&_pwmutex);
	(void)_pws_setpwfile(&_pws_storage, file, &result);
	mutex_unlock(&_pwmutex);
}
