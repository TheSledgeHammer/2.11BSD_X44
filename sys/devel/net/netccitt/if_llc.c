/*
 * if_llc.c
 *
 *  Created on: 5 July 2024
 *      Author: marti
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
