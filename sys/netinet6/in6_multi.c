/*	$NetBSD: in6.c,v 1.86 2004/03/28 08:28:06 christos Exp $	*/
/*	$KAME: in6.c,v 1.198 2001/07/18 09:12:38 itojun Exp $	*/

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
 *	@(#)in.c	8.2 (Berkeley) 11/15/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in6.c,v 1.86 2004/03/28 08:28:06 christos Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/if_dl.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <net/if_ether.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mld6_var.h>
#include <netinet6/ip6_mroute.h>
#include <netinet6/in6_ifattach.h>
#include <netinet6/scope6_var.h>

#ifdef MLDV2
#include <netinet/icmp6.h>
#include <netinet6/in6_msf.h>
#endif

#include <net/net_osdep.h>

/*
 * This structure is used to keep track of in6_multi chains which belong to
 * deleted interface addresses.
 */
static LIST_HEAD(, multi6_kludge) in6_mk; /* XXX BSS initialization */

struct multi6_kludge {
	LIST_ENTRY(multi6_kludge) mk_entry;
	struct ifnet *mk_ifp;
	struct in6_multihead mk_head;
};

struct in6_multi *in6_addmulti1(struct in6_addr *, struct ifnet *, int *, int);
void in6_delmulti1(struct in6_multi *);

/*
 * Add an address to the list of IP6 multicast addresses for a
 * given interface.
 */
struct	in6_multi *
in6_addmulti(maddr6, ifp, errorp)
	struct in6_addr *maddr6;
	struct ifnet *ifp;
	int *errorp;
{
#ifdef MLDV2
	return (in6_addmulti2(maddr6, ifp, errorp, 0, NULL, MCAST_EXCLUDE, 1, 0));
#else
	return (in6_addmulti1(maddr6, ifp, errorp, 0));
#endif
}

/*
 * Delete a multicast address record.
 */
void
in6_delmulti(in6m)
	struct in6_multi *in6m;
{
#ifdef MLDV2
	int error;

	in6_delmulti2(in6m, &error, 0, NULL, MCAST_EXCLUDE, 1);
#else
	in6_delmulti1(in6m);
#endif
}

/*
 * Add an address to the list of IP6 multicast addresses for a
 * given interface.
 */
struct in6_multi *
in6_addmulti1(maddr6, ifp, errorp, delay)
	struct in6_addr *maddr6;
	struct ifnet *ifp;
	int *errorp, delay;
{
	struct	in6_ifaddr *ia;
	struct	in6_ifreq ifr;
	struct	in6_multi *in6m;
	int	s = splsoftnet();

	*errorp = 0;
	/*
	 * See if address already in list.
	 */
	IN6_LOOKUP_MULTI(*maddr6, ifp, in6m);
	if (in6m != NULL) {
		/*
		 * Found it; just increment the refrence count.
		 */
		in6m->in6m_refcount++;
	} else {
		/*
		 * New address; allocate a new multicast record
		 * and link it into the interface's multicast list.
		 */
		in6m = (struct in6_multi *)
			malloc(sizeof(*in6m), M_IPMADDR, M_NOWAIT);
		if (in6m == NULL) {
			splx(s);
			*errorp = ENOBUFS;
			return (NULL);
		}
		bzero(in6m, sizeof(*in6m));
		in6m->in6m_addr = *maddr6;
		in6m->in6m_ifp = ifp;
		in6m->in6m_refcount = 1;

		IFP_TO_IA6(ifp, ia);
		if (ia == NULL) {
			free(in6m, M_IPMADDR);
			splx(s);
			*errorp = EADDRNOTAVAIL; /* appropriate? */
			return (NULL);
		}
		in6m->in6m_ia = ia;
		IFAREF(&ia->ia_ifa); /* gain a reference */
		LIST_INSERT_HEAD(&ia->ia6_multiaddrs, in6m, in6m_entry);

		/*
		 * Ask the network driver to update its multicast reception
		 * filter appropriately for the new address.
		 */
		bzero(&ifr.ifr_addr, sizeof(struct sockaddr_in6));
		ifr.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
		ifr.ifr_addr.sin6_family = AF_INET6;
		ifr.ifr_addr.sin6_addr = *maddr6;
		if (ifp->if_ioctl == NULL)
			*errorp = ENXIO; /* XXX: appropriate? */
		else
			*errorp = (*ifp->if_ioctl)(ifp, SIOCADDMULTI,
			    (caddr_t)&ifr);
		if (*errorp) {
			LIST_REMOVE(in6m, in6m_entry);
			free(in6m, M_IPMADDR);
			IFAFREE(&ia->ia_ifa);
			splx(s);
			return (NULL);
		}
		in6m->in6m_timer = delay;
		if (in6m->in6m_timer > 0) {
			in6m->in6m_state = MLD_REPORTPENDING;
			splx(s);
			return (in6m);
		}
		/*
		 * Let MLD6 know that we have joined a new IP6 multicast
		 * group.
		 */
		mld6_start_listening(in6m);
	}
	splx(s);
	return (in6m);
}

/*
 * Delete a multicast address record.
 */
void
in6_delmulti1(in6m)
	struct in6_multi *in6m;
{
	struct	in6_ifreq ifr;
	struct	in6_ifaddr *ia;
	int	s = splsoftnet();

	if (--in6m->in6m_refcount == 0) {
		/*
		 * No remaining claims to this record; let MLD6 know
		 * that we are leaving the multicast group.
		 */
		mld6_stop_listening(in6m);

		/*
		 * Unlink from list.
		 */
		LIST_REMOVE(in6m, in6m_entry);
		if (in6m->in6m_ia) {
			IFAFREE(&in6m->in6m_ia->ia_ifa); /* release reference */
		}
		/*
		 * Delete all references of this multicasting group from
		 * the membership arrays
		 */
		for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
			struct in6_multi_mship *imm;
			LIST_FOREACH(imm, &ia->ia6_memberships,
			    i6mm_chain) {
				if (imm->i6mm_maddr == in6m)
					imm->i6mm_maddr = NULL;
			}
		}

		/*
		 * Notify the network driver to update its multicast
		 * reception filter.
		 */
		bzero(&ifr.ifr_addr, sizeof(struct sockaddr_in6));
		ifr.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
		ifr.ifr_addr.sin6_family = AF_INET6;
		ifr.ifr_addr.sin6_addr = in6m->in6m_addr;
		(*in6m->in6m_ifp->if_ioctl)(in6m->in6m_ifp,
					    SIOCDELMULTI, (caddr_t)&ifr);
		free(in6m, M_IPMADDR);
	}
	splx(s);
}

