/*	$NetBSD: if.h,v 1.95 2003/12/10 11:46:33 itojun Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by William Studnemund and Jason R. Thorpe.
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
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)if.h	8.3 (Berkeley) 2/9/95
 */

#ifndef _NET_IFQ_H_
#define _NET_IFQ_H_

/*
 * Always include ALTQ glue here -- we use the ALTQ interface queue
 * structure even when ALTQ is not configured into the kernel so that
 * the size of struct ifnet does not changed based on the option.  The
 * ALTQ queue structure is API-compatible with the legacy ifqueue.
 */
#include <net/altq/if_altq.h>

struct mbuf;

/*
 * Structure defining a queue for a network interface.
 */
struct ifqueue {
	struct	mbuf 	*ifq_head;
	struct	mbuf 	*ifq_tail;
	int				ifq_len;
	int				ifq_maxlen;
	int				ifq_drops;
};

/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup level 1)
 * input routines have queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros, which should be called with ipl raised to splnet().
 */
#define	IF_QFULL(ifq)		((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define	IF_DROP(ifq)		((ifq)->ifq_drops++)
#define	IF_ENQUEUE(ifq, m) { 									\
	(m)->m_nextpkt = 0; 										\
	if ((ifq)->ifq_tail == 0) 									\
		(ifq)->ifq_head = m; 									\
	else 														\
		(ifq)->ifq_tail->m_nextpkt = m; 						\
	(ifq)->ifq_tail = m; 										\
	(ifq)->ifq_len++; 											\
}
#define	IF_PREPEND(ifq, m) { 									\
	(m)->m_nextpkt = (ifq)->ifq_head; 							\
	if ((ifq)->ifq_tail == 0) 									\
		(ifq)->ifq_tail = (m); 									\
	(ifq)->ifq_head = (m); 										\
	(ifq)->ifq_len++; 											\
}
#define	IF_DEQUEUE(ifq, m) { 									\
	(m) = (ifq)->ifq_head; 										\
	if (m) { 													\
		if (((ifq)->ifq_head = (m)->m_nextpkt) == 0) 			\
			(ifq)->ifq_tail = 0; 								\
		(m)->m_nextpkt = 0; 									\
		(ifq)->ifq_len--; 										\
	} 															\
}
#define	IF_POLL(ifq, m)		((m) = (ifq)->ifq_head)
#define	IF_PURGE(ifq)											\
do {															\
	struct mbuf *__m0;											\
																\
	for (;;) {													\
		IF_DEQUEUE((ifq), __m0);								\
		if (__m0 == NULL)										\
			break;												\
		else													\
			m_freem(__m0);										\
	}															\
} while (/*CONSTCOND*/ 0)
#define	IF_IS_EMPTY(ifq)	((ifq)->ifq_len == 0)

#ifndef IFQ_MAXLEN
#define	IFQ_MAXLEN		50
#endif

#ifdef ALTQ

#define	ALTQ_DECL(x)		(x)

#define IFQ_ENQUEUE(ifq, m, pattr, err)							\
do {															\
	if (ALTQ_IS_ENABLED((ifq)))									\
		ALTQ_ENQUEUE((ifq), (m), (pattr), (err));				\
	else {														\
		if (IF_QFULL((ifq))) {									\
			m_freem((m));										\
			(err) = ENOBUFS;									\
		} else {												\
			IF_ENQUEUE((ifq), (m));								\
			(err) = 0;											\
		}														\
	}															\
	if ((err))													\
		(ifq)->ifq_drops++;										\
} while (/*CONSTCOND*/ 0)

#define IFQ_DEQUEUE(ifq, m)										\
do {															\
	if (TBR_IS_ENABLED((ifq)))									\
		(m) = tbr_dequeue((ifq), ALTDQ_REMOVE);					\
	else if (ALTQ_IS_ENABLED((ifq)))							\
		ALTQ_DEQUEUE((ifq), (m));								\
	else														\
		IF_DEQUEUE((ifq), (m));									\
} while (/*CONSTCOND*/ 0)

