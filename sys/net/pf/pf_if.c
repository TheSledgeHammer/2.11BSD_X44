/*	$NetBSD: pf_if.c,v 1.8 2004/12/04 14:26:01 peter Exp $	*/
/*	$OpenBSD: pf_if.c,v 1.20 2004/08/15 15:31:46 henning Exp $ */

/*
 * Copyright (c) 2001 Daniel Hartmeier
 * Copyright (c) 2003 Cedric Berger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/power.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/time.h>

#include <net/if.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <net/pf/pfvar.h>

#ifdef INET6
#include <netinet/ip6.h>
#endif /* INET6 */

#define ACCEPT_FLAGS(oklist)		\
	do {							\
		if ((flags & ~(oklist)) &	\
		    PFI_FLAG_ALLMASK)		\
			return (EINVAL);		\
	} while (0)

#define senderr(e)      do { rv = (e); goto _bad; } while (0)

struct pfi_kif			**pfi_index2kif;
struct pfi_kif		 	*pfi_self;
int			  			pfi_indexlim;
struct pfi_ifhead	  	pfi_ifs;
struct pfi_statehead	pfi_statehead;
int			  			pfi_ifcnt;
long			  		pfi_update = 1;
struct pfr_addr			*pfi_buffer;
int			  			pfi_buffer_cnt;
int			  			pfi_buffer_max;

void		 pfi_kif_update(struct pfi_kif *);
void		 pfi_dynaddr_update(void *);
void		 pfi_kifaddr_update(void *);
void		 pfi_kifaddr_update_if(struct ifnet *);
void		 pfi_table_update(struct pfr_ktable *, struct pfi_kif *,int, int);
void		 pfi_instance_add(struct ifnet *, int, int);
void		 pfi_address_add(struct sockaddr *, int, int);
int		 	 pfi_if_compare(struct pfi_kif *, struct pfi_kif *);

struct pfi_kif	*pfi_if_create(const char *, struct pfi_kif *, int);
void		 pfi_copy_group(char *, const char *, int);
void		 pfi_newgroup(const char *, int);
int			 pfi_skip_if(const char *, struct pfi_kif *, int);
int			 pfi_unmask(void *);
void		 pfi_dohooks(struct pfi_kif *);

RB_PROTOTYPE(pfi_ifhead, pfi_kif, pfik_tree, pfi_if_compare);
RB_GENERATE(pfi_ifhead, pfi_kif, pfik_tree, pfi_if_compare);

#define PFI_BUFFER_MAX		0x10000
#define PFI_MTYPE			M_IFADDR

void
pfi_initialize(void)
{
	if (pfi_self != NULL) {	/* already initialized */
		return;
	}

	TAILQ_INIT(&pfi_statehead);
	pfi_buffer_max = 64;
	pfi_buffer = malloc(pfi_buffer_max * sizeof(*pfi_buffer), PFI_MTYPE, M_WAITOK);
	pfi_self = pfi_if_create("self", NULL, PFI_IFLAG_GROUP);
}

void
pfi_destroy(void)
{
	free(pfi_buffer, PFI_MTYPE);
}

void
pfi_attach_clone(struct if_clone *ifc)
{
	pfi_initialize();
	pfi_newgroup(ifc->ifc_name, PFI_IFLAG_CLONABLE);
}

