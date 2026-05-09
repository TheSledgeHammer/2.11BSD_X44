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
 *	@(#)iso_pcb.c	8.3 (Berkeley) 7/19/94
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
 * $Header: iso_pcb.c,v 4.5 88/06/29 14:59:56 hagens Exp $
 * $Source: /usr/argo/sys/netiso/RCS/iso_pcb.c,v $
 *
 * Iso address family net-layer(s) pcb stuff. NEH 1/29/87
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/protosw.h>

#include <net/if.h>
#include <net/route.h>

#include <netiso/argo_debug.h>
#include <netiso/tp_param.h>
#include <netiso/tp_timer.h>
#include <netiso/tp_stat.h>
#include <netiso/tp_pcb.h>
#include <netiso/tp_tpdu.h>
#include <netiso/tp_trace.h>
#include <netiso/tp_meas.h>
#include <netiso/tp_seq.h>
#include <netiso/tp_clnp.h>

#include <netiso/tp_proto/tp_ip.h>
#include <netiso/tp_proto/tp_ip6.h>
#include <netiso/tp_proto/tp_iso.h>
#include <netiso/tp_proto/tp_ns.h>

struct tp_protosw tp_protosw[] = {
		{ /* IPv4 */
				&tpin4_protosw
		},
		{ /* IPv6 */
				&tpin6_protosw
		},
		{ /* ISO */
				&tpiso_protosw
		},
		{ /* XNS */
				&tpns_protosw
		},
		{ /* X25 */
				&tpx25_protosw
		},
};

struct inpcbtable tp_inpcbtable;
struct isopcbtable tp_isopcbtable;

struct tppcbhead tp_list;
struct tppcbhead tp_listeners;

#define	TPHASHSIZE	128

u_long tp_sendspace = 1024 * 4;
u_long tp_recvspace = 1024 * 4;

static void tp_freerefinfo(struct tp_ref **, struct tp_refinfo *, RefNum);
static u_long tp_getrefinfo(struct tp_ref **, struct tp_refinfo *, struct tp_pcb *);

/*
 * NAME:  tp_init()
 *
 * CALLED FROM:
 *  autoconf through the protosw structure
 *
 * FUNCTION:
 *  initialize tp machine
 *
 * RETURNS:  Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
void
tp_init(void)
{
	static int init_done = 0;

	in_pcbinit(&tp_inpcbtable, TPHASHSIZE, TPHASHSIZE);
	iso_pcbinit(&tp_isopcbtable, TPHASHSIZE);

	LIST_INIT(&tp_list);
	LIST_INIT(&tp_listeners);
	tp_protosw_init();

	tp_start_win = 2;
	bzero((caddr_t)&tp_stat, sizeof(struct tp_stat));
}

/*
 * NAME: 	tp_soisdisconnecting()
 *
 * CALLED FROM:
 *  tp.trans
 *
 * FUNCTION and ARGUMENTS:
 *  Set state of the socket (so) to reflect that fact that we're disconnectING
 *
 * RETURNS: 	Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 *  This differs from the regular soisdisconnecting() in that the latter
 *  also sets the SS_CANTRECVMORE and SS_CANTSENDMORE flags.
 *  We don't want to set those flags because those flags will cause
 *  a SIGPIPE to be delivered in sosend() and we don't like that.
 *  If anyone else is sleeping on this socket, wake 'em up.
 */
void
tp_soisdisconnecting(struct socket *so)
{
	soisdisconnecting(so);
	so->so_state &= ~SS_CANTSENDMORE;

	struct tp_pcb *tpcb;
	u_int fsufx, lsufx;

	tpcb = sototpcb(so);
	bcopy((caddr_t)tpcb->tp_fsuffix, (caddr_t)&fsufx, sizeof(u_int));
	bcopy((caddr_t)tpcb->tp_lsuffix, (caddr_t)&lsufx, sizeof(u_int));
	tpmeas(tpcb->tp_lref, TPtime_close, &time, fsufx, lsufx, tpcb->tp_fref);
	tpcb->tp_perf_on = 0; /* turn perf off */
}

/*
 * NAME: tp_soisdisconnected()
 *
 * CALLED FROM:
 *	tp.trans
 *
 * FUNCTION and ARGUMENTS:
 *  Set state of the socket (so) to reflect that fact that we're disconnectED
 *  Set the state of the reference structure to closed, and
 *  recycle the suffix.
 *  Start a reference timer.
 *
 * RETURNS:	Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 *  This differs from the regular soisdisconnected() in that the latter
 *  also sets the SS_CANTRECVMORE and SS_CANTSENDMORE flags.
 *  We don't want to set those flags because those flags will cause
 *  a SIGPIPE to be delivered in sosend() and we don't like that.
 *  If anyone else is sleeping on this socket, wake 'em up.
 */
void
tp_soisdisconnected(struct tp_pcb *tpcb)
{
	struct socket *so;

	so = tpcb->tp_sock;
	soisdisconnecting(so);
	so->so_state &= ~SS_CANTSENDMORE;

	tpcb->tp_refstate = REF_FROZEN;
	tp_recycle_tsuffix(tpcb);
	tp_etimeout(tpcb, TM_reference, (int)tpcb->tp_refer_ticks);
}

static void
tp_freerefinfo(struct tp_ref **tpref, struct tp_refinfo *tprefinfo, RefNum n)
{
	struct tp_ref *r;
	struct tp_pcb *tpcb;

	r = *tpref + n;
	tpcb = r->tpr_pcb;
	if (tpcb == 0) {
		return;
	}

	r->tpr_pcb = (struct tp_pcb *)0;
	tpcb->tp_refstate = REF_FREE;

	for (r = *tpref + tprefinfo->tpr_maxopen; r > *tpref; r--) {
		if (r->tpr_pcb) {
			break;
		}
	}
	tprefinfo->tpr_maxopen = r - *tpref;
	tprefinfo->tpr_numopen--;
}

static u_long
tp_getrefinfo(struct tp_ref **tpref, struct tp_refinfo *tprefinfo, struct tp_pcb *tpcb)
{
	struct tpref *r, *rlim;
	int i;
	caddr_t obase;
	unsigned int size;


	if (++tprefinfo->tpr_numopen < tprefinfo->tpr_size) {
		for (r = tprefinfo->tpr_base, rlim = r + tprefinfo->tpr_size;
				++r < rlim;) { /* tp_ref[0] is never used */
			if (r->tpr_pcb == 0) {
				goto got_one;
			}
		}
	}

	/* else have to allocate more space */

	obase = (caddr_t)tprefinfo->tpr_base;
	size = tprefinfo->tpr_size * sizeof(struct tp_ref);
	r = (struct tp_ref *)malloc(size + size, M_PCB, M_NOWAIT);
	if (r == 0) {
		return (--tprefinfo->tpr_numopen, TP_ENOREF);
	}
	tprefinfo->tpr_base = *tpref = r;
	tprefinfo->tpr_size *= 2;
	bcopy(obase, (caddr_t) r, size);
	free(obase, M_PCB);
	r = (struct tpi_ref *)(size + (caddr_t)r);
	bzero((caddr_t) r, size);

got_one:
	r->tpr_pcb = tpcb;
	tpcb->tp_refstate = REF_OPENING;
	i = r - tprefinfo->tpr_base;
	if (tprefinfo->tpr_maxopen < i) {
		tprefinfo->tpr_maxopen = i;
	}
	return ((u_long)i);
}

/*
 * NAME:	tp_freeref()
 *
 * CALLED FROM:
 *  tp.trans when the reference timer goes off, and
 *  from tp_attach() and tp_detach() when a tpcb is partially set up but not
 *  set up enough to have a ref timer set for it, and it's discarded
 *  due to some sort of error or an early close()
 *
 * FUNCTION and ARGUMENTS:
 *  Frees the reference represented by (r) for re-use.
 *
 * RETURNS: Nothing
 *
 * SIDE EFFECTS:
 *
 * NOTES:	better be called at clock priority !!!!!
 */
void
tp_freeref(RefNum n)
{
	tp_freerefinfo(&tp_ref, &tp_refinfo, n);
}

/*
 * NAME:  tp_getref()
 *
 * CALLED FROM:
 *  tp_attach()
 *
 * FUNCTION and ARGUMENTS:
 *  obtains the next free reference and allocates the appropriate
 *  ref structure, links that structure to (tpcb)
 *
 * RETURN VALUE:
 *	a reference number
 *  or TP_ENOREF
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
u_long
tp_getref(struct tp_pcb *tpcb)
{
	return (tp_getrefinfo(&tp_ref, &tp_refinfo, tpcb));
}

/*
 * NAME: tp_set_npcb()
 *
 * CALLED FROM:
 *	tp_attach(), tp_route_to()
 *
 * FUNCTION and ARGUMENTS:
 *  given a tpcb, allocate an appropriate lower-lever npcb, freeing
 *  any old ones that might need re-assigning.
 */
int
tpi_set_npcb(struct tp_pcb *tpcb)
{
	register struct socket *so = tpcb->tp_sock;
	int error;

	so = tpcb->tp_sock;
	if (tpcb->tp_tpproto && tpcb->tp_npcb) {
		short so_state = so->so_state;
		so->so_state &= ~SS_NOFDREF;
		(tpcb->tp_tpproto->tp_pcbdetach)(tpcb->tp_npcb);
		so->so_state = so_state;
	}
	tpcb->tp_tpproto = &tpi_protosw[tpcb->tp_netservice];
	/* xx_pcballoc sets so_pcb */
	error = (tpcb->tp_tpproto->tp_pcballoc)(so, tpcb->tp_tpproto->tp_pcblist);
	tpcb->tp_npcb = so->so_pcb;
	so->so_pcb = (caddr_t)tpcb;
	return (error);
}

/*
 * NAME: tp_attach()
 *
 * CALLED FROM:
 *	tp_usrreq, PRU_ATTACH
 *
 * FUNCTION and ARGUMENTS:
 *  given a socket (so) and a protocol family (dom), allocate a tpcb
 *  and ref structure, initialize everything in the structures that
 *  needs to be initialized.
 *
 * RETURN VALUE:
 *  0 ok
 *  EINVAL if DEBUG(X) in is on and a disaster has occurred
 *  ENOPROTOOPT if TP hasn't been configured or if the
 *   socket wasn't created with tp as its protocol
 *  EISCONN if this socket is already part of a connection
 *  ETOOMANYREFS if ran out of tp reference numbers.
 *  E* whatever error is returned from soreserve()
 *    for from the network-layer pcb allocation routine
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 */
int
tp_attach(struct socket *so, int protocol)
{
	struct tp_pcb *tpcb;
	int error, dom;
	u_long lref;

	error = 0;
	dom = so->so_proto->pr_domain->dom_family;

	IFDEBUG(D_CONN)
		printf("tp_attach:dom 0x%x so 0x%x ", dom, so);
	ENDDEBUG
	IFTRACE(D_CONN)
		tptrace(TPPTmisc, "tp_attach:dom so", dom, so, 0, 0);
	ENDTRACE
	if (so->so_pcb != NULL) {
		return (EISCONN);	/* socket already part of a connection*/
	}

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, tp_sendspace, tp_recvspace);
	}
	if (error) {
		goto bad2;
	}

	MALLOC(tpcb, struct tp_pcb *, sizeof(*tpcb), M_PCB, M_NOWAIT);
	if (tpcb == NULL) {
		error = ENOBUFS;
		goto bad2;
	}
	bzero( (caddr_t)tpcb, sizeof (struct tp_pcb) );

	if ( ((lref = tp_getref(tpcb)) &  TP_ENOREF) != 0 ) {
		error = ETOOMANYREFS;
		goto bad3;
	}
	tpcb->tp_lref = lref;
	tpcb->tp_sock =  so;
	tpcb->tp_domain = dom;
	tpcb->tp_rhiwat = so->so_rcv.sb_hiwat;
	/* tpcb->tp_proto = protocol; someday maybe? */
	if (protocol && protocol < ISOPROTO_TP4) {
		tpcb->tp_netservice = ISO_CONS;
		tpcb->tp_snduna = (SeqNum) -1;/* kludge so the pseudo-ack from the CR/CC
								 * will generate correct fake-ack values
								 */
	} else {
		tpcb->tp_netservice = (dom== AF_INET)?IN_CLNS:ISO_CLNS;
		/* the default */
	}
	tpcb->_tp_param = tp_conn_param[tpcb->tp_netservice];

	tpcb->tp_state = TP_CLOSED;
	tpcb->tp_vers  = TP_VERSION;
	tpcb->tp_notdetached = 1;

		   /* Spec says default is 128 octets,
			* that is, if the tpdusize argument never appears, use 128.
			* As the initiator, we will always "propose" the 2048
			* size, that is, we will put this argument in the CR
			* always, but accept what the other side sends on the CC.
			* If the initiator sends us something larger on a CR,
			* we'll respond w/ this.
			* Our maximum is 4096.  See tp_chksum.c comments.
			*/
	tpcb->tp_cong_win =
		tpcb->tp_l_tpdusize = 1 << tpcb->tp_tpdusize;

	tpcb->tp_seqmask  = TP_NML_FMT_MASK;
	tpcb->tp_seqbit  =  TP_NML_FMT_BIT;
	tpcb->tp_seqhalf  =  tpcb->tp_seqbit >> 1;

	/* attach to a network-layer protoswitch */
	if ((error = tp_set_npcb(tpcb))) {
		goto bad4;
	}
	KASSERT(tpcb->tp_tpproto->tp_afamily == tpcb->tp_domain);

	/* nothing to do for iso case */
	if( dom == AF_INET ) {
		sotoinpcb(so)->inp_ppcb = (caddr_t)tpcb;
	}

	return (0);

