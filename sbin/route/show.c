/*	$NetBSD: show.c,v 1.21 2003/08/07 10:04:39 agc Exp $	*/

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
static char sccsid[] = "from: @(#)route.c	8.3 (Berkeley) 3/9/94";
#else
__RCSID("$NetBSD: show.c,v 1.21 2003/08/07 10:04:39 agc Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netns/ns.h>

#include <sys/sysctl.h>

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "extern.h"

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

/*
 * Definitions for showing gateway flags.
 */
struct bits {
	short	b_mask;
	char	b_val;
};
static const struct bits bits[] = {
	{ RTF_UP,	'U' },
	{ RTF_GATEWAY,	'G' },
	{ RTF_HOST,	'H' },
	{ RTF_REJECT,	'R' },
	{ RTF_DYNAMIC,	'D' },
	{ RTF_MODIFIED,	'M' },
	{ RTF_DONE,	'd' }, /* Completed -- for routing messages only */
	{ RTF_MASK,	'm' }, /* Mask Present -- for routing messages only */
	{ RTF_CLONING,	'C' },
	{ RTF_XRESOLVE,	'X' },
	{ RTF_LLINFO,	'L' },
	{ RTF_STATIC,	'S' },
	{ RTF_BLACKHOLE, 'B' },
	{ RTF_CLONED,	'c' },
	{ RTF_PROTO1,	'1' },
	{ RTF_PROTO2,	'2' },
	{ 0 }
};

static void pr_rthdr(void);
static void p_rtentry(struct rt_msghdr *);
static void pr_family(int);
static void p_sockaddr(struct sockaddr *, struct sockaddr *, int, int );
static void p_flags(int);

/*
 * Print routing tables.
 */
void
show(argc, argv)
	int argc;
	char **argv;
{
	size_t needed;
	int mib[6];
	char *buf, *next, *lim;
	struct rt_msghdr *rtm;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = 0;
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0)	{
		perror("route-sysctl-estimate");
		exit(1);
	}
	if ((buf = malloc(needed)) == 0)
		err(1, NULL);
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0)
		err(1, "sysctl of routing table");
	lim  = buf + needed;

	printf("Routing tables\n");

	/* for (i = 0; i <= AF_MAX; i++) ??? */
	{
		for (next = buf; next < lim; next += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)next;
			p_rtentry(rtm);
		}
	}
}


/* column widths; each followed by one space */
#define	WID_DST		17	/* width of destination column */
#define	WID_GW		18	/* width of gateway column */

/*
 * Print header for routing table columns.
 */
static void
pr_rthdr()
{

	printf("%-*.*s %-*.*s %-6.6s\n",
		WID_DST, WID_DST, "Destination",
		WID_GW, WID_GW, "Gateway",
		"Flags");
}


/*
 * Print a routing table entry.
 */
static void
p_rtentry(rtm)
	struct rt_msghdr *rtm;
{
	struct sockaddr *sa = (struct sockaddr *)(rtm + 1);
#ifdef notdef
	static int masks_done, banner_printed;
#endif
	static int old_af;
	int af = 0, interesting = RTF_UP | RTF_GATEWAY | RTF_HOST | RTF_REJECT;

#ifdef notdef
	/* for the moment, netmasks are skipped over */
	if (!banner_printed) {
		printf("Netmasks:\n");
		banner_printed = 1;
	}
	if (masks_done == 0) {
		if (rtm->rtm_addrs != RTA_DST ) {
			masks_done = 1;
			af = sa->sa_family;
		}
	} else
#endif
		af = sa->sa_family;
	if (old_af != af) {
		old_af = af;
		pr_family(af);
		pr_rthdr();
	}
	if (rtm->rtm_addrs == RTA_DST)
		p_sockaddr(sa, NULL, 0, WID_DST + 1 + WID_GW + 1);
	else {
		struct sockaddr *nm;

		if ((rtm->rtm_addrs & RTA_NETMASK) == 0)
			nm = NULL;
		else {
			/* skip to gateway */
			nm = (struct sockaddr *)
			    (ROUNDUP(sa->sa_len) + (char *)sa);
			/* skip over gateway to netmask */
			nm = (struct sockaddr *)
			    (ROUNDUP(nm->sa_len) + (char *)nm);
		}

		p_sockaddr(sa, nm, rtm->rtm_flags, WID_DST);
		sa = (struct sockaddr *)(ROUNDUP(sa->sa_len) + (char *)sa);
		p_sockaddr(sa, NULL, 0, WID_GW);
	}
	p_flags(rtm->rtm_flags & interesting);
	putchar('\n');
}


/*
 * Print address family header before a section of the routing table.
 */
static void
pr_family(af)
	int af;
{
	char *afname;

	switch (af) {
	case AF_INET:
		afname = "Internet";
		break;
#ifndef SMALL
#ifdef INET6
	case AF_INET6:
		afname = "Internet6";
		break;
#endif /* INET6 */
	case AF_NS:
		afname = "XNS";
		break;
	case AF_ISO:
		afname = "ISO";
		break;
	case AF_CCITT:
		afname = "X.25";
		break;
#endif /* SMALL */
	case AF_APPLETALK:
		afname = "AppleTalk";
		break;
	default:
		afname = NULL;
		break;
	}
	if (afname)
		printf("\n%s:\n", afname);
	else
		printf("\nProtocol Family %d:\n", af);
}


static void
p_sockaddr(sa, nm, flags, width)
	struct sockaddr *sa, *nm;
	int flags, width;
{
	char workbuf[128], *cplim;
	char *cp = workbuf;

	switch(sa->sa_family) {

	case AF_LINK:
		if (getnameinfo(sa, sa->sa_len, workbuf, sizeof(workbuf),
		    NULL, 0, NI_NUMERICHOST) != 0)
			strlcpy(workbuf, "invalid", sizeof(workbuf));
		cp = workbuf;
		break;

	case AF_INET:
		cp = routename(sa, nm, flags);
		break;

#ifndef SMALL
#ifdef INET6
	case AF_INET6:
		cp = routename(sa, nm, flags);
		/* make sure numeric address is not truncated */
		if (strchr(cp, ':') != NULL && strlen(cp) > width)
			width = strlen(cp);
		break;
#endif /* INET6 */

	case AF_NS:
		cp = ns_print((struct sockaddr_ns *)sa);
		break;
#endif /* SMALL */

	default:
	    {
		u_char *s = (u_char *)sa->sa_data, *slim;

		slim = sa->sa_len + (u_char *) sa;
		cplim = cp + sizeof(workbuf) - 6;
		cp += snprintf(cp, cplim - cp, "(%d)", sa->sa_family);
		while (s < slim && cp < cplim) {
			cp += snprintf(cp, cplim - cp, " %02x", *s++);
			if (s < slim)
			    cp += snprintf(cp, cplim - cp, "%02x", *s++);
		}
		cp = workbuf;
	    }
	}
	if (width < 0 )
		printf("%s ", cp);
	else {
		if (nflag)
			printf("%-*s ", width, cp);
		else
			printf("%-*.*s ", width, width, cp);
	}
}

static void
p_flags(f)
	int f;
{
	char name[33], *flags;
	const struct bits *p = bits;

	for (flags = name; p->b_mask; p++)
		if (p->b_mask & f)
			*flags++ = p->b_val;
	*flags = '\0';
	printf("%-6.6s ", name);
}

