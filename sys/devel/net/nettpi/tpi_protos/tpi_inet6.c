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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stdarg.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <net/if.h>

#include <tpi_pcb.h>
#include <tpi_protosw.h>
#include <tpi_ip6.h>

#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet6/in6_var.h>

struct tpi_protosw tpin6_protosw = {
		.tpi_afamily = AF_INET6,
		.tpi_putnetaddr = in6_putnetaddr,
		.tpi_getnetaddr = in6_getnetaddr,
		.tpi_cmpnetaddr = in6_cmpnetaddr,
		.tpi_putsufx = in6_putsufx,
		.tpi_getsufx = in6_getsufx,
		.tpi_recycle_suffix = in6_recycle_tsuffix,
		.tpi_mtu = tpip6_mtu,
		.tpi_pcbbind = in6_pcbbind,
		.tpi_pcbconn = in6_pcbconnect,
		.tpi_pcbdisc = in6_pcbdisconnect,
		.tpi_pcbdetach = in6_pcbdetach,
		.tpi_pcballoc = in6_pcballoc,
		.tpi_output = tpip6_output,
		.tpi_dgoutput = tpip6_output_dg,
		.tpi_ctloutput = 0,
		.tpi_pcblist = &tp_in6pcb,
};


void
in6_getsufx(void *v, u_short *lenp, caddr_t data_out, int which)
{
	struct in6pcb *in6p = (struct in6pcb *)v;

	*lenp = sizeof(u_short);
	switch (which) {
	case TPI_LOCAL:
		*data_out = (caddr_t)in6p->in6p_lport;
		break;
	case TPI_FOREIGN:
		*data_out = (caddr_t)in6p->in6p_fport;
		break;
	}
}

void
in6_putsufx(void *v, caddr_t sufxloc, int sufxlen, int which)
{
	struct in6pcb *in6p;

	in6p = (struct in6pcb *)v;
	switch (which) {
	case TPI_LOCAL:
		bcopy(sufxloc, (caddr_t)&in6p->in6p_lport, sizeof(in6p->in6p_lport));
		break;
	case TPI_FOREIGN:
		bcopy(sufxloc, (caddr_t)&in6p->in6p_fport, sizeof(in6p->in6p_fport));
		break;
	}
}

void
in6_recycle_tsuffix(void *v)
{
	struct in6pcb *in6p;

	in6p = (struct in6pcb *)v;
	in6p->in6p_fport = in6p->in6p_lport = 0;
}

void
in6_putnetaddr(void *v, struct sockaddr *name, int which)
{
	register struct in6pcb *in6p;
	struct sockaddr_in6 *sin6;

	in6p = (struct in6pcb *)v;
	sin6 = (struct sockaddr_in6 *)name;

	if (sin6 == NULL) {
		return;
	}
	switch (which) {
	case TPI_LOCAL:
		bcopy((caddr_t) &sin6->sin6_addr, (caddr_t) &in6p->in6p_laddr,
				sizeof(struct in6_addr));
		/* won't work if the dst address (name) is INADDR_ANY */
		break;
	case TPI_FOREIGN:
		bcopy((caddr_t) &sin6->sin6_addr, (caddr_t) &in6p->in6p_faddr,
				sizeof(struct in6_addr));
		break;
	}
}

int
in6_cmpnetaddr(void *v, struct sockaddr *name, int which)
{
	register struct in6pcb *in6p;
	struct sockaddr_in6 *sin6;
	int compare = 0;

	in6p = (struct in6pcb *)v;
	sin6 = (struct sockaddr_in6 *)name;
	switch (which) {
	case TPI_LOCAL:
		if ((sin6->sin6_port && sin6->sin6_port) != in6p->in6p_lport) {
			break;
		}
		compare = (sin6->sin6_addr.s6_addr32 == in6p->in6p_laddr.s6_addr32);
		break;
	case TPI_FOREIGN:
		if ((sin6->sin6_port && sin6->sin6_port) != in6p->in6p_fport) {
			break;
		}
		compare = (sin6->sin6_addr.s6_addr32 == in6p->in6p_faddr.s6_addr32);
		break;
	}
	return (compare);
}

