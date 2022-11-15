/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)netisr.h	7.3 (Berkeley) 12/30/87
 */

#ifndef _NET_NETISR_H_
#define _NET_NETISR_H_

/*
 * The networking code runs off software interrupts.
 *
 * You can switch into the network by doing splnet() and return by splx().
 * The software interrupt level for the network is higher than the software
 * level for the clock (so you can enter the network in routines called
 * at timeout time).
 */
#if defined(vax) || defined(tahoe)
#define	setsoftnet()	mtpr(SIRR, 12)
#endif
#ifdef pdp11
#define	setsoftnet()	{ extern int knetisr; mtkd(&knetisr, 1); }
#endif

/*
 * Each ``pup-level-1'' input queue has a bit in a ``netisr'' status
 * word which is used to de-multiplex a single software
 * interrupt used for scheduling the network code to calls
 * on the lowest level routine of each protocol.
 */
#define	NETISR_RAW		0		/* same as AF_UNSPEC */
#define	NETISR_IP		2		/* same as AF_INET */
#define	NETISR_IMP		3		/* same as AF_IMPLINK */
#define	NETISR_NS		6		/* same as AF_NS */
#define	NETISR_CLOCK	15		/* protocol timeout */

#define	schednetisr(anisr)	{ netisr |= 1<<(anisr); setsoftnet(); }

#ifndef _LOCORE
#ifdef _KERNEL
int	netisr;				/* scheduling bits for network */
#endif
#endif

#ifdef INET
#if NARP > 0
#include "arp.h"
	DONET(NETISR_ARP, _C_LABEL(arpintr))
#endif
	DONET(NETISR_IP, _C_LABEL(ipintr))
#endif
#ifdef IMP
	DONET(NETISR_IMP, _C_LABEL(impintr))
#endif
#ifdef NS
	DONET(NETISR_NS, _C_LABEL(nsintr))
#endif
#ifdef ISO
	DONET(NETISR_ISO, _C_LABEL(clnlintr))
#endif
#ifdef CCITT
	DONET(NETISR_CCITT, _C_LABEL(ccittintr))
#endif
#ifdef NATM
	DONET(NETISR_NATM, _C_LABEL(natmintr))
#endif
#ifdef NETATALK
	DONET(NETISR_ATALK, _C_LABEL(atintr))
#endif
#if NPPP > 0
#include "ppp.h"
	DONET(NETISR_PPP, _C_LABEL(pppintr))
#endif

#endif /* _NET_NETISR_H_ */
