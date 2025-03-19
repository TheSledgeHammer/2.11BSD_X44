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
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_debug.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/clnp.h>
#include <netiso/iso_pcb.h>
#include <netiso/iso_var.h>
#include <netiso/tuba_table.h>

struct inpcbtable	tubapcbtable;
struct inpcb		tuba_inpcb;
struct isopcb		tuba_isopcb;

#define	TUBAHASHSIZE	128
int	tubahashsize = TUBAHASHSIZE;

/*
 * Tuba Header Size
 */
#define LLC 			3
#define CLNP_FIXED		9
#define ADDRESSES		42
#define CLNP_SEGMENT	6
#define TCP				20
#define TUBAHDRSIZE 	((LLC)+(CLNP_FIXED)+(ADDRESSES)+(CLNP_SEGMENT)+(TCP))

/*
 * Tuba initialization
 */
void
tuba_init(void)
{
	in_pcbinit(&tubapcbtable, tubahashsize, tubahashsize);
	tuba_isopcb.isop_next = tuba_isopcb.isop_prev = &tuba_isopcb;
	tuba_isopcb.isop_faddr = &tuba_isopcb.isop_sfaddr;
	tuba_isopcb.isop_laddr = &tuba_isopcb.isop_sladdr;

	if (max_protohdr < TUBAHDRSIZE) {
		max_protohdr = TUBAHDRSIZE;
	}
	if ((max_linkhdr + TUBAHDRSIZE) > MHLEN) {
		panic("tuba_init");
	}
}

int
tuba_cksum(sum, offset, siso, index)
	u_long *sum;
	int *offset;
	struct sockaddr_iso **siso;
	u_long index;
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

int
tuba_tcp_output(m, isop, th)
	struct mbuf *m;
	struct isopcb *isop;
	struct tcpiphdr *th;
{
	int error, offset, thlen;
	u_long sum;

