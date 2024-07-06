/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/un.h>
#include <sys/socketvar.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_llc.h>

#define LLC
#include <netccitt/dll.h>
#include <netccitt/llc_var.h>

struct llc_softc {
	struct ifnet		sc_if;
	struct llc_linkcb 	sc_llc;
	u_char 				sc_cmdrsp;
};

int		llc2_clone_create(struct if_clone *, int);
void 	llc2_clone_destroy(struct ifnet *);


struct if_clone llc_cloner =
		   IF_CLONE_INITIALIZER("llc2", llc2_clone_create, llc2_clone_destroy);

void
llc2attach(int count)
{
	if_clone_attach(&llc_cloner);
}

int
llc2_clone_create(struct if_clone *ifc, int unit)
{

	struct llc_softc *sc;


	sc = malloc(sizeof(*sc), M_DEVBUF, M_WAITOK | M_ZERO);
	if_initname(&sc->sc_if, ifc->ifc_name, unit);
	sc->sc_if.if_softc = sc;
	sc->sc_if.if_type = IFT_ISO88022LLC;
	sc->sc_if.if_dlt = DLT_NULL;
	sc->sc_if.if_mtu = 1500;
	sc->sc_if.if_input = llc2_input;
	sc->sc_if.if_output = llc2_output;
	sc->sc_if.if_ioctl = llc2_ioctl;

	if_attach(&sc->sc_if);
	if_alloc_sadl(&sc->sc_if);
	//bpfattach(&sc->sc_if, DLT_NULL, sizeof(u_int));

	if_set_sadl();
	llc_init();

	return (0);
}

struct llc *
llc2_frame_init(struct llc_softc *sc, struct mbuf *m)
{
	struct llc 			*frame;

	if (m->m_len > sizeof(struct sockaddr_dl_header)) {
		frame = mtod((struct mbuf* )((struct sockaddr_dl_header *)(m + 1)), struct llc *);
	} else {
		frame = mtod(m->m_next, struct llc *);
	}
	if (frame == NULL) {
		return (NULL);
	}
	sc->sc_cmdrsp = (frame->llc_ssap & 0x01);
	frame->llc_ssap &= ~0x01;
	return (frame);
}

if_sapinfo()
{
	struct ifaddr *ifa;
	struct sockaddr *addr;

	ifa = ifa_ifwithaddr(addr);
}

void
llc2_clone_destroy(struct ifnet *ifp)
{
	int s;

	s = splnet();
	if_detach(ifp);
	splx(s);

	free(ifp->if_softc, M_DEVBUF);
}

void
llc2_input(ifp, m)
	struct ifnet *ifp;
	struct mbuf *m;
{
	struct llc_softc *sc;
	struct llc_linkcb *linkp;

	linkp = mtod(m, struct llc_linkcp);
	linkp = &sc->sc_llc;


	llc_input(m->m_pkthdr.rcvif, m, sc->sc_cmdrsp);

}


int
llc2_output(ifp, m, dst, rt)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *dst;
	struct rtentry *rt;
{

	llc_output(linkp, m);
}

static int
llc2_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct sockaddr *sa;
	int error;

	sa = (struct sockaddr *)ifp->if_sadl;

	switch (cmd) {
	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;
		llc_ctlinput(PRC_UP, sa, data);
		break;
	case SIOCALIFADDR:
	case SIOCDLIFADDR:
	case SIOCGLIFADDR:
	default:
	}

	return (error);
}
