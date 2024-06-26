/*	$NetBSD: if_bridge.c,v 1.22.2.1.4.2 2006/02/12 16:29:54 tron Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * Copyright (c) 1999, 2000 Jason L. Wright (jason@thought.net)
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jason L. Wright
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * OpenBSD: if_bridge.c,v 1.60 2001/06/15 03:38:33 itojun Exp
 */

/*
 * Network interface bridge support.
 *
 * TODO:
 *
 *	- Currently only supports Ethernet-like interfaces (Ethernet,
 *	  802.11, VLANs on Ethernet, etc.)  Figure out a nice way
 *	  to bridge other types of interfaces (FDDI-FDDI, and maybe
 *	  consider heterogenous bridges).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_bridge.c,v 1.22.2.1.4.2 2006/02/12 16:29:54 tron Exp $");

#include "bpfilter.h"

#include <sys/param.h> 
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/proc.h> 

#if NBPFILTER > 0
#include <net/bpf.h>
#endif 
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_llc.h>

#include <net/if_ether.h>
#include <net/if_bridgevar.h>
#include <net/radix.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <netinet/ip6.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6_var.h>

/*
 * Size of the route hash table.  Must be a power of two.
 */
#ifndef BRIDGE_RTHASH_SIZE
#define	BRIDGE_RTHASH_SIZE		1024
#endif

#define	BRIDGE_RTHASH_MASK		(BRIDGE_RTHASH_SIZE - 1)

#include "carp.h"
#if NCARP > 0
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip_carp.h>
#endif

/*
 * Maximum number of addresses to cache.
 */
#ifndef BRIDGE_RTABLE_MAX
#define	BRIDGE_RTABLE_MAX		100
#endif

/*
 * Spanning tree defaults.
 */
#define	BSTP_DEFAULT_MAX_AGE			(20 * 256)
#define	BSTP_DEFAULT_HELLO_TIME			(2 * 256)
#define	BSTP_DEFAULT_FORWARD_DELAY		(15 * 256)
#define	BSTP_DEFAULT_HOLD_TIME			(1 * 256)
#define	BSTP_DEFAULT_BRIDGE_PRIORITY	0x8000
#define	BSTP_DEFAULT_PORT_PRIORITY		0x80
#define	BSTP_DEFAULT_PATH_COST			55

/*
 * Timeout (in seconds) for entries learned dynamically.
 */
#ifndef BRIDGE_RTABLE_TIMEOUT
#define	BRIDGE_RTABLE_TIMEOUT		(20 * 60)	/* same as ARP */
#endif

/*
 * Number of seconds between walks of the route list.
 */
#ifndef BRIDGE_RTABLE_PRUNE_PERIOD
#define	BRIDGE_RTABLE_PRUNE_PERIOD	(5 * 60)
#endif

int	bridge_rtable_prune_period = BRIDGE_RTABLE_PRUNE_PERIOD;

void	bridgeattach(int);

int		bridge_clone_create(struct if_clone *, int);
void	bridge_clone_destroy(struct ifnet *);

int	bridge_ioctl(struct ifnet *, u_long, caddr_t);
int	bridge_init(struct ifnet *);
void	bridge_stop(struct ifnet *, int);
void	bridge_start(struct ifnet *);

void	bridge_forward(struct bridge_softc *, struct mbuf *m);

void	bridge_timer(void *);

void	bridge_broadcast(struct bridge_softc *, struct ifnet *, struct mbuf *);

int	bridge_rtupdate(struct bridge_softc *, const uint8_t *, struct ifnet *, int, uint8_t);
struct ifnet *bridge_rtlookup(struct bridge_softc *, const uint8_t *);
void	bridge_rttrim(struct bridge_softc *);
void	bridge_rtage(struct bridge_softc *);
void	bridge_rtflush(struct bridge_softc *, int);
int	bridge_rtdaddr(struct bridge_softc *, const uint8_t *);
void	bridge_rtdelete(struct bridge_softc *, struct ifnet *ifp);

int	bridge_rtable_init(struct bridge_softc *);
void	bridge_rtable_fini(struct bridge_softc *);

struct bridge_rtnode *bridge_rtnode_lookup(struct bridge_softc *, const uint8_t *);
int	bridge_rtnode_insert(struct bridge_softc *, struct bridge_rtnode *);
void	bridge_rtnode_destroy(struct bridge_softc *, struct bridge_rtnode *);

struct bridge_iflist *bridge_lookup_member(struct bridge_softc *, const char *name);
struct bridge_iflist *bridge_lookup_member_if(struct bridge_softc *, struct ifnet *ifp);
void	bridge_delete_member(struct bridge_softc *, struct bridge_iflist *);

int	bridge_ioctl_add(struct bridge_softc *, void *);
int	bridge_ioctl_del(struct bridge_softc *, void *);
int	bridge_ioctl_gifflags(struct bridge_softc *, void *);
int	bridge_ioctl_sifflags(struct bridge_softc *, void *);
int	bridge_ioctl_scache(struct bridge_softc *, void *);
int	bridge_ioctl_gcache(struct bridge_softc *, void *);
int	bridge_ioctl_gifs(struct bridge_softc *, void *);
int	bridge_ioctl_rts(struct bridge_softc *, void *);
int	bridge_ioctl_saddr(struct bridge_softc *, void *);
int	bridge_ioctl_sto(struct bridge_softc *, void *);
int	bridge_ioctl_gto(struct bridge_softc *, void *);
int	bridge_ioctl_daddr(struct bridge_softc *, void *);
int	bridge_ioctl_flush(struct bridge_softc *, void *);
int	bridge_ioctl_gpri(struct bridge_softc *, void *);
int	bridge_ioctl_spri(struct bridge_softc *, void *);
int	bridge_ioctl_ght(struct bridge_softc *, void *);
int	bridge_ioctl_sht(struct bridge_softc *, void *);
int	bridge_ioctl_gfd(struct bridge_softc *, void *);
int	bridge_ioctl_sfd(struct bridge_softc *, void *);
int	bridge_ioctl_gma(struct bridge_softc *, void *);
int	bridge_ioctl_sma(struct bridge_softc *, void *);
int	bridge_ioctl_sifprio(struct bridge_softc *, void *);
int	bridge_ioctl_sifcost(struct bridge_softc *, void *);
int	bridge_ioctl_gfilt(struct bridge_softc *, void *);
int	bridge_ioctl_sfilt(struct bridge_softc *, void *);
static int bridge_ipf(void *, struct mbuf **, struct ifnet *, int);
static int bridge_ip_checkbasic(struct mbuf **mp);
# ifdef INET6
static int bridge_ip6_checkbasic(struct mbuf **mp);
# endif /* INET6 */

struct bridge_control {
	int	(*bc_func)(struct bridge_softc *, void *);
	int	bc_argsize;
	int	bc_flags;
};

#define	BC_F_COPYIN		0x01	/* copy arguments in */
#define	BC_F_COPYOUT	0x02	/* copy arguments out */
#define	BC_F_SUSER		0x04	/* do super-user check */