void
pfi_attach_ifnet(struct ifnet *ifp)
{
	struct pfi_kif	*p, *q, key;
	int		 s;

	pfi_initialize();
	s = splsoftnet();
	pfi_update++;
	if (ifp->if_index >= pfi_indexlim) {
		/*
		 * grow pfi_index2kif,  similar to ifindex2ifnet code in if.c
		 */
		size_t m, n, oldlim;
		struct pfi_kif **mp, **np;

		oldlim = pfi_indexlim;
		if (pfi_indexlim == 0)
			pfi_indexlim = 64;
		while (ifp->if_index >= pfi_indexlim)
			pfi_indexlim <<= 1;

		m = oldlim * sizeof(struct pfi_kif *);
		mp = pfi_index2kif;
		n = pfi_indexlim * sizeof(struct pfi_kif *);
		np = malloc(n, PFI_MTYPE, M_DONTWAIT);
		if (np == NULL)
			panic("pfi_attach_ifnet: "
			    "cannot allocate translation table");
		bzero(np, n);
		if (mp != NULL)
			bcopy(mp, np, m);
		pfi_index2kif = np;
		if (mp != NULL)
			free(mp, PFI_MTYPE);
	}

	strlcpy(key.pfik_name, ifp->if_xname, sizeof(key.pfik_name));
	p = RB_FIND(pfi_ifhead, &pfi_ifs, &key);
	if (p == NULL) {
		/* add group */
		pfi_copy_group(key.pfik_name, ifp->if_xname, sizeof(key.pfik_name));
		q = RB_FIND(pfi_ifhead, &pfi_ifs, &key);
		if (q == NULL)
		    q = pfi_if_create(key.pfik_name, pfi_self, PFI_IFLAG_GROUP);
		if (q == NULL)
			panic("pfi_attach_ifnet: "
			    "cannot allocate '%s' group", key.pfik_name);

		/* add interface */
		p = pfi_if_create(ifp->if_xname, q, PFI_IFLAG_INSTANCE);
		if (p == NULL) {
			panic("pfi_attach_ifnet: "
			    "cannot allocate '%s' interface", ifp->if_xname);
		}
	} else {
		q = p->pfik_parent;
	}
	p->pfik_ifp = ifp;
	p->pfik_flags |= PFI_IFLAG_ATTACHED;

	p->pfik_ah_cookie = hook_establish(p->pfik_ifaddrhooks, pfi_kifaddr_update, p);

	pfi_index2kif[ifp->if_index] = p;
	pfi_dohooks(p);
	splx(s);
}

void
pfi_detach_ifnet(struct ifnet *ifp)
{
	struct pfi_kif	*p, *q, key;
	int		 s;

	strlcpy(key.pfik_name, ifp->if_xname, sizeof(key.pfik_name));

	s = splsoftnet();
	pfi_update++;
	p = RB_FIND(pfi_ifhead, &pfi_ifs, &key);
	if (p == NULL) {
		printf("pfi_detach_ifnet: cannot find %s", ifp->if_xname);
		splx(s);
		return;
	}

	hook_disestablish(p->pfik_ifaddrhooks, p->pfik_ah_cookie);

	q = p->pfik_parent;
	p->pfik_ifp = NULL;
	p->pfik_flags &= ~PFI_IFLAG_ATTACHED;
	pfi_index2kif[ifp->if_index] = NULL;
	pfi_dohooks(p);
	pfi_maybe_destroy(p);
	splx(s);
}

struct pfi_kif *
pfi_lookup_create(const char *name)
{
	struct pfi_kif	*p, *q, key;
	int		 s;

	s = splsoftnet();
	p = pfi_lookup_if(name);
	if (p == NULL) {
		pfi_copy_group(key.pfik_name, name, sizeof(key.pfik_name));
		q = pfi_lookup_if(key.pfik_name);
		if (q == NULL) {
			pfi_newgroup(key.pfik_name, PFI_IFLAG_DYNAMIC);
			q = pfi_lookup_if(key.pfik_name);
		}
		p = pfi_lookup_if(name);
		if (p == NULL && q != NULL)
			p = pfi_if_create(name, q, PFI_IFLAG_INSTANCE);
	}
	splx(s);
	return (p);
}

struct pfi_kif *
pfi_kif_get(const char *kif_name)
{
	return (pfi_lookup_create(kif_name));
}

void
pfi_kif_ref(struct pfi_kif *kif, enum pfi_kif_refs what)
{
	switch (what) {
	case PFI_KIF_REF_RULE:
		kif->pfik_rules++;
		break;
	case PFI_KIF_REF_STATE:
		if (!kif->pfik_states++) {
			TAILQ_INSERT_TAIL(&pfi_statehead, kif, pfik_w_states);
		}
		kif->pfik_states++;
		break;
	default:
		panic("pfi_kif_ref with unknown type");
	}
}

void
pfi_kif_unref(struct pfi_kif *kif, enum pfi_kif_refs what)
{
	if (kif == NULL)
		return;

	switch (what) {
	case PFI_KIF_REF_NONE:
		break;
	case PFI_KIF_REF_RULE:
		if (kif->pfik_rules <= 0) {
			printf("pfi_kif_unref: rules refcount <= 0\n");
			return;
		}
		kif->pfik_rules--;
		break;
	case PFI_KIF_REF_STATE:
		if (kif->pfik_states <= 0) {
			printf("pfi_kif_unref: state refcount <= 0\n");
			return;
		}
		if (!kif->pfik_states--) {
			TAILQ_REMOVE(&pfi_statehead, kif, pfik_w_states);
		}
		kif->pfik_states--;
		break;
	default:
		panic("pfi_kif_unref with unknown type");
	}

	if (kif->pfik_ifp != NULL || kif->pfik_group != NULL || kif == pfi_self) {
		return;
	}

	if (kif->pfik_rules || kif->pfik_states) {
		return;
	}

	pfi_maybe_destroy(kif);
}

