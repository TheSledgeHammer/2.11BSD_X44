/*-
 * Copyright (c) 1993
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
static char sccsid[] = "@(#)sysctl.c	8.2.2 (2.11BSD GTE) 1996/11/27";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/param.h>
#include <sys/sysctl.h>

#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

//extern	int	errno;
//static char _PATH_STDPATH[]="/usr/bin:/bin:/usr/ucb:/sbin:/usr/sbin:/usr/local:/usr/new";


int
sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp, *newp;
	size_t *oldlenp, newlen;
{
	if (name[0] != CTL_USER)
		return (__sysctl(name, namelen, oldp, oldlenp, newp, newlen));

	if (newp != NULL) {
		errno = EPERM;
		return (-1);
	}
	if (namelen != 2) {
		errno = EINVAL;
		return (-1);
	}

/*
 * This idea behind this section is silly.  Other than 'bc' who cares about
 * half of these?  A 3/4 hearted attempt is made however to return numbers 
 * that are not totally bogus.  
 *
 * Rather than port over the raft of include files with the attendant plethora
 * of #define statements we just plug in the numbers from 4.4-Lite.
 */

	switch (name[1]) {
	case USER_CS_PATH:
		if (oldp && *oldlenp < sizeof(_PATH_STDPATH))
			return (ENOMEM);
		*oldlenp = sizeof(_PATH_STDPATH);
		if (oldp != NULL)
			strcpy(oldp, _PATH_STDPATH);
		return (0);
	}

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	*oldlenp = sizeof(int);
	if (oldp == NULL)
		return (0);

	switch (name[1]) {
	case USER_BC_BASE_MAX:
		*(int *)oldp = BC_BASE_MAX;
		return (0);
	case USER_BC_SCALE_MAX:
		*(int *)oldp = BC_SCALE_MAX;
		return (0);
	case USER_BC_DIM_MAX:
		*(int *)oldp = BC_DIM_MAX;
		return (0);
	case USER_BC_STRING_MAX:
		*(int *)oldp = BC_STRING_MAX;
		return (0);
	case USER_COLL_WEIGHTS_MAX:
		*(int *)oldp = COLL_WEIGHTS_MAX;
		return (0);
	case USER_EXPR_NEST_MAX:
		*(int *)oldp = EXPR_NEST_MAX;
		return (0);
	case USER_LINE_MAX:
		*(int *)oldp = LINE_MAX;
		return (0);
	case USER_RE_DUP_MAX:
		*(int *)oldp = RE_DUP_MAX;
		return (0);
	case USER_POSIX2_VERSION:
	case USER_POSIX2_C_BIND:
	case USER_POSIX2_C_DEV:
	case USER_POSIX2_CHAR_TERM:
	case USER_POSIX2_FORT_DEV:
	case USER_POSIX2_FORT_RUN:
	case USER_POSIX2_LOCALEDEF:
	case USER_POSIX2_SW_DEV:
	case USER_POSIX2_UPE:
		*(int *)oldp = 0;
		return (0);
	case USER_STREAM_MAX:
		*(int *)oldp = FOPEN_MAX;
		return (0);
	case USER_TZNAME_MAX:
		*(int *)oldp = NAME_MAX;
		return (0);
	default:
		errno = EINVAL;
		return (-1);
	}
	/* NOTREACHED */
}