const struct bridge_control bridge_control_table[] = {
	{ bridge_ioctl_add,		sizeof(struct ifbreq),
	  BC_F_COPYIN|BC_F_SUSER },
	{ bridge_ioctl_del,		sizeof(struct ifbreq),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_gifflags,	sizeof(struct ifbreq),
	  BC_F_COPYIN|BC_F_COPYOUT },
	{ bridge_ioctl_sifflags,	sizeof(struct ifbreq),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_scache,		sizeof(struct ifbrparam),
	  BC_F_COPYIN|BC_F_SUSER },
	{ bridge_ioctl_gcache,		sizeof(struct ifbrparam),
	  BC_F_COPYOUT },

	{ bridge_ioctl_gifs,		sizeof(struct ifbifconf),
	  BC_F_COPYIN|BC_F_COPYOUT },
	{ bridge_ioctl_rts,		sizeof(struct ifbaconf),
	  BC_F_COPYIN|BC_F_COPYOUT },

	{ bridge_ioctl_saddr,		sizeof(struct ifbareq),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_sto,		sizeof(struct ifbrparam),
	  BC_F_COPYIN|BC_F_SUSER },
	{ bridge_ioctl_gto,		sizeof(struct ifbrparam),
	  BC_F_COPYOUT },

	{ bridge_ioctl_daddr,		sizeof(struct ifbareq),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_flush,		sizeof(struct ifbreq),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_gpri,		sizeof(struct ifbrparam),
	  BC_F_COPYOUT },
	{ bridge_ioctl_spri,		sizeof(struct ifbrparam),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_ght,		sizeof(struct ifbrparam),
	  BC_F_COPYOUT },
	{ bridge_ioctl_sht,		sizeof(struct ifbrparam),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_gfd,		sizeof(struct ifbrparam),
	  BC_F_COPYOUT },
	{ bridge_ioctl_sfd,		sizeof(struct ifbrparam),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_gma,		sizeof(struct ifbrparam),
	  BC_F_COPYOUT },
	{ bridge_ioctl_sma,		sizeof(struct ifbrparam),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_sifprio,		sizeof(struct ifbreq),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_sifcost,		sizeof(struct ifbreq),
	  BC_F_COPYIN|BC_F_SUSER },

	{ bridge_ioctl_gfilt,		sizeof(struct ifbrparam),
	  BC_F_COPYOUT },
	{ bridge_ioctl_sfilt,		sizeof(struct ifbrparam),
	  BC_F_COPYIN|BC_F_SUSER },
};
const int bridge_control_table_size =
    sizeof(bridge_control_table) / sizeof(bridge_control_table[0]);

LIST_HEAD(, bridge_softc) bridge_list;

struct if_clone bridge_cloner =
    IF_CLONE_INITIALIZER("bridge", bridge_clone_create, bridge_clone_destroy);

/*
 * bridgeattach:
 *
 *	Pseudo-device attach routine.
 */
void
bridgeattach(int n)
{
	LIST_INIT(&bridge_list);
	if_clone_attach(&bridge_cloner);
}

/*
 * bridge_clone_create:
 *
 *	Create a new bridge instance.
 */
int
bridge_clone_create(struct if_clone *ifc, int unit)
{
	struct bridge_softc *sc;
	struct ifnet *ifp;
	int s;

	sc = malloc(sizeof(*sc), M_DEVBUF, M_WAITOK);
	memset(sc, 0, sizeof(*sc));
	ifp = &sc->sc_if;

	sc->sc_brtmax = BRIDGE_RTABLE_MAX;
	sc->sc_brttimeout = BRIDGE_RTABLE_TIMEOUT;
	sc->sc_bridge_max_age = BSTP_DEFAULT_MAX_AGE;   
	sc->sc_bridge_hello_time = BSTP_DEFAULT_HELLO_TIME;
	sc->sc_bridge_forward_delay = BSTP_DEFAULT_FORWARD_DELAY;
	sc->sc_bridge_priority = BSTP_DEFAULT_BRIDGE_PRIORITY;
	sc->sc_hold_time = BSTP_DEFAULT_HOLD_TIME;
	sc->sc_filter_flags = 0;

	/* Initialize our routing table. */
	bridge_rtable_init(sc);

	callout_init(&sc->sc_brcallout);
	callout_init(&sc->sc_bstpcallout);

	LIST_INIT(&sc->sc_iflist);

	sprintf(ifp->if_xname, "%s%d", ifc->ifc_name, unit);
	ifp->if_softc = sc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_ioctl = bridge_ioctl;
	ifp->if_output = bridge_output;
	ifp->if_start = bridge_start;
	ifp->if_stop = bridge_stop;
	ifp->if_init = bridge_init;
	ifp->if_type = IFT_BRIDGE;
	ifp->if_addrlen = 0;
	ifp->if_dlt = DLT_EN10MB;
	ifp->if_hdrlen = ETHER_HDR_LEN;

	if_attach(ifp);

	if_alloc_sadl(ifp);

	s = splnet();
	LIST_INSERT_HEAD(&bridge_list, sc, sc_list);
	splx(s);

	return (0);
}

/*
 * bridge_clone_destroy:
 *
 *	Destroy a bridge instance.
 */
void
bridge_clone_destroy(struct ifnet *ifp)
{
	struct bridge_softc *sc = ifp->if_softc;
	struct bridge_iflist *bif;
	int s;

	s = splnet();

	bridge_stop(ifp, 1);

	while ((bif = LIST_FIRST(&sc->sc_iflist)) != NULL)
		bridge_delete_member(sc, bif);

	LIST_REMOVE(sc, sc_list);

	splx(s);

	if_detach(ifp);

	/* Tear down the routing table. */
	bridge_rtable_fini(sc);

	free(sc, M_DEVBUF);
}

/*
 * bridge_ioctl:
 *
 *	Handle a control request from the operator.
 */
int
bridge_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct bridge_softc *sc = ifp->if_softc;
	struct proc *p = curproc;	/* XXX */
	union {
		struct ifbreq ifbreq;
		struct ifbifconf ifbifconf;
		struct ifbareq ifbareq;
		struct ifbaconf ifbaconf;
		struct ifbrparam ifbrparam;
	} args;
	struct ifdrv *ifd = (struct ifdrv *) data;
	const struct bridge_control *bc;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCGDRVSPEC:
	case SIOCSDRVSPEC:
		if (ifd->ifd_cmd >= bridge_control_table_size) {
			error = EINVAL;
			break;
		}
		bc = &bridge_control_table[ifd->ifd_cmd];

		if (cmd == SIOCGDRVSPEC &&
		    (bc->bc_flags & BC_F_COPYOUT) == 0) {
			error = EINVAL;
			break;
		}
		else if (cmd == SIOCSDRVSPEC &&
		    (bc->bc_flags & BC_F_COPYOUT) != 0) {
			error = EINVAL;
			break;
		}

		if (bc->bc_flags & BC_F_SUSER) {
			error = suser(p->p_ucred, &p->p_acflag);
			if (error)
				break;
		}

		if (ifd->ifd_len != bc->bc_argsize ||
		    ifd->ifd_len > sizeof(args)) {
			error = EINVAL;
			break;
		}

		memset(&args, 0, sizeof(args));
		if (bc->bc_flags & BC_F_COPYIN) {
			error = copyin(ifd->ifd_data, &args, ifd->ifd_len);
			if (error)
				break;
		}

		error = (*bc->bc_func)(sc, &args);
		if (error)
			break;

		if (bc->bc_flags & BC_F_COPYOUT)
			error = copyout(&args, ifd->ifd_data, ifd->ifd_len);

		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_RUNNING) {
			/*
			 * If interface is marked down and it is running,
			 * then stop and disable it.
			 */
			(*ifp->if_stop)(ifp, 1);
		} else if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) == IFF_UP) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			error = (*ifp->if_init)(ifp);
		}
		break;

	default:
		error = ENOTTY;
		break;
	}

	splx(s);

	return (error);
}

/*
 * bridge_lookup_member:
 *
 *	Lookup a bridge member interface.  Must be called at splnet().
 */
struct bridge_iflist *
bridge_lookup_member(struct bridge_softc *sc, const char *name)
{
	struct bridge_iflist *bif;
	struct ifnet *ifp;

	LIST_FOREACH(bif, &sc->sc_iflist, bif_next) {
		ifp = bif->bif_ifp;
		if (strcmp(ifp->if_xname, name) == 0)
			return (bif);
	}

	return (NULL);
}