void
in6_getnetaddr(void *v, struct mbuf *name, int which)
{
	register struct sockaddr_in6 *sin6;
	register struct in6pcb	*in6p;

	in6p = (struct in6pcb *)v;
	sin6 = mtod(name, struct sockaddr_in6 *);
	bzero((caddr_t)sin, sizeof(*sin6));
	switch (which) {
	case TPI_LOCAL:
		sin6->sin6_addr = in6p->in6p_laddr;
		sin6->sin6_port = in6p->in6p_lport;
		break;
	case TPI_FOREIGN:
		sin6->sin6_addr = in6p->in6p_faddr;
		sin6->sin6_port = in6p->in6p_fport;
		break;
	default:
		return;
	}
	name->m_len = sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_family = AF_INET6;
}

/* tpip6 */
int
tpip6_mtu(struct tpipcb *tpcb)
{
	struct in6pcb *in6p = (struct in6pcb *)tpcb->tpp_npcb;
	return (tpi_mtu(tpcb, in6p->in6p_route.ro_rt, sizeof(struct ip6_hdr)));
}

int
tpip6_output(void *v, struct mbuf *m0, int datalen, int nochksum)
{
	struct in6pcb *in6p;

	in6p = (struct in6pcb *)v;
	return (tpip6_output_dg(&in6p->in6p_laddr, &in6p->in6p_faddr, m0, datalen, &in6p->in6p_route, nochksum));
}

int
tpip6_output_dg(void *laddr_arg, void *faddr_arg, struct mbuf *m0, int datalen, void *ro_arg, int nochksum)
{
	struct in6_addr *laddr, *faddr;
	struct route_in6 *ro;
	register struct mbuf *m;
	register struct ip6_hdr *ip6;
	int error;

	laddr = (struct in6_addr *)laddr_arg;
	faddr = (struct in6_addr *)faddr_arg;
	ro = (struct route_in6 *)ro_arg;

	MGETHDR(m, M_DONTWAIT, TPMT_IPHDR);
	if (m == 0) {
		error = ENOBUFS;
		goto bad;
	}
	m->m_next = m0;
	MH_ALIGN(m, sizeof(struct ip6_hdr));
	m->m_len = sizeof(struct ip6_hdr);

	ip6 = mtod(m, struct ip6_hdr *);
	bzero((caddr_t)ip6, sizeof *ip6);

	ip6->ip6_vfc = IPPROTO_TP & IPV6_VERSION_MASK;
	m->m_pkthdr.len = ip6->ip6_plen = sizeof(struct ip6_hdr) + datalen;

	ip6->ip6_src = *laddr;
	ip6->ip6_dst = *faddr;

	IncStat(ts_tpdu_sent);
	IFDEBUG(D_EMIT)
		dump_mbuf(m, "tpip6_output_dg before ip6_output\n");
	ENDDEBUG

	error = tpip6_output_sc(m, NULL, ro, IP_ALLOWBROADCAST, NULL, NULL, NULL);
	return (error);

bad:
	m_freem(m);
	IncStat(ts_send_drop);
	return (error);
}

int
tpip6_output_sc(struct mbuf *m0, ...)
{
	struct ip6_pktopts *opt;
	struct route_in6 *ro;
	int flags;
	struct ip6_moptions *im6o;
	struct socket *so;
	struct ifnet **ifpp;
	va_list ap;

	va_start(ap, m0);
	opt = va_arg(ap, struct ip6_pktopts *);
	ro = va_arg(ap, struct route_in6 *);
	flags = va_arg(ap, int);
	im6o = va_arg(ap, struct ip6_moptions *);
	so = va_arg(ap, struct socket *);
	ifpp = va_arg(ap, struct ifnet **);
	va_end(ap);
	return (ip6_output(m, opt, ro, flags, im6o, so, ifpp));
}

