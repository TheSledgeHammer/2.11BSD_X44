/*	$NetBSD: ipsec_osdep.h,v 1.9.2.1.4.1 2007/12/01 17:32:30 bouyer Exp $	*/
/*	$FreeBSD: /repoman/r/ncvs/src/sys/netipsec/ipsec_osdep.h,v 1.1 2003/09/29 22:47:45 sam Exp $	*/

/*
 * Copyright (c) 2003 Jonathan Stone (jonathan@cs.stanford.edu)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#ifndef NETIPSEC_OSDEP_H
#define NETIPSEC_OSDEP_H

#include <sys/systm.h>

#ifdef _KERNEL

#define IPSEC_SPLASSERT(x,y) 		(void)0
#define IPSEC_ASSERT(c,m) 			KASSERT(c)
#define IPSEC_SPLASSERT_SOFTNET(m) 	IPSEC_SPLASSERT(softnet, m)

#include <dev/disk/rnd/rnd.h>
static __inline u_int read_random(void *p, u_int len);

static __inline u_int
read_random(void *bufp, u_int len)
{
	return rnd_extract_data(bufp, len);
}

#define M_EXT_WRITABLE(m) (!M_READONLY(m))
#define M_MOVE_PKTHDR(_f, _t) m_copy_pkthdr(_f, _t)


static __inline struct mbuf *
m_getcl(int how, short type, int flags)
{
	struct mbuf *mp;
	if (flags & M_PKTHDR) {
		MGETHDR(mp, how, type);
	} else {
		MGET(mp, how,  type);
	}
	if (mp == NULL)
		return NULL;

	MCLGET(mp, how);
	if ((mp->m_flags & M_EXT) == 0) {
		m_free(mp);
		mp = NULL;
	}
	return mp;
}

#define IF_HANDOFF(ifq, m, f) if_handoff(ifq, m, f, 0)

#include <net/if.h>

static __inline int
if_handoff(struct ifqueue *ifq, struct mbuf *m, struct ifnet *ifp, int adjust)
{
	int need_if_start = 0;
	int s = splnet();

	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		splx(s);
		m_freem(m);
		return (0);
	}
	if (ifp != NULL) {
		ifp->if_obytes += m->m_pkthdr.len + adjust;
		if (m->m_flags & M_MCAST)
			ifp->if_omcasts++;
		need_if_start = !(ifp->if_flags & IFF_OACTIVE);
	}
	IF_ENQUEUE(ifq, m);
	if (need_if_start)
		(*ifp->if_start)(ifp);
	splx(s);
	return (1);
}

#include <sys/kernel.h>
#define time_second mono_time.tv_sec

#include <sys/protosw.h>
#define ipprotosw protosw

/* superuser opened socket? */
#define IPSEC_PRIVILEGED_SO(so) ((so)->so_uid == 0)

#define ia_link ia_list

#define INITFN extern

#define IP_OFF_CONVERT(x) (htons(x))

#define PCB_T			struct inpcb_hdr
#define PCB_FAMILY(p)	((p)->inph_af)
#define PCB_SOCKET(p)	((p)->inph_socket)

#define PCB_TO_IN4PCB(p) ((struct inpcb *)(p))
#define PCB_TO_IN6PCB(p) ((struct in6pcb *)(p))

#define IN4PCB_TO_PCB(p) ((PCB_T *)(&(p)->inp_head))
#define IN6PCB_TO_PCB(p) ((PCB_T *)(&(p)->in6p_head))

#endif /* _KERNEL */
#endif /* NETIPSEC_OSDEP_H */
