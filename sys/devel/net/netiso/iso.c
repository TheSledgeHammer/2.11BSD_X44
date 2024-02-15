/*	$NetBSD: iso.c,v 1.33 2003/08/07 16:33:36 agc Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
 *	@(#)iso.c	8.3 (Berkeley) 1/9/95
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */
/*
 * iso.c: miscellaneous routines to support the iso address family
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iso.c,v 1.33 2003/08/07 16:33:36 agc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netiso/iso.h>
#include <netiso/iso_var.h>
#include <netiso/iso_snpac.h>
#include <netiso/iso_pcb.h>
#include <netiso/clnp.h>
#include <netiso/argo_debug.h>

int  iso_interfaces = 0;	/* number of external interfaces */

/*
 * Generic iso control operations (ioctl's).
 * Ifp is 0 if not an interface-specific ioctl.
 */
/* ARGSUSED */
int
iso_control(so, cmd, data, ifp, p)
	struct socket *so;
	u_long cmd;
	caddr_t data;
	struct ifnet *ifp;
	struct proc *p;
{
	struct iso_ifreq *ifr = (struct iso_ifreq *) data;
	struct iso_ifaddr *ia = 0;
	struct iso_aliasreq *ifra = (struct iso_aliasreq *) data;
	int error, hostIsNew, maskIsNew;

	/*
	 * Find address for this interface, if it exists.
	 */
	if (ifp) {
		for (ia = TAILQ_FIRST(iso_ifaddr); ia != 0; ia = TAILQ_NEXT(ia, ia_list)) {
			if (ia->ia_ifp == ifp) {
				break;
			}
		}
	}

	switch (cmd) {

	case SIOCAIFADDR_ISO:
	case SIOCDIFADDR_ISO:
		if (ifra->ifra_addr.siso_family == AF_ISO)
			for (; ia != 0; ia = TAILQ_NEXT(ia, ia_list)) {
				if (ia->ia_ifp == ifp &&
				    SAME_ISOADDR(&ia->ia_addr, &ifra->ifra_addr))
					break;
			}
		if (cmd == SIOCDIFADDR_ISO && ia == 0)
			return (EADDRNOTAVAIL);
		/* FALLTHROUGH */
#if 0
	case SIOCSIFADDR:
	case SIOCSIFNETMASK:
	case SIOCSIFDSTADDR:
#endif
		if (p == 0 || (error = suser1(p->p_ucred, &p->p_acflag)))
			return (EPERM);

		if (ifp == 0)
			panic("iso_control");
		if (ia == 0) {
			MALLOC(ia, struct iso_ifaddr *, sizeof(*ia), M_IFADDR, M_WAITOK);
			if (ia == 0)
				return (ENOBUFS);
			bzero((caddr_t)ia, sizeof(*ia));
			TAILQ_INSERT_TAIL(&iso_ifaddr, ia, ia_list);
			IFAREF((struct ifaddr *)ia);
			TAILQ_INSERT_TAIL(&ifp->if_addrlist, (struct ifaddr *)ia,
			    ifa_list);
			IFAREF((struct ifaddr *)ia);
			ia->ia_ifa.ifa_addr = sisotosa(&ia->ia_addr);
			ia->ia_ifa.ifa_dstaddr = sisotosa(&ia->ia_dstaddr);
			ia->ia_ifa.ifa_netmask = sisotosa(&ia->ia_sockmask);
			ia->ia_ifp = ifp;
			if ((ifp->if_flags & IFF_LOOPBACK) == 0) {
				iso_interfaces++;
			}
		}
		break;

	case SIOCGIFADDR_ISO:
	case SIOCGIFNETMASK_ISO:
	case SIOCGIFDSTADDR_ISO:
		if (ia == 0)
			return (EADDRNOTAVAIL);
		break;
	}
	switch (cmd) {

	case SIOCGIFADDR_ISO:
		ifr->ifr_Addr = ia->ia_addr;
		break;

	case SIOCGIFDSTADDR_ISO:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		ifr->ifr_Addr = ia->ia_dstaddr;
		break;

	case SIOCGIFNETMASK_ISO:
		ifr->ifr_Addr = ia->ia_sockmask;
		break;

	case SIOCAIFADDR_ISO:
		maskIsNew = 0;
		hostIsNew = 1;
		error = 0;
		if (ia->ia_addr.siso_family == AF_ISO) {
			if (ifra->ifra_addr.siso_len == 0) {
				ifra->ifra_addr = ia->ia_addr;
				hostIsNew = 0;
			} else if (SAME_ISOADDR(&ia->ia_addr, &ifra->ifra_addr))
				hostIsNew = 0;
		}
		if (ifra->ifra_mask.siso_len) {
			iso_ifscrub(ifp, ia);
			ia->ia_sockmask = ifra->ifra_mask;
			maskIsNew = 1;
		}
		if ((ifp->if_flags & IFF_POINTOPOINT) &&
		    (ifra->ifra_dstaddr.siso_family == AF_ISO)) {
			iso_ifscrub(ifp, ia);
			ia->ia_dstaddr = ifra->ifra_dstaddr;
			maskIsNew = 1;	/* We lie; but the effect's the same */
		}
		if (ifra->ifra_addr.siso_family == AF_ISO &&
		    (hostIsNew || maskIsNew)) {
			error = iso_ifinit(ifp, ia, &ifra->ifra_addr, 0);
		}
		if (ifra->ifra_snpaoffset) {
			ia->ia_snpaoffset = ifra->ifra_snpaoffset;
		}
		return (error);

	case SIOCDIFADDR_ISO:
		iso_purgeaddr(&ia->ia_ifa, ifp);
		break;

#define cmdbyte(x)	(((x) >> 8) & 0xff)
	default:
		if (cmdbyte(cmd) == 'a')
			return (snpac_ioctl(so, cmd, data, p));
		if (ifp == 0 || ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		return ((*ifp->if_ioctl)(ifp, cmd, data));
	}
	return (0);
}

void
iso_purgeaddr(ifa, ifp)
	struct ifaddr *ifa;
	struct ifnet *ifp;
{
	struct iso_ifaddr *ia = (void *) ifa;

