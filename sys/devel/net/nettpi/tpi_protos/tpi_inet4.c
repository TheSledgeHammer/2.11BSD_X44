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

#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <netinet/in_pcb.h>

#include <tpi_pcb.h>
#include <tpi_protosw.h>

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

tpip_mtu(tpcb)
{

}

tpip_output()
{

}

tpip_output_dg()
{

}

tpip_input()
{

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
