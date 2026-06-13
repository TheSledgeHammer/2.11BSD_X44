/*
 * Copyright (c) 1992, 1993
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
 *	@(#)tuba_subr.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>

#include "opt_inet.h"
#include "opt_iso.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/in6_var.h>
#endif

#ifndef INET6
/* always need ip6.h for IP6_EXTHDR_GET */
#include <netinet/ip6.h>
#endif

#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>

#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/clnp.h>
#include <netiso/iso_pcb.h>
#include <netiso/iso_var.h>
#include <netiso/tuba_table.h>

#define	TUBAHASHSIZE	128
int	tubahashsize = TUBAHASHSIZE;

struct inpcbtable	tubainptable; /* in pcbtable */
struct isopcbtable  tubaisoptable; /* iso pcbtable */

struct sockaddr_iso tuba_addrs[2] = {
		{
				.siso_len = sizeof(tuba_addrs),
				.siso_family = AF_ISO,
		},
		{
				.siso_len = sizeof(tuba_addrs[1]),
				.siso_family = AF_ISO,
		},
};

/*
 * Tuba Header Size
 */
#define LLC 			3
#define CLNP_FIXED		9
#define ADDRESSES		42
#define CLNP_SEGMENT	6
#define TCP				20
#define TUBAHDRSIZE 	((LLC)+(CLNP_FIXED)+(ADDRESSES)+(CLNP_SEGMENT)+(TCP))

#ifdef INET
static void tuba_ip4_cksum(struct mbuf *, struct sockaddr_iso *, struct sockaddr_iso *, struct ip *, struct tcphdr *, int, u_long, u_long);
#endif

#ifdef INET6
static void tuba_ip6_cksum(struct mbuf *, struct sockaddr_iso *, struct sockaddr_iso *, struct ip6_hdr *, struct tcphdr *, int, u_long, u_long);
#endif

/*
 * Tuba initialization
 */
void
tuba_init(void)
{
#ifdef RADIX_ART
	rtable_art_init(AF_ISO, sizeof(struct iso_addr));
#endif
	/* INET */
	in_pcbinit(&tubainptable, tubahashsize, tubahashsize);
	/* ISO */
	iso_pcbinit(&tubaisoptable, tubahashsize);
	/* now preallocate sladdr and sfaddr */
	iso_pcbprealloc(&tubaisoptable, &tuba_addrs[1]);
	if (max_protohdr < TUBAHDRSIZE) {
		max_protohdr = TUBAHDRSIZE;
	}
	if ((max_linkhdr + TUBAHDRSIZE) > MHLEN) {
		panic("tuba_init");
	}
}

int
tuba_cksum(u_long *sum, int *offset, struct sockaddr_iso **siso, u_long index)
{
	register struct tuba_cache *tc;
	int error;

	error = 0;
	if (index <= tuba_table_size && (tc = tuba_table[index])) {
		if (siso) {
			*siso = &tc->tc_siso;
		}
		*sum += ((*offset & 1 ? tc->tc_ssum : tc->tc_sum) + (0xffff ^ index));
		*offset += (tc->tc_siso.siso_nlen + 1);
	} else {
		error = 1;
	}
	return (error);
}

void
tuba_refcnt(struct isopcb *isop, int delta)
{
	register struct tuba_cache *tc;
	u_int index, sum;

	if (delta != 1) {
		delta = -1;
	}
	if (isop == 0 || isop->isop_faddr == 0 || isop->isop_laddr == 0
			|| (delta == -1 && isop->isop_tuba_cached == 0)
			|| (delta == 1 && isop->isop_tuba_cached != 0)) {
		return;
	}
	isop->isop_tuba_cached = (delta == 1);
	index = tuba_lookup(tuba_tree, isop->isop_faddr);
	tc = tuba_table[index];
	if ((index != 0) && (tc != NULL) && (delta == 1 || tc->tc_refcnt > 0)) {
		tc->tc_refcnt += delta;
	}
	index = tuba_lookup(tuba_tree, isop->isop_laddr);
	tc = tuba_table[index];
	if ((index != 0) && (tc != NULL) && (delta == 1 || tc->tc_refcnt > 0)) {
		tc->tc_refcnt += delta;
	}
}

