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

#include <tpi_pcb.h>
#include <tpi_protosw.h>

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
	MALLOC(tpp, struct tpipcb *, sizeof(*tpp), M_PCB, M_WAITOK);
	if (tpp == NULL) {
		return (ENOBUFS);
	}
	bzero((caddr_t)tpp, sizeof(*tpp));
	tpp->tpp_af = af;
	tpp->tpp_table = table;
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
	s = splnet();
	CIRCLEQ_INSERT_HEAD(&table->tppt_queue, &tpp->tpp_head, tpph_queue);
	tpi_local_insert(table, tpp->tpp_laddr, tpp->tpp_lport);
	tpi_foreign_insert(table, tpp->tpp_faddr, tpp->tpp_fport);
	splx(s);
	return (error);
}

struct tpipcb *tp_bound_pcbs, *tp_listeners;

int
tpi_tselinuse(u_short tlen, char *tsel, struct sockaddr_iso *siso, int reuseaddr)
{
	struct tpipcbtable *table;
	struct tpipcb *b, *l, *t;

	b = CIRCLEQ_NEXT(&tp_bound_pcbs->tpp_head, tpph_queue);
	l = CIRCLEQ_FIRST(&tp_listeners->tpp_head);
	CIRCLEQ_FOREACH(&b->tpp_head, &table->tppt_queue, tpph_queue) {
		if (b != tp_bound_pcbs) {
			t = CIRCLEQ_NEXT(&b->tpp_head, tpph_queue);
			CIRCLEQ_INSERT_BEFORE(&table->tppt_queue, &t->tpp_head, &b->tpp_head, tpph_queue);
		} else if (l) {
			t = CIRCLEQ_NEXT(&l->tpp_head, tpph_queue);
			CIRCLEQ_INSERT_AFTER(&table->tppt_queue, &t->tpp_head, &l->tpp_head, tpph_queue);
		} else {
			break;
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
tpi_attach(struct socket *so, void *v, int af, int protocol)
{
	int error;

	if (so->so_pcb != NULL) {
		return (EISCONN);	/* socket already part of a connection*/
	}

	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {

	}

	if ((error = tpi_pcballoc(so, v, af))) {

	}
}

void
tpi_detach(struct tpipcb *tpp)
{

}

int
tpi_pcbbind(void *v, struct mbuf *nam, struct proc *p)
{
	struct sockaddr_iso *siso;
	int error;

	return (error);
}

int
tpi_pcbconnect(void *v, struct mbuf *nam, int which, int af)
{
	struct tpipcb *tpp;
	union tpi_sockaddr_union *tsu;

	tpp = v;
	tsu = &tpi_sockaddr;

	switch (af) {
	case AF_INET:
		tsu->tsu_sin4 = mtod(nam, struct sockaddr_in);
		tpi_setusockaddr(tpp, tsu, tsu->tsu_sin4.sin_addr, tsu->tsu_sin4.sin_port, which);
		break;
	case AF_INET6:
		tsu->tsu_sin6 = mtod(nam, struct sockaddr_in6);
		tpi_setusockaddr(tpp, tsu, tsu->tsu_sin6.sin6_addr, tsu->tsu_sin6.sin6_port, which);
		break;
	case AF_ISO:
		tsu->tsu_siso = mtod(nam, struct sockaddr_iso);
		tpi_setusockaddr(tpp, tsu, tsu->tsu_siso.siso_addr, tsu->tsu_siso.siso_len, which); /* port not correct variable */
		break;
	case AF_NS:
		tsu->tsu_sns = mtod(nam, struct sockaddr_ns);
		tpi_setusockaddr(tpp, tsu, tsu->tsu_sns.sns_addr, tsu->tsu_sns.sns_port, which);
		break;
	case AF_CCITT:
		tsu->tsu_sx25 = mtod(nam, struct sockaddr_x25);
		tpi_setusockaddr(tpp, tsu, tsu->tsu_sx25.x25_addr, tsu->tsu_sx25.x25_len, which);  /* port not correct variable */
		break;
/* Others: Not currently defined */
/*
	case AF_APPLETALK:
		break;
	case AF_SNA:
		break;
	case AF_NATM:
		break;
	case AF_IPX:
		break;
*/
	}
	return (0);
}

void
tpi_pcbdisconnect(struct tpipcb *tpp, int which, int af)
{
	if (tpp->tpp_af != af) {
		return;
	}

	switch (which) {
	case TPI_LOCAL:
		//tpp->tpp_laddr = tpi_zero_addr;
		//tpp->tpp_lport = 0;
		tpi_setusockaddr(tpp, &tpi_sockaddr, &tpi_zero_addr, 0, TPI_LOCAL);
		break;
	case TPI_FOREIGN:
		//tpp->tpp_faddr = tpi_zero_addr;
		//tpp->tpp_fport = 0;
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
tpi_pcbdetach(struct tpipcb *tpp, int which, int af)
{
	struct socket *so;
	int s;

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

void
tpi_setpeeraddr(struct mbuf *nam)
{

}

void
tpi_setsockaddr(void *pcb, struct mbuf *nam)
{

}

int
tpi_pcbnotify()
{
	struct tpipcbhead *head;

}

void
tpi_pcbnotifyall()
{

}

void
tpi_pcbpurgeif()
{

}

void
tpi_pcbpurgeif0()
{

}

void
tpi_losing(struct tpipcb *tpp, int af)
{
	struct rtentry *rt;
	struct rt_addrinfo info;

	if (tpp->tpp_af != af) {
		return;
	}

	if ((rt = tpp->tpp_route.ro_rt)) {
		tpp->tpp_route.ro_rt = 0;
		bzero((caddr_t)&info, sizeof(info));
		info.rti_info[RTAX_DST] = &tpp->tpp_route.ro_dst;
		info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
		info.rti_info[RTAX_NETMASK] = rt_mask(rt);
		rt_missmsg(RTM_LOSING, &info, rt->rt_flags, 0);
		if (rt->rt_flags & RTF_DYNAMIC) {
			(void) rtrequest(RTM_DELETE, rt_key(rt),
				rt->rt_gateway, rt_mask(rt), rt->rt_flags,
				(struct rtentry **)0);
		} else {
			/*
			 * A new route can be allocated
			 * the next time output is attempted.
			 */
			rtfree(rt);
		}
	}
}

void
tpi_rtchange(struct tpipcb *tpp, int af)
{
	if (tpp->tpp_af != af) {
		return;
	}

	if (tpp->tpp_route.ro_rt) {
		rtfree(tpp->tpp_route.ro_rt);
		tpp->tpp_route.ro_rt = 0;
	}
}

struct tpipcb *
tpi_pcblookup(struct tpipcbtable *table, void *addr, uint16_t port, int which, int af)
{
	struct tpipcb *tpp;
	struct tpipcb_hdr *tpph;
	struct tpi_local *tpl;
	struct tpi_foreign *tpf;

	switch (which) {
	case TPI_LOCAL:
		/* checks lport and laddr */
		tpl = tpi_local_lookup(table, addr, port);
		tpph = &tpl->tpl_head;
		break;
	case TPI_FOREIGN:
		/* checks fport and faddr */
		tpf = tpi_local_lookup(table, addr, port);
		tpph = &tpf->tpf_head;
		break;
	}
	if (tpph != NULL) {
		tpp = (struct tpipcb *)tpph;
		/* We don't want to return the wrong protocol information */
		if (tpp->tpp_af != af) {
			return (NULL);
		}
	}
	return (tpp);
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
		case TPI_CONNECTED:
			tpi_local_remove(&tpp->tpp_table, tpp->tpp_laddr, tpp->tpp_lport);
			tpi_foreign_remove(&tpp->tpp_table, tpp->tpp_faddr, tpp->tpp_fport);
			break;
		}
	}

	switch (state) {
	case TPI_BOUND:
		switch (which) {
		case TPI_LOCAL:
			tpi_local_insert(&tpp->tpp_table, tpp->tpp_laddr, tpp->tpp_lport);
			break;
		case TPI_FOREIGN:
			tpi_foreign_insert(&tpp->tpp_table, tpp->tpp_faddr, tpp->tpp_fport);
			break;
		}
		break;
	case TPI_CONNECTED:
		tpi_local_insert(&tpp->tpp_table, tpp->tpp_laddr, tpp->tpp_lport);
		tpi_foreign_insert(&tpp->tpp_table, tpp->tpp_faddr, tpp->tpp_fport);
		break;
	}
	tpp->tpp_state = state;
}

struct rtentry *
tpi_pcbrtentry()
{

}
