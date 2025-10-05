/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)in_pcb.h	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>

#include <sys/errno.h>
#include <sys/fnv_hash.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/route.h>

#include <netiso/tp_meas.h>
#include <netiso/tp_param.h>

#include <tpi_pcb.h>
#include <tpi_protosw.h>
#include <tpi_user.h>

union tpi_sockaddr_union tpi_sockaddr;
union tpi_addr_union tpi_zero_addr;

/* Transport Generic Network Hash */
/* fnv-1a Hash Algorithm is a placeholder */
uint32_t
tpi_pcbnethash(void *addr, uint16_t port)
{
    uint32_t ahash = fnva_32_buf(addr, sizeof(*addr), FNV1_32_INIT);	/* addr hash */
    uint32_t phash = fnva_32_buf(&port, sizeof(port), FNV1_32_INIT);	/* port hash */
    return ((ntohl(ahash) + ntohs(phash)));
}

void
tpi_pcbinit(struct tpipcbtable *table, int hashsize)
{
	CIRCLEQ_INIT(&table->tppt_queue);
	table->tppt_lhashtbl = hashinit(hashsize, M_PCB, &table->tppt_lhash);
	table->tppt_fhashtbl = hashinit(hashsize, M_PCB, &table->tppt_fhash);
}

int
tpi_pcballoc(struct socket *so, void *v, int af)
{
	struct tpipcbtable *table;
	struct tpipcb *tpp;
	int s, error;

	table = v;
	error = tpi_set_npcb(&tpp, so, af);
	if (error != 0) {
		return (error);
	}
	tpp->tpp_table = table;
	s = splnet();
	CIRCLEQ_INSERT_HEAD(&table->tppt_queue, &tpp->tpp_head, tpph_queue);
	tpi_local_insert(table, tpp->tpp_laddr, tpp->tpp_lport);
	tpi_foreign_insert(table, tpp->tpp_faddr, tpp->tpp_fport);
	splx(s);
	return (error);
}

void
tpi_soisdisconnecting(struct socket *so)
{
	soisdisconnecting(so);
	so->so_state &= ~SS_CANTSENDMORE;

	struct tpipcb *tpcb;
	u_int fsufx, lsufx;

	tpcb = sototpcb(so);
	bcopy((caddr_t)tpcb->tpp_fsuffix, (caddr_t)&fsufx, sizeof(u_int));
	bcopy((caddr_t)tpcb->tpp_lsuffix, (caddr_t)&lsufx, sizeof(u_int));
	tpmeas(tpcb->tpp_lref, TPtime_close, &time, fsufx, lsufx, tpcb->tpp_fref);
	tpcb->tpp_perf_on = 0; /* turn perf off */
}

void
tpi_soisdisconnected(struct tpipcb *tpp)
{
	struct socket *so;

	so = tpp->tpp_socket;
	soisdisconnecting(so);
	so->so_state &= ~SS_CANTSENDMORE;

	tpp->tpp_refstate = REF_FROZEN;
	tp_recycle_tsuffix(tpp);
	tp_etimeout(tpp, TM_reference, (int)tpp->tpp_refer_ticks);
}

void
tpi_freeref(struct tpi_ref **tpref, struct tpi_refinfo *tprefinfo, RefNum n)
{
	struct tpi_ref *r;
	struct tpipcb *tpp;

	r = *tpref + n;
	tpp = r->tpr_pcb;
	if (tpp == 0) {
		return;
	}

	r->tpr_pcb = (struct tp_pcb *)0;
	tpp->tpp_refstate = REF_FREE;

	for (r = *tpref + tprefinfo->tpr_maxopen; r > *tpref; r--) {
		if (r->tpr_pcb) {
			break;
		}
	}
	tprefinfo->tpr_maxopen = r - *tpref;
	tprefinfo->tpr_numopen--;
}