bad4:
	IFDEBUG(D_CONN)
		printf("BAD4 in tp_attach, so 0x%x\n", so);
	ENDDEBUG
	tp_freeref(tpcb->tp_lref);

bad3:
	IFDEBUG(D_CONN)
		printf("BAD3 in tp_attach, so 0x%x\n", so);
	ENDDEBUG

	free((caddr_t)tpcb, M_PCB); /* never a cluster  */

bad2:
	IFDEBUG(D_CONN)
		printf("BAD2 in tp_attach, so 0x%x\n", so);
	ENDDEBUG
	so->so_pcb = 0;

/*bad:*/
	IFDEBUG(D_CONN)
		printf("BAD in tp_attach, so 0x%x\n", so);
	ENDDEBUG
	return (error);
}

/*
 * NAME:  tp_detach()
 *
 * CALLED FROM:
 *	tp.trans, on behalf of a user close request
 *  and when the reference timer goes off
 * (if the disconnect  was initiated by the protocol entity
 * rather than by the user)
 *
 * FUNCTION and ARGUMENTS:
 *  remove the tpcb structure from the list of active or
 *  partially active connections, recycle all the mbufs
 *  associated with the pcb, ref structure, sockbufs, etc.
 *  Only free the ref structure if you know that a ref timer
 *  wasn't set for this tpcb.
 *
 * RETURNS:  Nada
 *
 * SIDE EFFECTS:
 *
 * NOTES:
 *  tp_soisdisconnected() was already when this is called
 */
