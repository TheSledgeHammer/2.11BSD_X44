/*
 * pf_state.c
 *
 *  Created on: 23 Oct 2023
 *      Author: marti
 */

//#include <net/pfvar.h>
#include <net/if_pfsync.h>
#include <devel/net/pf/pfvar_priv.h>

/* pf */
void			 pf_attach_state(struct pf_state_key *, struct pf_state *, int);
void			 pf_detach_state(struct pf_state *, int);

static __inline int pf_state_compare_lan_ext(struct pf_state_key *, struct pf_state_key *);
static __inline int pf_state_compare_ext_gwy(struct pf_state_key *,	struct pf_state_key *);

static __inline int
pf_state_compare_lan_ext(struct pf_state_key *a, struct pf_state_key *b)
{
	int	diff;

	if ((diff = a->proto - b->proto) != 0)
		return (diff);
	if ((diff = a->af - b->af) != 0)
		return (diff);
	switch (a->af) {
#ifdef INET
	case AF_INET:
		if (a->lan.addr.addr32[0] > b->lan.addr.addr32[0])
			return (1);
		if (a->lan.addr.addr32[0] < b->lan.addr.addr32[0])
			return (-1);
		if (a->ext.addr.addr32[0] > b->ext.addr.addr32[0])
			return (1);
		if (a->ext.addr.addr32[0] < b->ext.addr.addr32[0])
			return (-1);
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		if (a->lan.addr.addr32[3] > b->lan.addr.addr32[3])
			return (1);
		if (a->lan.addr.addr32[3] < b->lan.addr.addr32[3])
			return (-1);
		if (a->ext.addr.addr32[3] > b->ext.addr.addr32[3])
			return (1);
		if (a->ext.addr.addr32[3] < b->ext.addr.addr32[3])
			return (-1);
		if (a->lan.addr.addr32[2] > b->lan.addr.addr32[2])
			return (1);
		if (a->lan.addr.addr32[2] < b->lan.addr.addr32[2])
			return (-1);
		if (a->ext.addr.addr32[2] > b->ext.addr.addr32[2])
			return (1);
		if (a->ext.addr.addr32[2] < b->ext.addr.addr32[2])
			return (-1);
		if (a->lan.addr.addr32[1] > b->lan.addr.addr32[1])
			return (1);
		if (a->lan.addr.addr32[1] < b->lan.addr.addr32[1])
			return (-1);
		if (a->ext.addr.addr32[1] > b->ext.addr.addr32[1])
			return (1);
		if (a->ext.addr.addr32[1] < b->ext.addr.addr32[1])
			return (-1);
		if (a->lan.addr.addr32[0] > b->lan.addr.addr32[0])
			return (1);
		if (a->lan.addr.addr32[0] < b->lan.addr.addr32[0])
			return (-1);
		if (a->ext.addr.addr32[0] > b->ext.addr.addr32[0])
			return (1);
		if (a->ext.addr.addr32[0] < b->ext.addr.addr32[0])
			return (-1);
		break;
#endif /* INET6 */
	}

	if ((diff = a->lan.port - b->lan.port) != 0)
		return (diff);
	if ((diff = a->ext.port - b->ext.port) != 0)
		return (diff);

	return (0);
}

static __inline int
pf_state_compare_ext_gwy(struct pf_state_key *a, struct pf_state_key *b)
{
	int	diff;

	if ((diff = a->proto - b->proto) != 0)
		return (diff);
	if ((diff = a->af - b->af) != 0)
		return (diff);
	switch (a->af) {
#ifdef INET
	case AF_INET:
		if (a->ext.addr.addr32[0] > b->ext.addr.addr32[0])
			return (1);
		if (a->ext.addr.addr32[0] < b->ext.addr.addr32[0])
			return (-1);
		if (a->gwy.addr.addr32[0] > b->gwy.addr.addr32[0])
			return (1);
		if (a->gwy.addr.addr32[0] < b->gwy.addr.addr32[0])
			return (-1);
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		if (a->ext.addr.addr32[3] > b->ext.addr.addr32[3])
			return (1);
		if (a->ext.addr.addr32[3] < b->ext.addr.addr32[3])
			return (-1);
		if (a->gwy.addr.addr32[3] > b->gwy.addr.addr32[3])
			return (1);
		if (a->gwy.addr.addr32[3] < b->gwy.addr.addr32[3])
			return (-1);
		if (a->ext.addr.addr32[2] > b->ext.addr.addr32[2])
			return (1);
		if (a->ext.addr.addr32[2] < b->ext.addr.addr32[2])
			return (-1);
		if (a->gwy.addr.addr32[2] > b->gwy.addr.addr32[2])
			return (1);
		if (a->gwy.addr.addr32[2] < b->gwy.addr.addr32[2])
			return (-1);
		if (a->ext.addr.addr32[1] > b->ext.addr.addr32[1])
			return (1);
		if (a->ext.addr.addr32[1] < b->ext.addr.addr32[1])
			return (-1);
		if (a->gwy.addr.addr32[1] > b->gwy.addr.addr32[1])
			return (1);
		if (a->gwy.addr.addr32[1] < b->gwy.addr.addr32[1])
			return (-1);
		if (a->ext.addr.addr32[0] > b->ext.addr.addr32[0])
			return (1);
		if (a->ext.addr.addr32[0] < b->ext.addr.addr32[0])
			return (-1);
		if (a->gwy.addr.addr32[0] > b->gwy.addr.addr32[0])
			return (1);
		if (a->gwy.addr.addr32[0] < b->gwy.addr.addr32[0])
			return (-1);
		break;
#endif /* INET6 */
	}

	if ((diff = a->ext.port - b->ext.port) != 0)
		return (diff);
	if ((diff = a->gwy.port - b->gwy.port) != 0)
		return (diff);

	return (0);
}

void
pf_attach_state(struct pf_state_key *sk, struct pf_state *s, int tail)
{
	s->state_key = sk;
	sk->refcnt++;

	/* list is sorted, if-bound states before floating */
	if (tail) {
		TAILQ_INSERT_TAIL(&sk->states, s, next);
	} else {
		TAILQ_INSERT_HEAD(&sk->states, s, next);
	}
}

void
pf_detach_state(struct pf_state *s, int flags)
{
	struct pf_state_key	*sk = s->state_key;

	if (sk == NULL)
		return;

	s->state_key = NULL;
	TAILQ_REMOVE(&sk->states, s, next);
	if (--sk->refcnt == 0) {
		if (!(flags & PF_DT_SKIP_EXTGWY))
			RB_REMOVE(pf_state_tree_ext_gwy, &pf_statetbl_ext_gwy, sk);
		if (!(flags & PF_DT_SKIP_LANEXT))
			RB_REMOVE(pf_state_tree_lan_ext, &pf_statetbl_lan_ext, sk);
		pool_put(&pf_state_key_pl, sk);
	}
}


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
	strlcpy(sp->ifname, s->kif->pfik_name, sizeof(sp->ifname));
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
