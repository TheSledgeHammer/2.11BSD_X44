/*	$NetBSD: in.c,v 1.93.2.1.4.1 2006/04/02 17:48:18 riz Exp $	*/

/*
 * Copyright (c) 2002 INRIA. All rights reserved.
 *
 * Implementation of Internet Group Management Protocol, Version 3.
 * Developed by Hitoshi Asaeda, INRIA, February 2002.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of INRIA nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Public Access Networks Corporation ("Panix").  It was developed under
 * contract to Panix by Eric Haszlakiewicz and Thor Lancelot Simon.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)in.c	8.4 (Berkeley) 1/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in.c,v 1.93.2.1.4.1 2006/04/02 17:48:18 riz Exp $");

#include "opt_inet.h"
#include "opt_inet_conf.h"
#include "opt_mrouting.h"

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>

#include <net/if_ether.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/if_inarp.h>
#include <netinet/ip_mroute.h>
#include <netinet/igmp_var.h>

#ifdef INET

struct in_multi *in_addmulti1(struct in_addr *, struct ifnet *);
void in_delmulti1(struct in_multi *);

/*
 * Add an address to the list of IP multicast addresses for a given interface.
 */
struct in_multi *
in_addmulti(ap, ifp)
	struct in_addr *ap;
	struct ifnet *ifp;
{
	int error;
#ifdef IGMPV3
	return (in_addmulti2(ap, ifp, 0, NULL, MCAST_EXCLUDE, 1, &error));
#else
	return (in_addmulti1(ap, ifp));
#endif
}

/*
 * Delete a multicast address record.
 */
void
in_delmulti(inm)
	struct in_multi *inm;
{
	int error;
#ifdef IGMPV3
	return (in_delmulti2(inm, 0, NULL, MCAST_EXCLUDE, 1, &error));
#else
	return (in_delmulti1(inm));
#endif
}

/*
 * Add an address to the list of IP multicast addresses for a given interface.
 */
struct in_multi *
in_addmulti1(ap, ifp)
	struct in_addr *ap;
	struct ifnet *ifp;
{
	struct in_multi *inm;
	struct ifreq ifr;
	int s = splsoftnet();

	/*
	 * See if address already in list.
	 */
	IN_LOOKUP_MULTI(*ap, ifp, inm);
	if (inm != NULL) {
		/*
		 * Found it; just increment the reference count.
		 */
		++inm->inm_refcount;
	} else {
		/*
		 * New address; allocate a new multicast record
		 * and link it into the interface's multicast list.
		 */
		inm = (struct in_multi *)malloc(sizeof(*inm), M_IPMADDR, M_NOWAIT);
		if (inm == NULL) {
			splx(s);
			return (NULL);
		}
		inm->inm_addr = *ap;
		inm->inm_ifp = ifp;
		inm->inm_refcount = 1;
		LIST_INSERT_HEAD(
		    &IN_MULTI_HASH(inm->inm_addr.s_addr, ifp),
		    inm, inm_list);
		/*
		 * Ask the network driver to update its multicast reception
		 * filter appropriately for the new address.
		 */
		satosin(&ifr.ifr_addr)->sin_len = sizeof(struct sockaddr_in);
		satosin(&ifr.ifr_addr)->sin_family = AF_INET;
		satosin(&ifr.ifr_addr)->sin_addr = *ap;
		if ((ifp->if_ioctl == NULL) ||
		    (*ifp->if_ioctl)(ifp, SIOCADDMULTI,(caddr_t)&ifr) != 0) {
			LIST_REMOVE(inm, inm_list);
			free(inm, M_IPMADDR);
			splx(s);
			return (NULL);
		}
		/*
		 * Let IGMP know that we have joined a new IP multicast group.
		 */
		if (igmp_joingroup(inm) != 0) {
			LIST_REMOVE(inm, inm_list);
			free(inm, M_IPMADDR);
			splx(s);
			return (NULL);
		}
		in_multientries++;
	}
	splx(s);
	return (inm);
}

