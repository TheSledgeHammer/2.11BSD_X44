/*
 * Copyright (c) 1985, 1993
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
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)sethostent.c	6.3 (Berkeley) 4/10/86";
static char sccsid[] = "@(#)sethostent.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#if defined(_LIBC)
#include "namespace.h"
#endif

#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/nameser.h>

#include <netdb.h>
#include <resolv.h>

#include <resolv/res_private.h>

#include "hostent.h"

#if defined(_LIBC) && defined(__weak_alias)
__weak_alias(sethostent,_sethostent)
__weak_alias(endhostent,_endhostent)
#endif

void
sethostent_r(hd, statp, stayopen)
	struct hostent_data *hd;
	res_state statp;
	int stayopen;
{
	if (stayopen) {
		statp->options |= RES_STAYOPEN | RES_USEVC;
	}
	sethtent_r(stayopen, hd);
}

void
endhostent_r(hd, statp)
	struct hostent_data *hd;
	res_state statp;
{
	statp->options &= ~(RES_STAYOPEN | RES_USEVC);
	res_nclose(statp);
	endhtent_r(hd);
}

void
sethostfile_r(hd, name)
	struct hostent_data *hd;
	const char *name;
{
	sethtfile_r(name, hd);
}

void
sethostent(stayopen)
	int stayopen;
{
	res_state statp;

	statp = &_res;
	sethostent_r(&_hvs_hostd, statp, stayopen);
}

void
endhostent(void)
{
	res_state statp;

	statp = &_res;
	endhostent_r(&_hvs_hostd, statp);
}

void
sethostfile(name)
	const char *name;
{
	sethostfile_r(&_hvs_hostd, name);
}