int
tuba_pcbattach(struct socket *so, int family)
{
	struct tcpcb *tp;
	struct isopcb *isop;
#ifdef INET
	struct inpcb *inp;
#endif
#ifdef INET6
	struct in6pcb *in6p;
#endif
	int error;

	error = iso_pcballoc(so, &tubaisoptable);
	if (error != 0) {
		return (error);
	}
	isop = (struct isopcb *)so->so_pcb;
	so->so_pcb = 0;
	error = tcp_attach(so);
	if (error != 0) {
		isop->isop_socket = 0;
		iso_pcbdetach(isop);
	} else {
		switch (family) {
#ifdef INET
		case PF_INET:
			if (inp != 0) {
				return (EISCONN);
			}
			error = in_pcballoc(so, &tubainptable);
			if (error != 0) {
				return (error);
			}
			inp = sotoinpcb(so);
			tp = intotcpcb(inp);
			if (tp == 0) {
				in_pcbdetach(inp);
				return (ENOBUFS);
			}
			break;
#endif
#ifdef INET6
		case PF_INET6:
			if (inp != 0 || in6p != 0) {
				return (EISCONN);
			}
			error = in6_pcballoc(so, &tubainptable);
			if (error != 0) {
				return (error);
			}
			in6p = sotoin6pcb(so);
			tp = in6totcpcb(in6p);
			if (tp == 0) {
				in6_pcbdetach(in6p);
				return (ENOBUFS);
			}
			break;
#endif
		default:
			return (EAFNOSUPPORT);
		}
		tp->t_tuba_pcb = (caddr_t)isop;
	}
	return (0);
}

void
tuba_pcbdetach(struct isopcb *isop)
{
	if (isop == 0) {
		return;
	}
	tuba_refcnt(isop, -1);
	isop->isop_socket = 0;
	iso_pcbdetach(isop);
}

/*
 * Avoid  in_pcbconnect in faked out tcp_input()
 */
#ifdef INET
int
tuba4_pcbconnect(void *v, struct mbuf *nam)
{
	struct tcpcb *tp;
	struct inpcb *inp;
	struct sockaddr_iso *siso;
	struct sockaddr_in *sin;
	struct isopcb *isop;
	int error;

	inp = (struct inpcb *)v;
	sin = mtod(nam, struct sockaddr_in *);
	tp = intotcpcb(inp);
	isop = (struct isopcb *)tp->t_tuba_pcb;
	/* hardwire iso_pcbbind() here */
	siso = isop->isop_laddr = &isop->isop_sladdr;
	*siso = tuba_table[inp->inp_laddr.s_addr]->tc_siso;
	siso->siso_tlen = sizeof(inp->inp_lport);
	bcopy((caddr_t) &inp->inp_lport, TSEL(siso), sizeof(inp->inp_lport));

	/* hardwire in_pcbconnect() here without assigning route */
	inp->inp_fport = sin->sin_port;
	inp->inp_faddr = sin->sin_addr;

	/* reuse nam argument to call iso_pcbconnect() */
	nam->m_len = sizeof(*siso);
	siso = mtod(nam, struct sockaddr_iso *);
	*siso = tuba_table[inp->inp_faddr.s_addr]->tc_siso;
	siso->siso_tlen = sizeof(inp->inp_fport);
	bcopy((caddr_t)&inp->inp_fport, TSEL(siso), sizeof(inp->inp_fport));
	error = iso_pcbconnect(isop, nam);
	if (error == 0) {
		tuba_refcnt(isop, 1);
	}
	return (error);
}

