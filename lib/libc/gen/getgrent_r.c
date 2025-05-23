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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getgrent.c	5.2 (Berkeley) 3/9/86";
static char sccsid[] = "@(#)getgrent.c	8.2 (Berkeley) 3/21/94";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include "reentrant.h"

#include <sys/types.h>

#include <grp.h>
#include <limits.h>
#include <nsswitch.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(endgrent,_endgrent)
__weak_alias(getgrent,_getgrent)
__weak_alias(getgrent_r,_getgrent_r)
__weak_alias(getgrgid,_getgrgid)
__weak_alias(getgrgid_r,_getgrgid_r)
__weak_alias(getgrnam,_getgrnam)
__weak_alias(getgrnam_r,_getgrnam_r)
__weak_alias(setgrent,_setgrent)
__weak_alias(setgroupent,_setgroupent)
#endif

#ifdef _REENTRANT
static 	mutex_t			_grmutex = MUTEX_INITIALIZER;
#endif

#define	MAXGRP			200
#define	MAXLINELENGTH	_GETGR_R_SIZE_MAX

struct group_storage {
	char 	*mem[MAXGRP];
	FILE 	*fp;
	int 	stayopen;
};

static struct group_storage _grs_storage;
static struct group _grs_group;
static char _grs_groupbuf[MAXLINELENGTH];

static int _grs_grscan(int, gid_t, const char *, struct group *, struct group_storage *, char *, size_t);
static int _grs_start(struct group_storage *);
static int _grs_end(struct group_storage *);
static int _grs_setgrent(struct group_storage *);
static int _grs_setgroupent(struct group_storage *, int, int *);
static int _grs_endgrent(struct group_storage *);
static int _grs_getgrent(struct group *, struct group_storage *, char *, size_t, struct group **);
static int _grs_getgrnam(const char *, struct group *, struct group_storage *, char *, size_t, struct group **);
static int _grs_getgrgid(gid_t, struct group *, struct group_storage *, char *, size_t, struct group **);

static int
_grs_grscan(search, gid, name, group, state, buffer, buflen)
	register int search;
	register gid_t gid;
	register const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;
{
	register char *cp, **m;
	char *bp;

	for (;;) {
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
			continue;
		}

		group->gr_name = strsep(&bp, ":\n");
		if (search && name && strcmp(group->gr_name, name)) {
			continue;
		}
		group->gr_passwd = strsep(&bp, ":\n");
		if (!(cp = strsep(&bp, ":\n"))) {
			continue;
		}
		group->gr_gid = atoi(cp);
		if (search && name == NULL && group->gr_gid != gid) {
			continue;
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
	/* NOTREACHED */
}

static int
_grs_start(state)
	struct group_storage *state;
{
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
_grs_end(state)
	struct group_storage *state;
{
	if (state->fp) {
		fclose(state->fp);
		state->fp = NULL;
	}
	return (NS_SUCCESS);
}

static int
_grs_endgrent(state)
	struct group_storage *state;
{
	state->stayopen = 0;
	return (_grs_end(state));
}

static int
_grs_setgroupent(state, stayopen, result)
	struct group_storage *state;
	int stayopen, *result;
{
	int rval;

	state->stayopen = stayopen;
	rval = _grs_start(state);
	*result = (rval == NS_SUCCESS);
	return (rval);
}

static int
_grs_setgrent(state)
	struct group_storage *state;
{
	state->stayopen = 0;
	return (_grs_start(state));
}

static int
_grs_getgrent(group, state, buffer, buflen, result)
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;
	struct group **result;
{
	int rval;

	rval = _grs_start(state);
	if (rval != NS_SUCCESS) {
		return (rval);
	}
	rval = _grs_grscan(0, 0, NULL, group, state, buffer, buflen);
	if (rval == NS_SUCCESS) {
		result = &group;
	}
	return (rval);
}

static int
_grs_getgrnam(name, group, state, buffer, buflen, result)
	const char *name;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;
	struct group **result;
{
	int rval;

	rval = _grs_start(state);
	if (rval != NS_SUCCESS) {
		return (rval);
	}
	rval = _grs_grscan(1, 0, name, group, state, buffer, buflen);
	if (!state->stayopen) {
		_grs_end(state);
	}
	if (rval == NS_SUCCESS) {
		result = &group;
	}
	return (rval);
}

static int
_grs_getgrgid(gid, group, state, buffer, buflen, result)
	gid_t gid;
	struct group *group;
	struct group_storage *state;
	char *buffer;
	size_t buflen;
	struct group **result;
{
	int rval;

	rval = _grs_start(state);
	if (rval != NS_SUCCESS) {
		return (rval);
	}
	rval = _grs_grscan(1, gid, NULL, group, state, buffer, buflen);
	if (!state->stayopen) {
		_grs_end(state);
	}
	if (rval == NS_SUCCESS) {
		result = &group;
	}
	return (rval);
}

/*
 *	public functions
 */
int
getgrent_r(group, buffer, buflen, result)
	struct group *group;
	char *buffer;
	size_t buflen;
	struct group **result;
{
	int rval;

	mutex_lock(&_grmutex);
	rval = _grs_getgrent(group, &_grs_storage, buffer, buflen, result);
	mutex_unlock(&_grmutex);
	return (rval);
}

struct group *
getgrent(void)
{
	struct group *result;
	int rval;

	rval = getgrent_r(&_grs_group, _grs_groupbuf, sizeof(_grs_groupbuf), &result);
	return ((rval == NS_SUCCESS) ? result : NULL);
}

int
getgrnam_r(name, group, buffer, buflen, result)
	const char *name;
	struct group *group;
	char *buffer;
	size_t buflen;
	struct group **result;
{
	int rval;

	mutex_lock(&_grmutex);
	rval = _grs_getgrnam(name, group, &_grs_storage, buffer, buflen, result);
	mutex_unlock(&_grmutex);
	return (rval);
}

struct group *
getgrnam(name)
	const char *name;
{
	struct group *result;
	int rval;

	rval = getgrnam_r(name, &_grs_group, _grs_groupbuf, sizeof(_grs_groupbuf), &result);
	return ((rval == NS_SUCCESS) ? result : NULL);
}

int
getgrgid_r(gid, group, buffer, buflen, result)
	gid_t gid;
	struct group *group;
	char *buffer;
	size_t buflen;
	struct group **result;
{
	int rval;

	mutex_lock(&_grmutex);
	rval = _grs_getgrgid(gid, group, &_grs_storage, buffer, buflen, result);
	mutex_unlock(&_grmutex);
	return (rval);
}

struct group *
getgrgid(gid)
	gid_t gid;
{
	struct group *result;
	int rval;

	rval = getgrgid_r(gid, &_grs_group, _grs_groupbuf, sizeof(_grs_groupbuf), &result);
	return ((rval == NS_SUCCESS) ? result : NULL);
}

void
endgrent(void)
{
	mutex_lock(&_grmutex);
	(void)_grs_endgrent(&_grs_storage);
	mutex_unlock(&_grmutex);
}

int
setgroupent(stayopen)
	int stayopen;
{
	int rval, result;

	mutex_lock(&_grmutex);
	rval = _grs_setgroupent(&_grs_storage, stayopen, &result);
	mutex_unlock(&_grmutex);
	return ((rval == NS_SUCCESS) ? result : 0);
}

void
setgrent(void)
{
	mutex_lock(&_grmutex);
	(void)_grs_setgrent(&_grs_storage);
	mutex_unlock(&_grmutex);
}
