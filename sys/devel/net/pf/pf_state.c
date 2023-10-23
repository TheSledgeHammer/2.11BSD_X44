/*
 * pf_state.c
 *
 *  Created on: 23 Oct 2023
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/endian.h>

#include <net/pfvar.h>
#include <devel/net/pf/if_pfsync.h>
#include <devel/net/pf/pfvar_priv.h>


/* pf_ioctl */
void
pf_state_export(struct pfsync_state *sp, struct pf_state_key *sk, struct pf_state *s)
{
	int secs = time_second;
	bzero(sp, sizeof(struct pfsync_state));

	/* copy from state key */
	sp->lan.addr = sk->lan.addr;
	sp->lan.port = sk->lan.port;
	sp->gwy.addr = sk->gwy.addr;
	sp->gwy.port = sk->gwy.port;
	sp->ext.addr = sk->ext.addr;
	sp->ext.port = sk->ext.port;
	sp->proto = sk->proto;
	sp->af = sk->af;
	sp->direction = sk->direction;

	/* copy from state */
	memcpy(&sp->id, &s->id, sizeof(sp->id));
	sp->creatorid = s->creatorid;
	strlcpy(sp->ifname, s->u.s.kif->pfik_name, sizeof(sp->ifname));
	pf_state_peer_to_pfsync(&s->src, &sp->src);
	pf_state_peer_to_pfsync(&s->dst, &sp->dst);

	sp->rule = s->rule.ptr->nr;
	sp->nat_rule = (s->nat_rule.ptr == NULL) ?  -1 : s->nat_rule.ptr->nr;
	sp->anchor = (s->anchor.ptr == NULL) ?  -1 : s->anchor.ptr->nr;

	pf_state_counter_to_pfsync(s->bytes[0], sp->bytes[0]);
	pf_state_counter_to_pfsync(s->bytes[1], sp->bytes[1]);
	pf_state_counter_to_pfsync(s->packets[0], sp->packets[0]);
	pf_state_counter_to_pfsync(s->packets[1], sp->packets[1]);
	sp->creation = secs - s->creation;
	sp->expire = pf_state_expires(s);
	sp->log = s->log;
	sp->allow_opts = s->allow_opts;
	sp->timeout = s->timeout;

	if (s->src_node)
		sp->sync_flags |= PFSYNC_FLAG_SRCNODE;
	if (s->nat_src_node)
		sp->sync_flags |= PFSYNC_FLAG_NATSRCNODE;

	if (sp->expire > secs)
		sp->expire -= secs;
	else
		sp->expire = 0;
}

void
pf_state_import(struct pfsync_state *sp, struct pf_state_key *sk, struct pf_state *s)
{
	/* copy to state key */
	sk->lan.addr = sp->lan.addr;
	sk->lan.port = sp->lan.port;
	sk->gwy.addr = sp->gwy.addr;
	sk->gwy.port = sp->gwy.port;
	sk->ext.addr = sp->ext.addr;
	sk->ext.port = sp->ext.port;
	sk->proto = sp->proto;
	sk->af = sp->af;
	sk->direction = sp->direction;

	/* copy to state */
	memcpy(&s->id, &sp->id, sizeof(sp->id));
	s->creatorid = sp->creatorid;
	pf_state_peer_from_pfsync(&sp->src, &s->src);
	pf_state_peer_from_pfsync(&sp->dst, &s->dst);

	s->rule.ptr = &pf_default_rule;
	s->rule.ptr->states++;
	s->nat_rule.ptr = NULL;
	s->anchor.ptr = NULL;
	s->rt_kif = NULL;
	s->creation = time_second;
	s->expire = time_second;
	s->timeout = sp->timeout;
	if (sp->expire > 0)
		s->expire -= pf_default_rule.timeout[sp->timeout] - sp->expire;
	s->pfsync_time = 0;
	s->packets[0] = s->packets[1] = 0;
	s->bytes[0] = s->bytes[1] = 0;
}

int
pf_state_add(struct pfsync_state* sp)
{
	struct pf_state		*s;
	struct pf_state_key	*sk;
	struct pfi_kif		*kif;

	if (sp->timeout >= PFTM_MAX &&
			sp->timeout != PFTM_UNTIL_PACKET) {
		return EINVAL;
	}
	s = (struct pf_state *)malloc(sizeof(struct pf_state *), M_PF, M_NOWAIT);
	if (s == NULL) {
		return ENOMEM;
	}
	bzero(s, sizeof(struct pf_state));
	if ((sk = pf_alloc_state_key(s)) == NULL) {
		free(s, M_PF);
		return ENOMEM;
	}
	pf_state_import(sp, sk, s);
	kif = pfi_kif_get(sp->ifname);
	if (kif == NULL) {
		free(s, M_PF);
		free(sk, M_PF);
		return ENOENT;
	}
	if (pf_insert_state(kif, s)) {
		pfi_kif_unref(kif, PFI_KIF_REF_NONE);
		free(s, M_PF);
		return ENOMEM;
	}

	return 0;
}