/*
 * bridge_lookup_member_if:
 *
 *	Lookup a bridge member interface by ifnet*.  Must be called at splnet().
 */
struct bridge_iflist *
bridge_lookup_member_if(struct bridge_softc *sc, struct ifnet *member_ifp)
{
	struct bridge_iflist *bif;

	LIST_FOREACH(bif, &sc->sc_iflist, bif_next) {
		if (bif->bif_ifp == member_ifp)
			return (bif);
	}

	return (NULL);
}

/*
 * bridge_delete_member:
 *
 *	Delete the specified member interface.
 */
void
bridge_delete_member(struct bridge_softc *sc, struct bridge_iflist *bif)
{
	struct ifnet *ifs = bif->bif_ifp;

	switch (ifs->if_type) {
	case IFT_ETHER:
		/*
		 * Take the interface out of promiscuous mode.
		 */
		(void) ifpromisc(ifs, 0);
		break;

	default:
#ifdef DIAGNOSTIC
		panic("bridge_delete_member: impossible");
#endif
		break;
	}

	ifs->if_bridge = NULL;
	LIST_REMOVE(bif, bif_next);

	bridge_rtdelete(sc, ifs);

	free(bif, M_DEVBUF);

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);
}

int
bridge_ioctl_add(struct bridge_softc *sc, void *arg)
{
	struct ifbreq *req = arg;
	struct bridge_iflist *bif = NULL;
	struct ifnet *ifs;
	int error = 0;

	ifs = ifunit(req->ifbr_ifsname);
	if (ifs == NULL)
		return (ENOENT);

	if (sc->sc_if.if_mtu != ifs->if_mtu)
		return (EINVAL);

	if (ifs->if_bridge == sc)
		return (EEXIST);

	if (ifs->if_bridge != NULL)
		return (EBUSY);

	bif = malloc(sizeof(*bif), M_DEVBUF, M_NOWAIT);
	if (bif == NULL)
		return (ENOMEM);

	switch (ifs->if_type) {
	case IFT_ETHER:
		/*
		 * Place the interface into promiscuous mode.
		 */
		error = ifpromisc(ifs, 1);
		if (error)
			goto out;
		break;

	default:
		error = EINVAL;
		goto out;
	}

	bif->bif_ifp = ifs;
	bif->bif_flags = IFBIF_LEARNING | IFBIF_DISCOVER;
	bif->bif_priority = BSTP_DEFAULT_PORT_PRIORITY;
	bif->bif_path_cost = BSTP_DEFAULT_PATH_COST;

	ifs->if_bridge = sc;
	LIST_INSERT_HEAD(&sc->sc_iflist, bif, bif_next);

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);
	else
		bstp_stop(sc);

 out:
	if (error) {
		if (bif != NULL)
			free(bif, M_DEVBUF);
	}
	return (error);
}

int
bridge_ioctl_del(struct bridge_softc *sc, void *arg)
{
	struct ifbreq *req = arg;
	struct bridge_iflist *bif;

	bif = bridge_lookup_member(sc, req->ifbr_ifsname);
	if (bif == NULL)
		return (ENOENT);

	bridge_delete_member(sc, bif);

	return (0);
}

int
bridge_ioctl_gifflags(struct bridge_softc *sc, void *arg)
{
	struct ifbreq *req = arg;
	struct bridge_iflist *bif;

	bif = bridge_lookup_member(sc, req->ifbr_ifsname);
	if (bif == NULL)
		return (ENOENT);

	req->ifbr_ifsflags = bif->bif_flags;
	req->ifbr_state = bif->bif_state;
	req->ifbr_priority = bif->bif_priority;
	req->ifbr_path_cost = bif->bif_path_cost;
	req->ifbr_portno = bif->bif_ifp->if_index & 0xff;

	return (0);
}

int
bridge_ioctl_sifflags(struct bridge_softc *sc, void *arg)
{
	struct ifbreq *req = arg;
	struct bridge_iflist *bif;

	bif = bridge_lookup_member(sc, req->ifbr_ifsname);
	if (bif == NULL)
		return (ENOENT);

	if (req->ifbr_ifsflags & IFBIF_STP) {
		switch (bif->bif_ifp->if_type) {
		case IFT_ETHER:
			/* These can do spanning tree. */
			break;

		default:
			/* Nothing else can. */
			return (EINVAL);
		}
	}

	bif->bif_flags = req->ifbr_ifsflags;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);

	return (0);
}

int
bridge_ioctl_scache(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	sc->sc_brtmax = param->ifbrp_csize;
	bridge_rttrim(sc);

	return (0);
}

int
bridge_ioctl_gcache(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	param->ifbrp_csize = sc->sc_brtmax;

	return (0);
}

int
bridge_ioctl_gifs(struct bridge_softc *sc, void *arg)
{
	struct ifbifconf *bifc = arg;
	struct bridge_iflist *bif;
	struct ifbreq breq;
	int count, len, error = 0;

	count = 0;
	LIST_FOREACH(bif, &sc->sc_iflist, bif_next)
		count++;

	if (bifc->ifbic_len == 0) {
		bifc->ifbic_len = sizeof(breq) * count;
		return (0);
	}

	count = 0;
	len = bifc->ifbic_len;
	memset(&breq, 0, sizeof breq);
	LIST_FOREACH(bif, &sc->sc_iflist, bif_next) {
		if (len < sizeof(breq))
			break;

		strlcpy(breq.ifbr_ifsname, bif->bif_ifp->if_xname,
		    sizeof(breq.ifbr_ifsname));
		breq.ifbr_ifsflags = bif->bif_flags;
		breq.ifbr_state = bif->bif_state;
		breq.ifbr_priority = bif->bif_priority;
		breq.ifbr_path_cost = bif->bif_path_cost;
		breq.ifbr_portno = bif->bif_ifp->if_index & 0xff;
		error = copyout(&breq, bifc->ifbic_req + count, sizeof(breq));
		if (error)
			break;
		count++;
		len -= sizeof(breq);
	}

	bifc->ifbic_len = sizeof(breq) * count;
	return (error);
}

int
bridge_ioctl_rts(struct bridge_softc *sc, void *arg)
{
	struct ifbaconf *bac = arg;
	struct bridge_rtnode *brt;
	struct ifbareq bareq;
	int count = 0, error = 0, len;

	if (bac->ifbac_len == 0)
		return (0);

	len = bac->ifbac_len;
	LIST_FOREACH(brt, &sc->sc_rtlist, brt_list) {
		if (len < sizeof(bareq))
			goto out;
		memset(&bareq, 0, sizeof(bareq));
		strlcpy(bareq.ifba_ifsname, brt->brt_ifp->if_xname,
		    sizeof(bareq.ifba_ifsname));
		memcpy(bareq.ifba_dst, brt->brt_addr, sizeof(brt->brt_addr));
		if ((brt->brt_flags & IFBAF_TYPEMASK) == IFBAF_DYNAMIC)
			bareq.ifba_expire = brt->brt_expire - mono_time.tv_sec;
		else
			bareq.ifba_expire = 0;
		bareq.ifba_flags = brt->brt_flags;

		error = copyout(&bareq, bac->ifbac_req + count, sizeof(bareq));
		if (error)
			goto out;
		count++;
		len -= sizeof(bareq);
	}
 out:
	bac->ifbac_len = sizeof(bareq) * count;
	return (error);
}

