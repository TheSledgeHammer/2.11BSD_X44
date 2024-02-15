/*	$NetBSD: esis.c,v 1.30 2003/08/07 16:33:35 agc Exp $	*/

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
 *	@(#)esis.c	8.3 (Berkeley) 3/20/95
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: esis.c,v 1.30 2003/08/07 16:33:35 agc Exp $");

#include "opt_iso.h"
#ifdef ISO

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <netiso/iso.h>
#include <netiso/iso_pcb.h>
#include <netiso/iso_var.h>
#include <netiso/iso_snpac.h>
#include <netiso/clnl.h>
#include <netiso/clnp.h>
#include <netiso/clnp_stat.h>
#include <netiso/esis.h>
#include <netiso/argo_debug.h>

#include <machine/stdarg.h>


/*
 * FUNCTION:		isis_input
 *
 * PURPOSE:		Process an incoming isis packet
 *
 * RETURNS:		nothing
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
void
isis_input(struct mbuf *m0, ...)
{
	struct snpa_hdr *shp;	/* subnetwork header */
	struct rawcb *rp, *first_rp = 0;
	struct ifnet   *ifp;
	struct mbuf    *mm;
	va_list ap;

	va_start(ap, m0);
	shp = va_arg(ap, struct snpa_hdr *);
	va_end(ap);
	ifp = shp->snh_ifp;

#ifdef ARGO_DEBUG
	if (argo_debug[D_ISISINPUT]) {
		int             i;

		printf("isis_input: pkt on ifp %p (%s): from:",
		    ifp, ifp->if_xname);
		for (i = 0; i < 6; i++)
			printf("%x%c", shp->snh_shost[i] & 0xff,
			    (i < 5) ? ':' : ' ');
		printf(" to:");
		for (i = 0; i < 6; i++)
			printf("%x%c", shp->snh_dhost[i] & 0xff,
			    (i < 5) ? ':' : ' ');
		printf("\n");
	}
#endif
	esis_dl.sdl_alen = ifp->if_addrlen;
	esis_dl.sdl_index = ifp->if_index;
	bcopy(shp->snh_shost, (caddr_t) esis_dl.sdl_data, esis_dl.sdl_alen);
	for (rp = LIST_FIRST(esis_pcb); rp != 0; rp = LIST_NEXT(rp, rcb_list)) {
		if (first_rp == 0) {
			first_rp = rp;
			continue;
		}
		/* can't block at interrupt level */
		if ((mm = m_copy(m0, 0, M_COPYALL)) != NULL) {
			if (sbappendaddr(&rp->rcb_socket->so_rcv,
					 (struct sockaddr *) &esis_dl, mm,
					 (struct mbuf *) 0) != 0) {
				sorwakeup(rp->rcb_socket);
			} else {
#ifdef ARGO_DEBUG
				if (argo_debug[D_ISISINPUT]) {
					printf(
				    "Error in sbappenaddr, mm = %p\n", mm);
				}
#endif
				m_freem(mm);
			}
		}
	}
	if (first_rp && sbappendaddr(&first_rp->rcb_socket->so_rcv,
	       (struct sockaddr *) &esis_dl, m0, (struct mbuf *) 0) != 0) {
		sorwakeup(first_rp->rcb_socket);
		return;
	}
	m_freem(m0);
}

int
isis_output(struct mbuf *m, ...)
{
	struct sockaddr_dl *sdl;
	struct ifnet *ifp;
	struct ifaddr  *ifa;
	struct sockaddr_iso siso;
	int             error = 0;
	unsigned        sn_len;
	va_list ap;

	va_start(ap, m);
	sdl = va_arg(ap, struct sockaddr_dl *);
	va_end(ap);

	ifa = ifa_ifwithnet((struct sockaddr *) sdl);	/* get ifp from sdl */
	if (ifa == 0) {
#ifdef ARGO_DEBUG
		if (argo_debug[D_ISISOUTPUT]) {
			printf("isis_output: interface not found\n");
		}
#endif
		error = EINVAL;
		goto release;
	}
	ifp = ifa->ifa_ifp;
	sn_len = sdl->sdl_alen;
#ifdef ARGO_DEBUG
	if (argo_debug[D_ISISOUTPUT]) {
		u_char *cp = (u_char *) LLADDR(sdl), *cplim = cp + sn_len;
		printf("isis_output: ifp %p (%s), to: ",
		    ifp, ifp->if_xname);
		while (cp < cplim) {
			printf("%x", *cp++);
			printf("%c", (cp < cplim) ? ':' : ' ');
		}
		printf("\n");
	}
#endif
	bzero((caddr_t) & siso, sizeof(siso));
	siso.siso_family = AF_ISO;	/* This convention may be useful for
					 * X.25 */
	if (sn_len == 0)
		siso.siso_nlen = 0;
	else {
		siso.siso_data[0] = AFI_SNA;
		siso.siso_nlen = sn_len + 1;
		bcopy(LLADDR(sdl), siso.siso_data + 1, sn_len);
	}
	error = (ifp->if_output) (ifp, m, sisotosa(&siso), 0);
	if (error) {
#ifdef ARGO_DEBUG
		if (argo_debug[D_ISISOUTPUT]) {
			printf("isis_output: error from if_output is %d\n",
			    error);
		}
#endif
	}
	return (error);

release:
	if (m != NULL)
		m_freem(m);
	return (error);
}

#endif /* ISO */
