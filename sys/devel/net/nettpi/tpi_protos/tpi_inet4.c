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
#include <sys/errno.h>
#include <sys/time.h>

#include <net/if.h>

#include <tpi_pcb.h>
#include <tpi_protosw.h>
#include <tpi_ip.h>

#include <netinet/in_var.h>
#include <netinet/ip_icmp.h>

#ifndef ISO
#include <netiso/iso_chksum.c>
#endif

struct tpi_protosw tpin4_protosw = {
		.tpi_afamily = AF_INET,
		.tpi_putnetaddr = in_putnetaddr,
		.tpi_getnetaddr = in_getnetaddr,
		.tpi_cmpnetaddr = in_cmpnetaddr,
		.tpi_putsufx = in_putsufx,
		.tpi_getsufx = in_getsufx,
		.tpi_recycle_suffix = in_recycle_tsuffix,
		.tpi_mtu = tpip_mtu,
		.tpi_pcbbind = in_pcbbind,
		.tpi_pcbconn = in_pcbconnect,
		.tpi_pcbdisc = in_pcbdisconnect,
		.tpi_pcbdetach = in_pcbdetach,
		.tpi_pcballoc = in_pcballoc,
		.tpi_output = tpip_output,
		.tpi_dgoutput = tpip_output_dg,
		.tpi_ctloutput = 0,
		.tpi_pcblist = &tp_inpcb,
};

void
in_getsufx(void *v, u_short *lenp, caddr_t data_out, int which)
{
	struct inpcb *inp = (struct inpcb *)v;

	*lenp = sizeof(u_short);
	switch (which) {
	case TPI_LOCAL:
		*data_out = (caddr_t)inp->inp_lport;
		break;
	case TPI_FOREIGN:
		*data_out = (caddr_t)inp->inp_fport;
		break;
	}
}

void
in_putsufx(void *v, caddr_t sufxloc, int sufxlen, int which)
{
	struct inpcb *inp;

	inp = (struct inpcb *)v;
	switch (which) {
	case TPI_LOCAL:
		bcopy(sufxloc, (caddr_t)&inp->inp_lport, sizeof(inp->inp_lport));
		break;
	case TPI_FOREIGN:
		bcopy(sufxloc, (caddr_t)&inp->inp_fport, sizeof(inp->inp_fport));
		break;
	}
}

void
in_recycle_tsuffix(void *v)
{
	struct inpcb *inp;

	inp = (struct inpcb *)v;
	inp->inp_fport = inp->inp_lport = 0;
}

void
in_putnetaddr(void *v, struct sockaddr *name, int which)
{
	register struct inpcb *inp;
	struct sockaddr_in *sin;

	inp = (struct inpcb *)v;
	sin = (struct sockaddr_in *)name;

	if (sin == NULL) {
		return;
	}
	switch (which) {
	case TPI_LOCAL:
		bcopy((caddr_t) &sin->sin_addr, (caddr_t) &inp->inp_laddr,
				sizeof(struct in_addr));
		/* won't work if the dst address (name) is INADDR_ANY */
		break;
	case TPI_FOREIGN:
		bcopy((caddr_t) &sin->sin_addr, (caddr_t) &inp->inp_faddr,
				sizeof(struct in_addr));
		break;
	}
}

int
in_cmpnetaddr(void *v, struct sockaddr *name, int which)
{
	register struct inpcb *inp;
	struct sockaddr_in *sin;
	int compare = 0;

	inp = (struct inpcb *)v;
	sin = (struct sockaddr_in *)name;
	switch (which) {
	case TPI_LOCAL:
		if ((sin->sin_port && sin->sin_port) != inp->inp_lport) {
			break;
		}
		compare = (sin->sin_addr.s_addr == inp->inp_laddr.s_addr);
		break;
	case TPI_FOREIGN:
		if ((sin->sin_port && sin->sin_port) != inp->inp_fport) {
			break;
		}
		compare = (sin->sin_addr.s_addr == inp->inp_faddr.s_addr);
		break;
	}
	return (compare);
}

void
in_getnetaddr(void *v, struct mbuf *name, int which)
{
	register struct sockaddr_in *sin;
	register struct inpcb	*inp;

	inp = (struct inpcb *)v;
	sin = mtod(name, struct sockaddr_in *);
	bzero((caddr_t)sin, sizeof(*sin));
	switch (which) {
	case TPI_LOCAL:
		sin->sin_addr = inp->inp_laddr;
		sin->sin_port = inp->inp_lport;
		break;
	case TPI_FOREIGN:
		sin->sin_addr = inp->inp_faddr;
		sin->sin_port = inp->inp_fport;
		break;
	default:
		return;
	}
	name->m_len = sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
}

/* tpip */
int
tpip_mtu(struct tpipcb *tpcb)
{
	struct inpcb *inp = (struct inpcb *)tpcb->tpp_npcb;
	return (tpi_mtu(tpcb, inp->inp_route.ro_rt, sizeof(struct ip)));
}

