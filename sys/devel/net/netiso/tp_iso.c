/*	$NetBSD: tp_iso.c,v 1.17 2003/09/30 00:01:18 christos Exp $	*/

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
 *	@(#)tp_iso.c	8.2 (Berkeley) 9/22/94
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
 * Here is where you find the iso-dependent code.  We've tried keep all
 * net-level and (primarily) address-family-dependent stuff out of the tp
 * source, and everthing here is reached indirectly through a switch table
 * (struct nl_protosw *) tpcb->tp_nlproto (see tp_pcb.c). The routines here
 * are: iso_getsufx: gets transport suffix out of an isopcb structure.
 * iso_putsufx: put transport suffix into an isopcb structure.
 * iso_putnetaddr: put a whole net addr into an isopcb. iso_getnetaddr: get a
 * whole net addr from an isopcb. iso_cmpnetaddr: compare a whole net addr
 * from an isopcb. iso_recycle_suffix: clear suffix for reuse in isopcb
 * tpclnp_ctlinput: handle ER CNLPdu : icmp-like stuff tpclnp_mtu: figure out
 * what size tpdu to use tpclnp_input: take a pkt from clnp, strip off its
 * clnp header, give to tp tpclnp_output_dg: package a pkt for clnp given 2
 * addresses & some data tpclnp_output: package a pkt for clnp given an
 * isopcb & some data
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tp_iso.c,v 1.17 2003/09/30 00:01:18 christos Exp $");

#include "opt_iso.h"
#ifdef ISO

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/protosw.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>

#include <netiso/argo_debug.h>
#include <netiso/tp_iso.h>
#include <netiso/cltp_var.h>
#include <netiso/idrp_var.h>

#include <machine/stdarg.h>

/*
 * CALLED FROM:
 * 	pr_usrreq() on PRU_BIND, PRU_CONNECT, PRU_ACCEPT, and PRU_PEERADDR
 * FUNCTION, ARGUMENTS:
 * 	The argument (which) takes the value TP_LOCAL or TP_FOREIGN.
 */

void
iso_getsufx(v, lenp, data_out, which)
	void	       *v;
	u_short        *lenp;
	caddr_t         data_out;
	int             which;
{
	struct isopcb  *isop = v;
	struct sockaddr_iso *addr = 0;

	switch (which) {
	case TP_LOCAL:
		addr = isop->isop_laddr;
		break;

	case TP_FOREIGN:
		addr = isop->isop_faddr;
	}
	if (addr)
		bcopy(TSEL(addr), data_out, (*lenp = addr->siso_tlen));
}

/*
 * CALLED FROM: tp_newsocket(); i.e., when a connection is being established
 * by an incoming CR_TPDU.
 *
 * FUNCTION, ARGUMENTS: Put a transport suffix (found in name) into an isopcb
 * structure (isop). The argument (which) takes the value TP_LOCAL or
 * TP_FOREIGN.
 */
void
iso_putsufx(v, sufxloc, sufxlen, which)
	void	       *v;
	caddr_t         sufxloc;
	int             sufxlen, which;
{
	struct isopcb  *isop = v;
	struct sockaddr_iso **dst, *backup;
	struct sockaddr_iso *addr;
	struct mbuf    *m;
	int             len;

	switch (which) {
	default:
		return;

	case TP_LOCAL:
		dst = &isop->isop_laddr;
		backup = &isop->isop_sladdr;
		break;

	case TP_FOREIGN:
		dst = &isop->isop_faddr;
		backup = &isop->isop_sfaddr;
	}
	if ((addr = *dst) == 0) {
		addr = *dst = backup;
		addr->siso_nlen = 0;
		addr->siso_slen = 0;
		addr->siso_plen = 0;
		printf("iso_putsufx on un-initialized isopcb\n");
	}
	len = sufxlen + addr->siso_nlen +
		(sizeof(*addr) - sizeof(addr->siso_data));
	if (addr == backup) {
		if (len > sizeof(*addr)) {
			m = m_getclr(M_DONTWAIT, MT_SONAME);
			if (m == 0)
				return;
			addr = *dst = mtod(m, struct sockaddr_iso *);
			*addr = *backup;
			m->m_len = len;
		}
	}
	bcopy(sufxloc, TSEL(addr), sufxlen);
	addr->siso_tlen = sufxlen;
	addr->siso_len = len;
}

/*
 * CALLED FROM:
 * 	tp.trans whenever we go into REFWAIT state.
 * FUNCTION and ARGUMENT:
 *	 Called when a ref is frozen, to allow the suffix to be reused.
 * 	(isop) is the net level pcb.  This really shouldn't have to be
 * 	done in a NET level pcb but... for the internet world that just
 * 	the way it is done in BSD...
 * 	The alternative is to have the port unusable until the reference
 * 	timer goes off.
 */
void
iso_recycle_tsuffix(v)
	void *v;
{
	struct isopcb *isop = v;
	isop->isop_laddr->siso_tlen = isop->isop_faddr->siso_tlen = 0;
}

/*
 * CALLED FROM:
 * 	tp_newsocket(); i.e., when a connection is being established by an
 * 	incoming CR_TPDU.
 *
 * FUNCTION and ARGUMENTS:
 * 	Copy a whole net addr from a struct sockaddr (name).
 * 	into an isopcb (isop).
 * 	The argument (which) takes values TP_LOCAL or TP_FOREIGN
 */
void
iso_putnetaddr(v, nm, which)
	void *v;
	struct sockaddr *nm;
	int             which;
{
	struct isopcb *isop = v;
	struct sockaddr_iso *name = (struct sockaddr_iso *) nm;
	struct sockaddr_iso **sisop, *backup;
	struct sockaddr_iso *siso;

	switch (which) {
	default:
		printf("iso_putnetaddr: should panic\n");
		return;
	case TP_LOCAL:
		sisop = &isop->isop_laddr;
		backup = &isop->isop_sladdr;
		break;
	case TP_FOREIGN:
		sisop = &isop->isop_faddr;
		backup = &isop->isop_sfaddr;
	}
	siso = ((*sisop == 0) ? (*sisop = backup) : *sisop);
#ifdef ARGO_DEBUG
	if (argo_debug[D_TPISO]) {
		printf("ISO_PUTNETADDR\n");
		dump_isoaddr(isop->isop_faddr);
	}
#endif
	siso->siso_addr = name->siso_addr;
}

/*
 * CALLED FROM:
 * 	tp_input() when a connection is being established by an
 * 	incoming CR_TPDU, and considered for interception.
 *
 * FUNCTION and ARGUMENTS:
 * 	compare a whole net addr from a struct sockaddr (name),
 * 	with that implicitly stored in an isopcb (isop).
 * 	The argument (which) takes values TP_LOCAL or TP_FOREIGN.
 */
int
iso_cmpnetaddr(v, nm, which)
	void *v;
	struct sockaddr *nm;
	int             which;
{
	struct isopcb *isop = v;
	struct sockaddr_iso *name = (struct sockaddr_iso *) nm;
	struct sockaddr_iso **sisop, *backup;
	struct sockaddr_iso *siso;

	switch (which) {
	default:
		printf("iso_cmpnetaddr: should panic\n");
		return 0;
	case TP_LOCAL:
		sisop = &isop->isop_laddr;
		backup = &isop->isop_sladdr;
		break;
	case TP_FOREIGN:
		sisop = &isop->isop_faddr;
		backup = &isop->isop_sfaddr;
	}
	siso = ((*sisop == 0) ? (*sisop = backup) : *sisop);
#ifdef ARGO_DEBUG
	if (argo_debug[D_TPISO]) {
		printf("ISO_CMPNETADDR\n");
		dump_isoaddr(siso);
	}
#endif
	if (name->siso_tlen && bcmp(TSEL(name), TSEL(siso), name->siso_tlen))
		return (0);
	return (bcmp((caddr_t) name->siso_data,
		     (caddr_t) siso->siso_data, name->siso_nlen) == 0);
}

/*
 * CALLED FROM:
 *  pr_usrreq() PRU_SOCKADDR, PRU_ACCEPT, PRU_PEERADDR
 * FUNCTION and ARGUMENTS:
 * 	Copy a whole net addr from an isopcb (isop) into
 * 	a struct sockaddr (name).
 * 	The argument (which) takes values TP_LOCAL or TP_FOREIGN.
 */

void
iso_getnetaddr(v, name, which)
	void *v;
	struct mbuf    *name;
	int             which;
{
	struct inpcb *inp = v;
	struct isopcb *isop = (struct isopcb *) inp;
	struct sockaddr_iso *siso =
	(which == TP_LOCAL ? isop->isop_laddr : isop->isop_faddr);
	if (siso)
		bcopy((caddr_t) siso, mtod(name, caddr_t),
		      (unsigned) (name->m_len = siso->siso_len));
	else
		name->m_len = 0;
}

/*ARGSUSED*/
void
iso_rtchange(pcb)
	struct isopcb *pcb;
{

}

#endif				/* ISO */