#ifdef MLDV2

/*
 * Add an address to the list of IP6 multicast addresses for a given interface.
 * Add source addresses to the list also, if upstream router is MLDv2 capable
 * and the number of source is not 0.
 *
 * mode - requested filter mode by socket
 * init - indicate initial join by socket
 */
struct in6_multi *
in6_addmulti2(maddr6, ifp, errorp, numsrc, src, mode, init, delay)
	struct in6_addr *maddr6;
	struct ifnet *ifp;
	int *errorp;
	u_int16_t numsrc;
	struct sockaddr_storage *src;
	u_int mode;
	int init, delay;
{
	struct	in6_ifaddr *ia;
	struct	in6_ifreq ifr;
	struct	in6_multi *in6m;
	struct	i6as_head *newhead = NULL;/* this may become new current head */
	u_int	curmode;		/* current filter mode */
	u_int	newmode;		/* newly calculated filter mode */
	u_int16_t curnumsrc;		/* current i6ms_cur->numsrc */
	u_int16_t newnumsrc;		/* new i6ms_cur->numsrc */
	u_int8_t type = 0;		/* State-Change report type */
	struct	router6_info *rt6i;
	int	s = splsoftnet();

	*errorp = 0;

	/*
	 * MCAST_INCLUDE with empty source list means (*,G) leave.
	 */
	if ((mode == MCAST_INCLUDE) && (numsrc == 0)) {
		*errorp = EINVAL;
		splx(s);
		return NULL;
	}

	/*
	 * See if address already in list.
	 */
	IN6_LOOKUP_MULTI(*maddr6, ifp, in6m);
	if (in6m != NULL) {
		if (!in6_is_mld_target(&in6m->in6m_addr)) {
			++in6m->in6m_refcount;
			splx(s);
			return in6m;
		}

		/*
		 * Found it; merge source addresses in in6m_source and send
		 * State-Change Report.
		 */
		curmode = in6m->in6m_source->i6ms_mode;
		curnumsrc = in6m->in6m_source->i6ms_cur->numsrc;
		/*
		 * Add each source address to in6m_source and get new source
		 * filter mode and its calculated source list.
		 */
		*errorp = in6_addmultisrc(in6m, numsrc, src, mode, init,
					  &newhead, &newmode, &newnumsrc);
		if (*errorp != 0) {
			splx(s);
			return NULL;
		}
		if (newhead != NULL) {
			/*
			 * Merge new source list to current pending report's
			 * source list.
			 */
			*errorp = in6_merge_msf_state(in6m, newhead, newmode,
						      newnumsrc);
			if (*errorp > 0) {
				/* State-Change Report will not be sent.
				 * Just return immediately. */
				/* Each ias linked from newhead is used by new
				 * curhead, so only newhead is freed. */
				FREE(newhead, M_MSFILTER);
				*errorp = 0; /* to make caller behave as
					      * normal */
				splx(s);
				return in6m;
			}
		} else {
			/* Only newhead was merged in a former function. */
			in6m->in6m_source->i6ms_mode = newmode;
			in6m->in6m_source->i6ms_cur->numsrc = newnumsrc;
		}

		/*
		 * Let MLD know that we have joined an IP multicast group
		 * with source list if upstream router is MLDv2 capable.
		 * If there was no pending source list change, an ALLOW or a
		 * BLOCK State-Change Report will not be sent, but a TO_IN or
		 * a TO_EX State-Change Report will be sent in any case.
		 */
		if (in6m->in6m_rti->rt6i_type == MLD_V2_ROUTER) {
			if (curmode != newmode || curnumsrc != newnumsrc) {
				if (curmode != newmode) {
					if (newmode == MCAST_INCLUDE) {
						type = CHANGE_TO_INCLUDE_MODE;
					} else {
						type = CHANGE_TO_EXCLUDE_MODE;
					}
				}
				mld6_send_state_change_report(in6m, type, 1);
			}
		} else {
			/*
			 * If MSF's pending records exist, they must be deleted.
			 * Otherwise, ALW or BLK record will be blocked or
			 * pending list will never be clened when upstream
			 * router switches to MLDv2. XXX
			 */
			in6_clear_all_pending_report(in6m);
		}
		*errorp = 0;
		/* for this group address, init join request by the socket. */
		if (init) {
			in6m->in6m_refcount++;
		}
	} else {
		/*
		 * New address; allocate a new multicast record
		 * and link it into the interface's multicast list.
		 */
		in6m = (struct in6_multi *)
			malloc(sizeof(*in6m), M_IPMADDR, M_NOWAIT);
		if (in6m == NULL) {
			splx(s);
			*errorp = ENOBUFS;
			return (NULL);
		}

		bzero(in6m, sizeof(*in6m));
		in6m->in6m_addr = *maddr6;
		in6m->in6m_ifp = ifp;
		in6m->in6m_refcount = 1;
		IFP_TO_IA6(ifp, ia);
		if (ia == NULL) {
			free(in6m, M_IPMADDR);
			splx(s);
			*errorp = EADDRNOTAVAIL; /* appropriate? */
			return (NULL);
		}
		in6m->in6m_ia = ia;
		IFAREF(&ia->ia_ifa); /* gain a reference */
		LIST_INSERT_HEAD(&ia->ia6_multiaddrs, in6m, in6m_entry);

		/*
		 * Ask the network driver to update its multicast reception
		 * filter appropriately for the new address.
		 */
		bzero(&ifr.ifr_addr, sizeof(struct sockaddr_in6));
		ifr.ifr_addr.sin6_family = AF_INET6;
		ifr.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
		ifr.ifr_addr.sin6_addr = *maddr6;
		if (ifp->if_ioctl == NULL) {
			*errorp = ENXIO; /* XXX: appropriate? */
		} else {
			*errorp = (*ifp->if_ioctl)(ifp, SIOCADDMULTI,
			    (caddr_t)&ifr);
		}
		if (*errorp) {
			LIST_REMOVE(in6m, in6m_entry);
			free(in6m, M_IPMADDR);
			IFAFREE(&ia->ia_ifa);
			splx(s);
			return (NULL);
		}
		rt6i = rt6i_find(in6m->in6m_ifp);
		if (rt6i == NULL) {
			LIST_REMOVE(in6m, in6m_entry);
			free(in6m, M_IPMADDR);
			*errorp = ENOBUFS;
			splx(s);
			return NULL;
		}
		in6m->in6m_rti = rt6i;
		in6m->in6m_source = NULL;
		if (!in6_is_mld_target(&in6m->in6m_addr)) {
			splx(s);
			return in6m;
		}

		if ((*errorp = in6_addmultisrc(in6m, numsrc, src, mode, init,
		    &newhead, &newmode, &newnumsrc)) != 0) {
			in6_free_all_msf_source_list(in6m);
			LIST_REMOVE(in6m, in6m_entry);
			free(in6m, M_IPMADDR);
			splx(s);
			return NULL;
		}

		/* Only newhead was merged in a former function. */
		curmode = in6m->in6m_source->i6ms_mode;
		in6m->in6m_source->i6ms_mode = newmode;
		in6m->in6m_source->i6ms_cur->numsrc = newnumsrc;

		/*
		 * Let MLD know that we have joined a new IP6 multicast
		 * group (MLD version is checked in mld_start_listening()).
		 */
		if (in6m->in6m_rti->rt6i_type == MLD_V2_ROUTER) {
			if (curmode != newmode) {
				if (newmode == MCAST_INCLUDE) {
					/* never happen? */
					type = CHANGE_TO_INCLUDE_MODE;
				} else {
					type = CHANGE_TO_EXCLUDE_MODE;
				}
			}
		}

		in6m->in6m_timer = delay;
		if (in6m->in6m_timer > 0) {
			in6m->in6m_state = MLD_REPORTPENDING;
			splx(s);
			return (in6m);
		}

		mld6_start_listening(in6m, type);
	}
	if (newhead != NULL) {
		/* Each ias is linked from new curhead, so only newhead
		 * is freed */
		FREE(newhead, M_MSFILTER);
	}

	splx(s);
	return (in6m);
}

