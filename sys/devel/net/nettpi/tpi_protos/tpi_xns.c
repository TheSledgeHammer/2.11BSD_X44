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
#include <tpi_ns.h>

/* Notes:
 * - netns would require some changes to be made to the ns_pcb.c
 * to be compatible.
 *
 * Other Possible Inclusions:
 * - NSIP: could be supported
 */

struct tpi_protosw tpns_protosw = {
		.tpi_afamily = AF_NS,
		.tpi_putnetaddr = ns_putnetaddr,
		.tpi_getnetaddr = ns_getnetaddr,
		.tpi_cmpnetaddr = ns_cmpnetaddr,
		.tpi_putsufx = ns_putsufx,
		.tpi_getsufx = ns_getsufx,
		.tpi_recycle_suffix = ns_recycle_tsuffix,
		.tpi_mtu = tpidp_mtu,
		.tpi_pcbbind = ns_pcbbind,
		.tpi_pcbconn = ns_pcbconnect,
		.tpi_pcbdisc = ns_pcbdisconnect,
		.tpi_pcbdetach = ns_pcbdetach,
		.tpi_pcballoc = ns_pcballoc,
		.tpi_output = tpidp_output,
		.tpi_dgoutput = tpidp_output_dg,
		.tpi_ctloutput = 0,
		.tpi_pcblist = &tp_nspcb,
};

void
ns_getsufx(void *v, u_short *lenp, caddr_t data_out, int which)
{
	struct nspcb *nsp = (struct nspcb *)v;

	*lenp = sizeof(u_short);
	switch (which) {
	case TPI_LOCAL:
		*data_out = (caddr_t)nsp->nsp_lport;
		break;
	case TPI_FOREIGN:
		*data_out = (caddr_t)nsp->nsp_fport;
		break;
	}
}

void
ns_putsufx(void *v, caddr_t sufxloc, int sufxlen, int which)
{
	struct nspcb *nsp;

	nsp = (struct nspcb *)v;
	switch (which) {
	case TPI_LOCAL:
		bcopy(sufxloc, (caddr_t)&nsp->nsp_lport, sizeof(nsp->nsp_lport));
		break;
	case TPI_FOREIGN:
		bcopy(sufxloc, (caddr_t)&nsp->nsp_fport, sizeof(nsp->nsp_fport));
		break;
	}
}

void
ns_recycle_tsuffix(void *v)
{
	struct nspcb *nsp;

	nsp = (struct nspcb *)v;
	nsp->nsp_fport = nsp->nsp_lport = 0;
}

void
ns_putnetaddr(void *v, struct sockaddr *name, int which)
{
	register struct nspcb *nsp;
	struct sockaddr_ns *sns;

	nsp = (struct nspcb *)v;
	sns = (struct sockaddr_ns *)name;

	if (sns == NULL) {
		return;
	}
	switch (which) {
	case TPI_LOCAL:
		bcopy((caddr_t) &sns->sns_addr, (caddr_t) &nsp->nsp_laddr,
				sizeof(struct ns_addr));
		/* won't work if the dst address (name) is INADDR_ANY */
		break;
	case TPI_FOREIGN:
		bcopy((caddr_t) &sns->sns_addr, (caddr_t) &nsp->nsp_faddr,
				sizeof(struct ns_addr));
		break;
	}
}

int
ns_cmpnetaddr(void *v, struct sockaddr *name, int which)
{
	register struct nspcb *nsp;
	struct sockaddr_ns *sns;
	int compare = 0;

	nsp = (struct nspcb *)v;
	sns = (struct sockaddr_ns *)name;
	switch (which) {
	case TPI_LOCAL:
		if ((sns->sns_port && sns->sns_port) != nsp->nsp_lport) {
			break;
		}
		compare = (sns->sns_addr.x_port == nsp->nsp_laddr.x_port);
		break;
	case TPI_FOREIGN:
		if ((sns->sns_port && sns->sns_port) != nsp->nsp_fport) {
			break;
		}
		compare = (sns->sns_addr.x_port == nsp->nsp_faddr.x_port);
		break;
	}
	return (compare);
}

void
ns_getnetaddr(void *v, struct mbuf *name, int which)
{
	register struct sockaddr_ns *sns;
	register struct nspcb	*nsp;

	nsp = (struct nspcb *)v;
	sns = mtod(name, struct sockaddr_ns *);
	bzero((caddr_t)sns, sizeof(*sns));
	switch (which) {
	case TPI_LOCAL:
		sns->sns_addr = nsp->nsp_laddr;
		sns->sns_port = nsp->nsp_lport;
		break;
	case TPI_FOREIGN:
		sns->sns_addr = nsp->nsp_faddr;
		sns->sns_port = nsp->nsp_fport;
		break;
	default:
		return;
	}
	name->m_len = sns->sns_len = sizeof(*sns);
	sns->sns_family = AF_NS;
}

/* tpidp */
int
tpidp_mtu(struct tpipcb *tpcb)
{
	struct nspcb *nsp = (struct nspcb *)tpcb->tpp_npcb;
	return (tpi_mtu(tpcb, nsp->nsp_route.ro_rt, sizeof(struct idp)));
}