int
bridge_ioctl_saddr(struct bridge_softc *sc, void *arg)
{
	struct ifbareq *req = arg;
	struct bridge_iflist *bif;
	int error;

	bif = bridge_lookup_member(sc, req->ifba_ifsname);
	if (bif == NULL)
		return (ENOENT);

	error = bridge_rtupdate(sc, req->ifba_dst, bif->bif_ifp, 1,
	    req->ifba_flags);

	return (error);
}

int
bridge_ioctl_sto(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	sc->sc_brttimeout = param->ifbrp_ctime;

	return (0);
}

int
bridge_ioctl_gto(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	param->ifbrp_ctime = sc->sc_brttimeout;

	return (0);
}

int
bridge_ioctl_daddr(struct bridge_softc *sc, void *arg)
{
	struct ifbareq *req = arg;

	return (bridge_rtdaddr(sc, req->ifba_dst));
}

int
bridge_ioctl_flush(struct bridge_softc *sc, void *arg)
{
	struct ifbreq *req = arg;

	bridge_rtflush(sc, req->ifbr_ifsflags);

	return (0);
}

int
bridge_ioctl_gpri(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	param->ifbrp_prio = sc->sc_bridge_priority;

	return (0);
}

int
bridge_ioctl_spri(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	sc->sc_bridge_priority = param->ifbrp_prio;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);

	return (0);
}

int
bridge_ioctl_ght(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	param->ifbrp_hellotime = sc->sc_bridge_hello_time >> 8;

	return (0);
}

int
bridge_ioctl_sht(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	if (param->ifbrp_hellotime == 0)
		return (EINVAL);
	sc->sc_bridge_hello_time = param->ifbrp_hellotime << 8;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);

	return (0);
}

int
bridge_ioctl_gfd(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	param->ifbrp_fwddelay = sc->sc_bridge_forward_delay >> 8;

	return (0);
}

int
bridge_ioctl_sfd(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg; 

	if (param->ifbrp_fwddelay == 0)
		return (EINVAL);
	sc->sc_bridge_forward_delay = param->ifbrp_fwddelay << 8;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);

	return (0);
}

int
bridge_ioctl_gma(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	param->ifbrp_maxage = sc->sc_bridge_max_age >> 8;

	return (0);
}

int
bridge_ioctl_sma(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	if (param->ifbrp_maxage == 0)
		return (EINVAL);
	sc->sc_bridge_max_age = param->ifbrp_maxage << 8;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);

	return (0);
}

int
bridge_ioctl_sifprio(struct bridge_softc *sc, void *arg)
{
	struct ifbreq *req = arg;
	struct bridge_iflist *bif;

	bif = bridge_lookup_member(sc, req->ifbr_ifsname);
	if (bif == NULL)
		return (ENOENT);

	bif->bif_priority = req->ifbr_priority;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);

	return (0);
}


int
bridge_ioctl_gfilt(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg;

	param->ifbrp_filter = sc->sc_filter_flags;

	return (0);
}

int
bridge_ioctl_sfilt(struct bridge_softc *sc, void *arg)
{
	struct ifbrparam *param = arg; 
	uint32_t nflags, oflags;

	if (param->ifbrp_filter & ~IFBF_FILT_MASK)
		return (EINVAL);

	nflags = param->ifbrp_filter;
	oflags = sc->sc_filter_flags;

	if ((nflags & IFBF_FILT_USEIPF) && !(oflags & IFBF_FILT_USEIPF)) {
		pfil_add_hook((void *)bridge_ipf, NULL, PFIL_IN|PFIL_OUT,
			&sc->sc_if.if_pfil);
	}
	if (!(nflags & IFBF_FILT_USEIPF) && (oflags & IFBF_FILT_USEIPF)) {
		pfil_remove_hook((void *)bridge_ipf, NULL, PFIL_IN|PFIL_OUT,
			&sc->sc_if.if_pfil);
	}

	sc->sc_filter_flags = nflags;

	return (0);
}

int
bridge_ioctl_sifcost(struct bridge_softc *sc, void *arg)
{
	struct ifbreq *req = arg;
	struct bridge_iflist *bif;

	bif = bridge_lookup_member(sc, req->ifbr_ifsname);
	if (bif == NULL)
		return (ENOENT);

	bif->bif_path_cost = req->ifbr_path_cost;

	if (sc->sc_if.if_flags & IFF_RUNNING)
		bstp_initialization(sc);

	return (0);
}

/*
 * bridge_ifdetach:
 *
 *	Detach an interface from a bridge.  Called when a member
 *	interface is detaching.
 */
void
bridge_ifdetach(struct ifnet *ifp)
{
	struct bridge_softc *sc = ifp->if_bridge;
	struct ifbreq breq;

	memset(&breq, 0, sizeof(breq));
	sprintf(breq.ifbr_ifsname, ifp->if_xname);

	(void) bridge_ioctl_del(sc, &breq);
}

/*
 * bridge_init:
 *
 *	Initialize a bridge interface.
 */
int
bridge_init(struct ifnet *ifp)
{
	struct bridge_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_RUNNING)
		return (0);

	callout_reset(&sc->sc_brcallout, bridge_rtable_prune_period * hz,
	    bridge_timer, sc);

	ifp->if_flags |= IFF_RUNNING;
	bstp_initialization(sc);
	return (0);
}

/*
 * bridge_stop:
 *
 *	Stop the bridge interface.
 */
void
bridge_stop(struct ifnet *ifp, int disable)
{
	struct bridge_softc *sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	callout_stop(&sc->sc_brcallout);
	bstp_stop(sc);

	IF_PURGE(&ifp->if_snd);

	bridge_rtflush(sc, IFBF_FLUSHDYN);

	ifp->if_flags &= ~IFF_RUNNING;
}

/*
 * bridge_enqueue:
 *
 *	Enqueue a packet on a bridge member interface.
 *
 *	NOTE: must be called at splnet().
 */
__inline void
bridge_enqueue(struct bridge_softc *sc, struct ifnet *dst_ifp, struct mbuf *m,
    int runfilt)
{
	ALTQ_DECL(struct altq_pktattr pktattr;)
	int len, error;
	short mflags;

	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m->m_pkthdr.csum_flags = 0;

#ifdef PFIL_HOOKS
	if (runfilt) {
		if (pfil_run_hooks(&sc->sc_if.if_pfil, &m,
		    dst_ifp, PFIL_OUT) != 0) {
			if (m != NULL)
				m_freem(m);
			return;
		}
		if (m == NULL)
			return;
	}
#endif /* PFIL_HOOKS */

#ifdef ALTQ
	/*
	 * If ALTQ is enabled on the member interface, do
	 * classification; the queueing discipline might
	 * not require classification, but might require
	 * the address family/header pointer in the pktattr.
	 */
	if (ALTQ_IS_ENABLED(&dst_ifp->if_snd)) {
		/* XXX IFT_ETHER */
		altq_etherclassify(&dst_ifp->if_snd, m, &pktattr);
	}
#endif /* ALTQ */

	len = m->m_pkthdr.len;
	mflags = m->m_flags;
	IFQ_ENQUEUE(&dst_ifp->if_snd, m, &pktattr, error);
	if (error) {
		/* mbuf is already freed */
		sc->sc_if.if_oerrors++;
		return;
	}

	sc->sc_if.if_opackets++;
	sc->sc_if.if_obytes += len;

	dst_ifp->if_obytes += len;

	if (mflags & M_MCAST) {
		sc->sc_if.if_omcasts++;
		dst_ifp->if_omcasts++;
	}

	if ((dst_ifp->if_flags & IFF_OACTIVE) == 0)
		(*dst_ifp->if_start)(dst_ifp);
}