/*
 * Delete a multicast address record.
 *
 * errorp - return code of each sub routine
 * mode - requested filter mode by socket
 * final - indicate complete leave by socket
 */
void
in6_delmulti2(in6m, errorp, numsrc, src, mode, final)
	struct in6_multi *in6m;
	int *errorp;
	u_int16_t numsrc;
	struct sockaddr_stoarge *src;
	u_int mode;
	int final;
{
	struct in6_ifreq ifr;
	struct in6_ifaddr *ia;
	struct i6as_head *newhead = NULL;	/* this may become new current head */
	u_int curmode; 						/* current filter mode */
	u_int newmode; 						/* newly calculated filter mode */
	u_int16_t curnumsrc; 				/* current i6ms_cur->numsrc */
	u_int16_t newnumsrc; 				/* new i6ms_cur->numsrc */
	u_int8_t type = 0; 					/* State-Change report type */
	int s = splsoftnet();

	if ((mode == MCAST_INCLUDE) && (numsrc == 0)) {
		*errorp = EINVAL;
		splx(s);
		return;
	}

	if (!in6_is_mld_target(&in6m->in6m_addr)) {
		if (--in6m->in6m_refcount == 0) {
			/*
			 * Unlink from list.
			 */
			LIST_REMOVE(in6m, in6m_entry);
			if (in6m->in6m_ia) {
				/* release reference */
				IFAFREE(&in6m->in6m_ia->ia_ifa);
			}

			/*
			 * Delete all references of this multicasting group
			 * from the membership arrays
			 */
			for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
				struct in6_multi_mship *imm;
				LIST_FOREACH(imm, &ia->ia6_memberships, i6mm_chain) {
					if (imm->i6mm_maddr == in6m) {
						imm->i6mm_maddr = NULL;
					}
				}
			}

			/*
			 * Notify the network driver to update its multicast
			 * reception filter.
			 */
			bzero(&ifr.ifr_addr, sizeof(struct sockaddr_in6));
			ifr.ifr_addr.sin6_family = AF_INET6;
			ifr.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
			ifr.ifr_addr.sin6_addr = in6m->in6m_addr;
			(*in6m->in6m_ifp->if_ioctl)(in6m->in6m_ifp, SIOCDELMULTI,
					(caddr_t) &ifr);
			mld6_stop_listening(in6m);
			free(in6m->in6m_timer_ch, M_IPMADDR);
			free(in6m, M_IPMADDR);
		}
		splx(s);
		return;
	}

	curmode = in6m->in6m_source->i6ms_mode;
	curnumsrc = in6m->in6m_source->i6ms_cur->numsrc;
	/*
	 * Delete each source address from in6m_source and get new source
	 * filter mode and its calculated source list, and send State-Change
	 * Report if needed.
	 */
	if ((*errorp = in6_delmultisrc(in6m, numsrc, src, mode, final, &newhead,
			&newmode, &newnumsrc)) != 0) {
		splx(s);
		return;
	}
	if (newhead != NULL) {
		if ((*errorp = in6_merge_msf_state(in6m, newhead, newmode, newnumsrc))
				> 0) {
			/* State-Change Report will not be sent. Just return
			 * immediately. */
			FREE(newhead, M_MSFILTER);
			splx(s);
			return;
		}
	} else {
		/* Only newhead was merged in a former function. */
		in6m->in6m_source->i6ms_mode = newmode;
		in6m->in6m_source->i6ms_cur->numsrc = newnumsrc;
	}

	/* for this group address, final leave request by the socket. */
	if (final) {
		--in6m->in6m_refcount;
	}

	if (in6m->in6m_rti->rt6i_type == MLD_V2_ROUTER) {
		if (curmode != newmode || curnumsrc != newnumsrc) {
			if (curmode != newmode) {
				if (newmode == MCAST_INCLUDE) {
					type = CHANGE_TO_INCLUDE_MODE;
				} else {
					type = CHANGE_TO_EXCLUDE_MODE;
				}
			}
			mld6_send_state_change_report(in6m, type, 1);
			if (!final) {
				mld6_start_listening(in6m, type);
			}
		}
	} else {
		/*
		 * If MSF's pending records exist, they must be deleted.
		 * Otherwise, ALW or BLK record will be blocked or pending
		 * list will never be clened when upstream router switches
		 * to MLDv2. XXX
		 */
		in6_clear_all_pending_report(in6m);
		if (in6m->in6m_refcount == 0) {
			in6m->in6m_source->i6ms_robvar = 0;
			mld6_stop_listening(in6m);
		}
	}

	if (in6m->in6m_refcount == 0) {
		/*
		 * We cannot use timer for robstness times report
		 * transmission when in6m_refcount becomes 0, since in6m
		 * itself will be removed here. So, in this case, report
		 * retransmission will be done quickly. XXX my spec.
		 */
		while (in6m->in6m_source->i6ms_robvar > 0) {
			mld6_send_state_change_report(in6m, type, 0);
		}

		in6_free_all_msf_source_list(in6m);
		LIST_REMOVE(in6m, in6m_entry);
		if (in6m->in6m_ia) {
			/* release reference */
			IFAFREE(&in6m->in6m_ia->ia_ifa);
		}
		bzero(&ifr.ifr_addr, sizeof(struct sockaddr_in6));
		ifr.ifr_addr.sin6_family = AF_INET6;
		ifr.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
		ifr.ifr_addr.sin6_addr = in6m->in6m_addr;
		(*in6m->in6m_ifp->if_ioctl)(in6m->in6m_ifp, SIOCDELMULTI,
				(caddr_t) &ifr);
		mld6_stop_listening(in6m);
		free(in6m->in6m_timer_ch, M_IPMADDR);
		free(in6m, M_IPMADDR);
	}
	*errorp = 0;
	if (newhead != NULL) {
		FREE(newhead, M_MSFILTER);
	}
	splx(s);
}

