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

#ifndef _GR_STORAGE_H_
#define _GR_STORAGE_H_

#if defined(YP) || defined(HESIOD)
#define _GROUP_COMPAT
#endif

#define	MAXGRP	200

struct group_storage {
	char 	*mem[MAXGRP];
	FILE 	*fp;
	int 	stayopen;
#ifdef HESIOD
	void 	*context;	/* Hesiod context */
	int 	num;		/* group index, -1 if no more */
#endif /* HESIOD */
#ifdef YP
	char	*domain;	/* NIS domain */
	int	 	done;		/* non-zero if search exhausted */
	char	*current;	/* current first/next match */
	int	 	currentlen;	/* length of _nis_current */
#endif /* YP */
#ifdef _GROUP_COMPAT
	char 	*name;
#endif /* _GROUP_COMPAT */
};

int _grs_start(struct group_storage *);
int _grs_end(struct group_storage *);
int _grs_grscan(int, gid_t, const char *, struct group *, struct group_storage *, char *, size_t);

#endif /* _GR_STORAGE_H_ */
