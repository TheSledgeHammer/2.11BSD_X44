/*	$NetBSD: in6.c,v 1.86 2004/03/28 08:28:06 christos Exp $	*/
/*	$KAME: in6.c,v 1.198 2001/07/18 09:12:38 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *
 *	@(#)in.c	8.2 (Berkeley) 11/15/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in6.c,v 1.86 2004/03/28 08:28:06 christos Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/mld6_var.h>

#include <net/net_osdep.h>

/*
 * This structure is used to keep track of in6_multi chains which belong to
 * deleted interface addresses.
 */
static LIST_HEAD(, multi6_kludge) in6_mk; /* XXX BSS initialization */

struct multi6_kludge {
	LIST_ENTRY(multi6_kludge) mk_entry;
	struct ifnet *mk_ifp;
	struct in6_multihead mk_head;
};

/*
 * Multicast address kludge:
 * If there were any multicast addresses attached to this interface address,
 * either move them to another address on this interface, or save them until
 * such time as this interface is reconfigured for IPv6.
 */
void
in6_savemkludge(struct in6_ifaddr *oia)
{
	struct in6_ifaddr *ia;
	struct in6_multi *in6m, *next;

	IFP_TO_IA6(oia->ia_ifp, ia);
	if (ia) {	/* there is another address */
		for (in6m = LIST_FIRST(&oia->ia6_multiaddrs); in6m; in6m = next) {
			next = LIST_NEXT(in6m, in6m_entry);
			IFAFREE(&in6m->in6m_ia->ia_ifa);
			IFAREF(&ia->ia_ifa);
			in6m->in6m_ia = ia;
			LIST_INSERT_HEAD(&ia->ia6_multiaddrs, in6m, in6m_entry);
		}
	} else {	/* last address on this if deleted, save */
		struct multi6_kludge *mk;

		for (mk = LIST_FIRST(&in6_mk); mk; mk = LIST_NEXT(mk, mk_entry)) {
			if (mk->mk_ifp == oia->ia_ifp)
				break;
		}
		if (mk == NULL) /* this should not happen! */
			panic("in6_savemkludge: no kludge space");

		for (in6m = LIST_FIRST(&oia->ia6_multiaddrs); in6m; in6m = next) {
			next = LIST_NEXT(in6m, in6m_entry);
			IFAFREE(&in6m->in6m_ia->ia_ifa); /* release reference */
			in6m->in6m_ia = NULL;
			LIST_INSERT_HEAD(&mk->mk_head, in6m, in6m_entry);
		}
	}
}

/*
 * Continuation of multicast address hack:
 * If there was a multicast group list previously saved for this interface,
 * then we re-attach it to the first address configured on the i/f.
 */
void
in6_restoremkludge(struct in6_ifaddr *ia, struct ifnet *ifp)
{
	struct multi6_kludge *mk;

	for (mk = LIST_FIRST(&in6_mk); mk; mk = LIST_NEXT(mk, mk_entry)) {
		if (mk->mk_ifp == ifp) {
			struct in6_multi *in6m, *next;

			for (in6m = LIST_FIRST(&mk->mk_head); in6m; in6m = next) {
				next = LIST_NEXT(in6m, in6m_entry);
				in6m->in6m_ia = ia;
				IFAREF(&ia->ia_ifa);
				LIST_INSERT_HEAD(&ia->ia6_multiaddrs,
						 in6m, in6m_entry);
			}
			LIST_INIT(&mk->mk_head);
			break;
		}
	}
}

/*
 * Allocate space for the kludge at interface initialization time.
 * Formerly, we dynamically allocated the space in in6_savemkludge() with
 * malloc(M_WAITOK).  However, it was wrong since the function could be called
 * under an interrupt context (software timer on address lifetime expiration).
 * Also, we cannot just give up allocating the strucutre, since the group
 * membership structure is very complex and we need to keep it anyway.
 * Of course, this function MUST NOT be called under an interrupt context.
 * Specifically, it is expected to be called only from in6_ifattach(), though
 * it is a global function.
 */
void
in6_createmkludge(struct ifnet *ifp)
{
	struct multi6_kludge *mk;

	for (mk = LIST_FIRST(&in6_mk); mk; mk = LIST_NEXT(mk, mk_entry)) {
		/* If we've already had one, do not allocate. */
		if (mk->mk_ifp == ifp)
			return;
	}

	mk = malloc(sizeof(*mk), M_IPMADDR, M_WAITOK);

	bzero(mk, sizeof(*mk));
	LIST_INIT(&mk->mk_head);
	mk->mk_ifp = ifp;
	LIST_INSERT_HEAD(&in6_mk, mk, mk_entry);
}

void
in6_purgemkludge(struct ifnet *ifp)
{
	struct multi6_kludge *mk;
	struct in6_multi *in6m;

	for (mk = LIST_FIRST(&in6_mk); mk; mk = LIST_NEXT(mk, mk_entry)) {
		if (mk->mk_ifp != ifp)
			continue;

		/* leave from all multicast groups joined */
		while ((in6m = LIST_FIRST(&mk->mk_head)) != NULL) {
#ifdef MLDV2
			int error;
			in6_delmulti2(in6m, &error, 0, NULL, MCAST_EXCLUDE, 1);
#else
			in6_delmulti(in6m);
#endif
		}
		LIST_REMOVE(mk, mk_entry);
		free(mk, M_IPMADDR);
		break;
	}
}
