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
 *	@(#)tp_inet.c	8.1 (Berkeley) 6/10/93
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
 * ARGO TP
 * $Header: tp_inet.c,v 5.3 88/11/18 17:27:29 nhall Exp $
 * $Source: /usr/argo/sys/netiso/RCS/tp_inet.c,v $
 *
 * Here is where you find the inet-dependent code.  We've tried
 * keep all net-level and (primarily) address-family-dependent stuff
 * out of the tp source, and everthing here is reached indirectly
 * through a switch table (struct nl_protosw *) tpcb->tp_nlproto
 * (see tp_pcb.c).
 * The routines here are:
 * 	in_getsufx: gets transport suffix out of an inpcb structure.
 * 	in_putsufx: put transport suffix into an inpcb structure.
 *	in_putnetaddr: put a whole net addr into an inpcb.
 *	in_getnetaddr: get a whole net addr from an inpcb.
 *	in_cmpnetaddr: compare a whole net addr from an isopcb.
 *	in_recycle_suffix: clear suffix for reuse in inpcb
 *	tpip_mtu: figure out what size tpdu to use
 *	tpip_input: take a pkt from ip, strip off its ip header, give to tp
 *	tpip_output_dg: package a pkt for ip given 2 addresses & some data
 *	tpip_output: package a pkt for ip given an inpcb & some data
 */

//#ifdef INET

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <net/if.h>

#include <netiso/argo_debug.h>
#include <netiso/tp_iso.h>
#include <netinet/in_var.h>
#include <netinet/in_pcb.h>
#ifndef ISO
#include <netiso/iso_chksum.c>
#endif

/*
 * NAME:			in_getsufx()

 * CALLED FROM: 	pr_usrreq() on PRU_BIND,
 *					PRU_CONNECT, PRU_ACCEPT, and PRU_PEERADDR
 *
 * FUNCTION, ARGUMENTS, and RETURN VALUE:
 * 	Get a transport suffix from an inpcb structure (inp).
 * 	The argument (which) takes the value TP_LOCAL or TP_FOREIGN.
 *
 * RETURNS:		internet port / transport suffix
 *  			(CAST TO AN INT)
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
void
in_getsufx(inp, lenp, data_out, which)
	struct inpcb *inp;
	u_short *lenp;
	caddr_t data_out;
	int which;
{
	*lenp = sizeof(u_short);
	switch (which) {
	case TP_LOCAL:
		*(u_short *)data_out = inp->inp_lport;
		return;

	case TP_FOREIGN:
		*(u_short *)data_out = inp->inp_fport;
	}

}

/*
 * NAME:		in_putsufx()
 *
 * CALLED FROM: tp_newsocket(); i.e., when a connection
 *		is being established by an incoming CR_TPDU.
 *
 * FUNCTION, ARGUMENTS:
 * 	Put a transport suffix (found in name) into an inpcb structure (inp).
 * 	The argument (which) takes the value TP_LOCAL or TP_FOREIGN.
 *
 * RETURNS:		Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
/*ARGSUSED*/
void
in_putsufx(inp, sufxloc, sufxlen, which)
	struct inpcb *inp;
	caddr_t sufxloc;
	int which;
{
	if (which == TP_FOREIGN) {
		bcopy(sufxloc, (caddr_t)&inp->inp_fport, sizeof(inp->inp_fport));
	}
}

/*
 * NAME:	in_recycle_tsuffix()
 *
 * CALLED FROM:	tp.trans whenever we go into REFWAIT state.
 *
 * FUNCTION and ARGUMENT:
 *	 Called when a ref is frozen, to allow the suffix to be reused.
 * 	(inp) is the net level pcb.
 *
 * RETURNS:			Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:	This really shouldn't have to be done in a NET level pcb
 *	but... for the internet world that just the way it is done in BSD...
 * 	The alternative is to have the port unusable until the reference
 * 	timer goes off.
 */
void
in_recycle_tsuffix(inp)
	struct inpcb	*inp;
{
	inp->inp_fport = inp->inp_lport = 0;
}

/*
 * NAME:	in_putnetaddr()
 *
 * CALLED FROM:
 * 	tp_newsocket(); i.e., when a connection is being established by an
 * 	incoming CR_TPDU.
 *
 * FUNCTION and ARGUMENTS:
 * 	Copy a whole net addr from a struct sockaddr (name).
 * 	into an inpcb (inp).
 * 	The argument (which) takes values TP_LOCAL or TP_FOREIGN
 *
 * RETURNS:		Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
void
in_putnetaddr(inp, name, which)
	register struct inpcb	*inp;
	struct sockaddr_in	*name;
	int which;
{
	switch (which) {
	case TP_LOCAL:
		bcopy((caddr_t)&name->sin_addr, (caddr_t)&inp->inp_laddr, sizeof(struct in_addr));
			/* won't work if the dst address (name) is INADDR_ANY */

		break;
	case TP_FOREIGN:
		if(name != NULL) {
			bcopy((caddr_t)&name->sin_addr, (caddr_t)&inp->inp_faddr, sizeof(struct in_addr));
		}
	}
}

/*
 * NAME:	in_putnetaddr()
 *
 * CALLED FROM:
 * 	tp_input() when a connection is being established by an
 * 	incoming CR_TPDU, and considered for interception.
 *
 * FUNCTION and ARGUMENTS:
 * 	Compare a whole net addr from a struct sockaddr (name),
 * 	with that implicitly stored in an inpcb (inp).
 * 	The argument (which) takes values TP_LOCAL or TP_FOREIGN
 *
 * RETURNS:		Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
int
in_cmpnetaddr(inp, name, which)
	register struct inpcb	*inp;
	register struct sockaddr_in	*name;
	int which;
{
	if (which == TP_LOCAL) {
		if (name->sin_port && name->sin_port != inp->inp_lport)
			return 0;
		return (name->sin_addr.s_addr == inp->inp_laddr.s_addr);
	}
	if (name->sin_port && name->sin_port != inp->inp_fport)
		return 0;
	return (name->sin_addr.s_addr == inp->inp_faddr.s_addr);
}

/*
 * NAME:	in_getnetaddr()
 *
 * CALLED FROM:
 *  pr_usrreq() PRU_SOCKADDR, PRU_ACCEPT, PRU_PEERADDR
 * FUNCTION and ARGUMENTS:
 * 	Copy a whole net addr from an inpcb (inp) into
 * 	an mbuf (name);
 * 	The argument (which) takes values TP_LOCAL or TP_FOREIGN.
 *
 * RETURNS:		Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */

void
in_getnetaddr(inp, name, which)
	register struct mbuf *name;
	struct inpcb *inp;
	int which;
{
	register struct sockaddr_in *sin = mtod(name, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof(*sin));
	switch (which) {
	case TP_LOCAL:
		sin->sin_addr = inp->inp_laddr;
		sin->sin_port = inp->inp_lport;
		break;
	case TP_FOREIGN:
		sin->sin_addr = inp->inp_faddr;
		sin->sin_port = inp->inp_fport;
		break;
	default:
		return;
	}
	name->m_len = sin->sin_len = sizeof (*sin);
	sin->sin_family = AF_INET;
}
