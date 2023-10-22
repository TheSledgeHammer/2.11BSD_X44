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

#include <net/pfvar.h>

#ifdef INET6
#include <netinet/ip6.h>
#endif /* INET6 */

#include <devel/net/pf/ifg_group.h>

struct pfi_kif *
pfi_kif_get(const char *kif_name)
{
	struct pfi_kif		*kif;
	struct pfi_kif_cmp	 s;

	bzero(&s, sizeof(s));
	strlcpy(s.pfik_name, kif_name, sizeof(s.pfik_name));
	if ((kif = RB_FIND(pfi_ifhead, &pfi_ifs, (struct pfi_kif *)&s)) != NULL) {
		return (kif);
	}

	kif = pfi_lookup_if(kif_name);
	if (kif != NULL) {
		return (kif);
	}

	/* create new one */
	if ((kif = malloc(sizeof(*kif), PFI_MTYPE, M_DONTWAIT)) == NULL) {
		return (NULL);
	}

	bzero(kif, sizeof(*kif));
	strlcpy(kif->pfik_name, kif_name, sizeof(kif->pfik_name));
#ifdef __NetBSD__
	/* time_second is not valid yet */
	kif->pfik_tzero = (time_second > 7200) ? time_second : 0;
#else
	kif->pfik_tzero = time_second;
#endif /* !__NetBSD__ */
	TAILQ_INIT(&kif->pfik_dynaddrs);

	RB_INSERT(pfi_ifhead, &pfi_ifs, kif);
	return (kif);
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
	if ((kif = pfi_lookup_create(ifg->ifg_group)) == NULL) {
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
	if ((kif = pfi_lookup_create(group)) == NULL) {
		panic("pfi_lookup_create failed");
	}

	pfi_kif_update(kif);

	splx(s);
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

		TAILQ_FOREACH(ifgl, ifgh, ifgl_next)
			pfi_kif_update((struct pfi_kif *)
			    ifgl->ifgl_group->ifg_pf_kif);
	}
}

void
pfi_dynaddr_update(struct pfi_dynaddr *dyn)
{
	struct pfi_kif		*kif;
	struct pfr_ktable	*kt;

	if (dyn == NULL || dyn->pfid_kif == NULL || dyn->pfid_kt == NULL)
		panic("pfi_dynaddr_update");

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
    if ((e = pfr_set_addrs(&kt->pfrkt_t, pfi_buffer, pfi_buffer_cnt, &size2, NULL, NULL, NULL, 0, PFR_TFLAG_ALLMASK))){
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