int
tpidp_output(void *v, struct mbuf *m0, int datalen, int nochksum)
{
	struct nspcb *nsp;

	nsp = (struct nspcb *)v;
	return (tpip_output_dg(&nsp->nsp_laddr, &nsp->nsp_faddr, m0, datalen, &nsp->nsp_route, nochksum));
}

int
tpidp_output_dg(void *laddr_arg, void *faddr_arg, struct mbuf *m0, int datalen, void *ro_arg, int nochksum)
{
	struct ns_addr *laddr, *faddr;
	struct route *ro;
	register struct mbuf *m;
	register struct idp *idp;
	int error;

	laddr = (struct ns_addr *)laddr_arg;
	faddr = (struct ns_addr *)faddr_arg;
	ro = (struct route *)ro_arg;

	MGETHDR(m, M_DONTWAIT, TPMT_IPHDR);
	if (m == 0) {
		error = ENOBUFS;
		goto bad;
	}
	m->m_next = m0;
	MH_ALIGN(m, sizeof(struct idp));
	m->m_len = sizeof(struct idp);

	idp = mtod(m, struct idp *);
	bzero((caddr_t)idp, sizeof *idp);

	m->m_pkthdr.len = idp->idp_len = sizeof(struct idp) + datalen;

	idp->idp_sna = *laddr;
	idp->idp_dna = *faddr;

	IncStat(ts_tpdu_sent);
	IFDEBUG(D_EMIT)
		dump_mbuf(m, "tpns_output_dg before idp_output\n");
	ENDDEBUG

	error = idp_output2(m, ro, NS_ALLOWBROADCAST, idp->idp_pt, laddr, faddr, NULL);
	return (error);

bad:
	m_freem(m);
	IncStat(ts_send_drop);
	return (error);
}

int
tpidp_input(struct mbuf *m, int idplen)
{
	struct sockaddr_ns src, dst;
	register struct idp *idp;
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

	if((m = m_pullup(m, idplen + 1)) == MNULL)
		goto discard;
	CHANGE_MTYPE(m, TPMT_DATA);

	/*
	 * Now pull up the whole tp header:
	 * Unfortunately, there may be IP options to skip past so we
	 * just fetch it as an unsigned char.
	 */
	hdrlen = idplen + 1 + mtod(m, u_char *)[idplen];

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
		dump_mbuf(m, "after tpidp_input both pullups");
	ENDDEBUG
	/*
	 * m_pullup may have returned a different mbuf
	 */
	idp = mtod(m, struct idp *);

	/*
	 * drop the ip header from the front of the mbuf
	 * this is necessary for the tp checksum
	 */
	m->m_len -= idplen;
	m->m_data += idplen;
	src.sns_addr = *(struct ns_addr *)&(idp->idp_sna);
	src.sns_family  = AF_NS;
	src.sns_len  = sizeof(src);
	dst.sns_addr = *(struct ns_addr *)&(idp->idp_dna);
	dst.sns_family  = AF_NS;
	dst.sns_len  = sizeof(dst);

	(void)tp_input(m, (struct sockaddr *)&src, (struct sockaddr *)&dst, 0, tpidp_output_dg, 0);
	return (0);

discard:
	IFDEBUG(D_TPINPUT)
		printf("tpidp_input DISCARD\n");
	ENDDEBUG
	IFTRACE(D_TPINPUT)
		tptrace(TPPTmisc, "tpidp_input DISCARD m", m, 0, 0, 0);
	ENDTRACE
	m_freem(m);
	IncStat(ts_recv_drop);
	splx(s);
	return (0);
}

void
tpidp_quench(struct nspcb *nsp, int cmd)
{
	tpi_quench((struct tpipcb *)sotonspcb(nsp->nsp_socket), cmd);
}

void
tpidp_abort(struct nspcb *nsp, int cmd)
{
	tpi_abort((struct nspcb *)nsp->nsp_pcb, cmd);
}

void *
tpip_ctlinput(int cmd, struct sockaddr *sa, void *v)
{
	struct sockaddr_ns *sns;

	sns = (struct sockaddr *)sa;
	if (sns->sns_family != AF_NS && sns->sns_family != AF_IMPLINK) {
		return (NULL);
	}
	if (sns->sns_port == INADDR_ANY) {
		return (NULL);
	}
	if (cmd < 0 || cmd > PRC_NCMDS) {
		return (NULL);
	}
	switch (cmd) {
	case PRC_QUENCH:
		ns_pcbnotify(&tp_nspcb, (struct sockaddr *)sns, 0, zerons_addr, 0, cmd, tpidp_quench);
		break;
	case PRC_ROUTEDEAD:
	case PRC_HOSTUNREACH:
	case PRC_UNREACH_NET:
	case PRC_IFDOWN:
	case PRC_HOSTDEAD:
		ns_pcbnotify(&tp_nspcb, (struct sockaddr *)sns, 0,
			zerons_addr, 0, cmd, ns_rtchange);
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

		ns_pcbnotify(&tp_nspcb, (struct sockaddr *)sns, 0, zerons_addr, 0, cmd, tpidp_abort);
	}
	return (NULL);
}

/* tpns */

void
tpns_quench(struct nspcb *nsp)
{
	tpidp_quench(nsp, PRC_QUENCH);
}

void
tpns_abort(struct nspcb *nsp)
{
	tpidp_abort(nsp, 0);
}