/*
 * Add an address to the list of IPv6 multicast addresses for a given interface.
 * Add source addresses to the list also, if upstream router is MLDv2 capable
 * and the number of source is not 0.
 *
 * errorp - return code of each sub routine
 * mode, old_mode - requested/current filter mode
 * init - indicate initial join by socket
 * grpjoin - on/off of (*,G) join by socket
 */
struct in6_multi *
in6_modmulti2(ap, ifp, errorp, numsrc, src, mode, old_num, old_src, old_mode, init, grpjoin)
	struct in6_addr *ap;
	struct ifnet *ifp;
	int *errorp;
	u_int16_t numsrc;
	struct sockaddr_storage *src;
	u_int mode;
	u_int16_t old_num;
	struct sockaddr_storage *old_src;
	u_int old_mode;
	int init;
	u_int grpjoin;
{
	struct in6_multi *in6m;
	struct in6_ifreq ifr;
	struct in6_ifaddr *ia;
	struct i6as_head *newhead = NULL;/* this becomes new ims_cur->head */
	u_int curmode;			/* current filter mode */
	u_int newmode;			/* newly calculated filter mode */
	u_int16_t newnumsrc;		/* new ims_cur->numsrc */
	u_int16_t curnumsrc;		/* current ims_cur->numsrc */
	u_int8_t type = 0;		/* State-Change report type */
	struct router6_info *rti;
	int s;

	*errorp = 0; /* initialize */

	if ((mode != MCAST_INCLUDE && mode != MCAST_EXCLUDE) ||
		(old_mode != MCAST_INCLUDE && old_mode != MCAST_EXCLUDE)) {
	    *errorp = EINVAL;
	    return NULL;
	}

	s = splsoftnet();

	/*
	 * See if address already in list.
	 */
	IN6_LOOKUP_MULTI(*ap, ifp, in6m);

	if (in6m != NULL) {
		/*
		 * If requested multicast address is local address, update
		 * the condition, join or leave, based on a requested filter.
		 */
		if (!in6_is_mld_target(&in6m->in6m_addr)) {
			if (numsrc != 0) {
				mldlog(
						(LOG_DEBUG, "in6_modmulti: "
								"source filter not supported for %s\n", ip6_sprintf(
								&in6m->in6m_addr)));
				splx(s);
				*errorp = EINVAL;
				return NULL;
				/*
				 * source filter is not supported for
				 * local group address.
				 */
			}
			if (mode == MCAST_INCLUDE) {
				if (--in6m->in6m_refcount == 0) {
					/*
					 * Unlink from list.
					 */
					LIST_REMOVE(in6m, in6m_entry);
					IFAFREE(&in6m->in6m_ia->ia_ifa);
					/*
					 * Notify the network driver to update
					 * its multicast reception filter.
					 */
					ifr.ifr_addr.sin6_family = AF_INET6;
					ifr.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
					bcopy(&in6m->in6m_addr, &ifr.ifr_addr.sin6_addr,
							sizeof(ifr.ifr_addr.sin6_addr));
					(*in6m->in6m_ifp->if_ioctl)(in6m->in6m_ifp, SIOCDELMULTI,
							(caddr_t) &ifr);
					mld6_stop_listening(in6m);
					free(in6m->in6m_timer_ch, M_IPMADDR);
					free(in6m, M_IPMADDR);
				}
				splx(s);
				return NULL; /* not an error! */
			} else if (mode == MCAST_EXCLUDE) {
				++in6m->in6m_refcount;
				splx(s);
				return in6m;
			}
		}

		curmode = in6m->in6m_source->i6ms_mode;
		curnumsrc = in6m->in6m_source->i6ms_cur->numsrc;
		*errorp = in6_modmultisrc(in6m, numsrc, src, mode, old_num, old_src,
				old_mode, grpjoin, &newhead, &newmode, &newnumsrc);
		if (*errorp != 0) {
			splx(s);
			return NULL;
		}
		if (newhead != NULL) {
			/*
			 * Merge new source list to current pending report's
			 * source list.
			 */
			*errorp = in6_merge_msf_state(in6m, newhead, newmode, newnumsrc);
			if (*errorp > 0) {
				/*
				 * State-Change Report will not be sent.
				 * Just return immediately.
				 */
				FREE(newhead, M_MSFILTER);
				splx(s);
				return in6m;
			}
		} else {
			/* Only newhead was merged. */
			in6m->in6m_source->i6ms_mode = newmode;
			in6m->in6m_source->i6ms_cur->numsrc = newnumsrc;
		}

		/*
		 * Let MLD know that we have joined an IPv6 multicast group with
		 * source list if upstream router is MLDv2 capable.
		 * If there was no pending source list change, an ALLOW or a
		 * BLOCK State-Change Report will not be sent, but a TO_IN or a
		 * TO_EX State-Change Report will be sent in any case.
		 */
		if (in6m->in6m_rti->rt6i_type == MLD_V2_ROUTER) {
			if (curmode != newmode || curnumsrc != newnumsrc || old_num) {
				if (curmode != newmode) {
					if (newmode == MCAST_INCLUDE)
						type = CHANGE_TO_INCLUDE_MODE;
					else
						type = CHANGE_TO_EXCLUDE_MODE;
				}
				mld6_send_state_change_report(in6m, type, 1);
				mld6_start_listening(in6m, type);
			}
		} else {
			/*
			 * If MSF's pending records exist, they must be deleted.
			 */
			in6_clear_all_pending_report(in6m);
		}
		*errorp = 0;
		/* for this group address, initial join request by the socket */
		if (init) {
			++in6m->in6m_refcount;
		}

	} else {
		/*
		 * If there is some sources to be deleted, or if the request is
		 * join a local group address with some filtered address,
		 * return.
		 */
		if ((old_num != 0) || (!in6_is_mld_target(ap) && numsrc != 0)) {
			*errorp = EINVAL;
			splx(s);
			return NULL;
		}

		/*
		 * New address; allocate a new multicast record and link it into
		 * the interface's multicast list.
		 */
		in6m = (struct in6_multi*) malloc(sizeof(*in6m), M_IPMADDR, M_NOWAIT);
		if (in6m == NULL) {
			*errorp = ENOBUFS;
			splx(s);
			return NULL;
		}

		bzero(in6m, sizeof(*in6m));
		in6m->in6m_addr = *ap;
		in6m->in6m_ifp = ifp;
		in6m->in6m_refcount = 1;

		IFP_TO_IA6(ifp, ia);
		if (ia == NULL) {
			mld6_stop_listening(in6m);
			free(in6m, M_IPMADDR);
			*errorp = ENOBUFS /*???*/;
			splx(s);
			return NULL;
		}
		in6m->in6m_ia = ia;
		IFAREF(&in6m->in6m_ia->ia_ifa);
		LIST_INSERT_HEAD(&ia->ia6_multiaddrs, in6m, in6m_entry);
		/*
		 * Ask the network driver to update its multicast reception
		 * filter appropriately for the new address.
		 */
		bzero(&ifr.ifr_addr, sizeof(struct sockaddr_in6));
		ifr.ifr_addr.sin6_family = AF_INET6;
		ifr.ifr_addr.sin6_len = sizeof(struct sockaddr_in6);
		ifr.ifr_addr.sin6_addr = *ap;
		if ((ifp->if_ioctl == NULL)
				|| (*ifp->if_ioctl)(ifp, SIOCADDMULTI, (caddr_t) &ifr) != 0) {
			mld6_stop_listening(in6m);
			LIST_REMOVE(in6m, in6m_entry);
			free(in6m, M_IPMADDR);
			*errorp = EINVAL /*???*/;
			splx(s);
			return NULL;
		}
		rti = find_rt6i(in6m->in6m_ifp);
		if (rti == NULL) {
			mld6_stop_listening(in6m);
			LIST_REMOVE(in6m, in6m_entry);
			free(in6m, M_IPMADDR);
			*errorp = ENOBUFS;
			splx(s);
			return NULL;
		}
		in6m->in6m_rti = rti;
		in6m->in6m_source = NULL;
		if (!in6_is_mld_target(&in6m->in6m_addr)) {
			splx(s);
			return in6m;
		}

		if ((*errorp = in6_modmultisrc(in6m, numsrc, src, mode, 0, NULL,
				MCAST_INCLUDE, grpjoin, &newhead, &newmode, &newnumsrc)) != 0) {
			in6_free_all_msf_source_list(in6m);
			mld6_stop_listening(in6m);
			LIST_REMOVE(in6m, in6m_entry);
			free(in6m, M_IPMADDR);
			splx(s);
			return NULL;
		}
		/* Only newhead was merged in a former function. */
		curmode = in6m->in6m_source->i6ms_mode;
		in6m->in6m_source->i6ms_mode = newmode;
		in6m->in6m_source->i6ms_cur->numsrc = newnumsrc;

		if (in6m->in6m_rti->rt6i_type == MLD_V2_ROUTER) {
			if (curmode != newmode) {
				if (newmode == MCAST_INCLUDE) {
					/* never happen??? */
					type = CHANGE_TO_INCLUDE_MODE;
				} else {
					type = CHANGE_TO_EXCLUDE_MODE;
				}
			}
			mld6_send_state_change_report(in6m, type, 1);
			mld6_start_listening(in6m, type);
		} else {
			struct in6_multi_mship *imm;
			/*
			 * If MSF's pending records exist, they must be deleted.
			 */
			in6_clear_all_pending_report(in6m);
			imm = in6_joingroup(in6m->in6m_ifp, &in6m->in6m_addr, errorp, 0);
			if (imm) {
				LIST_INSERT_HEAD(&ia->ia6_multiaddrs, in6m, in6m_entry);
			} else {
				mldlog(
						(LOG_WARNING, "in6_modmulti: addmulti failed for "
								"%s on %s (errno=%d)\n", ip6_sprintf(
								&in6m->in6m_addr), if_name(in6m->in6m_ifp), *errorp));
			}
		}
		*errorp = 0;
	}
	if (newhead != NULL) {
		FREE(newhead, M_MSFILTER);
	}

	splx(s);
	return in6m;
}


