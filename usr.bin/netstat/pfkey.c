/*	$NetBSD: pfkey.c,v 1.1.44.2 2022/09/12 14:23:41 martin Exp $	*/
/*	$KAME: ipsec.c,v 1.33 2003/07/25 09:54:32 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1983, 1988, 1993
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)inet.c	8.4 (Berkeley) 4/20/94";
#else
#ifdef __NetBSD__
__RCSID("$NetBSD: pfkey.c,v 1.1.44.2 2022/09/12 14:23:41 martin Exp $");
#endif
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>

#ifdef IPSEC
#include <netipsec/keysock.h>
#endif

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "netstat.h"

#ifdef IPSEC

static const char *pfkey_msgtypenames[] = {
	"reserved", "getspi", "update", "add", "delete",
	"get", "acquire", "register", "expire", "flush",
	"dump", "x_promisc", "x_pchange", "x_spdupdate", "x_spdadd",
	"x_spddelete", "x_spdget", "x_spdacquire", "x_spddump", "x_spdflush",
	"x_spdsetidx", "x_spdexpire", "x_spddelete2"
};

static const char *pfkey_msgtype_names(int);

static const char *
pfkey_msgtype_names(int x)
{
	const int max =
	    sizeof(pfkey_msgtypenames)/sizeof(pfkey_msgtypenames[0]);
	static char buf[20];

	if (x < max && pfkey_msgtypenames[x])
		return pfkey_msgtypenames[x];
	snprintf(buf, sizeof(buf), "#%d", x);
	return buf;
}

void
pfkey_stats(u_long off, const char *name)
{
	struct pfkeystat pfkeystat;
	int first, type;

	if (off == 0)
		return;
	printf ("%s:\n", name);
	kread(off, (char *)&pfkeystat, sizeof(pfkeystat));

#define	p(f, m) if (pfkeystat.f || sflag <= 1) \
    printf(m, (unsigned long long)pfkeystat.f, plural(pfkeystat.f))

	/* userland -> kernel */
	p(out_total, "\t%llu request%s sent from userland\n");
	p(out_bytes, "\t%llu byte%s sent from userland\n");
	for (first = 1, type = 0;
	     type < sizeof(pfkeystat.out_msgtype)/sizeof(pfkeystat.out_msgtype[0]);
	     type++) {
		if (pfkeystat.out_msgtype[type] <= 0)
			continue;
		if (first) {
			printf("\thistogram by message type:\n");
			first = 0;
		}
		printf("\t\t%s: %llu\n", pfkey_msgtype_names(type),
		    (unsigned long long)pfkeystat.out_msgtype[type]);
	}
	p(out_invlen, "\t%llu message%s with invalid length field\n");
	p(out_invver, "\t%llu message%s with invalid version field\n");
	p(out_invmsgtype, "\t%llu message%s with invalid message type field\n");
	p(out_tooshort, "\t%llu message%s too short\n");
	p(out_nomem, "\t%llu message%s with memory allocation failure\n");
	p(out_dupext, "\t%llu message%s with duplicate extension\n");
	p(out_invexttype, "\t%llu message%s with invalid extension type\n");
	p(out_invsatype, "\t%llu message%s with invalid sa type\n");
	p(out_invaddr, "\t%llu message%s with invalid address extension\n");

	/* kernel -> userland */
	p(in_total, "\t%llu request%s sent to userland\n");
	p(in_bytes, "\t%llu byte%s sent to userland\n");
	for (first = 1, type = 0;
	     type < sizeof(pfkeystat.in_msgtype)/sizeof(pfkeystat.in_msgtype[0]);
	     type++) {
		if (pfkeystat.in_msgtype[type] <= 0)
			continue;
		if (first) {
			printf("\thistogram by message type:\n");
			first = 0;
		}
		printf("\t\t%s: %llu\n", pfkey_msgtype_names(type),
		    (unsigned long long)pfkeystat.in_msgtype[type]);
	}
	p(in_msgtarget[KEY_SENDUP_ONE],
	    "\t%llu message%s toward single socket\n");
	p(in_msgtarget[KEY_SENDUP_ALL],
	    "\t%llu message%s toward all sockets\n");
	p(in_msgtarget[KEY_SENDUP_REGISTERED],
	    "\t%llu message%s toward registered sockets\n");
	p(in_nomem, "\t%llu message%s with memory allocation failure\n");
#undef p
}
#endif /*IPSEC*/