int
tpip_output(void *v, struct mbuf *m0, int datalen, int nochksum)
{
	struct inpcb *inp;

	inp = (struct inpcb *)v;
	return (tpip_output_dg(&inp->inp_laddr, &inp->inp_faddr, m0, datalen, &inp->inp_route, nochksum));
}

int
tpip_output_dg(void *laddr_arg, void *faddr_arg, struct mbuf *m0, int datalen, void *ro_arg, int nochksum)
{
	struct in_addr *laddr, *faddr;
	struct route *ro;
	register struct mbuf *m;
	register struct ip *ip;
	int error;

	laddr = (struct in_addr *)laddr_arg;
	faddr = (struct in_addr *)faddr_arg;
	ro = (struct route *)ro_arg;

	MGETHDR(m, M_DONTWAIT, TPMT_IPHDR);
	if (m == 0) {
		error = ENOBUFS;
		goto bad;
	}
	m->m_next = m0;
	MH_ALIGN(m, sizeof(struct ip));
	m->m_len = sizeof(struct ip);

	ip = mtod(m, struct ip *);
	bzero((caddr_t)ip, sizeof *ip);

	ip->ip_p = IPPROTO_TP;
	m->m_pkthdr.len = ip->ip_len = sizeof(struct ip) + datalen;
	ip->ip_ttl = MAXTTL;
		/* don't know why you need to set ttl;
		 * overlay doesn't even make this available
		 */

	ip->ip_src = *laddr;
	ip->ip_dst = *faddr;

	IncStat(ts_tpdu_sent);
	IFDEBUG(D_EMIT)
		dump_mbuf(m, "tpip_output_dg before ip_output\n");
	ENDDEBUG

	error = ip_output(m, (struct mbuf *)0, ro, IP_ALLOWBROADCAST, NULL);
	return (error);

bad:
	m_freem(m);
	IncStat(ts_send_drop);
	return (error);
}

int
tpip_input(struct mbuf *m, int iplen)
{
	struct sockaddr_in src, dst;
	register struct ip 	*ip;
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
		dump_mbuf(m, "after tpip_input both pullups");
	ENDDEBUG
	/*
	 * m_pullup may have returned a different mbuf
	 */
	ip = mtod(m, struct ip *);

	/*
	 * drop the ip header from the front of the mbuf
	 * this is necessary for the tp checksum
	 */
	m->m_len -= iplen;
	m->m_data += iplen;

	src.sin_addr = *(struct in_addr *)&(ip->ip_src);
	src.sin_family  = AF_INET;
	src.sin_len  = sizeof(src);
	dst.sin_addr = *(struct in_addr *)&(ip->ip_dst);
	dst.sin_family  = AF_INET;
	dst.sin_len  = sizeof(dst);

	(void)tp_input(m, (struct sockaddr *)&src, (struct sockaddr *)&dst, 0, tpip_output_dg, 0);
	return (0);

discard:
	IFDEBUG(D_TPINPUT)
		printf("tpip_input DISCARD\n");
	ENDDEBUG
	IFTRACE(D_TPINPUT)
		tptrace(TPPTmisc, "tpip_input DISCARD m", m, 0, 0, 0);
	ENDTRACE
	m_freem(m);
	IncStat(ts_recv_drop);
	splx(s);
	return (0);
}

void
tpip_quench(struct inpcb *inp, int cmd)
{
	tpi_quench((struct tpipcb *)sotoinpcb(inp->inp_socket), cmd);
}

void
tpip_abort(struct inpcb *inp, int cmd)
{
	tpi_abort((struct tpipcb *)inp->inp_ppcb, cmd);
}

void *
tpip_ctlinput(int cmd, struct sockaddr *sa, void *v)
{
	struct sockaddr_in *sin;

	sin = (struct sockaddr *)sa;
	if (sin->sin_family != AF_INET && sin->sin_family != AF_IMPLINK) {
		return (NULL);
	}
	if (sin->sin_addr.s_addr == INADDR_ANY) {
		return (NULL);
	}
	if (cmd < 0 || cmd > PRC_NCMDS) {
		return (NULL);
	}
	switch (cmd) {
	case PRC_QUENCH:
		in_pcbnotify(&tp_inpcb, (struct sockaddr *)sin, 0,
			zeroin_addr, 0, cmd, tpip_quench);
		break;
	case PRC_ROUTEDEAD:
	case PRC_HOSTUNREACH:
	case PRC_UNREACH_NET:
	case PRC_IFDOWN:
	case PRC_HOSTDEAD:
		in_pcbnotify(&tp_inpcb, (struct sockaddr *)sin, 0,
			zeroin_addr, 0, cmd, in_rtchange);
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

		in_pcbnotify(&tp_inpcb, (struct sockaddr*) sin, 0, zeroin_addr, 0, cmd, tpip_abort);
	}
	return (NULL);
}

/* tpin */

void
tpin_quench(struct inpcb *inp)
{
	tpip_quench(inp, PRC_QUENCH);
}

void
tpin_abort(struct inpcb *inp)
{
	tpip_abort(inp, 0);
}
