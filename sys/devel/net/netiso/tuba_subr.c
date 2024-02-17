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

void
tuba_init()
{

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
tuba_tcp_output_checksum(af, m, isop, th)
	int af;
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

	switch (af) {
	case AF_INET:
		m->m_len -= sizeof(struct ip);
		m->m_pkthdr.len -= sizeof(struct ip);
		m->m_data += sizeof(struct ip);
		break;
	case AF_INET6:
		m->m_len -= sizeof(struct ip6_hdr);
		m->m_pkthdr.len -= sizeof(struct ip6_hdr);
		m->m_data += sizeof(struct ip6_hdr);
		break;
	}
	return (clnp_output(m, isop, m->m_pkthdr.len, 0));
}

int
tuba_udp_output_checksum(af, m, isop, uh)
	int af;
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

	switch (af) {
	case AF_INET:
		m->m_len -= sizeof(struct ip);
		m->m_pkthdr.len -= sizeof(struct ip);
		m->m_data += sizeof(struct ip);
		break;
	case AF_INET6:
		m->m_len -= sizeof(struct ip6_hdr);
		m->m_pkthdr.len -= sizeof(struct ip6_hdr);
		m->m_data += sizeof(struct ip6_hdr);
		break;
	}
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