int
pfi_kif_match(struct pfi_kif *rule_kif, struct pfi_kif *packet_kif)
{
	struct ifg_list *p;

	if (rule_kif == NULL || rule_kif == packet_kif) {
		return (1);
	}

	if (rule_kif->pfik_group != NULL) {
		struct ifg_list_head *ifgh = if_get_groups(packet_kif->pfik_ifp);

		TAILQ_FOREACH(p, ifgh, ifgl_next) {
			if (p->ifgl_group == rule_kif->pfik_group) {
				return (1);
			}
		}
	}
	return (0);
}

void
pfi_attach_ifgroup(struct ifg_group *ifg)
{
	struct pfi_kif	*kif;
	int		 s;

	pfi_initialize();
	s = splsoftnet();
	pfi_update++;
	if ((kif = pfi_kif_get(ifg->ifg_group)) == NULL) {
		panic("pfi_kif_get failed");
	}

	kif->pfik_group = ifg;
	ifg->ifg_pf_kif = kif;

	splx(s);
}

void
pfi_detach_ifgroup(struct ifg_group *ifg)
{
	int		 s;
	struct pfi_kif	*kif;

	if ((kif = (struct pfi_kif *)ifg->ifg_pf_kif) == NULL)
		return;

	s = splsoftnet();
	pfi_update++;

	kif->pfik_group = NULL;
	ifg->ifg_pf_kif = NULL;
	pfi_kif_unref(kif, PFI_KIF_REF_NONE);
	splx(s);
}

void
pfi_group_change(const char *group)
{
	struct pfi_kif		*kif;
	int			 s;

	s = splsoftnet();
	pfi_update++;
	if ((kif = pfi_kif_get(group)) == NULL) {
		panic("pfi_kif_get failed");
	}

	pfi_kif_update(kif);

	splx(s);
}

int
pfi_dynaddr_setup(struct pf_addr_wrap *aw, sa_family_t af)
{
	struct pfi_dynaddr	*dyn;
	char			 tblname[PF_TABLE_NAME_SIZE];
	struct pf_ruleset	*ruleset = NULL;
	int			 s, rv = 0;

	if (aw->type != PF_ADDR_DYNIFTL)
		return (0);
	dyn = (struct pfi_dynaddr *)malloc(sizeof(struct pfi_dynaddr *), M_DEVBUF, M_NOWAIT);
	if (dyn == NULL)
		return (1);
	bzero(dyn, sizeof(*dyn));

	s = splsoftnet();
	if (!strcmp(aw->v.ifname, "self")) {
		dyn->pfid_kif = pfi_kif_get(IFG_ALL);
	} else {
		dyn->pfid_kif = pfi_kif_get(aw->v.ifname);
	}
	if (dyn->pfid_kif == NULL) {
		senderr(1);
	}

	dyn->pfid_net = pfi_unmask(&aw->v.a.mask);
	if (af == AF_INET && dyn->pfid_net == 32)
		dyn->pfid_net = 128;
	strlcpy(tblname, aw->v.ifname, sizeof(tblname));
	if (aw->iflags & PFI_AFLAG_NETWORK)
		strlcat(tblname, ":network", sizeof(tblname));
	if (aw->iflags & PFI_AFLAG_BROADCAST)
		strlcat(tblname, ":broadcast", sizeof(tblname));
	if (aw->iflags & PFI_AFLAG_PEER)
		strlcat(tblname, ":peer", sizeof(tblname));
	if (aw->iflags & PFI_AFLAG_NOALIAS)
		strlcat(tblname, ":0", sizeof(tblname));
	if (dyn->pfid_net != 128)
		snprintf(tblname + strlen(tblname),
		    sizeof(tblname) - strlen(tblname), "/%d", dyn->pfid_net);
	ruleset = pf_find_or_create_ruleset(PF_RESERVED_ANCHOR);
	if (ruleset == NULL)
		senderr(1);

	dyn->pfid_kt = pfr_attach_table(ruleset, tblname);
	if (dyn->pfid_kt == NULL)
		senderr(1);

	dyn->pfid_kt->pfrkt_flags |= PFR_TFLAG_ACTIVE;
	dyn->pfid_iflags = aw->iflags;
	dyn->pfid_af = af;
	dyn->pfid_hook_cookie = hook_establish(dyn->pfid_kif->pfik_ah_head, pfi_dynaddr_update, dyn);
	if (dyn->pfid_hook_cookie == NULL)
		senderr(1);

	aw->p.dyn = dyn;
	pfi_kif_update(dyn->pfid_kif);
	splx(s);
	return (0);

_bad:
	if (dyn->pfid_kt != NULL)
		pfr_detach_table(dyn->pfid_kt);
	if (ruleset != NULL)
		pf_remove_if_empty_ruleset(ruleset);
	if (dyn->pfid_kif != NULL)
        	pfi_kif_unref(dyn->pfid_kif, PFI_KIF_REF_RULE);
	free(dyn, M_DEVBUF);
	splx(s);
	return (rv);
}