u_long
tpi_getref(struct tpi_ref **tpref, struct tpi_refinfo *tprefinfo, struct tpipcb *tpp)
{
	struct tpi_ref *r, *rlim;
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

	obase = (caddr_t) tprefinfo->tpr_base;
	size = tprefinfo->tpr_size * sizeof(struct tpi_ref);
	r = (struct tpi_ref*) malloc(size + size, M_PCB, M_NOWAIT);
	if (r == 0) {
		return (--tprefinfo->tpr_numopen, TP_ENOREF);
	}
	tprefinfo->tpr_base = *tpref = r;
	tprefinfo->tpr_size *= 2;
	bcopy(obase, (caddr_t) r, size);
	free(obase, M_PCB);
	r = (struct tpi_ref*) (size + (caddr_t) r);
	bzero((caddr_t) r, size);

got_one:
	r->tpr_pcb = tpp;
	tpp->tpp_refstate = REF_OPENING;
	i = r - tprefinfo->tpr_base;
	if (tprefinfo->tpr_maxopen < i) {
		tprefinfo->tpr_maxopen = i;
	}
	return ((u_long)i);
}

void
tp_freeref(RefNum n)
{
	tpi_freeref(&tpi_ref, &tpi_refinfo, n);
}

u_long
tp_getref(struct tpipcb *tpcb)
{
	return (tpi_getref(&tpi_ref, &tpi_refinfo, tpcb));
}