void
tuba4_mbuf(struct mbuf *m, struct sockaddr_iso *src, struct sockaddr_iso *dst, int len, int off, size_t size)
{
	m->m_data -= sizeof(struct ip);
	m->m_len += sizeof(struct ip);
	m->m_pkthdr.len += sizeof(struct ip);
	m->m_flags &= ~(M_MCAST|M_BCAST); /* XXX should do this in clnp_input */

	/*
	 * The reassembly code assumes it will be overwriting a useless
	 * part of the packet, which is why we need to have it point
	 * into the packet itself.
	 *
	 * Check to see if the data is properly alligned
	 * so that we can save copying the tcp header.
	 * This code knows way too much about the structure of mbufs!
	 */
	off = ((sizeof (long) - 1) & ((m->m_flags & M_EXT) ?
		(m->m_data - m->m_ext.ext_buf) :  (m->m_data - m->m_pktdat)));
	if (off || len < sizeof(struct tcphdr)) {
		struct mbuf *m0 = m;
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			m = m0;
			m_freem(m);
			return;
		}
		m->m_next = m0;
		m->m_data += max_linkhdr;
		m->m_pkthdr = m0->m_pkthdr;
		m->m_flags = m0->m_flags & M_COPYFLAGS;
		if (len < sizeof(struct tcphdr)) {
			m->m_len = 0;
			m = m_pullup(m, size);
			if (m == 0) {
				tcpstat.tcps_rcvshort++;
				return;
			}
		} else {
			bcopy(mtod(m0, caddr_t) + sizeof(struct ip),
					mtod(m, caddr_t) + sizeof(struct ip),
					sizeof(struct tcphdr));
			m0->m_len -= size;
			m0->m_data += size;
			m->m_len = size;
		}
	}
}

static void
tuba_ip4_cksum(struct mbuf *m, struct sockaddr_iso *src, struct sockaddr_iso *dst, struct ip *ip, struct tcphdr *th, int tlen, u_long lindex, u_long findex)
{
	ip->ip_dst.s_addr = tuba_table[lindex]->tc_sum;
	if (dst->siso_nlen & 1) {
		ip->ip_src.s_addr = tuba_table[findex]->tc_sum;
	} else {
		ip->ip_src.s_addr = tuba_table[findex]->tc_ssum;
	}
	ip->ip_p = ISOPROTO_TCP;
	ip->ip_len = htons((u_short)tlen);
	if (th->th_sum == in4_cksum(m, ISOPROTO_TCP, sizeof(struct ip), m->m_pkthdr.len)) {
		tcpstat.tcps_rcvbadsum++;
		m_freem(m);
		return;
	}
	ip->ip_src.s_addr = findex;
	ip->ip_dst.s_addr = lindex;
}