/*
 * check if the given address should be announced via MLDv1/v2.
 */
int
in6_is_mld_target(group)
	struct in6_addr *group;
{
	struct in6_addr tmp = *group;

	if (!IN6_IS_ADDR_MULTICAST(group))
		return 0;

	if (IPV6_ADDR_MC_SCOPE(group) < IPV6_ADDR_SCOPE_LINKLOCAL)
		return 0;

	/*
	 * link index may be embedded into group address, so it has to be
	 * cleared before being compared to ff02::1.
	 */
	in6_clearscope(&tmp);
	if (IN6_ARE_ADDR_EQUAL(&tmp, &mld_all_nodes_nodelocal)) {
		return 0;
	}

	return 1;
}
#endif /* MLDV2 */

/*
 * Multicast address kludge:
 * If there were any multicast addresses attached to this interface address,
 * either move them to another address on this interface, or save them until
 * such time as this interface is reconfigured for IPv6.
 */
void
in6_savemkludge(struct in6_ifaddr *oia)
{
	struct in6_ifaddr *ia;
	struct in6_multi *in6m, *next;

	IFP_TO_IA6(oia->ia_ifp, ia);
	if (ia) {	/* there is another address */
		for (in6m = LIST_FIRST(&oia->ia6_multiaddrs); in6m; in6m = next) {
			next = LIST_NEXT(in6m, in6m_entry);
			IFAFREE(&in6m->in6m_ia->ia_ifa);
			IFAREF(&ia->ia_ifa);
			in6m->in6m_ia = ia;
			LIST_INSERT_HEAD(&ia->ia6_multiaddrs, in6m, in6m_entry);
		}
	} else {	/* last address on this if deleted, save */
		struct multi6_kludge *mk;

		for (mk = LIST_FIRST(&in6_mk); mk; mk = LIST_NEXT(mk, mk_entry)) {
			if (mk->mk_ifp == oia->ia_ifp)
				break;
		}
		if (mk == NULL) /* this should not happen! */
			panic("in6_savemkludge: no kludge space");

		for (in6m = LIST_FIRST(&oia->ia6_multiaddrs); in6m; in6m = next) {
			next = LIST_NEXT(in6m, in6m_entry);
			IFAFREE(&in6m->in6m_ia->ia_ifa); /* release reference */
			in6m->in6m_ia = NULL;
			LIST_INSERT_HEAD(&mk->mk_head, in6m, in6m_entry);
		}
	}
}

