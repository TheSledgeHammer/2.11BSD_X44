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

#ifndef _PW_STORAGE_H_
#define _PW_STORAGE_H_

#if defined(YP) || defined(HESIOD)
#define _GROUP_COMPAT
#endif

struct passwd_storage {
	int 	stayopen;		/* see getpassent(3) */
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	DBM		*db;			/* DBM passwd file handle */
	FILE 	*fp;			/* DBM passwd file */
	const char 	*file;		/* DBM passwd file name */
	int 	rewind;
#else
	DB		*db;			/* DB passwd file handle */
#endif
	int	 	keynum;			/* key counter, -1 if no more */
	int	 	version;		/* version */
#ifdef HESIOD
	void 	*context;		/* Hesiod context */
	int	 	num;			/* passwd index, -1 if no more */
#endif /* HESIOD */
#ifdef YP
	char	*domain;		/* NIS domain */
	int		done;			/* non-zero if search exhausted */
	char	*current;		/* current first/next match */
	int		currentlen;		/* length of _nis_current */
	enum {					/* shadow map type */
		YPMAP_UNKNOWN = 0,	/*  unknown ... */
		YPMAP_NONE,			/*  none: use "passwd.by*" */
		YPMAP_ADJUNCT,		/*  pw_passwd from "passwd.adjunct.*" */
		YPMAP_MASTER		/*  all from "master.passwd.by*" */
	} maptype;
#endif /* YP */
#ifdef _GROUP_COMPAT
	enum {
		COMPAT_NOTOKEN = 0,	/*  no compat token present */
		COMPAT_NONE,		/*  parsing normal pwd.db line */
		COMPAT_FULL,		/*  parsing `+' entries */
		COMPAT_USER,		/*  parsing `+name' entries */
		COMPAT_NETGROUP		/*  parsing `+@netgroup' entries */
	} mode;
	char 	*user;			/* COMPAT_USER "+name" */
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
	DBM 	*exclude;		/* compat exclude DBM */
#else
	DB		*exclude;		/* compat exclude DB */
#endif
	struct passwd proto;	/* proto passwd entry */
	char	protobuf[_GETPW_R_SIZE_MAX]; /* buffer for proto ptrs */
	int		protoflags;		/* proto passwd flags */
#endif /* _GROUP_COMPAT */
};

extern struct passwd_storage _pws_storage;
extern struct passwd _pws_passwd;
extern char _pws_passwdbuf[_GETPW_R_SIZE_MAX];

int _pws_start(struct passwd_storage *);
int _pws_end(struct passwd_storage *);
int _pws_search(struct passwd *, char *, size_t, struct passwd_storage *, int, const char *, uid_t);

#endif /* _PW_STORAGE_H_ */