/*
 * Delete a multicast address record.
 */
void
in_delmulti1(inm)
	struct in_multi *inm;
{
	struct ifreq ifr;
	int s = splsoftnet();

	if (--inm->inm_refcount == 0) {
		/*
		 * No remaining claims to this record; let IGMP know that
		 * we are leaving the multicast group.
		 */
		igmp_leavegroup(inm);
		/*
		 * Unlink from list.
		 */
		LIST_REMOVE(inm, inm_list);
		in_multientries--;
		/*
		 * Notify the network driver to update its multicast reception
		 * filter.
		 */
		satosin(&ifr.ifr_addr)->sin_family = AF_INET;
		satosin(&ifr.ifr_addr)->sin_addr = inm->inm_addr;
		(*inm->inm_ifp->if_ioctl)(inm->inm_ifp, SIOCDELMULTI,
							     (caddr_t)&ifr);
		free(inm, M_IPMADDR);
	}
	splx(s);
}

#ifdef IGMPV3

/*
 * Add an address to the list of IP multicast addresses for a given interface.
 * Add source addresses to the list also, if upstream router is IGMPv3 capable
 * and the number of source is not 0.
 */
struct in_multi *
in_addmulti2(ap, ifp, numsrc, ss, mode, init, error)
	struct in_addr *ap;
	struct ifnet *ifp;
	u_int16_t numsrc;
	struct sockaddr_storage *ss;
	u_int mode;			/* requested filter mode by socket */
	int init;			/* indicate initial join by socket */
	int *error;			/* return code of each sub routine */
{
	struct in_multi *inm;
	struct ifreq ifr;
	struct in_ifaddr *ia;
	struct mbuf *m = NULL;
	struct ias_head *newhead = NULL;/* this may become new ims_cur->head */
	u_int curmode; 			/* current filter mode */
	u_int newmode; 			/* newly calculated filter mode */
	u_int16_t curnumsrc; 	/* current ims_cur->numsrc */
	u_int16_t newnumsrc; 	/* new ims_cur->numsrc */
	int timer_init = 1; 	/* indicate timer initialization */
	int buflen = 0;
	u_int8_t type = 0; 		/* State-Change report type */
	struct router_info *rti;
	int s = splsoftnet();

	/*
	 * See if address already in list.
	 */
	IN_LOOKUP_MULTI(*ap, ifp, inm);

	/*
	 * MCAST_INCLUDE with empty source list means (*,G) leave.
	 */
	if ((mode == MCAST_INCLUDE) && (numsrc == 0)) {
		*error = EINVAL;
		splx(s);
		return NULL;
	}

	if (inm != NULL) {
		/*
		 * Found it; merge source addresses in inm_source and send
		 * State-Change Report if needed, and increment the reference
		 * count. just increment the refrence count if group address
		 * is local.
		 */
		if (!is_igmp_target(&inm->inm_addr)) {
			++inm->inm_refcount;
			splx(s);
			return inm;
		}

		/* inm_source is already allocated. */
		curmode = inm->inm_source->ims_mode;
		curnumsrc = inm->inm_source->ims_cur->numsrc;
		/*
		 * Add each source address to inm_source and get new source
		 * filter mode and its calculated source list.
		 */
		if ((*error = in_addmultisrc(inm, numsrc, ss, mode, init, &newhead,
				&newmode, &newnumsrc)) != 0) {
			splx(s);
			return NULL;
		}
		if (newhead != NULL) {
			/*
			 * Merge new source list to current pending report's source
			 * list.
			 */
			if ((*error = in_merge_msf_state(inm, newhead, newmode, newnumsrc))
					> 0) {
				/* State-Change Report will not be sent. Just return
				 * immediately. */
				/* Each ias linked from newhead is used by new curhead,
				 * so only newhead is freed. */
				FREE(newhead, M_MSFILTER);
				*error = 0; /* to make caller behave as normal */
				splx(s);
				return inm;
			}
		} else {
			/* Only newhead was merged in a former function. */
			inm->inm_source->ims_mode = newmode;
			inm->inm_source->ims_cur->numsrc = newnumsrc;
		}

		/*
		 * Let IGMP know that we have joined an IP multicast group with
		 * source list if upstream router is IGMPv3 capable.
		 * If there was no pending source list change, an ALLOW or a
		 * BLOCK State-Change Report will not be sent, but a TO_IN or a
		 * TO_EX State-Change Report will be sent in any case.
		 */
		if (inm->inm_rti->rti_type == IGMP_v3_ROUTER) {
			if (curmode != newmode || curnumsrc != newnumsrc) {
				if (curmode != newmode) {
					if (newmode == MCAST_INCLUDE)
						type = CHANGE_TO_INCLUDE_MODE;
					else
						type = CHANGE_TO_EXCLUDE_MODE;
				}
				igmp_send_state_change_report(&m, &buflen, inm, type,
						timer_init);
			}
		} else {
			/*
			 * If MSF's pending records exist, they must be deleted.
			 * Otherwise, ALW or BLK record will be blocked or pending
			 * list will never be cleaned when upstream router switches
			 * to IGMPv3. XXX
			 */
			in_clear_all_pending_report(inm);
		}
		*error = 0;
		/*
		 * If this is an initial join request by the socket, increase
		 * refcount.
		 */
		if (init)
			++inm->inm_refcount;
	} else {
		/*
		 * New address; allocate a new multicast record and link it into
		 * the interface's multicast list.
		 */
		inm = (struct in_multi*) malloc(sizeof(*inm), M_IPMADDR, M_NOWAIT);
		if (inm == NULL) {
			*error = ENOBUFS;
			splx(s);
			return (NULL);
		}
		inm->inm_addr = *ap;
		inm->inm_ifp = ifp;
		inm->inm_refcount = 1;
		inm->inm_timer = 0;
		IFP_TO_IA(ifp, ia);
		if (ia == NULL) {
			free(inm, M_IPMADDR);
			*error = ENOBUFS /*???*/;
			splx(s);
			return (NULL);
		}
		inm->inm_ia = ia;
		IFAREF(&inm->inm_ia->ia_ifa);
		LIST_INSERT_HEAD(&ia->ia_multiaddrs, inm, inm_list);
		/*
		 * Ask the network driver to update its multicast reception filter
		 * appropriately for the new address.
		 */
		satosin(&ifr.ifr_addr)->sin_len = sizeof(struct sockaddr_in);
		satosin(&ifr.ifr_addr)->sin_family = AF_INET;
		satosin(&ifr.ifr_addr)->sin_addr = *ap;
		if ((ifp->if_ioctl == NULL)
				|| (*ifp->if_ioctl)(ifp, SIOCADDMULTI, (caddr_t) &ifr) != 0) {
			LIST_REMOVE(inm, inm_list);
			free(inm, M_IPMADDR);
			*error = EINVAL /*???*/;
			splx(s);
			return (NULL);
		}

	    LIST_FOREACH(rti, &rti_head, rti_link) {
			if (rti->rti_ifp == inm->inm_ifp) {
				inm->inm_rti = rti;
				break;
			}
		}
		if (rti == 0) {
			if ((rti = rti_init(inm->inm_ifp)) == NULL) {
				LIST_REMOVE(inm, inm_list);
				free(inm, M_IPMADDR);
				*error = ENOBUFS;
				splx(s);
				return NULL;
			} else
				inm->inm_rti = rti;
		}

		inm->inm_source = NULL;
		if (!is_igmp_target(&inm->inm_addr)) {
			splx(s);
			return inm;
		}

		if ((*error = in_addmultisrc(inm, numsrc, ss, mode, init, &newhead,
				&newmode, &newnumsrc)) != 0) {
			in_free_all_msf_source_list(inm);
			LIST_REMOVE(inm, inm_list);
			free(inm, M_IPMADDR);
			splx(s);
			return NULL;
		}
		/* Only newhead was merged in a former function. */
		curmode = inm->inm_source->ims_mode;
		inm->inm_source->ims_mode = newmode;
		inm->inm_source->ims_cur->numsrc = newnumsrc;

		/*
		 * Let IGMP know that we have joined a new IP multicast group
		 * with source list if upstream router is IGMPv3 capable.
		 * If the router doesn't speak IGMPv3, then send Report message
		 * with no source address since it is a first join request.
		 */
		if (inm->inm_rti->rti_type == IGMP_v3_ROUTER) {
			if (curmode != newmode) {
				if (newmode == MCAST_INCLUDE)
					type = CHANGE_TO_INCLUDE_MODE; /* never happen? */
				else
					type = CHANGE_TO_EXCLUDE_MODE;
			}
			igmp_send_state_change_report(&m, &buflen, inm, type, timer_init);
		} else {
			/*
			 * If MSF's pending records exist, they must be deleted.
			 */
			in_clear_all_pending_report(inm);
			igmp_joingroup(inm);
		}
		*error = 0;
	}
	if (newhead != NULL)
		/* Each ias is linked from new curhead, so only newhead (not
		 * ias_list) is freed */
		FREE(newhead, M_MSFILTER);

	splx(s);
	return (inm);
}