	iso_ifscrub(ifp, ia);
	TAILQ_REMOVE(&ifp->if_addrlist, (struct ifaddr *)ia, ifa_list);
	IFAFREE(&ia->ia_ifa);
	TAILQ_REMOVE(&iso_ifaddr, ia, ia_list);
	IFAFREE((&ia->ia_ifa));
}

void
iso_purgeif(ifp)
	struct ifnet *ifp;
{
	struct ifaddr *ifa, *nifa;

	for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL; ifa = nifa) {
		nifa = TAILQ_NEXT(ifa, ifa_list);
		if (ifa->ifa_addr->sa_family != AF_ISO)
			continue;
		iso_purgeaddr(ifa, ifp);
	}
}

/*
 * Delete any existing route for an interface.
 */
void
iso_ifscrub(ifp, ia)
	struct ifnet *ifp;
	struct iso_ifaddr *ia;
{
	int  nsellength = ia->ia_addr.siso_tlen;
	if ((ia->ia_flags & IFA_ROUTE) == 0)
		return;
	ia->ia_addr.siso_tlen = 0;
	if (ifp->if_flags & IFF_LOOPBACK) {
		rtinit(&(ia->ia_ifa), (int) RTM_DELETE, RTF_HOST);
	} else if (ifp->if_flags & IFF_POINTOPOINT) {
		rtinit(&(ia->ia_ifa), (int) RTM_DELETE, RTF_HOST);
	} else {
		rtinit(&(ia->ia_ifa), (int) RTM_DELETE, 0);
	}
	ia->ia_addr.siso_tlen = nsellength;
	ia->ia_flags &= ~IFA_ROUTE;
}

/*
 * Initialize an interface's internet address
 * and routing table entry.
 */