#define	IFQ_POLL(ifq, m)										\
do {															\
	if (TBR_IS_ENABLED((ifq)))									\
		(m) = tbr_dequeue((ifq), ALTDQ_POLL);					\
	else if (ALTQ_IS_ENABLED((ifq)))							\
		ALTQ_POLL((ifq), (m));									\
	else														\
		IF_POLL((ifq), (m));									\
} while (/*CONSTCOND*/ 0)

#define	IFQ_PURGE(ifq)											\
do {															\
	if (ALTQ_IS_ENABLED((ifq)))									\
		ALTQ_PURGE((ifq));										\
	else														\
		IF_PURGE((ifq));										\
} while (/*CONSTCOND*/ 0)

#define	IFQ_SET_READY(ifq)										\
do {															\
	(ifq)->altq_flags |= ALTQF_READY;							\
} while (/*CONSTCOND*/ 0)

#define	IFQ_CLASSIFY(ifq, m, af, pa)							\
do {															\
	if (ALTQ_IS_ENABLED((ifq))) {								\
		if (ALTQ_NEEDS_CLASSIFY((ifq)))							\
			(pa)->pattr_class = (*(ifq)->altq_classify)			\
				((ifq)->altq_clfier, (m), (af));				\
		(pa)->pattr_af = (af);									\
		(pa)->pattr_hdr = mtod((m), caddr_t);					\
	}															\
} while (/*CONSTCOND*/ 0)

#else /* ! ALTQ */
#define	ALTQ_DECL(x)		/* nothing */

#define	IFQ_ENQUEUE(ifq, m, pattr, err)							\
do {															\
	if (IF_QFULL((ifq))) {										\
		m_freem((m));											\
		(err) = ENOBUFS;										\
	} else {													\
		IF_ENQUEUE((ifq), (m));									\
		(err) = 0;												\
	}															\
	if ((err))													\
		(ifq)->ifq_drops++;										\
} while (/*CONSTCOND*/ 0)

#define	IFQ_DEQUEUE(ifq, m)	IF_DEQUEUE((ifq), (m))

#define	IFQ_POLL(ifq, m)	IF_POLL((ifq), (m))

#define	IFQ_PURGE(ifq)		IF_PURGE((ifq))

#define	IFQ_SET_READY(ifq)			/* nothing */

#define	IFQ_CLASSIFY(ifq, m, af, pa) /* nothing */

#endif /* ALTQ */

#define	IFQ_IS_EMPTY(ifq)		IF_IS_EMPTY((ifq))
#define	IFQ_INC_LEN(ifq)		((ifq)->ifq_len++)
#define	IFQ_DEC_LEN(ifq)		(--(ifq)->ifq_len)
#define	IFQ_INC_DROPS(ifq)		((ifq)->ifq_drops++)
#define	IFQ_SET_MAXLEN(ifq, len)((ifq)->ifq_maxlen = (len))

#ifdef _KERNEL

/* symbolic names for terminal (per-protocol) CTL_IFQ_ nodes */
#define IFQCTL_LEN 		1
#define IFQCTL_MAXLEN 	2
#define IFQCTL_PEAK 	3
#define IFQCTL_DROPS 	4
#define IFQCTL_MAXID 	5

/*
 * ifq sysctl support
 */
#ifdef INET
int sysctl_ifq_ip4(int *, void *, size_t *, void *, size_t);
#endif
#ifdef INET6
int	sysctl_ifq_ip6(int *, void *, size_t *, void *, size_t);
#endif
#endif /* _KERNEL */

#ifdef __BSD_VISIBLE
/*
 * sysctl for ifq (per-protocol packet input queue variant of ifqueue)
 */
#define CTL_IFQ_NAMES  { 		\
	{ 0, 0 }, 					\
	{ "len", CTLTYPE_INT }, 	\
	{ "maxlen", CTLTYPE_INT }, 	\
	{ "peak", CTLTYPE_INT }, 	\
	{ "drops", CTLTYPE_INT }, 	\
}
#endif /* __BSD_VISIBLE */
#endif /* _NET_IFQ_H_ */