int
tpip6_input(struct mbuf *m, int iplen)
{
	struct sockaddr_in6 src6, dst6;
	register struct ip6_hdr *ip6;
	int	s, hdrlen;

	s = splnet();
	IncStat(ts_pkt_rcvd);

	/*
	 * IP layer has already pulled up the IP header,
	 * but the first byte after the IP header may not be there,
	 * e.g. if you came in via loopback, so you have to do an
	 * m_pullup to before you can even look to see how much you
	 * really need.  The good news is that m_pullup will round
	 * up to almost the next mbuf's worth.
	 */

	if((m = m_pullup(m, iplen + 1)) == MNULL)
		goto discard;
	CHANGE_MTYPE(m, TPMT_DATA);

	/*
	 * Now pull up the whole tp header:
	 * Unfortunately, there may be IP options to skip past so we
	 * just fetch it as an unsigned char.
	 */
	hdrlen = iplen + 1 + mtod(m, u_char *)[iplen];

	if( m->m_len < hdrlen ) {
		if((m = m_pullup(m, hdrlen)) == MNULL){
			IFDEBUG(D_TPINPUT)
				printf("tp_input, pullup 2!\n");
			ENDDEBUG
			goto discard;
		}
	}

	/*
	 * cannot use tp_inputprep() here 'cause you don't
	 * have quite the same situation
	 */

	IFDEBUG(D_TPINPUT)
		dump_mbuf(m, "after tpip6_input both pullups");
	ENDDEBUG
	/*
	 * m_pullup may have returned a different mbuf
	 */
	ip6 = mtod(m, struct ip6_hdr *);

	/*
	 * drop the ip header from the front of the mbuf
	 * this is necessary for the tp checksum
	 */
	m->m_len -= iplen;
	m->m_data += iplen;

	src6.sin6_addr = *(struct in6_addr *)&(ip6->ip6_src);
	src6.sin6_family  = AF_INET6;
	src6.sin6_len  = sizeof(src6);
	dst6.sin6_addr = *(struct in6_addr *)&(ip6->ip6_dst);
	dst6.sin6_family  = AF_INET6;
	dst6.sin6_len  = sizeof(dst6);

	(void)tp_input(m, (struct sockaddr *)&src6, (struct sockaddr *)&dst6, 0, tpip6_output_dg, 0);
	return (0);

discard:
	IFDEBUG(D_TPINPUT)
		printf("tpip6_input DISCARD\n");
	ENDDEBUG
	IFTRACE(D_TPINPUT)
		tptrace(TPPTmisc, "tpip6_input DISCARD m", m, 0, 0, 0);
	ENDTRACE
	m_freem(m);
	IncStat(ts_recv_drop);
	splx(s);
	return (0);
}

void
tpip6_quench(struct in6pcb *in6p, int cmd)
{
	tpi_quench((struct tpipcb *)sotoin6pcb(in6p->in6p_socket), cmd);
}

void
tpip6_abort(struct in6pcb *in6p, int cmd)
{
	tpi_abort((struct tpipcb *)in6p->in6p_ppcb, cmd);
}

void *
tpip6_ctlinput(int cmd, struct sockaddr *sa, void *v)
{
	struct sockaddr_in6 *sin6;

	sin6 = (struct sockaddr *)sa;
	if (sin6->sin6_family != AF_INET6 && sin6->sin6_family != AF_IMPLINK) {
		return (NULL);
	}
	if (sin6->sin6_addr == in6addr_any) {
		return (NULL);
	}
	if (cmd < 0 || cmd > PRC_NCMDS) {
		return (NULL);
	}
	switch (cmd) {
	case PRC_QUENCH:
		in6_pcbnotify(&tp_in6pcb, (struct sockaddr *)sin6, 0,
			zeroin6_addr, 0, cmd, tpip6_quench);
		break;
	case PRC_ROUTEDEAD:
	case PRC_HOSTUNREACH:
	case PRC_UNREACH_NET:
	case PRC_IFDOWN:
	case PRC_HOSTDEAD:
		in6_pcbnotify(&tp_in6pcb, (struct sockaddr *)sin6, 0,
			zeroin6_addr, 0, cmd, in6_rtchange);
		break;
	default:
/*
	case PRC_MSGSIZE:
	case PRC_UNREACH_HOST:
	case PRC_UNREACH_PROTOCOL:
	case PRC_UNREACH_PORT:
	case PRC_UNREACH_NEEDFRAG:
	case PRC_UNREACH_SRCFAIL:
	case PRC_REDIRECT_NET:
	case PRC_REDIRECT_HOST:
	case PRC_REDIRECT_TOSNET:
	case PRC_REDIRECT_TOSHOST:
	case PRC_TIMXCEED_INTRANS:
	case PRC_TIMXCEED_REASS:
	case PRC_PARAMPROB:
*/

		in6_pcbnotify(&tp_in6pcb, (struct sockaddr *)sin6, 0, zeroin6_addr, 0, cmd, tpip6_abort);
	}
	return (NULL);
}

/* tpin6 */
void
tpin6_quench(struct in6pcb *in6p)
{
	tpip6_quench(in6p, PRC_QUENCH);
}

void
tpin6_abort(struct in6pcb *in6p)
{
	tpip6_abort(in6p, 0);
}