int
iso_ifinit(ifp, ia, siso, scrub)
	struct ifnet *ifp;
	struct iso_ifaddr *ia;
	struct sockaddr_iso *siso;
	int scrub;
{
	struct sockaddr_iso oldaddr;
	int s = splnet(), error, nsellength;

	oldaddr = ia->ia_addr;
	ia->ia_addr = *siso;
	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	if (ifp->if_ioctl &&
	    (error = (*ifp->if_ioctl) (ifp, SIOCSIFADDR, (caddr_t) ia))) {
		splx(s);
		ia->ia_addr = oldaddr;
		return (error);
	}
	if (scrub) {
		ia->ia_ifa.ifa_addr = sisotosa(&oldaddr);
		iso_ifscrub(ifp, ia);
		ia->ia_ifa.ifa_addr = sisotosa(&ia->ia_addr);
	}
	/*
	 * XXX -- The following is here temporarily out of laziness in not
	 * changing every ethernet driver's if_ioctl routine
	 */
	if (ifp->if_type == IFT_ETHER || ifp->if_type == IFT_FDDI) {
		ia->ia_ifa.ifa_rtrequest = llc_rtrequest;
		ia->ia_ifa.ifa_flags |= RTF_CLONING;
	}
	/*
	 * Add route for the network.
	 */
	nsellength = ia->ia_addr.siso_tlen;
	ia->ia_addr.siso_tlen = 0;
	if (ifp->if_flags & IFF_LOOPBACK) {
		ia->ia_ifa.ifa_dstaddr = ia->ia_ifa.ifa_addr;
		error = rtinit(&(ia->ia_ifa), (int) RTM_ADD, RTF_HOST | RTF_UP);
	} else if ((ifp->if_flags & IFF_POINTOPOINT) && ia->ia_dstaddr.siso_family == AF_ISO) {
		error = rtinit(&(ia->ia_ifa), (int) RTM_ADD, RTF_HOST | RTF_UP);
	} else {
		rt_maskedcopy(ia->ia_ifa.ifa_addr, ia->ia_ifa.ifa_dstaddr, ia->ia_ifa.ifa_netmask);
		ia->ia_dstaddr.siso_nlen = min(ia->ia_addr.siso_nlen, (ia->ia_sockmask.siso_len - 6));
		error = rtinit(&(ia->ia_ifa), (int) RTM_ADD, RTF_UP);
	}
	ia->ia_addr.siso_tlen = nsellength;
	ia->ia_flags |= IFA_ROUTE;
	splx(s);
	return (error);
}

struct iso_ifaddr *
iso_localifa(siso)
	struct sockaddr_iso *siso;
{
	struct iso_ifaddr *ia;
	char  *cp1, *cp2, *cp3;
	struct ifnet *ifp;
	struct iso_ifaddr *ia_maybe = 0;
	/*
	 * We make one pass looking for both net matches and an exact
	 * dst addr.
	 */
	for (ia = TAILQ_FIRST(iso_ifaddr); ia != 0; ia = TAILQ_NEXT(ia, ia_list)) {
		if ((ifp = ia->ia_ifp) == 0 || ((ifp->if_flags & IFF_UP) == 0))
			continue;
		if (ifp->if_flags & IFF_POINTOPOINT) {
			if ((ia->ia_dstaddr.siso_family == AF_ISO) &&
			    SAME_ISOADDR(&ia->ia_dstaddr, siso))
				return (ia);
			else if (SAME_ISOADDR(&ia->ia_addr, siso))
				ia_maybe = ia;
			continue;
		}
		if (ia->ia_sockmask.siso_len) {
			char *cplim = ia->ia_sockmask.siso_len +
				      (char *) &ia->ia_sockmask;
			cp1 = ia->ia_sockmask.siso_data;
			cp2 = siso->siso_data;
			cp3 = ia->ia_addr.siso_data;
			while (cp1 < cplim)
				if (*cp1++ & (*cp2++ ^ *cp3++))
					goto next;
			ia_maybe = ia;
		}
		if (SAME_ISOADDR(&ia->ia_addr, siso))
			return ia;
next:		;
	}
	return ia_maybe;
}