void
pfi_kif_update(struct pfi_kif *kif)
{
	struct ifg_list		*ifgl;
	struct pfi_dynaddr	*p;

	/* update all dynaddr */
	TAILQ_FOREACH(p, &kif->pfik_dynaddrs, entry) {
		pfi_dynaddr_update(p);
	}

	/* again for all groups kif is member of */
	if (kif->pfik_ifp != NULL) {
		struct ifg_list_head *ifgh = if_get_groups(kif->pfik_ifp);

		TAILQ_FOREACH(ifgl, ifgh, ifgl_next) {
			pfi_kif_update((struct pfi_kif *)ifgl->ifgl_group->ifg_pf_kif);
		}
	}
}

void
pfi_dynaddr_update(void *v)
{
	struct pfi_dynaddr *dyn;
	struct pfi_kif		*kif;
	struct pfr_ktable	*kt;

	dyn = (struct pfi_dynaddr *)v;
	if (dyn == NULL || kif == NULL || kt == NULL) {
		panic("pfi_dynaddr_update");
	}

	kif = dyn->pfid_kif;
	kt = dyn->pfid_kt;

	if (kt->pfrkt_larg != pfi_update) {
		/* this table needs to be brought up-to-date */
		pfi_table_update(kt, kif, dyn->pfid_net, dyn->pfid_iflags);
		kt->pfrkt_larg = pfi_update;
	}
	pfr_dynaddr_update(kt, dyn);
}

static void
pfi_group_update(struct pfr_ktable *kt, struct pfi_kif *kif, int net, int flags)
{
    int	 e, size2 = 0;
    struct ifg_member	*ifgm;

    pfi_buffer_cnt = 0;
    if ((kif->pfik_flags & PFI_IFLAG_GROUP)) {
        pfi_instance_add(kif->pfik_ifp, net, flags);
    } else if (kif->pfik_group != NULL) {
        TAILQ_FOREACH(ifgm, &kif->pfik_group->ifg_members, ifgm_next) {
            pfi_instance_add(ifgm->ifgm_ifp, net, flags);
        }
    }
    if ((e = pfr_set_addrs(&kt->pfrkt_t, pfi_buffer, pfi_buffer_cnt, &size2, NULL, NULL, NULL, 0))){
        printf("pfi_table_update: cannot set %d new addresses "
		    "into table %s: %d\n", pfi_buffer_cnt, kt->pfrkt_name, e);
    }
}

static void
pfi_instance_update(struct pfr_ktable *kt, struct pfi_kif *kif, int net, int flags)
{
	int	 e, size2 = 0;
    struct pfi_kif		*p;
	struct pfr_table	 t;

    pfi_buffer_cnt = 0;
    if ((kif->pfik_flags & PFI_IFLAG_INSTANCE)) {
        pfi_instance_add(kif->pfik_ifp, net, flags);
    } else if (strcmp(kif->pfik_name, "self")) {
        TAILQ_FOREACH(p, &kif->pfik_grouphead, pfik_instances) {
            pfi_instance_add(p->pfik_ifp, net, flags);
        }
    } else {
        RB_FOREACH(p, pfi_ifhead, &pfi_ifs) {
			if (p->pfik_flags & PFI_IFLAG_INSTANCE) {
				pfi_instance_add(p->pfik_ifp, net, flags);
            }
        }
    }
	t = kt->pfrkt_t;
	t.pfrt_flags = 0;
	if ((e = pfr_set_addrs(&t, pfi_buffer, pfi_buffer_cnt, &size2, NULL, NULL, NULL, 0))) {
		printf("pfi_table_update: cannot set %d new addresses "
		    "into table %s: %d\n", pfi_buffer_cnt, kt->pfrkt_name, e);
	}
}