/*
 * Delete a multicast address record.
 */
void
in_delmulti2(inm, numsrc, ss, mode, final, error)
	struct in_multi *inm;
	u_int16_t numsrc;
	struct sockaddr_storage *ss;
	u_int mode;			/* requested filter mode by socket */
	int final;			/* indicate complete leave by socket */
	int *error;			/* return code of each sub routine */
{
	struct ifreq ifr;
	struct mbuf *m = NULL;
	struct ias_head *newhead = NULL;/* this may become new ims_cur->head */
	u_int curmode;			/* current filter mode */
	u_int newmode;			/* newly calculated filter mode */
	u_int16_t curnumsrc;		/* current ims_cur->numsrc */
	u_int16_t newnumsrc;		/* new ims_cur->numsrc */
	int timer_init = 1;		/* indicate timer initialization */
	int buflen = 0;
	u_int8_t type = 0;		/* State-Change report type */
	int s = splsoftnet();

	if ((mode == MCAST_INCLUDE) && (numsrc == 0)) {
		*error = EINVAL;
		splx(s);
		return;
	}
	if (!is_igmp_target(&inm->inm_addr)) {
		if (--inm->inm_refcount == 0) {
			/*
			 * Unlink from list.
			 */
			LIST_REMOVE(inm, inm_list);
			IFAFREE(&inm->inm_ia->ia_ifa);
			/*
			 * Notify the network driver to update its multicast
			 * reception filter.
			 */
			satosin(&ifr.ifr_addr)->sin_family = AF_INET;
			satosin(&ifr.ifr_addr)->sin_addr = inm->inm_addr;
			(*inm->inm_ifp->if_ioctl)(inm->inm_ifp, SIOCDELMULTI,
					(caddr_t) &ifr);
			free(inm, M_IPMADDR);
		}
		splx(s);
		return;
	}

	/* inm_source is already allocated. */
	curmode = inm->inm_source->ims_mode;
	curnumsrc = inm->inm_source->ims_cur->numsrc;
	/*
	 * Delete each source address from inm_source and get new source
	 * filter mode and its calculated source list, and send State-Change
	 * Report if needed.
	 */
	if ((*error = in_delmultisrc(inm, numsrc, ss, mode, final, &newhead,
			&newmode, &newnumsrc)) != 0) {
		splx(s);
		return;
	}
	if (newhead != NULL) {
		if ((*error = in_merge_msf_state(inm, newhead, newmode, newnumsrc))
				> 0) {
			/* State-Change Report will not be sent. Just return
			 * immediately. */
			FREE(newhead, M_MSFILTER);
			splx(s);
			return;
		}
	} else {
		/* Only newhead was merged in a former function. */
		inm->inm_source->ims_mode = newmode;
		inm->inm_source->ims_cur->numsrc = newnumsrc;
	}

	/*
	 * If this is a final leave request by the socket, decrease
	 * refcount.
	 */
	if (final) {
		--inm->inm_refcount;
	}

	if (inm->inm_rti->rti_type == IGMP_v3_ROUTER) {
		if (curmode != newmode || curnumsrc != newnumsrc) {
			if (curmode != newmode) {
				if (newmode == MCAST_INCLUDE)
					type = CHANGE_TO_INCLUDE_MODE;
				else
					type = CHANGE_TO_EXCLUDE_MODE;
			}
			igmp_send_state_change_report(&m, &buflen, inm, type, timer_init);
		}
	} else {
		/*
		 * If MSF's pending records exist, they must be deleted.
		 * Otherwise, ALW or BLK record will be blocked or pending
		 * list will never be cleaned when upstream router switches
		 * to IGMPv3. XXX
		 */
		in_clear_all_pending_report(inm);
		if (inm->inm_refcount == 0) {
			igmp_leavegroup(inm);
			inm->inm_source->ims_robvar = 0;
		}
	}

	if (inm->inm_refcount == 0) {
		/*
		 * We cannot use timer for robstness times report
		 * transmission when inm_refcount becomes 0, since inm
		 * itself will be removed here. So, in this case, report
		 * retransmission will be done quickly. XXX my spec.
		 */
		timer_init = 0;
		while (inm->inm_source->ims_robvar > 0) {
			m = NULL;
			buflen = 0;
			igmp_send_state_change_report(&m, &buflen, inm, type, timer_init);
			if (m != NULL)
				igmp_sendbuf(m, inm->inm_ifp);
		}
		in_free_all_msf_source_list(inm);
		LIST_REMOVE(inm, inm_list);
		IFAFREE(&inm->inm_ia->ia_ifa);
		satosin(&ifr.ifr_addr)->sin_family = AF_INET;
		satosin(&ifr.ifr_addr)->sin_addr = inm->inm_addr;
		(*inm->inm_ifp->if_ioctl)(inm->inm_ifp, SIOCDELMULTI, (caddr_t) &ifr);
		free(inm, M_IPMADDR);
	}
	*error = 0;
	if (newhead != NULL)
		FREE(newhead, M_MSFILTER);

	splx(s);
}

