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

#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/clnp.h>
#include <netiso/iso_pcb.h>
#include <netiso/iso_var.h>
#include <netiso/tuba_table.h>

void
tuba_tcp_input(struct mbuf *m, ...)
{
	unsigned long sum, lindex, findex;
	struct tcphdr *th;
	struct ip *ip;
	struct inpcb *inp;
#ifdef INET6
	struct ip6_hdr *ip6;
	struct in6pcb *in6p;
#endif
	u_int8_t *optp = NULL;
	int optlen = 0;
	int len, tlen, toff, hdroptlen = 0;
	struct tcpcb *tp = 0;
	int tiflags;
	struct socket *so = NULL;
	int todrop, acked, ourfinisacked, needoutput = 0;
#ifdef TCP_DEBUG
	short ostate = 0;
#endif
	int iss = 0;
	u_long tiwin;
	struct tcp_opt_info opti;
	int off, iphlen;
	va_list ap;
	int af;		/* af on the wire */
	struct sockaddr_iso *src, *dst;
	struct mbuf *tcp_saveti = NULL;

	va_start(ap, m);
	src = va_arg(ap, struct sockaddr_iso *);
	dst = va_arg(ap, struct sockaddr_iso *);
	toff = va_arg(ap, int);
	(void)va_arg(ap, int);		/* ignore value, advance ap */
	va_end(ap);

	/*
	 * Do some housekeeping looking up CLNP addresses.
	 * If we are out of space might as well drop the packet now.
	 */
	tcpstat.tcps_rcvtotal++;

	bzero(&opti, sizeof(opti));
	opti.ts_present = 0;
	opti.maxseg = 0;

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
#ifdef INET
	iphlen = (sizeof(struct tcphdr) + sizeof(struct ip));
	tuba4_mbuf(m, src, dst, len, off, iphlen);
#endif
#ifdef INET6
	iphlen = (sizeof(struct tcphdr) + sizeof(struct ip6_hdr));
	tuba6_mbuf(m, src, dst, len, off, iphlen);
#endif

	/*
	 * Get IP and TCP header together in first mbuf.
	 * Note: IP leaves IP header in first mbuf.
	 */
	ip = mtod(m, struct ip *);
#ifdef INET6
	ip6 = NULL;
#endif
	switch (ip->ip_v) {
#ifdef INET
	case 4:
		af = AF_INET;
		tuba4_tcp_input(m, src, dst, ip, th, toff, tlen, lindex, findex);
		break;
#endif
#ifdef INET6
	case 6:
		af = AF_INET6;
		tuba6_tcp_input(m, src, dst, ip6, th, toff, tlen, lindex, findex);
		break;
#endif
	default:
		m_freem(m);
		return;
	}

#define TUBA_INCLUDE
#ifdef INET
#define	in_pcbconnect	tuba4_pcbconnect
#endif
#ifdef INET6
#define	in6_pcbconnect	tuba6_pcbconnect
#endif
#define	tcbtable		tubainptable
#include <netinet/tcp_input.c>
}

#define tcp_slowtimo	tuba_slowtimo
#define tcp_fasttimo	tuba_fasttimo

#include <netinet/tcp_timer.c>
