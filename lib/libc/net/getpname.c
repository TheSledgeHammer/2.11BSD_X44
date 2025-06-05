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
static char sccsid[] = "@(#)getprotoname.c	5.3 (Berkeley) 5/19/86";
static char sccsid[] = "@(#)getprotoname.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <netdb.h>
#include <string.h>

#include "protoent.h"

#ifdef __weak_alias
//__weak_alias(getprotobyname_r,getprotobyname_r)
__weak_alias(getprotobyname,_getprotobyname)
#endif

int
getprotobyname_r(pp, pd, name, buffer, buflen, result)
	struct protoent *pp;
	struct protoent_data *pd;
	const char *name;
	char *buffer;
	size_t buflen;
	struct protoent **result;
{
	register char **cp;
	int rval;

	setprotoent_r(pd->stayopen, pd);
	while ((rval = getprotoent_r(pp, pd, buffer, buflen, result))) {
		if (strcmp(pp->p_name, name) == 0) {
			break;
		}
		for (cp = pp->p_aliases; *cp != 0; cp++) {
			if (strcmp(*cp, name) == 0) {
				goto found;
			}
		}
	}
found:
	if (!pd->stayopen) {
		endprotoent_r(pd);
	}
	return (rval);
}

struct protoent *
getprotobyname(name)
	register const char *name;
{
	struct protoent *p;
	int rval;

	rval = getprotobyname_r(&_pvs_proto, &_pvs_protod, name, _pvs_protobuf, sizeof(_pvs_protobuf), &p);
	return ((rval == 1) ? p : NULL);
}

#ifdef original

extern int _proto_stayopen;

struct protoent *
getprotobyname(name)
	register const char *name;
{
	register struct protoent *p;
	register char **cp;

	setprotoent(_proto_stayopen);
	while ((p = getprotoent())) {
		if (strcmp(p->p_name, name) == 0)
			break;
		for (cp = p->p_aliases; *cp != 0; cp++)
			if (strcmp(*cp, name) == 0)
				goto found;
	}
found:
	if (!_proto_stayopen)
		endprotoent();
	return (p);
}

#endif
