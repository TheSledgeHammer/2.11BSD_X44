/*
 * Copyright (c) 1990, 1993
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
 *
 *	@(#)ccitt_addr.c	8.2 (Berkeley) 4/28/95
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <stdlib.h>

#include "x25.h"

static int x121_address(const char *, struct x25_addr *, int, int);
static int optional_priority(const char *, char, char);
static int optional_udata(const char *, char *, int);
static const char *copychar(const char *, char *);

int
ccitt_addr(addr, xp)
	register const char *addr;
	struct sockaddr_x25 *xp;
{
	struct x25opts *opts;
	struct x25_addr *xaddr;
	int havenet = 0;
	int rval;

	memset(xp, 0, sizeof(*xp));
	xp->x25_family = AF_CCITT;
	xp->x25_len = sizeof(*xp);
	xaddr = &xp->x25_addr;
	opts = &xp->x25_opts;

	/*
	 * process optional priority and reverse charging flags
	 */
	rval = optional_priority(addr, opts->op_psize, opts->op_flags);
	if (rval != 1) {
		return (rval);
	}

	/*
	 * [network id:]X.121 address
	 */
	rval = x121_address(addr, xaddr, xp->x25_net, havenet);
	if (rval != 2) {
		return (rval);
	}
	/*
	 * optional user data, bytes 4 to 16
	 */
	rval = optional_udata(addr, xp->x25_udata, xp->x25_udlen);
	if (rval != 2) {
		return (rval);
	}
	return (1);
}

/*
 * [network id:]X.121 address
 */
static int
x121_address(addr, xaddr, net, havenet)
	const char *addr;
	struct x25_addr *xaddr;
	int net, havenet;
{
	register char *ap, *limit;
	int rval;

	rval = 2;
	ap = xaddr->xa_addr;
	limit = ap + sizeof(xaddr->xa_addr) - 1;
	while (*addr) {
		if (*addr == ',') {
			break;
		}
		if (*addr == '.' || *addr == ':') {
			if (havenet) {
				rval = 0;
				goto out;
			}
			havenet++;
			net = atoi(xaddr->xa_addr);
			addr++;
			ap = xaddr->xa_addr;
			*ap = '\0';
		}
		if (*addr < '0' || *addr > '9') {
			rval = 0;
			goto out;
		}
		if (ap >= limit) {
			rval = 0;
			goto out;
		}
		*ap++ = *addr++;
	}
	if (*addr == '\0') {
		rval = 1;
		goto out;
	}
out:
	return (rval);
}

/*
 * process optional priority and reverse charging flags
 */
static int
optional_priority(addr, psize, flags)
	const char *addr;
	char psize, flags;
{
	int rval = 1;

	if (*addr == 'p' || *addr == 'r' || *addr == 'h') {
		while (*addr == 'p' || *addr == 'r' || *addr == 'h') {
			if (*addr == 'p' || *addr == 'h') {
				psize = X25_PS128;
			} else if (*addr == 'r') {
				flags |= X25_REVERSE_CHARGE;
			}
			addr++;
		}
		if (*addr != ',') {
			rval = 0;
			goto out;
		}
		addr++;
	}
	if (*addr == '\0') {
		rval = 0;
		goto out;
	}

out:
	return (rval);
}

/*
 * optional user data, bytes 4 to 16
 */
static int
optional_udata(addr, udata, udlen)
	const char *addr;
	char *udata;
	int udlen;
{
	register char *ap, *limit;
	int rval = 2;

	addr++;
	ap = udata + 4; /* first four bytes are protocol id */
	limit = ap + sizeof(udata) - 4;
	udlen = 4;
	while (*addr) {
		if (*addr == ',') {
			break;
		}
		if (ap >= limit) {
			rval = 0;
			goto out;
		}
		addr = copychar(addr, ap++);
		udlen++;
	}
	if (udlen == 4) {
		udlen = 0;
	}
	if (*addr == '\0') {
		rval = 1;
		goto out;
	}

	addr++;
	ap = udata; /* protocol id */
	limit = ap + (udlen ? 4 : sizeof(udata));
	while (*addr) {
		if (*addr == ',') {
			rval = 0;
			goto out;
		}
		if (ap >= limit) {
			rval = 0;
			goto out;
		}
		addr = copychar(addr, ap++);
	}
	if (udlen == 0) {
		udlen = ap - udata;
	}
out:
	return (rval);
}

static const char *
copychar(from, to)
	const char *from;
	char *to;
{
	register int n = 0;

	if (*from != '\\' || from[1] < '0' || from[1] > '7') {
		*to = *from++;
		return (from);
	}
	n = *++from - '0';
	from++;
	if (*from >= '0' && *from <= '7') {
		register int n1 = 0;

		n = n * 8 + *from++ - '0';
		if (*from >= '0' && *from <= '7' && (n1 = n * 8 + *from - '0') < 256) {
			n = n1;
			from++;
		}
	}
	*to = (char) n;
	return (from);
}