int
tpi_tselinuse(struct tpipcbtable *table, struct tpipcb *tpcb, u_short tlen, char *tsel, struct sockaddr_iso *siso, int reuseaddr)
{
	struct tpipcb *t;

	CIRCLEQ_FOREACH(&tpcb->tpp_head, &table->tppt_queue, tpph_queue) {
		t = (struct tpipcb *)CIRCLEQ_NEXT(&tpcb->tpp_head, tpph_queue);
		if (t == NULL) {
			return (1);
		}
		if (tlen == t->tpp_lsuffixlen && bcmp(tsel, t->tpp_lsuffix, tlen) == 0) {
			if (t->tpp_flags & TPF_GENERAL_ADDR) {
				if (siso == 0 || reuseaddr == 0) {
					return (1);
				}
			} else if (siso) {
				if (siso->siso_family == t->tpp_af
						&& (t->tpp_tpproto->tpi_cmpnetaddr)(t->tpp_npcb, siso, TPI_LOCAL)) {
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
tpi_set_npcb(struct tpipcb **tpcb, struct socket *so, int af)
{
	struct tpipcb *tpp;
	int error;

	MALLOC(tpp, struct tpipcb *, sizeof(*tpp), M_PCB, M_WAITOK);
	if (tpp == NULL) {
		return (ENOBUFS);
	}
	bzero((caddr_t)tpp, sizeof(*tpp));
	tpp->tpp_af = af;
	tpp->tpp_socket = so;
	if (tpp->tpp_tpproto && tpp->tpp_npcb) {
		short so_state = so->so_state;
		so->so_state &= ~SS_NOFDREF;
		(tpp->tpp_tpproto->tpi_pcbdetach)(tpp->tpp_npcb);
		so->so_state = so_state;
	}
	tpp->tpp_tpproto = &tpi_protosw[tpp->tpp_netservice];
	/* xx_pcballoc sets so_pcb */
	error = (tpp->tpp_tpproto->tpi_pcballoc)(so, tpp->tpp_tpproto->tpi_pcblist);
	tpp->tpp_npcb = so->so_pcb;
	so->so_pcb = tpp;
	if (error == 0) {
		tpcb = &tpp;
	} else {
		*tpcb = NULL;
	}
	return (error);
}

int
tpi_get_npcb(struct tpipcb *tpp, struct socket *so, int af)
{
	switch (af) {
	case AF_INET:
		struct inpcb *inp;

		inp = sotoinpcb(so);
		inp->inp_ppcb = (caddr_t)tpp;
		break;
	case AF_INET6:
		struct in6pcb *in6p;

		in6p = sotoin6pcb(so);
		in6p->in6p_ppcb = (caddr_t)tpp;
		break;
	case AF_ISO:
		struct isopcb *isop;

		isop = sotoisopcb(so);
		isop->isop_ppcb = (caddr_t)tpp;
		break;
	case AF_NS:
		struct nspcb *nsp;

		nsp = sotonspcb(so);
		nsp->nsp_pcb = (caddr_t)tpp;
		break;
	case AF_CCITT:
		goto bad;
		//break;
	/* Others: Not implemented */
	case AF_APPLETALK:
		goto bad;
		//break;
	case AF_SNA:
		goto bad;
		//break;
	case AF_NATM:
		goto bad;
		//break;
	case AF_IPX:
		goto bad;
		//break;
	default:
bad:
		return (EPROTONOSUPPORT);
	}
	return (0);
}

int
tpi_attach(struct socket *so, int af, int protocol)
{
	struct tpipcb *tpp;
	int error, dom;
	u_long lref;

	error = 0;
	dom = so->so_proto->pr_domain->dom_family;
	if (so->so_pcb != NULL) {
		return (EISCONN);	/* socket already part of a connection*/
	}

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		error = soreserve(so, tpi_sendspace, tpi_recvspace);
	}
	if (error) {
		goto bad2;
	}

	error = tpi_set_npcb(&tpp, so, af);
	if (error != 0) {
		if (error == ENOBUFS) {
			goto bad2;
		}
		goto bad4;
	}

	lref = tpi_getref(&tpi_ref, &tpi_refinfo, tpp);
	if ((lref & TP_ENOREF) != 0) {
		error = ETOOMANYREFS;
		goto bad3;
	}
	tpp->tpp_lref = lref;
//	tpp->tpp_socket = so; /* already setup in tpi_set_npcb */
	tpp->tpp_domain = dom;
	tpp->tpp_rhiwat = so->so_rcv.sb_hiwat;
	/* tpcb->tp_proto = protocol; someday maybe? */
	if (protocol && protocol < ISOPROTO_TP4) {
		tpp->tpp_netservice = ISO_CONS;
		tpp->tpp_snduna = (SeqNum) - 1;/* kludge so the pseudo-ack from the CR/CC
		 * will generate correct fake-ack values
		 */
	} else {
		tpp->tpp_netservice = (dom == AF_INET) ? IN_CLNS : ISO_CLNS;
		/* the default */
	}
	tpp->tpp_param = tpi_conn_param[tpp->tpp_netservice];

	tpp->tpp_state = TPI_CLOSED;
	tpp->tpp_vers = TPI_VERSION;
	tpp->tpp_notdetached = 1;

	/* Spec says default is 128 octets,
	 * that is, if the tpdusize argument never appears, use 128.
	 * As the initiator, we will always "propose" the 2048
	 * size, that is, we will put this argument in the CR
	 * always, but accept what the other side sends on the CC.
	 * If the initiator sends us something larger on a CR,
	 * we'll respond w/ this.
	 * Our maximum is 4096.  See tp_chksum.c comments.
	 */
	tpp->tpp_cong_win = tpp->tpp_l_tpdusize = 1 << tpp->tpp_tpdusize;

	tpp->tpp_seqmask = TP_NML_FMT_MASK;
	tpp->tpp_seqbit = TP_NML_FMT_BIT;
	tpp->tpp_seqhalf = tpp->tpp_seqbit >> 1;

	/* attach to a network-layer protoswitch */
	KASSERT(tpp->tpp_tpproto->tpi_afamily == tpp->tpp_domain);
	if (dom == af) {
		error = tpi_get_npcb(tpp, so, dom);
		if (error != 0) {
			return (error);
		}
	}
	return (0);

bad4:
	tpi_freeref(&tpi_ref, &tpi_refinfo, tpp->tpp_lref);

bad3:
	free((caddr_t)tpp, M_PCB); /* never a cluster  */

bad2:
	so->so_pcb = NULL;

	return (error);
}

void
tpi_detach(struct tpipcb *tpp)
{

}

int
tpi_pcbbind(void *v, struct mbuf *nam, struct proc *p)
{
	struct tpipcb *tpp;
	struct tpipcbtable *table;
	struct sockaddr_iso *siso;
	int tlen, wrapped;
	caddr_t tsel;
	u_short tutil;
	int error;

	tpp = v;
	table = &tpp->tpp_table;
	tlen = 0;
	wrapped = 0;
	if (nam) {
		siso = mtod(nam, struct sockaddr_iso *);
		switch (siso->siso_family) {
		case AF_ISO:
			tlen = siso->siso_tlen;
			tsel = TSEL(siso);
			if (siso->siso_nlen == 0) {
				siso = NULL;
			}
			break;
		case AF_INET:
			tsel = (caddr_t)&tutil;
			if ((tutil = ((struct sockaddr_in *)siso)->sin_port)) {
				tlen = 2;
			}
			if (((struct sockaddr_in *)siso)->sin_addr.s_addr == 0) {
				siso = 0;
			}
			break;
		case AF_INET6:
		default:
			return (EAFNOSUPPORT);
		}
	}
	if (tpp->tpp_lsuffixlen == 0) {
		if (tlen) {
			if (tpi_tselinuse(table, tpp, tlen, tsel, siso, tpp->tpp_socket->so_options & SO_REUSEADDR)) {
				return (EINVAL);
			}
		} else {
			for (tsel = (caddr_t)&tutil, tlen = 2;;) {
				if (tpi_tselinuse(table, tpp, tlen, tsel, siso, 0) == 0) {
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
		bcopy(tsel, tpp->tpp_lsuffix, (tpp->tpp_lsuffixlen = tlen));
		CIRCLEQ_INSERT_HEAD(&table->tppt_queue, &tpp->tpp_head, tpph_queue);
	} else {
		if (tlen || siso == 0) {
			return (EINVAL);
		}
	}
	if (siso == 0) {
		tpp->tpp_flags |= TPF_GENERAL_ADDR;
		return (0);
	}
	return ((tpp->tpp_tpproto->tpi_pcbbind)(tpp->tpp_npcb, nam, p));
}

int
tpi_pcbconnect(void *v, struct mbuf *nam, int which, int af)
{
	struct tpipcb *tpp;
	union tpi_sockaddr_union *tsu;

	tpp = v;
	tsu = &tpi_sockaddr;
	if (tpp->tpp_af != af) {
		return (EINVAL);
	}
	switch (af) {
	case AF_INET:
		tsu->tsu_sin4 = mtod(nam, struct sockaddr_in);
		if (nam->m_len != sizeof(tsu->tsu_sin4)) {
			return (EINVAL);
		}
		if (tsu->tsu_sin4.sin_family != AF_INET) {
			return (EAFNOSUPPORT);
		}
		if (tsu->tsu_sin4.sin_port == 0) {
			return (EADDRNOTAVAIL);
		}
		tpi_setusockaddr(tpp, tsu, tsu->tsu_sin4.sin_addr, tsu->tsu_sin4.sin_port, which);
		break;
	case AF_INET6:
		tsu->tsu_sin6 = mtod(nam, struct sockaddr_in6);
		if (nam->m_len != sizeof(tsu->tsu_sin6)) {
			return (EINVAL);
		}
		if (tsu->tsu_sin6.sin6_family != AF_INET6) {
			return (EAFNOSUPPORT);
		}
		if (tsu->tsu_sin6.sin6_port == 0) {
			return (EADDRNOTAVAIL);
		}
		tpi_setusockaddr(tpp, tsu, tsu->tsu_sin6.sin6_addr, tsu->tsu_sin6.sin6_port, which);
		break;
	case AF_ISO:
		tsu->tsu_siso = mtod(nam, struct sockaddr_iso);
		if (nam->m_len != sizeof(tsu->tsu_siso)) {
			return (EINVAL);
		}
		if (tsu->tsu_siso.siso_family != AF_ISO) {
			return (EAFNOSUPPORT);
		}
		if (tsu->tsu_siso.siso_nlen == 0) {
			return (EADDRNOTAVAIL);
		}
		tpi_setusockaddr(tpp, tsu, tsu->tsu_siso.siso_addr, tsu->tsu_siso.siso_nlen, which);
		break;
	case AF_NS:
		tsu->tsu_sns = mtod(nam, struct sockaddr_ns);
		if (nam->m_len != sizeof(tsu->tsu_sns)) {
			return (EINVAL);
		}
		if (tsu->tsu_sns.sns_family != AF_NS) {
			return (EAFNOSUPPORT);
		}
		if (tsu->tsu_sns.sns_port == 0) {
			return (EADDRNOTAVAIL);
		}
		tpi_setusockaddr(tpp, tsu, tsu->tsu_sns.sns_addr, tsu->tsu_sns.sns_port, which);
		break;
	case AF_CCITT:
		tsu->tsu_sx25 = mtod(nam, struct sockaddr_x25);
		if (nam->m_len != sizeof(tsu->tsu_sx25)) {
			return (EINVAL);
		}
		if (tsu->tsu_sx25.x25_family != AF_CCITT) {
			return (EAFNOSUPPORT);
		}
		if (tsu->tsu_sx25.x25_len == 0) {
			return (EADDRNOTAVAIL);
		}
		tpi_setusockaddr(tpp, tsu, tsu->tsu_sx25.x25_addr, tsu->tsu_sx25.x25_len, which);  /* port not correct variable */
		break;
/* Others: Not implemented */
	case AF_APPLETALK:
		break;
	case AF_SNA:
		break;
	case AF_NATM:
		break;
	case AF_IPX:
		break;
	}
	tpi_pcbstate(tpp, TPI_CONNECTED, which, af);
	if (tpp->tpp_socket->so_type == SOCK_STREAM) {
		(tpp->tpp_tpproto->tpi_secconn)(tpp->tpp_npcb);
	}
	return ((tpp->tpp_tpproto->tpi_pcbconn)(tpp->tpp_npcb, nam));
}

void
tpi_pcbdisconnect(void *v, int which, int af)
{
	struct tpipcb *tpp;

	tpp = v;
	if (tpp->tpp_af != af) {
		return;
	}

	switch (which) {
	case TPI_LOCAL:
		tpi_setusockaddr(tpp, &tpi_sockaddr, &tpi_zero_addr, 0, TPI_LOCAL);
		break;
	case TPI_FOREIGN:
		tpi_setusockaddr(tpp, &tpi_sockaddr, &tpi_zero_addr, 0, TPI_FOREIGN);
		break;
	}
	tpi_pcbstate(tpp, TPI_BOUND, which, af);
	// tpproto ipsec pcbdisconn??
	(tpp->tpp_tpproto->tpi_secdisc)(tpp->tpp_npcb);
	if (tpp->tpp_socket->so_state & SS_NOFDREF) {
		tpi_pcbdetach(tpp, which, af);
	}
}

void
tpi_pcbdetach(void *v, int which, int af)
{
	struct tpipcb *tpp;
	struct socket *so;
	int s;

	tpp = v;
	so = tpp->tpp_socket;
	if (tpp->tpp_af != af) {
		return;
	}
	// tpproto detach pcbpolicy??
	(tpp->tpp_tpproto->tpi_poldisc)(tpp->tpp_npcb);
	so->so_pcb = 0;
	sofree(so);
	// tpproto pcbdetach??
	(tpp->tpp_tpproto->tpi_pcbdisc)(tpp->tpp_npcb);
	s = splnet();
	switch (which) {
	case TPI_LOCAL:
		tpi_local_remove(&tpp->tpp_table, tpp->tpp_laddr, tpp->tpp_lport);
		break;
	case TPI_FOREIGN:
		tpi_foreign_remove(&tpp->tpp_table, tpp->tpp_faddr, tpp->tpp_fport);
		break;
	}
	CIRCLEQ_REMOVE(&tpp->tpp_table->tppt_queue, &tpp->tpp_head, tpph_queue);
	splx(s);
	FREE(tpp, M_PCB);
}

void
tpi_setusockaddr(struct tpipcb *tpp, union tpi_sockaddr_union *tsu, void *addr, uint16_t port, int which)
{
	bzero(tsu, sizeof(*tsu));
	switch (which) {
	case TPI_LOCAL:
		struct tpi_local *tpl;

		tpl = &tpp->tpp_local;
		if (tpl != NULL) {
			tpi_local_set_lsockaddr(tpl, tsu, addr, port);
		}
		break;
	case TPI_FOREIGN:
		struct tpi_foreign *tpf;

		tpf = &tpp->tpp_foreign;
		if (tpf != NULL) {
			tpi_foreign_set_fsockaddr(tpf, tsu, addr, port);
		}
		break;
	}
}

union tpi_sockaddr_union *
tpi_getusockaddr(struct tpipcbtable *table, void *addr, uint16_t port, int which)
{
	union tpi_sockaddr_union *tsu;

	switch (which) {
	case TPI_LOCAL:
		struct tpi_local   *tpl;

		tpl = tpi_local_lookup(table, addr, port);
		if (tpl != NULL) {
			tsu = (union tpi_sockaddr_union *)tpi_local_get_lsockaddr(tpl);
		}
		break;
	case TPI_FOREIGN:
		struct tpi_foreign *tpf;

		tpf = tpi_foreign_lookup(table, addr, port);
		if (tpf != NULL) {
			tsu = (union tpi_sockaddr_union *)tpi_foreign_get_fsockaddr(tpf);
		}
		break;
	}
	return (tsu);
}

struct tpipcb *
tpi_pcblookup(struct tpipcbtable *table, void *addr, uint16_t port, int which, int af)
{
	struct tpipcbhead *head;
	struct tpipcb *tpp;
	struct tpipcb_hdr *tpph;
	struct tpi_local *tpl;
	struct tpi_foreign *tpf;

	switch (which) {
	case TPI_LOCAL:
		/* checks lport and laddr */
		head = tpi_local_hash(table, addr, port);
		tpl = tpi_local_lookup(table, addr, port);
		tpph = &tpl->tpl_head;
		break;
	case TPI_FOREIGN:
		/* checks fport and faddr */
		head = tpi_foreign_hash(table, addr, port);
		tpf = tpi_foreign_lookup(table, addr, port);
		tpph = &tpf->tpf_head;
		break;
	}
	if (tpph != NULL) {
		tpp = (struct tpipcb *)tpph;
		/* We don't want to return the wrong protocol information */
		if (tpp->tpp_af != af) {
			return (NULL);
		}
		goto out;
	}
	return (NULL);

out:
	if (tpph != LIST_FIRST(head)) {
		switch (which) {
		case TPI_LOCAL:
			LIST_REMOVE(tpph, tpph_lhash);
			LIST_INSERT_HEAD(head, tpph, tpph_lhash);
			break;
		case TPI_FOREIGN:
			LIST_REMOVE(tpph, tpph_fhash);
			LIST_INSERT_HEAD(head, tpph, tpph_fhash);
			break;
		}
	}
	return (tpp);
}

struct tpipcb *
tpi_pcblookup_connect(struct tpipcbtable *table, void *faddr_arg, u_int16_t fport_arg, void *laddr_arg, u_int16_t lport_arg, int which, int af)
{
	struct tpipcb *tpp;
	void *laddr, *faddr;
	u_int16_t lport, fport;

	faddr = faddr_arg;
	fport = fport_arg;
	laddr = laddr_arg;
	lport = lport_arg;
	switch (which) {
	case TPI_LOCAL:
		tpp = tpi_pcblookup(table, laddr, lport, TPI_LOCAL, af);
		break;
	case TPI_FOREIGN:
		tpp = tpi_pcblookup(table, faddr, fport, TPI_FOREIGN, af);
		break;
	}
	return (tpp);
}

struct tpipcb *
tpi_pcblookup_bind(struct tpipcbtable *table, void *laddr, u_int16_t lport, int af)
{
	return (tpi_pcblookup(table, laddr, lport, TPI_LOCAL, af));
}

void
tpi_pcbstate(struct tpipcb *tpp, int state, int which, int af)
{
	if (tpp->tpp_af != af) {
		return;
	}

	if (tpp->tpp_state > TPI_ATTACHED) {
		switch (state) {
		case TPI_BOUND: {
			switch (which) {
			case TPI_LOCAL:
				tpi_local_remove(&tpp->tpp_table, tpp->tpp_laddr, tpp->tpp_lport);
				break;
			case TPI_FOREIGN:
				tpi_foreign_remove(&tpp->tpp_table, tpp->tpp_faddr, tpp->tpp_fport);
				break;
			}
			break;
		}
		case TPI_CONNECTED:
			tpi_local_remove(&tpp->tpp_table, tpp->tpp_laddr, tpp->tpp_lport);
			tpi_foreign_remove(&tpp->tpp_table, tpp->tpp_faddr, tpp->tpp_fport);
			break;
		}
	}

	switch (state) {
	case TPI_BOUND: {
		switch (which) {
		case TPI_LOCAL:
			tpi_local_insert(&tpp->tpp_table, tpp->tpp_laddr, tpp->tpp_lport);
			break;
		case TPI_FOREIGN:
			tpi_foreign_insert(&tpp->tpp_table, tpp->tpp_faddr, tpp->tpp_fport);
			break;
		}
		break;
	}
	case TPI_CONNECTED:
		tpi_local_insert(&tpp->tpp_table, tpp->tpp_laddr, tpp->tpp_lport);
		tpi_foreign_insert(&tpp->tpp_table, tpp->tpp_faddr, tpp->tpp_fport);
		break;
	}
	tpp->tpp_state = state;
}

int
tpi_pcbisvalid(void *pcb, void *faddr_arg, u_int16_t fport_arg, void *laddr_arg, u_int16_t lport_arg, int which, int af)
{
	struct tpipcb *tpp;
	struct tpipcbtable *table;
	union tpi_sockaddr_union *tsu;
	void *laddr, *faddr;
	u_int16_t lport, fport;
	int error;

	faddr = faddr_arg;
	fport = fport_arg;
	laddr = laddr_arg;
	lport = lport_arg;

	tpp = pcb;
	table = &tpp->tpp_table;
	switch (which) {
	case TPI_LOCAL:
		if (tpp != NULL) {
			if (tpp->tpp_af != af) {
				error = 1;
				break;
			}
			if (laddr == tpp->tpp_laddr && lport == tpp->tpp_lport) {
				tsu = tpi_getusockaddr(table, tpp->tpp_laddr, tpp->tpp_lport, TPI_LOCAL);
			} else {
				tsu = tpi_getusockaddr(table, laddr, lport, TPI_LOCAL);
			}
			if (tsu != NULL) {
				if (tsu == &tpp->tpp_local.tpl_lsockaddr) {
					error = 0;
					break;
				}
			}
		}
		error = 1;
		break;

	case TPI_FOREIGN:
		if (tpp != NULL) {
			if (tpp->tpp_af != af) {
				error = 1;
				break;
			}
			if (faddr == tpp->tpp_faddr && fport == tpp->tpp_fport) {
				tsu = tpi_getusockaddr(table, tpp->tpp_faddr, tpp->tpp_fport, TPI_FOREIGN);
			} else {
				tsu = tpi_getusockaddr(table, faddr, fport, TPI_FOREIGN);
			}
			if (tsu != NULL) {
				if (tsu == &tpp->tpp_foreign.tpf_fsockaddr) {
					error = 0;
					break;
				}
			}

		}
		error = 1;
		break;
	}
	return (error);
}

int
tpi_pcbisvalid_sockaddr(union tpi_sockaddr_union *tsu, void *sockaddr, int af)
{
	int error;

	if (tsu == NULL || sockaddr == NULL) {
		return (1);
	}

	switch (af) {
	case AF_INET:
		struct sockaddr_in *sin4 = &tsu->tsu_sin4;
		if (sockaddr == sin4) {
			error = 0;
		} else {
			error = 1;
		}
		break;
	case AF_INET6:
		struct sockaddr_in6 *sin6 = &tsu->tsu_sin6;
		if (sockaddr == sin6) {
			error = 0;
		} else {
			error = 1;
		}
		break;
	case AF_ISO:
		struct sockaddr_iso *siso = &tsu->tsu_siso;
		if (sockaddr == siso) {
			error = 0;
		} else {
			error = 1;
		}
		break;
	case AF_NS:
		struct sockaddr_ns *sns = &tsu->tsu_sns;
		if (sockaddr == sns) {
			error = 0;
		} else {
			error = 1;
		}
		break;
	case AF_CCITT:
		struct sockaddr_x25 *sx25 = &tsu->tsu_sx25;
		if (sockaddr == sx25) {
			error = 0;
		} else {
			error = 1;
		}
		break;

	/* Others: Not implemented */
	case AF_APPLETALK:
		break;
	case AF_SNA:
		break;
	case AF_NATM:
		break;
	case AF_IPX:
		break;
	}
	return (error);
}

void
tpi_quench(struct tpipcb *tpcb, int cmd)
{
	switch (cmd) {
	case PRC_QUENCH:
		tpcb->tpp_cong_win = tpcb->tpp_l_tpdusize;
		IncStat(ts_quench);
		break;
	case PRC_QUENCH2:
		tpcb->tpp_cong_win = tpcb->tpp_l_tpdusize; /* might as well quench source also */
		tpcb->tpp_decbit = TP_DECBIT_CLEAR_COUNT;
		IncStat(ts_rcvdecbit);
		break;
	}
}

void
tpi_abort(struct tpipcb *tpcb, int cmd)
{
	struct tp_event e;

	e.ev_number = ER_TPDU;
	e.ATTR(ER_TPDU).e_reason = ENETRESET;
	tp_driver(tpcb, &e);
	return;
}