/*
 * bridge_output:
 *
 *	Send output from a bridge member interface.  This
 *	performs the bridging function for locally originated
 *	packets.
 *
 *	The mbuf has the Ethernet header already attached.  We must
 *	enqueue or free the mbuf before returning.
 */
int
bridge_output(struct ifnet *ifp, struct mbuf *m, struct sockaddr *sa,
    struct rtentry *rt)
{
	struct ether_header *eh;
	struct ifnet *dst_if;
	struct bridge_softc *sc;
	int s;

	if (m->m_len < ETHER_HDR_LEN) {
		m = m_pullup(m, ETHER_HDR_LEN);
		if (m == NULL)
			return (0);
	}

	eh = mtod(m, struct ether_header *);
	sc = ifp->if_bridge;

	s = splnet();

	/*
	 * If bridge is down, but the original output interface is up,
	 * go ahead and send out that interface.  Otherwise, the packet
	 * is dropped below.
	 */
	if ((sc->sc_if.if_flags & IFF_RUNNING) == 0) {
		dst_if = ifp;
		goto sendunicast;
	}

	/*
	 * If the packet is a multicast, or we don't know a better way to
	 * get there, send to all interfaces.
	 */
	if (ETHER_IS_MULTICAST(eh->ether_dhost))
		dst_if = NULL;
	else
		dst_if = bridge_rtlookup(sc, eh->ether_dhost);
	if (dst_if == NULL) {
		struct bridge_iflist *bif;
		struct mbuf *mc;
		int used = 0;

		LIST_FOREACH(bif, &sc->sc_iflist, bif_next) {
			dst_if = bif->bif_ifp;
			if ((dst_if->if_flags & IFF_RUNNING) == 0)
				continue;

			/*
			 * If this is not the original output interface,
			 * and the interface is participating in spanning
			 * tree, make sure the port is in a state that
			 * allows forwarding.
			 */
			if (dst_if != ifp &&
			    (bif->bif_flags & IFBIF_STP) != 0) {
				switch (bif->bif_state) {
				case BSTP_IFSTATE_BLOCKING:
				case BSTP_IFSTATE_LISTENING:
				case BSTP_IFSTATE_DISABLED:
					continue;
				}
			}

			if (LIST_NEXT(bif, bif_next) == NULL) {
				used = 1;
				mc = m;
			} else {
				mc = m_copy(m, 0, M_COPYALL);
				if (mc == NULL) {
					sc->sc_if.if_oerrors++;
					continue;
				}
			}

			bridge_enqueue(sc, dst_if, mc, 0);
		}
		if (used == 0)
			m_freem(m);
		splx(s);
		return (0);
	}

 sendunicast:
	/*
	 * XXX Spanning tree consideration here?
	 */

	if ((dst_if->if_flags & IFF_RUNNING) == 0) {
		m_freem(m);
		splx(s);
		return (0);
	}

	bridge_enqueue(sc, dst_if, m, 0);

	splx(s);
	return (0);
}

/*
 * bridge_start:
 *
 *	Start output on a bridge.
 *
 *	NOTE: This routine should never be called in this implementation.
 */
void
bridge_start(struct ifnet *ifp)
{

	printf("%s: bridge_start() called\n", ifp->if_xname);
}

/*
 * bridge_forward:
 *
 *	The forwarding function of the bridge.
 */
void
bridge_forward(struct bridge_softc *sc, struct mbuf *m)
{
	struct bridge_iflist *bif;
	struct ifnet *src_if, *dst_if;
	struct ether_header *eh;

	src_if = m->m_pkthdr.rcvif;

	sc->sc_if.if_ipackets++;
	sc->sc_if.if_ibytes += m->m_pkthdr.len;

	/*
	 * Look up the bridge_iflist.
	 */
	bif = bridge_lookup_member_if(sc, src_if);
	if (bif == NULL) {
		/* Interface is not a bridge member (anymore?) */
		m_freem(m);
		return;
	}

	if (bif->bif_flags & IFBIF_STP) {
		switch (bif->bif_state) {
		case BSTP_IFSTATE_BLOCKING:
		case BSTP_IFSTATE_LISTENING:
		case BSTP_IFSTATE_DISABLED:
			m_freem(m);
			return;
		}
	}

	eh = mtod(m, struct ether_header *);

	/*
	 * If the interface is learning, and the source
	 * address is valid and not multicast, record
	 * the address.
	 */
	if ((bif->bif_flags & IFBIF_LEARNING) != 0 &&
	    ETHER_IS_MULTICAST(eh->ether_shost) == 0 &&
	    (eh->ether_shost[0] == 0 &&
	     eh->ether_shost[1] == 0 &&
	     eh->ether_shost[2] == 0 &&
	     eh->ether_shost[3] == 0 &&
	     eh->ether_shost[4] == 0 &&
	     eh->ether_shost[5] == 0) == 0) {
		(void) bridge_rtupdate(sc, eh->ether_shost,
		    src_if, 0, IFBAF_DYNAMIC);
	}

	if ((bif->bif_flags & IFBIF_STP) != 0 &&
	    bif->bif_state == BSTP_IFSTATE_LEARNING) {
		m_freem(m);
		return;
	}

	/*
	 * At this point, the port either doesn't participate
	 * in spanning tree or it is in the forwarding state.
	 */

	/*
	 * If the packet is unicast, destined for someone on
	 * "this" side of the bridge, drop it.
	 */
	if ((m->m_flags & (M_BCAST|M_MCAST)) == 0) {
		dst_if = bridge_rtlookup(sc, eh->ether_dhost);
		if (src_if == dst_if) {
			m_freem(m);
			return;
		}
	} else {
		/* ...forward it to all interfaces. */
		sc->sc_if.if_imcasts++;
		dst_if = NULL;
	}

#ifdef PFIL_HOOKS
	if (pfil_run_hooks(&sc->sc_if.if_pfil, &m,
	    m->m_pkthdr.rcvif, PFIL_IN) != 0) {
		if (m != NULL)
			m_freem(m);
		return;
	}
	if (m == NULL)
		return;
#endif /* PFIL_HOOKS */

	if (dst_if == NULL) {
		bridge_broadcast(sc, src_if, m);
		return;
	}

	/*
	 * At this point, we're dealing with a unicast frame
	 * going to a different interface.
	 */
	if ((dst_if->if_flags & IFF_RUNNING) == 0) {
		m_freem(m);
		return;
	}
	bif = bridge_lookup_member_if(sc, dst_if);
	if (bif == NULL) {
		/* Not a member of the bridge (anymore?) */
		m_freem(m);
		return;
	}

	if (bif->bif_flags & IFBIF_STP) {
		switch (bif->bif_state) {
		case BSTP_IFSTATE_DISABLED:
		case BSTP_IFSTATE_BLOCKING:
			m_freem(m);
			return;
		}
	}

	bridge_enqueue(sc, dst_if, m, 1);
}

/*
 * bridge_input:
 *
 *	Receive input from a member interface.  Queue the packet for
 *	bridging if it is not for us.
 */
