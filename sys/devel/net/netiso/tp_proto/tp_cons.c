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
 *	@(#)tp_cons.c	8.1 (Berkeley) 6/10/93
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
 * $Header: tp_cons.c,v 5.6 88/11/18 17:27:13 nhall Exp $
 * $Source: /usr/argo/sys/netiso/RCS/tp_cons.c,v $
 *
 * Here is where you find the iso- and cons-dependent code.  We've tried
 * keep all net-level and (primarily) address-family-dependent stuff
 * out of the tp source, and everthing here is reached indirectly
 * through a switch table (struct nl_protosw *) tpcb->tp_nlproto
 * (see tp_pcb.c).
 * The routines here are:
 *	tpcons_input: pullup and call tp_input w/ correct arguments
 *	tpcons_output: package a pkt for cons given an isopcb & some data
 *	cons_chan_to_tpcb: find a tpcb based on the channel #
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <net/if.h>
#include <net/route.h>

#include <netiso/tp_protosw.h>

#include <netiso/tp_proto/tp_iso.h>

#include <netccitt/x25.h>
#include <netccitt/pk.h>
#include <netccitt/pk_var.h>

#include <netiso/if_cons.c>

struct tp_protosw tpcons_protosw = {
 	.tp_afamily = AF_ISO,
	.tp_xsapattach = iso_sapattach,
	.tp_xsapdetach = iso_sapdetach,
 	.tp_putnetaddr = iso_putnetaddr,
 	.tp_getnetaddr = iso_getnetaddr,
 	.tp_cmpnetaddr = iso_cmpnetaddr,
 	.tp_putsufx = iso_putsufx,
 	.tp_getsufx = iso_getsufx,
 	.tp_recycle_suffix = iso_recycle_tsuffix,
 	.tp_mtu = tpclnp_mtu,
 	.tp_pcbbind = iso_pcbbind,
 	.tp_pcbconnect = tpcons_pcbconnect,
 	.tp_pcbdisconnect = iso_pcbdisconnect,
 	.tp_pcbdetach = iso_pcbdetach,
 	.tp_pcballoc = iso_pcballoc,
 	.tp_output = tpcons_output,
 	.tp_dgoutput = tpcons_output_dg,
 	.tp_ctloutput = 0,
 	.tp_pcblist = &tp_isopcb
};

/*
 * CALLED FROM:
 *  tp_route_to() for PRU_CONNECT
 * FUNCTION, ARGUMENTS, SIDE EFFECTS and RETURN VALUE:
 *  version of the previous procedure for X.25
 */
int
tpcons_pcbconnect(void *v, struct mbuf *name)
{
	struct isopcb *isop;
	int error;

	isop = (struct isopcb *)v;
	error = iso_pcbconnect(isop, nam);
	if (error) {
		return (error);
	}
	isop->isop_chan = (caddr_t)pk_attach((struct socket *)0);
	if (isop->isop_chan == 0) {
		IFDEBUG(D_CCONS)
			printf("tpcons_pcbconnect: no pklcd; returns 0x%x\n", error);
		ENDDEBUG
		return (ENOBUFS);
	}
	error = cons_connect(isop);
	if (error) { /* if it doesn't work */
		/* oh, dear, throw packet away */
		pk_disconnect((struct pklcd *)isop->isop_chan);
		isop->isop_chan = 0;
	} else {
		isop->isop_refcnt = 1;
	}
	return (error);
}

/*
 * CALLED FROM:
 * 	cons
 * FUNCTION and ARGUMENTS:
 * THIS MAYBE BELONGS IN SOME OTHER PLACE??? but i think not -
 */
void *
tpcons_ctlinput(int cmd, struct sockaddr *sa, void *v)
{
	struct sockaddr_iso *siso;
	struct isopcb *isop;
	register struct tp_pcb *tpcb = 0;

	siso = (struct sockaddr_iso *)sa;
	isop = (struct isopcb *)v;
	if (isop->isop_socket) {
		tpcb = (struct tp_pcb *)sotoisopcb(isop->isop_socket);
	}
	switch (cmd) {
	case PRC_CONS_SEND_DONE:
		if (tpcb) {
			struct tp_event E;
			int error = 0;

			if (tpcb->tp_class == TP_CLASS_0) {
				/* only if class is exactly class zero, not
				 * still in class negotiation
				 */
				/* fake an ack */
				register SeqNum seq = SEQ_ADD(tpcb, tpcb->tp_snduna, 1);

				IFTRACE(D_DATA)
					tptrace(TPPTmisc, "FAKE ACK seq cdt 1",
						seq, 0,0,0);
				ENDTRACE
				IFDEBUG(D_DATA)
					printf("FAKE ACK seq 0x%x cdt 1\n", seq );
				ENDDEBUG
				E.ATTR(AK_TPDU).e_cdt = 1;
				E.ATTR(AK_TPDU).e_seq = seq;
				E.ATTR(AK_TPDU).e_subseq = 0;
				E.ATTR(AK_TPDU).e_fcc_present = 0;
				error = DoEvent(AK_TPDU);
				if (error) {
					tpcb->tp_sock->so_error = error;
				}
			} /* else ignore it */
		}
		break;
	case PRC_ROUTEDEAD:
		if (tpcb && tpcb->tp_class == TP_CLASS_0) {
			tpiso_reset(isop, 0);
			break;
		} /* else drop through */
	default:
		return (tpclnp_ctlinput(cmd, (struct sockaddr_iso *)siso, (struct isopcb *)isop));
	}
	return (NULL);
}

/*
 * CALLED FROM:
 * 	cons's intr routine
 * FUNCTION and ARGUMENTS:
 * Take a packet (m) from cons, pullup m as required by tp,
 *  ignore the socket argument, and call tp_input.
 * No return value.
 */
int
tpcons_input(struct mbuf *m, struct sockaddr_iso *faddr, struct sockaddr_iso *laddr, caddr_t channel)
{
	if ( m == NULL) {
		return (0);
	}

	m = (struct mbuf *)tp_inputprep(m);
	IFDEBUG(D_TPINPUT)
		printf("tpcons_input before tp_input(m 0x%x)\n", m);
		dump_buf( m, 12+ m->m_len);
	ENDDEBUG
	tp_input(m, faddr, laddr, channel, tpcons_output, 0);
	return (0);
}

static int
tpcons_output_sc(struct isopcb *isop, struct mbuf *m0, struct mbuf *m, struct iso_addr *laddr, struct iso_addr *faddr, int datalen, struct route *ro, int nochksum)
{
	int error;

	if (m == NULL) {
		return (0);
	}
	if ((m->m_flags & M_PKTHDR) == 0) {
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			return (ENOBUFS);
		}
		m->m_next = m0;
	}
	m->m_pkthdr.len = datalen;
	if (isop->isop_chan == 0) {
		/* got a restart maybe? */
		isop->isop_chan = (caddr_t)pk_attach((struct socket *)0);
		if (isop->isop_chan == 0) {
			IFDEBUG(D_CCONS)
				printf("tpcons_output: no pklcd\n");
			ENDDEBUG
			error = ENOBUFS;
		}
		error = cons_connect(isop);
		if (error) {
			pk_disconnect((struct pklcd *)isop->isop_chan);
			isop->isop_chan = 0;
			IFDEBUG(D_CCONS)
				printf("tpcons_output: can't reconnect\n");
			ENDDEBUG
		}
	} else {
		error = pk_send(isop->isop_chan, m);
		IncStat(ts_tpdu_sent);
	}
	return (error);
}