/*
 * Continuation of multicast address hack:
 * If there was a multicast group list previously saved for this interface,
 * then we re-attach it to the first address configured on the i/f.
 */
void
in6_restoremkludge(struct in6_ifaddr *ia, struct ifnet *ifp)
{
	struct multi6_kludge *mk;

	for (mk = LIST_FIRST(&in6_mk); mk; mk = LIST_NEXT(mk, mk_entry)) {
		if (mk->mk_ifp == ifp) {
			struct in6_multi *in6m, *next;

			for (in6m = LIST_FIRST(&mk->mk_head); in6m; in6m = next) {
				next = LIST_NEXT(in6m, in6m_entry);
				in6m->in6m_ia = ia;
				IFAREF(&ia->ia_ifa);
				LIST_INSERT_HEAD(&ia->ia6_multiaddrs,
						 in6m, in6m_entry);
			}
			LIST_INIT(&mk->mk_head);
			break;
		}
	}
}

/*
 * Allocate space for the kludge at interface initialization time.
 * Formerly, we dynamically allocated the space in in6_savemkludge() with
 * malloc(M_WAITOK).  However, it was wrong since the function could be called
 * under an interrupt context (software timer on address lifetime expiration).
 * Also, we cannot just give up allocating the strucutre, since the group
 * membership structure is very complex and we need to keep it anyway.
 * Of course, this function MUST NOT be called under an interrupt context.
 * Specifically, it is expected to be called only from in6_ifattach(), though
 * it is a global function.
 */