struct mbuf *
bridge_input(struct ifnet *ifp, struct mbuf *m)
{
	struct bridge_softc *sc = ifp->if_bridge;
	struct bridge_iflist *bif;
	struct ether_header *eh;
	struct mbuf *mc;

	if ((sc->sc_if.if_flags & IFF_RUNNING) == 0)
		return (m);

	bif = bridge_lookup_member_if(sc, ifp);
	if (bif == NULL)
		return (m);

	eh = mtod(m, struct ether_header *);

	if (m->m_flags & (M_BCAST|M_MCAST)) {
		/* Tap off 802.1D packets; they do not get forwarded. */
		if (memcmp(eh->ether_dhost, bstp_etheraddr,
		    ETHER_ADDR_LEN) == 0) {
			m = bstp_input(ifp, m);
			if (m == NULL)
				return (NULL);
		}

		if (bif->bif_flags & IFBIF_STP) {
			switch (bif->bif_state) {
			case BSTP_IFSTATE_BLOCKING:
			case BSTP_IFSTATE_LISTENING:
			case BSTP_IFSTATE_DISABLED:
				return (m);
			}
		}

		/*
		 * Make a deep copy of the packet and enqueue the copy
		 * for bridge processing; return the original packet for
		 * local processing.
		 */
		mc = m_copy(m, 0, M_COPYALL);
		if (mc == NULL)
			return (m);

		/* Perform the bridge forwarding function with the copy. */
		bridge_forward(sc, mc);

		/* Return the original packet for local processing. */
		return (m);
	}

	if (bif->bif_flags & IFBIF_STP) {
		switch (bif->bif_state) {
		case BSTP_IFSTATE_BLOCKING:
		case BSTP_IFSTATE_LISTENING:
		case BSTP_IFSTATE_DISABLED:
			return (m);
		}
	}

	/*
	 * Unicast.  Make sure it's not for us.
	 */
	LIST_FOREACH(bif, &sc->sc_iflist, bif_next) {
		/* It is destined for us. */
		if (memcmp(LLADDR(bif->bif_ifp->if_sadl), eh->ether_dhost, ETHER_ADDR_LEN) == 0
#if NCARP > 0
		    || (bif->bif_ifp->if_carp && carp_ourether(bif->bif_ifp->if_carp, eh, IFT_ETHER, 0) != NULL)
#endif /* NCARP > 0 */
				) {
			if (bif->bif_flags & IFBIF_LEARNING) {
				(void) bridge_rtupdate(sc, eh->ether_shost, ifp, 0, IFBAF_DYNAMIC);
			}
			m->m_pkthdr.rcvif = bif->bif_ifp;
			m->m_flags &= ~M_PROMISC;
			return (m);
		}
		/* We just received a packet that we sent out. */
		if (memcmp(LLADDR(bif->bif_ifp->if_sadl), eh->ether_shost, ETHER_ADDR_LEN) == 0
#if NCARP > 0
		    || (bif->bif_ifp->if_carp && carp_ourether(bif->bif_ifp->if_carp, eh, IFT_ETHER, 1) != NULL)
#endif /* NCARP > 0 */
				) {
			m_freem(m);
			return (NULL);
		}
	}

	/* Perform the bridge forwarding function. */
	bridge_forward(sc, m);

	return (NULL);
}

/*
 * bridge_broadcast:
 *
 *	Send a frame to all interfaces that are members of
 *	the bridge, except for the one on which the packet
 *	arrived.
 */
void
bridge_broadcast(struct bridge_softc *sc, struct ifnet *src_if,
    struct mbuf *m)
{
	struct bridge_iflist *bif;
	struct mbuf *mc;
	struct ifnet *dst_if;
	int used = 0;

	LIST_FOREACH(bif, &sc->sc_iflist, bif_next) {
		dst_if = bif->bif_ifp;
		if (dst_if == src_if)
			continue;

		if (bif->bif_flags & IFBIF_STP) {
			switch (bif->bif_state) {
			case BSTP_IFSTATE_BLOCKING:
			case BSTP_IFSTATE_DISABLED:
				continue;
			}
		}

		if ((bif->bif_flags & IFBIF_DISCOVER) == 0 &&
		    (m->m_flags & (M_BCAST|M_MCAST)) == 0)
			continue;

		if ((dst_if->if_flags & IFF_RUNNING) == 0)
			continue;

		if (LIST_NEXT(bif, bif_next) == NULL) {
			mc = m;
			used = 1;
		} else {
			mc = m_copy(m, 0, M_COPYALL);
			if (mc == NULL) {
				sc->sc_if.if_oerrors++;
				continue;
			}
		}

		bridge_enqueue(sc, dst_if, mc, 1);
	}
	if (used == 0)
		m_freem(m);
}

/*
 * bridge_rtupdate:
 *
 *	Add a bridge routing entry.
 */
int
bridge_rtupdate(struct bridge_softc *sc, const uint8_t *dst,
    struct ifnet *dst_if, int setflags, uint8_t flags)
{
	struct bridge_rtnode *brt;
	int error;

	/*
	 * A route for this destination might already exist.  If so,
	 * update it, otherwise create a new one.
	 */
	if ((brt = bridge_rtnode_lookup(sc, dst)) == NULL) {
		if (sc->sc_brtcnt >= sc->sc_brtmax)
			return (ENOSPC);

		/*
		 * Allocate a new bridge forwarding node, and
		 * initialize the expiration time and Ethernet
		 * address.
		 */
		R_Malloc(brt, struct bridge_rtnode *, sizeof(*brt));
		if (brt == NULL)
			return (ENOMEM);

		memset(brt, 0, sizeof(*brt));
		brt->brt_expire = mono_time.tv_sec + sc->sc_brttimeout;
		brt->brt_flags = IFBAF_DYNAMIC;
		memcpy(brt->brt_addr, dst, ETHER_ADDR_LEN);

		if ((error = bridge_rtnode_insert(sc, brt)) != 0) {
			Free(brt);
			return (error);
		}
	}

	brt->brt_ifp = dst_if;
	if (setflags) {
		brt->brt_flags = flags;
		brt->brt_expire = (flags & IFBAF_STATIC) ? 0 :
		    mono_time.tv_sec + sc->sc_brttimeout;
	}

	return (0);
}

/*
 * bridge_rtlookup:
 *
 *	Lookup the destination interface for an address.
 */
struct ifnet *
bridge_rtlookup(struct bridge_softc *sc, const uint8_t *addr)
{
	struct bridge_rtnode *brt;

	if ((brt = bridge_rtnode_lookup(sc, addr)) == NULL)
		return (NULL);

	return (brt->brt_ifp);
}

/*
 * bridge_rttrim:
 *
 *	Trim the routine table so that we have a number
 *	of routing entries less than or equal to the
 *	maximum number.
 */
void
bridge_rttrim(struct bridge_softc *sc)
{
	struct bridge_rtnode *brt, *nbrt;

	/* Make sure we actually need to do this. */
	if (sc->sc_brtcnt <= sc->sc_brtmax)
		return;

	/* Force an aging cycle; this might trim enough addresses. */
	bridge_rtage(sc);
	if (sc->sc_brtcnt <= sc->sc_brtmax)
		return;

	for (brt = LIST_FIRST(&sc->sc_rtlist); brt != NULL; brt = nbrt) {
		nbrt = LIST_NEXT(brt, brt_list);
		if ((brt->brt_flags & IFBAF_TYPEMASK) == IFBAF_DYNAMIC) {
			bridge_rtnode_destroy(sc, brt);
			if (sc->sc_brtcnt <= sc->sc_brtmax)
				return;
		}
	}
}

/*
 * bridge_timer:
 *
 *	Aging timer for the bridge.
 */
void
bridge_timer(void *arg)
{
	struct bridge_softc *sc = arg;
	int s;

	s = splnet();
	bridge_rtage(sc);
	splx(s);

	if (sc->sc_if.if_flags & IFF_RUNNING)
		callout_reset(&sc->sc_brcallout,
		    bridge_rtable_prune_period * hz, bridge_timer, sc);
}

/*
 * bridge_rtage:
 *
 *	Perform an aging cycle.
 */