void
pfi_table_update(struct pfr_ktable *kt, struct pfi_kif *kif, int net, int flags)
{
    if ((kif->pfik_flags & (PFI_IFLAG_GROUP | PFI_IFLAG_INSTANCE)) && kif->pfik_ifp == NULL) {
        pfr_clr_addrs(&kt->pfrkt_t, NULL, 0);
        return;
    }
    if (kif->pfik_ifp != NULL) {
        switch(kif->pfik_flags) {
            case PFI_IFLAG_INSTANCE:
            pfi_instance_update(kt, kif, net, flags);
            return;

            case PFI_IFLAG_GROUP:
            pfi_group_update(kt, kif, net, flags);
            return;
        }
    }
}

void
pfi_instance_add(struct ifnet *ifp, int net, int flags)
{
	struct ifaddr	*ia;
	int		 got4 = 0, got6 = 0;
	int		 net2, af;

	if (ifp == NULL)
		return;
	TAILQ_FOREACH(ia, &ifp->if_addrlist, ifa_list) {
		if (ia->ifa_addr == NULL)
			continue;
		af = ia->ifa_addr->sa_family;
		if (af != AF_INET && af != AF_INET6)
			continue;
		if ((flags & PFI_AFLAG_BROADCAST) && af == AF_INET6)
			continue;
		if ((flags & PFI_AFLAG_BROADCAST) &&
		    !(ifp->if_flags & IFF_BROADCAST))
			continue;
		if ((flags & PFI_AFLAG_PEER) &&
		    !(ifp->if_flags & IFF_POINTOPOINT))
			continue;
		if ((flags & PFI_AFLAG_NETWORK) && af == AF_INET6 &&
		    IN6_IS_ADDR_LINKLOCAL(
		    &((struct sockaddr_in6 *)ia->ifa_addr)->sin6_addr))
			continue;
		if (flags & PFI_AFLAG_NOALIAS) {
			if (af == AF_INET && got4)
				continue;
			if (af == AF_INET6 && got6)
				continue;
		}
		if (af == AF_INET)
			got4 = 1;
		else if (af == AF_INET6)
			got6 = 1;
		net2 = net;
		if (net2 == 128 && (flags & PFI_AFLAG_NETWORK)) {
			if (af == AF_INET) {
				net2 = pfi_unmask(&((struct sockaddr_in *)
				    ia->ifa_netmask)->sin_addr);
			} else if (af == AF_INET6) {
				net2 = pfi_unmask(&((struct sockaddr_in6 *)
				    ia->ifa_netmask)->sin6_addr);
			}
		}
		if (af == AF_INET && net2 > 32)
			net2 = 32;
		if (flags & PFI_AFLAG_BROADCAST)
			pfi_address_add(ia->ifa_broadaddr, af, net2);
		else if (flags & PFI_AFLAG_PEER)
			pfi_address_add(ia->ifa_dstaddr, af, net2);
		else
			pfi_address_add(ia->ifa_addr, af, net2);
	}
}

void
pfi_address_add(struct sockaddr *sa, int af, int net)
{
	struct pfr_addr	*p;
	int		 i;

	if (pfi_buffer_cnt >= pfi_buffer_max) {
		int		 new_max = pfi_buffer_max * 2;

		if (new_max > PFI_BUFFER_MAX) {
			printf("pfi_address_add: address buffer full (%d/%d)\n",
			    pfi_buffer_cnt, PFI_BUFFER_MAX);
			return;
		}
		p = malloc(new_max * sizeof(*pfi_buffer), PFI_MTYPE,
		    M_DONTWAIT);
		if (p == NULL) {
			printf("pfi_address_add: no memory to grow buffer "
			    "(%d/%d)\n", pfi_buffer_cnt, PFI_BUFFER_MAX);
			return;
		}
		memcpy(pfi_buffer, p, pfi_buffer_cnt * sizeof(*pfi_buffer));
		/* no need to zero buffer */
		free(pfi_buffer, PFI_MTYPE);
		pfi_buffer = p;
		pfi_buffer_max = new_max;
	}
	if (af == AF_INET && net > 32)
		net = 128;
	p = pfi_buffer + pfi_buffer_cnt++;
	bzero(p, sizeof(*p));
	p->pfra_af = af;
	p->pfra_net = net;
	if (af == AF_INET)
		p->pfra_ip4addr = ((struct sockaddr_in *)sa)->sin_addr;
	if (af == AF_INET6) {
		p->pfra_ip6addr = ((struct sockaddr_in6 *)sa)->sin6_addr;
		if (IN6_IS_ADDR_LINKLOCAL(&p->pfra_ip6addr))
			p->pfra_ip6addr.s6_addr16[1] = 0;
	}
	/* mask network address bits */
	if (net < 128)
		((caddr_t)p)[p->pfra_net/8] &= ~(0xFF >> (p->pfra_net%8));
	for (i = (p->pfra_net+7)/8; i < sizeof(p->pfra_u); i++)
		((caddr_t)p)[i] = 0;
}