	if (th == NULL || isop == NULL) {
		isop = &tuba_isopcb;
		th = mtod(m, struct tcpiphdr *);
		sum = 0;
		offset = 0;
		error = tuba_cksum(&sum, &offset, &isop->isop_faddr, th->ti_dst.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		error = tuba_cksum(&sum, &offset, &isop->isop_laddr, th->ti_src.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		REDUCE(sum, sum);
	}
	if (th->ti_sum == 0) {
		isop = NULL;
		sum = 0;
		offset = 0;
		error = tuba_cksum(&sum, &offset, NULL, th->ti_dst.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		error = tuba_cksum(&sum, &offset, NULL, th->ti_src.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		REDUCE(sum, sum);
		th->ti_sum = sum;
		th = mtod(m, struct tcpiphdr *);
	}
	REDUCE(th->ti_sum, (th->ti_sum + (0xffff ^ sum)));

#ifdef INET
	m->m_len -= sizeof(struct ip);
	m->m_pkthdr.len -= sizeof(struct ip);
	m->m_data += sizeof(struct ip);
#endif
#ifdef INET6
	m->m_len -= sizeof(struct ip6_hdr);
	m->m_pkthdr.len -= sizeof(struct ip6_hdr);
	m->m_data += sizeof(struct ip6_hdr);
#endif
	return (clnp_output(m, isop, m->m_pkthdr.len, 0));
}

int
tuba_udp_output(m, isop, uh)
	struct mbuf *m;
	struct isopcb *isop;
	struct udpiphdr *uh;
{
	int error, offset, thlen;
	u_long sum;

	if (th == NULL || isop == NULL) {
		isop = &tuba_isopcb;
		th = mtod(m, struct udpiphdr *);
		sum = 0;
		offset = 0;
		error = tuba_cksum(&sum, &offset, &isop->isop_faddr, uh->ui_dst.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		error = tuba_cksum(&sum, &offset, &isop->isop_laddr, uh->ui_src.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		REDUCE(sum, sum);
	}
	if (uh->ui_sum == 0) {
		isop = NULL;
		sum = 0;
		offset = 0;
		error = tuba_cksum(&sum, &offset, NULL, uh->ui_dst.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		error = tuba_cksum(&sum, &offset, NULL, uh->ui_src.s_addr);
		if (error) {
			m_freem(m);
			return (EADDRNOTAVAIL);
		}
		REDUCE(sum, sum);
		uh->ui_sum = sum;
		uh = mtod(m, struct udpiphdr *);
	}
	REDUCE(uh->ui_sum, (uh->ui_sum + (0xffff ^ sum)));

#ifdef INET
	m->m_len -= sizeof(struct ip);
	m->m_pkthdr.len -= sizeof(struct ip);
	m->m_data += sizeof(struct ip);
#endif
#ifdef INET6
	m->m_len -= sizeof(struct ip6_hdr);
	m->m_pkthdr.len -= sizeof(struct ip6_hdr);
	m->m_data += sizeof(struct ip6_hdr);
#endif
	return (clnp_output(m, isop, m->m_pkthdr.len, 0));
}

void
tuba_refcnt(isop, delta)
	struct isopcb *isop;
	int delta;
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

void
tuba_pcbdetach(isop)
	struct isopcb *isop;
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
int
tuba_pcbconnect(inp, nam)
	register struct inpcb *inp;
	struct mbuf *nam;
{
	register struct sockaddr_iso *siso;
	struct sockaddr_in *sin;
	struct isopcb *isop;
	struct tcpcb *tp;
	int error;

	sin = mtod(nam, struct sockaddr_in*);
	tp = intotcpcb(inp);
	isop = (struct isopcb *) tp->t_tuba_pcb;
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
	bcopy((caddr_t) &inp->inp_fport, TSEL(siso), sizeof(inp->inp_fport));
	error = iso_pcbconnect(isop, nam);
	if (error == 0) {
		tuba_refcnt(isop, 1);
	}
	return (error);
}

void
tuba_tcp_input(m, src, dst)
	register struct mbuf *m;
	struct sockaddr_iso *src, *dst;
{
	unsigned long sum, lindex, findex;
	register struct tcpiphdr *ti;
	register struct inpcb *inp;
	caddr_t optp = NULL;
	int optlen;
	int len, tlen, off;
	register struct tcpcb *tp = 0;
	int tiflags;
	struct socket *so;
	int todrop, acked, ourfinisacked, needoutput = 0;
	short ostate;
	struct in_addr laddr;
	int dropsocket = 0, iss = 0;
	u_long tiwin, ts_val, ts_ecr;
	int ts_present = 0;

	/*
	 * Do some housekeeping looking up CLNP addresses.
	 * If we are out of space might as well drop the packet now.
	 */
	tcpstat.tcps_rcvtotal++;
	lindex = tuba_lookup(dst, M_DONTWAIT);
	findex = tuba_lookup(src, M_DONTWAIT);
	if (lindex == 0 || findex == 0) {
		goto drop;
	}

	/*
	 * CLNP gave us an mbuf chain WITH the clnp header pulled up,
	 * but the data pointer pushed past it.
	 */
	len = m->m_len;
	tlen = m->m_pkthdr.len;
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
			goto drop;
		}
		m->m_next = m0;
		m->m_data += max_linkhdr;
		m->m_pkthdr = m0->m_pkthdr;
		m->m_flags = m0->m_flags & M_COPYFLAGS;
		if (len < sizeof(struct tcphdr)) {
			m->m_len = 0;
			if ((m = m_pullup(m, sizeof(struct tcpiphdr))) == 0) {
				tcpstat.tcps_rcvshort++;
				return;
			}
		} else {
			bcopy(mtod(m0, caddr_t) + sizeof(struct ip),
			      mtod(m, caddr_t) + sizeof(struct ip),
			      sizeof(struct tcphdr));
			m0->m_len -= sizeof(struct tcpiphdr);
			m0->m_data += sizeof(struct tcpiphdr);
			m->m_len = sizeof(struct tcpiphdr);
		}
	}
	/*
	 * Calculate checksum of extended TCP header and data,
	 * replacing what would have been IP addresses by
	 * the IP checksum of the CLNP addresses.
	 */
	ti = mtod(m, struct tcpiphdr *);
	ti->ti_dst.s_addr = tuba_table[lindex]->tc_sum;
	if (dst->siso_nlen & 1)
		ti->ti_src.s_addr = tuba_table[findex]->tc_sum;
	else
		ti->ti_src.s_addr = tuba_table[findex]->tc_ssum;
	//ti->ti_prev = ti->ti_next = 0;
	ti->ti_x1 = 0;
	ti->ti_pr = ISOPROTO_TCP;
	ti->ti_len = htons((u_short)tlen);
	if (ti->ti_sum == in_cksum(m, m->m_pkthdr.len)) {
		tcpstat.tcps_rcvbadsum++;
		goto drop;
	}
	ti->ti_src.s_addr = findex;
	ti->ti_dst.s_addr = lindex;

#ifdef notyet
#define TUBA_INCLUDE
#define	in_pcbconnect	tuba_pcbconnect
#define	tcb				tuba_inpcb

#include <netinet/tcp_input.c>
}

#define tcp_slowtimo	tuba_slowtimo
#define tcp_fasttimo	tuba_fasttimo

#include <netinet/tcp_timer.c>
#endif

void
tuba_udp_input(m, src, dst)
	register struct mbuf *m;
	struct sockaddr_iso *src, *dst;
{

}