void
bridge_rtage(struct bridge_softc *sc)
{
	struct bridge_rtnode *brt, *nbrt;

	for (brt = LIST_FIRST(&sc->sc_rtlist); brt != NULL; brt = nbrt) {
		nbrt = LIST_NEXT(brt, brt_list);
		if ((brt->brt_flags & IFBAF_TYPEMASK) == IFBAF_DYNAMIC) {
			if (mono_time.tv_sec >= brt->brt_expire)
				bridge_rtnode_destroy(sc, brt);
		}
	}
}

/*
 * bridge_rtflush:
 *
 *	Remove all dynamic addresses from the bridge.
 */
void
bridge_rtflush(struct bridge_softc *sc, int full)
{
	struct bridge_rtnode *brt, *nbrt;

	for (brt = LIST_FIRST(&sc->sc_rtlist); brt != NULL; brt = nbrt) {
		nbrt = LIST_NEXT(brt, brt_list);
		if (full || (brt->brt_flags & IFBAF_TYPEMASK) == IFBAF_DYNAMIC)
			bridge_rtnode_destroy(sc, brt);
	}
}

/*
 * bridge_rtdaddr:
 *
 *	Remove an address from the table.
 */
int
bridge_rtdaddr(struct bridge_softc *sc, const uint8_t *addr)
{
	struct bridge_rtnode *brt;

	if ((brt = bridge_rtnode_lookup(sc, addr)) == NULL)
		return (ENOENT);

	bridge_rtnode_destroy(sc, brt);
	return (0);
}

/*
 * bridge_rtdelete:
 *
 *	Delete routes to a speicifc member interface.
 */
void
bridge_rtdelete(struct bridge_softc *sc, struct ifnet *ifp)
{
	struct bridge_rtnode *brt, *nbrt;

	for (brt = LIST_FIRST(&sc->sc_rtlist); brt != NULL; brt = nbrt) {
		nbrt = LIST_NEXT(brt, brt_list);
		if (brt->brt_ifp == ifp)
			bridge_rtnode_destroy(sc, brt);
	}
}

/*
 * bridge_rtable_init:
 *
 *	Initialize the route table for this bridge.
 */
int
bridge_rtable_init(struct bridge_softc *sc)
{
	int i;

	sc->sc_rthash = malloc(sizeof(*sc->sc_rthash) * BRIDGE_RTHASH_SIZE,
	    M_DEVBUF, M_NOWAIT);
	if (sc->sc_rthash == NULL)
		return (ENOMEM);

	for (i = 0; i < BRIDGE_RTHASH_SIZE; i++)
		LIST_INIT(&sc->sc_rthash[i]);

	sc->sc_rthash_key = arc4random();

	LIST_INIT(&sc->sc_rtlist);

	return (0);
}

/*
 * bridge_rtable_fini:
 *
 *	Deconstruct the route table for this bridge.
 */
void
bridge_rtable_fini(struct bridge_softc *sc)
{

	free(sc->sc_rthash, M_DEVBUF);
}

/*
 * The following hash function is adapted from "Hash Functions" by Bob Jenkins
 * ("Algorithm Alley", Dr. Dobbs Journal, September 1997).
 */
#define	mix(a, b, c)							\
do {									\
	a -= b; a -= c; a ^= (c >> 13);					\
	b -= c; b -= a; b ^= (a << 8);					\
	c -= a; c -= b; c ^= (b >> 13);					\
	a -= b; a -= c; a ^= (c >> 12);					\
	b -= c; b -= a; b ^= (a << 16);					\
	c -= a; c -= b; c ^= (b >> 5);					\
	a -= b; a -= c; a ^= (c >> 3);					\
	b -= c; b -= a; b ^= (a << 10);					\
	c -= a; c -= b; c ^= (b >> 15);					\
} while (/*CONSTCOND*/0)

static __inline uint32_t
bridge_rthash(struct bridge_softc *sc, const uint8_t *addr)
{
	uint32_t a = 0x9e3779b9, b = 0x9e3779b9, c = sc->sc_rthash_key;

	b += addr[5] << 8;
	b += addr[4];
	a += addr[3] << 24;
	a += addr[2] << 16;
	a += addr[1] << 8;
	a += addr[0];

	mix(a, b, c);

	return (c & BRIDGE_RTHASH_MASK);
}

#undef mix

/*
 * bridge_rtnode_lookup:
 *
 *	Look up a bridge route node for the specified destination.
 */
struct bridge_rtnode *
bridge_rtnode_lookup(struct bridge_softc *sc, const uint8_t *addr)
{
	struct bridge_rtnode *brt;
	uint32_t hash;
	int dir;

	hash = bridge_rthash(sc, addr);
	LIST_FOREACH(brt, &sc->sc_rthash[hash], brt_hash) {
		dir = memcmp(addr, brt->brt_addr, ETHER_ADDR_LEN);
		if (dir == 0)
			return (brt);
		if (dir > 0)
			return (NULL);
	}

	return (NULL);
}

/*
 * bridge_rtnode_insert:
 *
 *	Insert the specified bridge node into the route table.  We
 *	assume the entry is not already in the table.
 */
int
bridge_rtnode_insert(struct bridge_softc *sc, struct bridge_rtnode *brt)
{
	struct bridge_rtnode *lbrt;
	uint32_t hash;
	int dir;

	hash = bridge_rthash(sc, brt->brt_addr);

	lbrt = LIST_FIRST(&sc->sc_rthash[hash]);
	if (lbrt == NULL) {
		LIST_INSERT_HEAD(&sc->sc_rthash[hash], brt, brt_hash);
		goto out;
	}

	do {
		dir = memcmp(brt->brt_addr, lbrt->brt_addr, ETHER_ADDR_LEN);
		if (dir == 0)
			return (EEXIST);
		if (dir > 0) {
			LIST_INSERT_BEFORE(lbrt, brt, brt_hash);
			goto out;
		}
		if (LIST_NEXT(lbrt, brt_hash) == NULL) {
			LIST_INSERT_AFTER(lbrt, brt, brt_hash);
			goto out;
		}
		lbrt = LIST_NEXT(lbrt, brt_hash);
	} while (lbrt != NULL);

#ifdef DIAGNOSTIC
	panic("bridge_rtnode_insert: impossible");
#endif

 out:
	LIST_INSERT_HEAD(&sc->sc_rtlist, brt, brt_list);
	sc->sc_brtcnt++;

	return (0);
}

/*
 * bridge_rtnode_destroy:
 *
 *	Destroy a bridge rtnode.
 */
void
bridge_rtnode_destroy(struct bridge_softc *sc, struct bridge_rtnode *brt)
{

	LIST_REMOVE(brt, brt_hash);

	LIST_REMOVE(brt, brt_list);
	sc->sc_brtcnt--;
	Free(brt);
}

extern struct pfil_head inet_pfil_hook;                 /* XXX */
extern struct pfil_head inet6_pfil_hook;                /* XXX */

/*
 * Send bridge packets through IPF if they are one of the types IPF can deal
 * with, or if they are ARP or REVARP.  (IPF will pass ARP and REVARP without
 * question.)
 */