void
tp_detach(struct tp_pcb *tpcb)
{
	struct socket *so;

	so = tpcb->tp_sock;
	IFDEBUG(D_CONN)
		printf("tp_detach(tpcb 0x%x, so 0x%x)\n", tpcb, so);
	ENDDEBUG
	IFTRACE(D_CONN)
		tptraceTPCB(TPPTmisc, "tp_detach tpcb so lsufx", tpcb, so,
			*(u_short*) (tpcb->tp_lsuffix), 0);
	ENDTRACE

	IFDEBUG(D_CONN)
		printf("so_snd at 0x%x so_rcv at 0x%x\n", &so->so_snd, &so->so_rcv);
		dump_mbuf(so->so_snd.sb_mb, "so_snd at detach ");
		printf("about to call LL detach, nlproto 0x%x, nl_detach 0x%x\n",
			tpcb->tp_tpproto, tpcb->tp_tpproto->tp_pcbdetach);
	ENDDEBUG

	if (tpcb->tp_Xsnd.sb_mb) {
		printf("Unsent Xdata on detach; would panic");
		sbflush(&tpcb->tp_Xsnd);
	}
	if (tpcb->tp_ucddata)
		m_freem(tpcb->tp_ucddata);

	IFDEBUG(D_CONN)
		printf("reassembly info cnt %d rsyq 0x%x\n", tpcb->tp_rsycnt,
			tpcb->tp_rsyq);
	ENDDEBUG
	if (tpcb->tp_rsyq)
		tp_rsyflush(tpcb);

	if (LIST_NEXT(tpcb, tp_list)) {
		LIST_REMOVE(tpcb, tp_list);
	}
	tpcb->tp_notdetached = 0;

	IFDEBUG(D_CONN)
		printf("calling (...nlproto->...)(0x%x, so 0x%x)\n", tpcb->tp_npcb, so);
		printf("so 0x%x so_head 0x%x,  qlen %d q0len %d qlimit %d\n", so,
			so->so_head, so->so_q0len, so->so_qlen, so->so_qlimit);
	ENDDEBUG

	(tpcb->tp_tpproto->tp_pcbdetach)(tpcb->tp_npcb);
	/* does an so->so_pcb = 0; sofree(so) */

	IFDEBUG(D_CONN)
		printf("after xxx_pcbdetach\n");
	ENDDEBUG

	if (tpcb->tp_state == TP_LISTENING) {
		struct tp_pcb *tt;

		LIST_FOREACH(tt, &tp_listeners, tp_nextlisten)
		{
			if (tt == tpcb) {
				break;
			}
		}
		if (tt) {
			tt = LIST_NEXT(tpcb, tp_nextlisten);
		} else {
			printf("tp_detach from listen: should panic\n");
		}
	}
	if (tpcb->tp_refstate == REF_OPENING) {
		/* no connection existed here so no reference timer will be called */
		IFDEBUG(D_CONN)
				printf("SETTING ref %d to REF_FREE\n", tpcb->tp_lref);
		ENDDEBUG

		tp_freeref(tpcb->tp_lref);
	}
#ifdef TP_PERF_MEAS
	/*
	 * Get rid of the cluster mbuf allocated for performance measurements, if
	 * there is one.  Note that tpcb->tp_perf_on says nothing about whether or
	 * not a cluster mbuf was allocated, so you have to check for a pointer
	 * to one (that is, we need the TP_PERF_MEASs around the following section
	 * of code, not the IFPERFs)
	 */
	if (tpcb->tp_p_mbuf) {
		register struct mbuf *m = tpcb->tp_p_mbuf;
		struct mbuf *n;
		IFDEBUG(D_PERF_MEAS)
			printf("freeing tp_p_meas 0x%x  ", tpcb->tp_p_meas);
		ENDDEBUG
		do {
			MFREE(m, n);
			m = n;
		} while (n);
		tpcb->tp_p_meas = 0;
		tpcb->tp_p_mbuf = 0;
	}
#endif /* TP_PERF_MEAS */

	IFDEBUG(D_CONN)
		printf("end of detach, NOT single, tpcb 0x%x\n", tpcb);
	ENDDEBUG
	/* free((caddr_t)tpcb, M_PCB); WHere to put this ? */
}