/*
 * CALLED FROM:
 *  tp_emit()
 * FUNCTION and ARGUMENTS:
 *  Take a packet(m0) from tp and package it so that cons will accept it.
 *  This means filling in a few of the fields.
 *  inp is the isopcb structure; datalen is the length of the data in the
 *  mbuf string m0.
 * RETURN VALUE:
 *  whatever (E*) is returned form the net layer output routine.
 */
int
tpcons_output(void *v, struct mbuf *m0, int datalen, int nochksum)
{
	struct isopcb *isop;
	struct mbuf *m;

	isop = (struct isopcb *)v;
	m = m0;

	IFDEBUG(D_EMIT)
		printf(
		"tpcons_output(isop 0x%x, m 0x%x, len 0x%x socket 0x%x\n",
			isop, m0, datalen, isop->isop_socket);
	ENDDEBUG
	return (tpcons_output_sc(isop, m0, m, isop->isop_laddr, isop->isop_faddr, datalen, &isop->isop_route, nochksum));
}

int
tpcons_output_dg(void *laddr_arg, void *faddr_arg, struct mbuf *m0, int datalen, void *ro_arg, int nochksum)
{
	struct iso_addr *laddr, *faddr;
	struct route *ro;
	struct mbuf *m;

	laddr = (struct iso_addr *)laddr_arg;
	faddr = (struct iso_addr *)faddr_arg;
	ro = (struct route *)ro_arg;

	return (tpcons_output_sc());
}

static int
tpcons_dg_output_sc(caddr_t chan, struct mbuf *m0, int datalen, int nochksum)
{
	struct pklcd *lcp;

	lcp = (struct pklcd *)chan;
	return (tpcons_output(lcp->lcd_upnext, m0, datalen, nochksum));
}

/*
 * CALLED FROM:
 *  tp_error_emit()
 * FUNCTION and ARGUMENTS:
 *  Take a packet(m0) from tp and package it so that cons will accept it.
 *  chan is the cons channel to use; datalen is the length of the data in the
 *  mbuf string m0.
 * RETURN VALUE:
 *  whatever (E*) is returned form the net layer output routine.
 */
int
tpcons_dg_output(caddr_t chan, struct mbuf *m0, int datalen)
{
	return (tpcons_dg_output_sc(chan, m0, datalen, 0));
}