static int
bridge_ipf(void *arg, struct mbuf **mp, struct ifnet *ifp, int dir)
{
	int snap, error;
	struct ether_header *eh1, eh2;
	struct llc llc;
	u_int16_t ether_type;

	snap = 0;
	error = -1;	/* Default error if not error == 0 */
	eh1 = mtod(*mp, struct ether_header *);
	ether_type = ntohs(eh1->ether_type);

	/*
	 * Check for SNAP/LLC.
	 */
	if (ether_type < ETHERMTU) {
		struct llc *llc = (struct llc*) (eh1 + 1);

		if ((*mp)->m_len >= ETHER_HDR_LEN + 8&&
		llc->llc_dsap == LLC_SNAP_LSAP &&
		llc->llc_ssap == LLC_SNAP_LSAP &&
		llc->llc_control == LLC_UI) {
			ether_type = htons(llc->llc_un.type_snap.ether_type);
			snap = 1;
		}
	}

	/*
	 * If we're trying to filter bridge traffic, don't look at anything
	 * other than IP and ARP traffic.  If the filter doesn't understand
	 * IPv6, don't allow IPv6 through the bridge either.  This is lame
	 * since if we really wanted, say, an AppleTalk filter, we are hosed,
	 * but of course we don't have an AppleTalk filter to begin with.
	 * (Note that since IPF doesn't understand ARP it will pass *ALL*
	 * ARP traffic.)
	 */
	switch (ether_type) {
	case ETHERTYPE_ARP:
	case ETHERTYPE_REVARP:
		return 0; /* Automatically pass */
	case ETHERTYPE_IP:
# ifdef INET6
		case ETHERTYPE_IPV6:
# endif /* INET6 */
		break;
	default:
		goto bad;
	}

	/* Strip off the Ethernet header and keep a copy. */
	m_copydata(*mp, 0, ETHER_HDR_LEN, (caddr_t) &eh2);
	m_adj(*mp, ETHER_HDR_LEN);

	/* Strip off snap header, if present */
	if (snap) {
		m_copydata(*mp, 0, sizeof(struct llc), (caddr_t) &llc);
		m_adj(*mp, sizeof(struct llc));
	}

	/*
	 * Check basic packet sanity and run IPF through pfil.
	 */
	switch (ether_type) {
	case ETHERTYPE_IP:
		error = (dir == PFIL_IN) ? bridge_ip_checkbasic(mp) : 0;
		if (error == 0)
			error = pfil_run_hooks(&inet_pfil_hook, mp, ifp, dir);
		break;
# ifdef INET6
	case ETHERTYPE_IPV6 :
		error = (dir == PFIL_IN) ? bridge_ip6_checkbasic(mp) : 0;
		if (error == 0)
			error = pfil_run_hooks(&inet6_pfil_hook, mp, ifp, dir);
		break;
#endif
	default:
		error = 0;
		break;
	}

	if (*mp == NULL)
		return error;
	if (error != 0)
		goto bad;

	error = -1;

	/*
	 * Finally, put everything back the way it was and return
	 */
	if (snap) {
		M_PREPEND(*mp, sizeof(struct llc), M_DONTWAIT);
		if (*mp == NULL)
			return error;
		bcopy(&llc, mtod(*mp, caddr_t), sizeof(struct llc));
	}

	M_PREPEND(*mp, ETHER_HDR_LEN, M_DONTWAIT);
	if (*mp == NULL)
		return error;
	bcopy(&eh2, mtod(*mp, caddr_t), ETHER_HDR_LEN);

	return 0;

    bad:
	m_freem(*mp);
	*mp = NULL;
	return error;
}

/*
 * Perform basic checks on header size since
 * IPF assumes ip_input has already processed
 * it for it.  Cut-and-pasted from ip_input.c.
 * Given how simple the IPv6 version is,
 * does the IPv4 version really need to be
 * this complicated?
 *
 * XXX Should we update ipstat here, or not?
 * XXX Right now we update ipstat but not
 * XXX csum_counter.
 */
static int
bridge_ip_checkbasic(struct mbuf **mp)
{
	struct mbuf *m = *mp;
	struct ip *ip;
	int len, hlen;

	if (*mp == NULL)
		return -1;

	if (IP_HDR_ALIGNED_P(mtod(m, caddr_t)) == 0) {
		if ((m = m_copyup(m, sizeof(struct ip), (max_linkhdr + 3) & ~3)) == NULL) {
			/* XXXJRT new stat, please */
			ipstat.ips_toosmall++;
			goto bad;
		}
	} else if (__predict_false(m->m_len < sizeof(struct ip))) {
		if ((m = m_pullup(m, sizeof(struct ip))) == NULL) {
			ipstat.ips_toosmall++;
			goto bad;
		}
	}
	ip = mtod(m, struct ip*);
	if (ip == NULL)
		goto bad;

	if (ip->ip_v != IPVERSION) {
		ipstat.ips_badvers++;
		goto bad;
	}
	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(struct ip)) { /* minimum header length */
		ipstat.ips_badhlen++;
		goto bad;
	}
	if (hlen > m->m_len) {
		if ((m = m_pullup(m, hlen)) == 0) {
			ipstat.ips_badhlen++;
			goto bad;
		}
		ip = mtod(m, struct ip*);
		if (ip == NULL)
			goto bad;
	}

	switch (m->m_pkthdr.csum_flags
			& ((m->m_pkthdr.rcvif->if_csum_flags_rx & M_CSUM_IPv4) |
			M_CSUM_IPv4_BAD)) {
	case M_CSUM_IPv4 | M_CSUM_IPv4_BAD:
		/* INET_CSUM_COUNTER_INCR(&ip_hwcsum_bad); */
		goto bad;

	case M_CSUM_IPv4:
		/* Checksum was okay. */
		/* INET_CSUM_COUNTER_INCR(&ip_hwcsum_ok); */
		break;

	default:
		/* Must compute it ourselves. */
		/* INET_CSUM_COUNTER_INCR(&ip_swcsum); */
		if (in_cksum(m, hlen) != 0)
			goto bad;
		break;
	}

	/* Retrieve the packet length. */
	len = ntohs(ip->ip_len);

	/*
	 * Check for additional length bogosity
	 */
	if (len < hlen) {
		ipstat.ips_badlen++;
		goto bad;
	}

	/*
	 * Check that the amount of data in the buffers
	 * is as at least much as the IP header would have us expect.
	 * Drop packet if shorter than we expect.
	 */
	if (m->m_pkthdr.len < len) {
		ipstat.ips_tooshort++;
		goto bad;
	}

	/* Checks out, proceed */
	*mp = m;
	return 0;

    bad:
	*mp = m;
	return -1;
}

# ifdef INET6
/*
 * Same as above, but for IPv6.
 * Cut-and-pasted from ip6_input.c.
 * XXX Should we update ip6stat, or not?
 */
static int
bridge_ip6_checkbasic(struct mbuf **mp)
{
	struct mbuf *m = *mp;
	struct ip6_hdr *ip6;

	/*
	 * If the IPv6 header is not aligned, slurp it up into a new
	 * mbuf with space for link headers, in the event we forward
	 * it.  Otherwise, if it is aligned, make sure the entire base
	 * IPv6 header is in the first mbuf of the chain.
	 */
	if (IP6_HDR_ALIGNED_P(mtod(m, caddr_t)) == 0) {
		struct ifnet *inifp = m->m_pkthdr.rcvif;
		if ((m = m_copyup(m, sizeof(struct ip6_hdr), (max_linkhdr + 3) & ~3))
				== NULL) {
			/* XXXJRT new stat, please */
			ip6stat.ip6s_toosmall++;
			in6_ifstat_inc(inifp, ifs6_in_hdrerr);
			goto bad;
		}
	} else if (__predict_false(m->m_len < sizeof(struct ip6_hdr))) {
		struct ifnet *inifp = m->m_pkthdr.rcvif;
		if ((m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL) {
			ip6stat.ip6s_toosmall++;
			in6_ifstat_inc(inifp, ifs6_in_hdrerr);
			goto bad;
		}
	}

	ip6 = mtod(m, struct ip6_hdr*);

	if ((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION) {
		ip6stat.ip6s_badvers++;
		in6_ifstat_inc(m->m_pkthdr.rcvif, ifs6_in_hdrerr);
		goto bad;
	}

	/* Checks out, proceed */
	*mp = m;
	return 0;

    bad:
	*mp = m;
	return -1;
}
# endif /* INET6 */
