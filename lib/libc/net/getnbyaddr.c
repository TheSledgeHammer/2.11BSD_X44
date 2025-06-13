/*
 * Copyright (c) 1983, 1993
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
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getnetbyaddr.c	5.3 (Berkeley) 5/19/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <netdb.h>

#include "netent.h"

int
getnetbyaddr_r(np, nd, net, type, buffer, buflen, result)
	struct netent *np;
	struct netent_data *nd;
	long net;
	int type;
	char *buffer;
	size_t buflen;
	struct netent **result;
{
	int rval;

	setnetent_r(nd->stayopen, nd);
	while ((rval = getnetent_r(np, nd, buffer, buflen, result))) {
		if (np->n_addrtype == type && np->n_net == net) {
			break;
		}
	}
	if (!nd->stayopen) {
		endnetent_r(nd);
	}
	return (rval);
}

struct netent *
getnetbyaddr(net, type)
	register long net;
	register int type;
{
	struct netent *p;
	int rval;

	rval = getnetbyaddr_r(&_nvs_net, &_nvs_netd, net, type, _nvs_netbuf, sizeof(_nvs_netbuf), &p);
	return ((rval == 1) ? p : NULL);
}

#ifdef original

extern int _net_stayopen;

struct netent *
getnetbyaddr(net, type)
	register long net;
	register int type;
{
	register struct netent *p;

	setnetent(_net_stayopen);
	while ((p = getnetent()))
		if (p->n_addrtype == type && p->n_net == net)
			break;
	if (!_net_stayopen)
		endnetent();
	return (p);
}
#endif