u_short tp_unique;

int
tp_tselinuse(u_short tlen, char *tsel, struct sockaddr_iso *siso, int reuseaddr)
{
	struct tp_pcb *tpcb;
	struct tp_pcb *t;

	LIST_FOREACH(tpcb, &tp_list, tp_list) {
		t = (struct tp_pcb *)LIST_NEXT(tpcb, tp_list);
		if (t == NULL) {
			return (1);
		}
		if (tlen == t->tp_lsuffixlen && bcmp(tsel, t->tp_lsuffix, tlen) == 0) {
			if (t->tp_flags & TPF_GENERAL_ADDR) {
				if (siso == 0 || reuseaddr == 0) {
					return (1);
				}
			} else if (siso) {
				if (siso->siso_family == t->tp_domain
						&& (t->tp_tpproto->tp_cmpnetaddr)(t->tp_npcb, siso, TP_LOCAL)) {
					return (1);
				}
			} else if (reuseaddr == 0) {
				return (1);
			}
		}
	}
	return (0);
}

int
tp_pcbbind(void *v, struct mbuf *nam, struct proc *p)
{
	struct tp_pcb *tpcb;
	struct sockaddr_iso *siso;
	int tlen, wrapped;
	caddr_t tsel;
	u_short tutil;
	int error;

	tpcb = v;
	tlen = 0;
	wrapped = 0;
	if (tpcb->tp_state != TP_CLOSED) {
		return (EINVAL);
	}
	if (nam) {
		siso = mtod(nam, struct sockaddr_iso *);
		switch (siso->siso_family) {
#ifdef ISO
		case AF_ISO:
			tlen = siso->siso_tlen;
			tsel = TSEL(siso);
			if (siso->siso_nlen == 0) {
				siso = NULL;
			}
			break;
#endif
#ifdef INET
		case AF_INET:
			tsel = (caddr_t)&tutil;
			if ((tutil = ((struct sockaddr_in *)siso)->sin_port)) {
				tlen = 2;
			}
			if (((struct sockaddr_in *)siso)->sin_addr.s_addr == 0) {
				siso = 0;
			}
			break;
#endif
#ifdef INET6
		case AF_INET6:
#endif
		default:
			return (EAFNOSUPPORT);
		}
	}
	if (tpcb->tp_lsuffixlen == 0) {
		if (tlen) {
			if (tpi_tselinuse(tlen, tsel, siso, tpcb->tp_sock->so_options & SO_REUSEADDR)) {
				return (EINVAL);
			}
		} else {
			for (tsel = (caddr_t)&tutil, tlen = 2;;) {
				if (tp_unique++ < ISO_PORT_RESERVED
						|| tp_unique > ISO_PORT_USERRESERVED) {
					if (wrapped++) {
						return ESRCH;
					}
					tp_unique = ISO_PORT_RESERVED;
				}
				tutil = htons(tp_unique);
				if (tpi_tselinuse(tlen, tsel, siso, 0) == 0) {
					break;
				}
			}
			if (siso) {
				switch (siso->siso_family) {
				case AF_ISO:
					bcopy(tsel, TSEL(siso), tlen);
					siso->siso_tlen = tlen;
					break;
				case AF_INET:
					((struct sockaddr_in *)siso)->sin_port = tutil;
					break;
				case AF_INET6:
				}
			}
		}
		bcopy(tsel, tpcb->tp_lsuffix, (tpcb->tp_lsuffixlen = tlen));
		LIST_INSERT_HEAD(&tp_list, tpcb, tp_list);
	} else {
		if (tlen || siso == 0) {
			return (EINVAL);
		}
	}
	if (siso == 0) {
		tpcb->tp_flags |= TPF_GENERAL_ADDR;
		return (0);
	}
	return ((tpcb->tp_tpproto->tp_pcbbind)(tpcb->tp_npcb, nam));
}


int
tp_mtu(struct tp_pcb *tpcb, struct	rtentry *rt, int size)
{
	tpcb->tp_routep = &rt;
	return (size);
}

void
tp_quench(struct tp_pcb *tpcb, int cmd)
{
	switch (cmd) {
	case PRC_QUENCH:
		tpcb->tp_cong_win = tpcb->tp_l_tpdusize;
		IncStat(ts_quench);
		break;
	case PRC_QUENCH2:
		tpcb->tp_cong_win = tpcb->tp_l_tpdusize; /* might as well quench source also */
		tpcb->tp_decbit = TP_DECBIT_CLEAR_COUNT;
		IncStat(ts_rcvdecbit);
		break;
	}
}

void
tp_abort(struct tp_pcb *tpcb, int cmd)
{
	struct tp_event e;

	e.ev_number = ER_TPDU;
	e.ATTR(ER_TPDU).e_reason = ENETRESET;
	tp_driver(tpcb, &e);
	return;
}