void
pfi_dynaddr_remove(struct pf_addr_wrap *aw)
{
	int	s;

	if (aw->type != PF_ADDR_DYNIFTL || aw->p.dyn == NULL ||
	    aw->p.dyn->pfid_kif == NULL || aw->p.dyn->pfid_kt == NULL)
		return;

	s = splsoftnet();
	hook_disestablish(aw->p.dyn->pfid_kif->pfik_ah_head, aw->p.dyn->pfid_hook_cookie);
	pfi_kif_unref(aw->p.dyn->pfid_kif, PFI_KIF_REF_RULE);
	aw->p.dyn->pfid_kif = NULL;
	pfr_detach_table(aw->p.dyn->pfid_kt);
	aw->p.dyn->pfid_kt = NULL;
	free(aw->p.dyn, M_DEVBUF);
	aw->p.dyn = NULL;
	splx(s);
}

void
pfi_dynaddr_copyout(struct pf_addr_wrap *aw)
{
	if (aw->type != PF_ADDR_DYNIFTL || aw->p.dyn == NULL ||
	    aw->p.dyn->pfid_kif == NULL)
		return;
	aw->p.dyncnt = aw->p.dyn->pfid_acnt4 + aw->p.dyn->pfid_acnt6;
}

void
pfi_kifaddr_update(void *v)
{
	struct pfi_kif *kif = (struct pfi_kif *)v;
	int		 s;

	s = splsoftnet();
	pfi_update++;
	pfi_dohooks(v);
	pfi_kif_update(kif);
	splx(s);
}

void
pfi_kifaddr_update_if(struct ifnet *ifp)
{
	struct pfi_kif *p;

	p = pfi_lookup_if(ifp->if_xname);
	if (p == NULL)
		panic("can't find interface");
	pfi_kifaddr_update(p);
}

int
pfi_if_compare(struct pfi_kif *p, struct pfi_kif *q)
{
	return (strncmp(p->pfik_name, q->pfik_name, IFNAMSIZ));
}

struct pfi_kif *
pfi_if_create(const char *name, struct pfi_kif *q, int flags)
{
	struct pfi_kif *p;

	p = malloc(sizeof(*p), PFI_MTYPE, M_DONTWAIT);
	if (p == NULL)
		return (NULL);
	bzero(p, sizeof(*p));
	p->pfik_ah_head = malloc(sizeof(*p->pfik_ah_head), PFI_MTYPE, M_DONTWAIT);
	if (p->pfik_ah_head == NULL) {
		free(p, PFI_MTYPE);
		return (NULL);
	}
	bzero(p->pfik_ah_head, sizeof(*p->pfik_ah_head));
	TAILQ_INIT(p->pfik_ah_head);
	TAILQ_INIT(&p->pfik_grouphead);

	bzero(p->pfik_ifaddrhooks, sizeof(p->pfik_ifaddrhooks));
	TAILQ_INIT(p->pfik_ifaddrhooks);

	strlcpy(p->pfik_name, name, sizeof(p->pfik_name));
	RB_INIT(&p->pfik_lan_ext);
	RB_INIT(&p->pfik_ext_gwy);
	p->pfik_flags = flags;
	p->pfik_parent = q;
	p->pfik_tzero = time_second;
	TAILQ_INIT(&p->pfik_dynaddrs);
	RB_INSERT(pfi_ifhead, &pfi_ifs, p);
	if (q != NULL) {
		q->pfik_addcnt++;
		TAILQ_INSERT_TAIL(&q->pfik_grouphead, p, pfik_instances);
	}
	pfi_ifcnt++;
	return (p);
}