/*
 * Add an address to the list of IP multicast addresses for a given interface.
 * Add source addresses to the list also, if upstream router is IGMPv3 capable
 * and the number of source is not 0.
 */
struct in_multi *
in_modmulti2(ap, ifp, numsrc, ss, mode,
		old_num, old_ss, old_mode, init, grpjoin, error)
	struct in_addr *ap;
	struct ifnet *ifp;
	u_int16_t numsrc, old_num;
	struct sockaddr_storage *ss, *old_ss;
	u_int mode, old_mode;		/* requested/current filter mode */
	int init;			/* indicate initial join by socket */
	u_int grpjoin;			/* on/off of (*,G) join by socket */
	int *error;			/* return code of each sub routine */
{
	struct mbuf *m = NULL;
	struct in_multi *inm;
	struct ifreq ifr;
	struct in_ifaddr *ia;
	struct ias_head *newhead = NULL;/* this becomes new ims_cur->head */
	u_int curmode; 			/* current filter mode */
	u_int newmode; 			/* newly calculated filter mode */
	u_int16_t curnumsrc; 	/* current ims_cur->numsrc */
	u_int16_t newnumsrc;	/* new ims_cur->numsrc */
	int timer_init = 1; 	/* indicate timer initialization */
	int buflen = 0;
	u_int8_t type = 0; 		/* State-Change report type */
	struct router_info *rti;
	int s;

	*error = 0; /* initialize */

	if ((mode != MCAST_INCLUDE && mode != MCAST_EXCLUDE)
			|| (old_mode != MCAST_INCLUDE && old_mode != MCAST_EXCLUDE)) {
		*error = EINVAL;
		return NULL;
	}

	s = splsoftnet();

	/*
	 * See if address already in list.
	 */
	IN_LOOKUP_MULTI(*ap, ifp, inm);

	if (inm != NULL) {
		/*
		 * If requested multicast address is local address, update
		 * the condition, join or leave, based on a requested filter.
		 */
		if (!is_igmp_target(&inm->inm_addr)) {
			if (numsrc != 0) {
				splx(s);
				*error = EINVAL;
				return NULL; /* source filter is not supported for
				 local group address. */
			}
			if (mode == MCAST_INCLUDE) {
				if (--inm->inm_refcount == 0) {
					/*
					 * Unlink from list.
					 */
					LIST_REMOVE(inm, inm_list);
					IFAFREE(&inm->inm_ia->ia_ifa);
					/*
					 * Notify the network driver to update its multicast
					 * reception filter.
					 */
					satosin(&ifr.ifr_addr)->sin_family = AF_INET;
					satosin(&ifr.ifr_addr)->sin_addr = inm->inm_addr;
					(*inm->inm_ifp->if_ioctl)(inm->inm_ifp, SIOCDELMULTI,
							(caddr_t) &ifr);
					free(inm, M_IPMADDR);
				}
				splx(s);
				return NULL; /* not an error! */
			} else if (mode == MCAST_EXCLUDE) {
				++inm->inm_refcount;
				splx(s);
				return inm;
			}
		}

		/* inm_source is already allocated. */
		curmode = inm->inm_source->ims_mode;
		curnumsrc = inm->inm_source->ims_cur->numsrc;
		if ((*error = in_modmultisrc(inm, numsrc, ss, mode, old_num, old_ss,
				old_mode, grpjoin, &newhead, &newmode, &newnumsrc)) != 0) {
			splx(s);
			return NULL;
		}
		if (newhead != NULL) {
			/*
			 * Merge new source list to current pending report's source
			 * list.
			 */
			if ((*error = in_merge_msf_state(inm, newhead, newmode, newnumsrc))
					> 0) {
				/* State-Change Report will not be sent. Just return
				 * immediately. */
				FREE(newhead, M_MSFILTER);
				splx(s);
				return inm;
			}
		} else {
			/* Only newhead was merged. */
			inm->inm_source->ims_mode = newmode;
			inm->inm_source->ims_cur->numsrc = newnumsrc;
		}

		/*
		 * Let IGMP know that we have joined an IP multicast group with
		 * source list if upstream router is IGMPv3 capable.
		 * If there was no pending source list change, an ALLOW or a
		 * BLOCK State-Change Report will not be sent, but a TO_IN or a
		 * TO_EX State-Change Report will be sent in any case.
		 */
		if (inm->inm_rti->rti_type == IGMP_v3_ROUTER) {
			if (curmode != newmode || curnumsrc != newnumsrc || old_num) {
				if (curmode != newmode) {
					if (newmode == MCAST_INCLUDE)
						type = CHANGE_TO_INCLUDE_MODE;
					else
						type = CHANGE_TO_EXCLUDE_MODE;
				}
				igmp_send_state_change_report(&m, &buflen, inm, type,
						timer_init);
			}
		} else {
			/*
			 * If MSF's pending records exist, they must be deleted.
			 */
			in_clear_all_pending_report(inm);
		}
		*error = 0;
		/* for this group address, initial join request by the socket. */
		if (init)
			++inm->inm_refcount;

	} else {
		/*
		 * If there is some sources to be deleted, or if the request is
		 * join a local group address with some filtered address, return.
		 */
		if ((old_num != 0) || (!is_igmp_target(ap) && numsrc != 0)) {
			*error = EINVAL;
			splx(s);
			return NULL;
		}

	    /*
	     * New address; allocate a new multicast record and link it into
	     * the interface's multicast list.
	     */
		inm = (struct in_multi *)malloc(sizeof(*inm), M_IPMADDR, M_NOWAIT);
		if (inm == NULL) {
			*error = ENOBUFS;
			splx(s);
			return NULL;
		}
		inm->inm_addr = *ap;
		inm->inm_ifp = ifp;
		inm->inm_refcount = 1;
		inm->inm_timer = 0;
		IFP_TO_IA(ifp, ia);
		if (ia == NULL) {
			free(inm, M_IPMADDR);
			*error = ENOBUFS /*???*/;
			splx(s);
			return NULL;
		}
		inm->inm_ia = ia;
		IFAREF(&inm->inm_ia->ia_ifa);
		LIST_INSERT_HEAD(&ia->ia_multiaddrs, inm, inm_list);
		/*
		 * Ask the network driver to update its multicast reception filter
		 * appropriately for the new address.
		 */
		satosin(&ifr.ifr_addr)->sin_len = sizeof(struct sockaddr_in);
		satosin(&ifr.ifr_addr)->sin_family = AF_INET;
		satosin(&ifr.ifr_addr)->sin_addr = *ap;
		if ((ifp->if_ioctl == NULL)
				|| (*ifp->if_ioctl)(ifp, SIOCADDMULTI, (caddr_t) &ifr) != 0) {
			LIST_REMOVE(inm, inm_list);
			free(inm, M_IPMADDR);
			*error = EINVAL /*???*/;
			splx(s);
			return NULL;
		}

		LIST_FOREACH(rti, &rti_head, rti_link) {
			if (rti->rti_ifp == inm->inm_ifp) {
				inm->inm_rti = rti;
				break;
			}
		}
		if (rti == 0) {
			if ((rti = rti_init(inm->inm_ifp)) == NULL) {
				LIST_REMOVE(inm, inm_list);
				free(inm, M_IPMADDR);
				*error = ENOBUFS;
				splx(s);
				return NULL;
			} else
				inm->inm_rti = rti;
		}

		inm->inm_source = NULL;
		if (!is_igmp_target(&inm->inm_addr)) {
			splx(s);
			return inm;
		}

		if ((*error = in_modmultisrc(inm, numsrc, ss, mode, 0, NULL,
				MCAST_INCLUDE, grpjoin, &newhead, &newmode, &newnumsrc)) != 0) {
			in_free_all_msf_source_list(inm);
			LIST_REMOVE(inm, inm_list);
			free(inm, M_IPMADDR);
			splx(s);
			return NULL;
		}
		/* Only newhead was merged in a former function. */
		curmode = inm->inm_source->ims_mode;
		inm->inm_source->ims_mode = newmode;
		inm->inm_source->ims_cur->numsrc = newnumsrc;

		if (inm->inm_rti->rti_type == IGMP_v3_ROUTER) {
			if (curmode != newmode) {
				if (newmode == MCAST_INCLUDE)
					type = CHANGE_TO_INCLUDE_MODE;/* never happen??? */
				else
					type = CHANGE_TO_EXCLUDE_MODE;
			}
			igmp_send_state_change_report(&m, &buflen, inm, type, timer_init);
		} else {
			/*
			 * If MSF's pending records exist, they must be deleted.
			 */
			in_clear_all_pending_report(inm);
			igmp_joingroup(inm);
		}
		*error = 0;
	}
	if (newhead != NULL)
		FREE(newhead, M_MSFILTER);

	splx(s);
	return inm;
}

#endif /* IGMPV3 */

#endif