void
in6_createmkludge(struct ifnet *ifp)
{
	struct multi6_kludge *mk;

	for (mk = LIST_FIRST(&in6_mk); mk; mk = LIST_NEXT(mk, mk_entry)) {
		/* If we've already had one, do not allocate. */
		if (mk->mk_ifp == ifp)
			return;
	}

	mk = malloc(sizeof(*mk), M_IPMADDR, M_WAITOK);

	bzero(mk, sizeof(*mk));
	LIST_INIT(&mk->mk_head);
	mk->mk_ifp = ifp;
	LIST_INSERT_HEAD(&in6_mk, mk, mk_entry);
}

void
in6_purgemkludge(struct ifnet *ifp)
{
	struct multi6_kludge *mk;
	struct in6_multi *in6m;

	for (mk = LIST_FIRST(&in6_mk); mk; mk = LIST_NEXT(mk, mk_entry)) {
		if (mk->mk_ifp != ifp)
			continue;

		/* leave from all multicast groups joined */
		while ((in6m = LIST_FIRST(&mk->mk_head)) != NULL) {
#ifdef MLDV2
			int error;
			in6_delmulti2(in6m, &error, 0, NULL, MCAST_EXCLUDE, 1);
#else
			in6_delmulti(in6m);
#endif
		}
		LIST_REMOVE(mk, mk_entry);
		free(mk, M_IPMADDR);
		break;
	}
}