int
pfi_maybe_destroy(struct pfi_kif *p)
{
	int		 i, j, k, s;
	struct pfi_kif	*q = p->pfik_parent;

	if ((p->pfik_flags & (PFI_IFLAG_ATTACHED | PFI_IFLAG_GROUP)) ||
	    p->pfik_rules > 0 || p->pfik_states > 0)
		return (0);

	s = splsoftnet();
	if (q != NULL) {
		for (i = 0; i < 2; i++)
			for (j = 0; j < 2; j++)
				for (k = 0; k < 2; k++) {
					q->pfik_bytes[i][j][k] +=
					    p->pfik_bytes[i][j][k];
					q->pfik_packets[i][j][k] +=
					    p->pfik_packets[i][j][k];
				}
		q->pfik_delcnt++;
		TAILQ_REMOVE(&q->pfik_grouphead, p, pfik_instances);
	}
	pfi_ifcnt--;
	RB_REMOVE(pfi_ifhead, &pfi_ifs, p);
	splx(s);

	free(p->pfik_ah_head, PFI_MTYPE);
	free(p, PFI_MTYPE);
	return (1);
}

void
pfi_copy_group(char *p, const char *q, int m)
{
	while (m > 1 && *q && !(*q >= '0' && *q <= '9')) {
		*p++ = *q++;
		m--;
	}
	if (m > 0) {
		*p++ = '\0';
	}
}

void
pfi_newgroup(const char *name, int flags)
{
	struct pfi_kif	*p;

	p = pfi_lookup_if(name);
	if (p == NULL)
		p = pfi_if_create(name, pfi_self, PFI_IFLAG_GROUP);
	if (p == NULL) {
		printf("pfi_newgroup: cannot allocate '%s' group", name);
		return;
	}
	p->pfik_flags |= flags;
}

void
pfi_fill_oldstatus(struct pf_status *pfs)
{
	struct pfi_kif	*p, key;
	int		 i, j, k, s;

	strlcpy(key.pfik_name, pfs->ifname, sizeof(key.pfik_name));
	s = splsoftnet();
	p = RB_FIND(pfi_ifhead, &pfi_ifs, &key);
	if (p == NULL) {
		splx(s);
		return;
	}
	bzero(pfs->pcounters, sizeof(pfs->pcounters));
	bzero(pfs->bcounters, sizeof(pfs->bcounters));
	for (i = 0; i < 2; i++)
		for (j = 0; j < 2; j++)
			for (k = 0; k < 2; k++) {
				pfs->pcounters[i][j][k] =
					p->pfik_packets[i][j][k];
				pfs->bcounters[i][j] +=
					p->pfik_bytes[i][j][k];
			}
	splx(s);
}

int
pfi_clr_istats(const char *name, int *nzero, int flags)
{
	struct pfi_kif	*p;
	int		 n = 0, s;
	long		 tzero = time_second;

	s = splsoftnet();
	ACCEPT_FLAGS(PFI_FLAG_GROUP|PFI_FLAG_INSTANCE);
	RB_FOREACH(p, pfi_ifhead, &pfi_ifs) {
		if (pfi_skip_if(name, p, flags))
			continue;
		bzero(p->pfik_packets, sizeof(p->pfik_packets));
		bzero(p->pfik_bytes, sizeof(p->pfik_bytes));
		p->pfik_tzero = tzero;
		n++;
	}
	splx(s);
	if (nzero != NULL)
		*nzero = n;
	return (0);
}

int
pfi_get_ifaces(const char *name, struct pfi_if *buf, int *size, int flags)
{
	struct pfi_kif	*p;
	int		 s, n = 0;

	ACCEPT_FLAGS(PFI_FLAG_GROUP|PFI_FLAG_INSTANCE);
	s = splsoftnet();
	RB_FOREACH(p, pfi_ifhead, &pfi_ifs) {
		if (pfi_skip_if(name, p, flags))
			continue;
		if (*size > n++) {
			if (!p->pfik_tzero)
				p->pfik_tzero = time_second;
			pfi_kif_ref(p, PFI_KIF_REF_RULE);
			if (copyout(p, buf++, sizeof(*buf))) {
				pfi_kif_unref(p, PFI_KIF_REF_RULE);
				splx(s);
				return (EFAULT);
			}
			pfi_kif_unref(p, PFI_KIF_REF_RULE);
		}
	}
	splx(s);
	*size = n;
	return (0);
}