int
tuba4_tcp_output(struct mbuf *m, struct tcpcb *tp)
{
	struct ip *ip;
	struct tcphdr *th;
	struct isopcb *isop;
	int error, offset;
	u_long sum;

	isop = (struct isopcb *)tp->t_tuba_pcb;
	th = (struct tcphdr *)tp->t_template;
	ip = mtod(m, struct ip *);
	if (tp == 0 || th == 0 || isop == 0) {
		error = iso_pcballoc(NULL, &tubaisoptable);
        if (error != 0) {
            return (error);
        }
        isop = iso_pcblookup(&tubaisoptable, isop->isop_faddr, (caddr_t)0, 0, isop->isop_laddr);
		th = mtod(m, struct tcphdr *);
		sum = 0;
		offset = 0;
		error = tuba_cksum(&sum, &offset, &isop->isop_faddr, ip->ip_dst.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		error = tuba_cksum(&sum, &offset, &isop->isop_laddr, ip->ip_src.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		REDUCE(sum, sum);
	}
	if (th->th_sum == 0) {
		isop = NULL;
		sum = 0;
		offset = 0;
		error = tuba_cksum(&sum, &offset, NULL, ip->ip_dst.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		error = tuba_cksum(&sum, &offset, NULL, ip->ip_src.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		REDUCE(sum, sum);
		th->th_sum = sum;
		th = mtod(m, struct tcphdr *);
		REDUCE(th->th_sum, (th->th_sum + (0xffff ^ sum)));
	}

	m->m_len -= sizeof(struct ip);
	m->m_pkthdr.len -= sizeof(struct ip);
	m->m_data += sizeof(struct ip);
	return (clnp_output(m, isop, m->m_pkthdr.len, 0));
}

void
tuba4_tcp_input(struct mbuf *m, struct sockaddr_iso *src, struct sockaddr_iso *dst, struct ip *ip, struct tcphdr *th, int toff, int tlen, u_long lindex, u_long findex)
{
	ip = mtod(m, struct ip *);
	IP6_EXTHDR_GET(th, struct tcphdr *, m, toff, sizeof(struct tcphdr));
	if (th == NULL) {
		tcpstat.tcps_rcvshort++;
		return;
	}
	tuba_ip4_cksum(m, src, dst, ip, th, tlen, lindex, findex);
}

#ifdef notyet
int
tuba4_udp_output(struct mbuf *m, )
{

}

void
tuba4_udp_input(struct mbuf *m, struct ip *ip, struct udphdr *uh)
{
	ip = mtod(m, struct ip *);
	IP6_EXTHDR_GET(uh, struct udphdr *, m, toff, sizeof(struct udphdr));
	if (uh == NULL) {
		udpstat.udps_hdrops++;
		return;
	}
}
#endif
#endif

#ifdef INET6
int
tuba6_pcbconnect(void *v, struct mbuf *nam)
{
	struct tcpcb *tp;
	struct in6pcb *in6p;
	struct sockaddr_iso *siso;
	struct sockaddr_in6 *sin6;
	struct isopcb *isop;
	int error;

	in6p = (struct in6pcb *)v;
	sin6 = mtod(nam, struct sockaddr_in6 *);
	tp = in6totcpcb(in6p);
	isop = (struct isopcb *)tp->t_tuba_pcb;
	/* hardwire iso_pcbbind() here */
	siso = isop->isop_laddr = &isop->isop_sladdr;
	*siso = tuba_table[in6p->in6p_laddr.s6_addr32[3]]->tc_siso;
	siso->siso_tlen = sizeof(in6p->in6p_lport);
	bcopy((caddr_t)&in6p->in6p_lport, TSEL(siso), sizeof(in6p->in6p_lport));

	/* hardwire in_pcbconnect() here without assigning route */
	in6p->in6p_fport = sin6->sin6_port;
	in6p->in6p_faddr = sin6->sin6_addr;

	/* reuse nam argument to call iso_pcbconnect() */
	nam->m_len = sizeof(*siso);
	siso = mtod(nam, struct sockaddr_iso *);
	*siso = tuba_table[in6p->in6p_faddr.s6_addr32[3]]->tc_siso;
	siso->siso_tlen = sizeof(in6p->in6p_fport);
	bcopy((caddr_t)&in6p->in6p_fport, TSEL(siso), sizeof(in6p->in6p_fport));
	error = iso_pcbconnect(isop, nam);
	if (error == 0) {
		tuba_refcnt(isop, 1);
	}
	return (error);
}

void
tuba6_mbuf(struct mbuf *m, struct sockaddr_iso *src, struct sockaddr_iso *dst, int len, int off, size_t size)
{
	m->m_data -= sizeof(struct ip6_hdr);
	m->m_len += sizeof(struct ip6_hdr);
	m->m_pkthdr.len += sizeof(struct ip6_hdr);
	m->m_flags &= ~(M_MCAST|M_BCAST|M_ANYCAST6); /* XXX should do this in clnp_input */

	/*
	 * The reassembly code assumes it will be overwriting a useless
	 * part of the packet, which is why we need to have it point
	 * into the packet itself.
	 *
	 * Check to see if the data is properly alligned
	 * so that we can save copying the tcp header.
	 * This code knows way too much about the structure of mbufs!
	 */
	off = ((sizeof(long) - 1) & ((m->m_flags & M_EXT) ?
		(m->m_data - m->m_ext.ext_buf) :  (m->m_data - m->m_pktdat)));
	if (off || len < sizeof(struct tcphdr)) {
		struct mbuf *m0 = m;
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			m = m0;
			m_freem(m);
			return;
		}
		m->m_next = m0;
		m->m_data += max_linkhdr;
		m->m_pkthdr = m0->m_pkthdr;
		m->m_flags = m0->m_flags & M_COPYFLAGS;
		if (len < sizeof(struct tcphdr)) {
			m->m_len = 0;
			m = m_pullup(m, size);
			if (m == 0) {
				tcpstat.tcps_rcvshort++;
				return;
			}
		} else {
			bcopy(mtod(m0, caddr_t) + sizeof(struct ip6_hdr),
					mtod(m, caddr_t) + sizeof(struct ip6_hdr),
					sizeof(struct tcphdr));
			m0->m_len -= size;
			m0->m_data += size;
			m->m_len = size;
		}
	}
}

static void
tuba_ip6_cksum(struct mbuf *m, struct sockaddr_iso *src, struct sockaddr_iso *dst, struct ip6_hdr *ip6, struct tcphdr *th, int tlen, u_long lindex, u_long findex)
{
	ip6->ip6_dst.s6_addr32[3] = tuba_table[lindex]->tc_sum;
	if (dst->siso_nlen & 1) {
		ip6->ip6_src.s6_addr32[3] = tuba_table[findex]->tc_sum;
	} else {
		ip6->ip6_src.s6_addr32[3] = tuba_table[findex]->tc_ssum;
	}
	ip6->ip6_nxt = ISOPROTO_TCP;
	ip6->ip6_plen = htons((u_short)tlen);
	if (th->th_sum == in6_cksum(m, ISOPROTO_TCP, sizeof(struct ip6_hdr), m->m_pkthdr.len)) {
		tcpstat.tcps_rcvbadsum++;
		m_freem(m);
		return;
	}
	ip6->ip6_src.s6_addr32[3] = findex;
	ip6->ip6_dst.s6_addr32[3] = lindex;
}

int
tuba6_tcp_output(struct mbuf *m, struct tcpcb *tp)
{
	struct ip6_hdr *ip6;
	struct tcphdr *th;
	struct isopcb *isop;
	int error, offset;
	u_long sum;

	isop = (struct isopcb *)tp->t_tuba_pcb;
	th = (struct tcphdr *)tp->t_template;
	ip6 = mtod(m, struct ip6_hdr *);
	if (tp == 0 || th == 0 || isop == 0) {
        error = iso_pcballoc(NULL, &tubaisoptable);
        if (error != 0) {
            return (error);
        }
        isop = iso_pcblookup(&tubaisoptable, isop->isop_faddr, (caddr_t)0, 0, isop->isop_laddr);
		th = mtod(m, struct tcphdr *);
		sum = 0;
		offset = 0;
		error = tuba_cksum(&sum, &offset, &isop->isop_faddr, ip6->ip6_dst.s6_addr32[3]);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		error = tuba_cksum(&sum, &offset, &isop->isop_laddr, ip6->ip6_src.s6_addr32[3]);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		REDUCE(sum, sum);
	}
	if (th->th_sum == 0) {
		isop = NULL;
		sum = 0;
		offset = 0;
		error = tuba_cksum(&sum, &offset, NULL, ip6->ip6_dst.s6_addr32[3]);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		error = tuba_cksum(&sum, &offset, NULL, ip6->ip6_src.s6_addr32[3]);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		REDUCE(sum, sum);
		th->th_sum = sum;
		th = mtod(m, struct tcphdr *);
		REDUCE(th->th_sum, (th->th_sum + (0xffff ^ sum)));
	}

	m->m_len -= sizeof(struct ip6_hdr);
	m->m_pkthdr.len -= sizeof(struct ip6_hdr);
	m->m_data += sizeof(struct ip6_hdr);
	return (clnp_output(m, isop, m->m_pkthdr.len, 0));
}

void
tuba6_tcp_input(struct mbuf *m, struct sockaddr_iso *src, struct sockaddr_iso *dst, struct ip6_hdr *ip6, struct tcphdr *th, int toff, int tlen, u_long lindex, u_long findex)
{
	ip6 = mtod(m, struct ip6_hdr *);
	IP6_EXTHDR_GET(th, struct tcphdr *, m, toff, sizeof(struct tcphdr));
	if (th == NULL) {
		tcpstat.tcps_rcvshort++;
		return;
	}
	tuba_ip6_cksum(m, src, dst, ip6, th, tlen, lindex, findex);
}

#ifdef notyet
int
tuba6_udp_output(struct mbuf *m, )
{

}

void
tuba6_udp_input(struct mbuf *m, struct ip6_hdr *ip6, struct udphdr *uh)
{
	ip6 = mtod(m, struct ip6_hdr *);
	IP6_EXTHDR_GET(uh, struct udphdr *, m, toff, sizeof(struct udphdr));
	if (uh == NULL) {
		udpstat.udps_hdrops++;
		return;
	}
}
#endif
#endif
