/* $NetBSD: netisr_dispatch.h,v 1.10 2002/11/02 12:00:03 kristerw Exp $ */

/*
 * netisr_dispatch: This file is included by the
 *	machine dependant softnet function.  The
 *	DONETISR macro should be set before including
 *	this file.  i.e.:
 *
 * softintr() {
 *	...do setup stuff...
 *	#define DONETISR(bit, fn) do { ... } while (0)
 *	#include <net/netisr_dispatch.h>
 *	#undef DONETISR
 *	...do cleanup stuff.
 * }
 */

#ifndef _NET_NETISR_H_
#error <net/netisr.h> must be included before <net/netisr_dispatch.h>
#endif

/*
 * When adding functions to this list, be sure to add headers to provide
 * their prototypes in <net/netisr.h> (if necessary).
 */
#ifdef INET
#if NARP > 0
	DONET(NETISR_ARP,arpintr)
#endif
	DONET(NETISR_IP,ipintr)
#endif
#ifdef INET6
	DONET(NETISR_IPV6,ip6intr);
#endif
#ifdef MPLS
	DONET(NETISR_MPLS,mplsintr)
#endif
#ifdef NS
	DONET(NETISR_NS,nsintr)
#endif
#if NSL > 0 && !defined(__HAVE_GENERIC_SOFT_INTERRUPTS)
	DONET(NETISR_SLIP,slnetisr);
#endif
#if NSTRIP > 0 && !defined(__HAVE_GENERIC_SOFT_INTERRUPTS)
	DONET(NETISR_STRIP,stripnetisr);
#endif
#if NPPP > 0 && !defined(__HAVE_GENERIC_SOFT_INTERRUPTS)
	DONET(NETISR_PPP,pppnetisr)
#endif