struct pfi_kif *
pfi_lookup_if(const char *name)
{
	struct pfi_kif	*p, key;

	strlcpy(key.pfik_name, name, sizeof(key.pfik_name));
	p = RB_FIND(pfi_ifhead, &pfi_ifs, &key);
	return (p);
}

int
pfi_skip_if(const char *filter, struct pfi_kif *p, int f)
{
	int	n;

	if ((p->pfik_flags & PFI_IFLAG_GROUP) && !(f & PFI_FLAG_GROUP))
		return (1);
	if ((p->pfik_flags & PFI_IFLAG_INSTANCE) && !(f & PFI_FLAG_INSTANCE))
		return (1);
	if (filter == NULL || !*filter)
		return (0);
	if (!strcmp(p->pfik_name, filter))
		return (0);	/* exact match */
	n = strlen(filter);
	if (n < 1 || n >= IFNAMSIZ)
		return (1);	/* sanity check */
	if (filter[n-1] >= '0' && filter[n-1] <= '9')
		return (1);	/* only do exact match in that case */
	if (strncmp(p->pfik_name, filter, n))
		return (1);	/* prefix doesn't match */
	return (p->pfik_name[n] < '0' || p->pfik_name[n] > '9');
}

int
pfi_set_flags(const char *name, int flags)
{
	struct pfi_kif	*p;
	int		 s;

	s = splsoftnet();
	RB_FOREACH(p, pfi_ifhead, &pfi_ifs) {
		if (pfi_skip_if(name, p, flags)) {
			continue;
		}
		p->pfik_flags |= flags;
	}
	splx(s);
	return (0);
}

int
pfi_clear_flags(const char *name, int flags)
{
	struct pfi_kif	*p;
	int		 s;

	s = splsoftnet();
	RB_FOREACH(p, pfi_ifhead, &pfi_ifs) {
		if (pfi_skip_if(name, p, flags)) {
			continue;
		}
		p->pfik_flags &= ~flags;
	}
	splx(s);
	return (0);
}

/* from pf_print_state.c */
int
pfi_unmask(void *addr)
{
	struct pf_addr *m = addr;
	int i = 31, j = 0, b = 0;
	u_int32_t tmp;

	while (j < 4 && m->addr32[j] == 0xffffffff) {
		b += 32;
		j++;
	}
	if (j < 4) {
		tmp = ntohl(m->addr32[j]);
		for (i = 31; tmp & (1 << i); --i) {
			b++;
		}
	}
	return (b);
}

void
pfi_dohooks(struct pfi_kif *p)
{
	for (; p != NULL; p = p->pfik_parent) {
		dohooks(p->pfik_ah_head, 0);
	}
}

int
pfi_match_addr(struct pfi_dynaddr *dyn, struct pf_addr *a, sa_family_t af)
{
	switch (af) {
#ifdef INET
	case AF_INET:
		switch (dyn->pfid_acnt4) {
		case 0:
			return (0);
		case 1:
			return (PF_MATCHA(0, &dyn->pfid_addr4,
			    &dyn->pfid_mask4, a, AF_INET));
		default:
			return (pfr_match_addr(dyn->pfid_kt, a, AF_INET));
		}
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		switch (dyn->pfid_acnt6) {
		case 0:
			return (0);
		case 1:
			return (PF_MATCHA(0, &dyn->pfid_addr6,
			    &dyn->pfid_mask6, a, AF_INET6));
		default:
			return (pfr_match_addr(dyn->pfid_kt, a, AF_INET6));
		}
		break;
#endif /* INET6 */
	default:
		return (0);
	}
}

void
pfi_init_groups(struct ifnet *ifp)
{
	char group[IFNAMSIZ];

	if_init_groups(ifp);
	if_addgroup(ifp, IFG_ALL);

	pfi_copy_group(group, ifp->if_xname, sizeof(group));
	if_addgroup(ifp, group);
}

void
pfi_destroy_groups(struct ifnet *ifp)
{
	char group[IFNAMSIZ];

	pfi_copy_group(group, ifp->if_xname, sizeof(group));
	if_delgroup(ifp, group);

	if_delgroup(ifp, IFG_ALL);
	if_destroy_groups(ifp);
}
