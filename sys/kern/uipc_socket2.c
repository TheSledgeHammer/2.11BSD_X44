/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)uipc_socket2.c	8.2 (Berkeley) 2/14/95
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)uipc_socket2.c	7.3 (Berkeley) 1/28/88
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/event.h>
#include <sys/protosw.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

/*
 * Primitive routines for operating on sockets and socket buffers
 */

/* strings for sleep message: */
char	netio[] = "netio";
char	netcon[] = "netcon";
char	netcls[] = "netcls";

u_long	sb_max = SB_MAX;		/* patchable */

/*
 * Procedures to manipulate state flags of socket
 * and do appropriate wakeups.  Normal sequence from the
 * active (originating) side is that soisconnecting() is
 * called during processing of connect() call,
 * resulting in an eventual call to soisconnected() if/when the
 * connection is established.  When the connection is torn down
 * soisdisconnecting() is called during processing of disconnect() call,
 * and soisdisconnected() is called when the connection to the peer
 * is totally severed.  The semantics of these routines are such that
 * connectionless protocols can call soisconnected() and soisdisconnected()
 * only, bypassing the in-progress calls when setting up a ``connection''
 * takes no time.
 *
 * From the passive side, a socket is created with
 * two queues of sockets: so_q0 for connections in progress
 * and so_q for connections already made and awaiting user acceptance.
 * As a protocol is preparing incoming connections, it creates a socket
 * structure queued on so_q0 by calling sonewconn().  When the connection
 * is established, soisconnected() is called, and transfers the
 * socket structure to so_q, making it available to accept().
 * 
 * If a socket is closed with sockets on either
 * so_q0 or so_q, these sockets are dropped.
 *
 * If higher level protocols are implemented in
 * the kernel, the wakeups done here will sometimes
 * cause software-interrupt process scheduling.
 */
void
soisconnecting(so)
	register struct socket *so;
{

	so->so_state &= ~(SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTING;
	wakeup((caddr_t)&so->so_timeo);
}

void
soisconnected(so)
	register struct socket *so;
{
	register struct socket *head = so->so_head;

	if (head) {
		if (soqremque(so, 0) == 0)
			panic("soisconnected");
		soqinsque(head, so, 1);
		sorwakeup(head);
		wakeup((caddr_t)&head->so_timeo);
	}
	so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTED;
	wakeup((caddr_t)&so->so_timeo);
	sorwakeup(so);
	sowwakeup(so);
}

void
soisdisconnecting(so)
	register struct socket *so;
{

	so->so_state &= ~SS_ISCONNECTING;
	so->so_state |= (SS_ISDISCONNECTING|SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

void
soisdisconnected(so)
	register struct socket *so;
{

	so->so_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

/*
 * soconnstatus: external static declaration of connstatus.
 * - compatibility with 4.4BSD-lite2 and later. That
 * provides the missing "connstatus" argument in 2.11BSD's sonewconn function.
 *
 * Notes:
 * Calling sonewconn will always default the connstatus to 0.
 * This means that you must call sonewconn1 if the connstatus
 * should be anything other than 0.
 */
static int soconnstatus = 0;

/*
 * When an attempt at a new connection is noted on a socket
 * which accepts connections, sonewconn is called.  If the
 * connection is possible (subject to space constraints, etc.)
 * then we allocate a new structure, properly linked into the
 * data structure of the original socket, and return this.
 */
/*
 * For appending the missing second argument. use sonewconn1
 */
struct socket *
sonewconn(head)
	register struct socket *head;
{
	register struct socket *so;
	register struct mbuf *m;

	if (head->so_qlen + head->so_q0len > 3 * head->so_qlimit / 2)
		goto bad;
	m = m_getclr(M_DONTWAIT, MT_SOCKET);
	if (m == NULL)
		goto bad;
	so = mtod(m, struct socket *);
	so->so_type = head->so_type;
	so->so_options = head->so_options &~ SO_ACCEPTCONN;
	so->so_linger = head->so_linger;
	so->so_state = head->so_state | SS_NOFDREF;
	so->so_proto = head->so_proto;
	so->so_timeo = head->so_timeo;
	so->so_pgrp = head->so_pgrp;
	(void) soreserve(so, head->so_snd.sb_hiwat, head->so_rcv.sb_hiwat);
	soqinsque(head, so, soconnstatus);
	if ((*so->so_proto->pr_usrreq)(so, PRU_ATTACH,
	    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0, u.u_procp)) {
		(void) soqremque(so, soconnstatus);
		(void) m_free(m);
		goto bad;
	}
	return (so);
bad:
	return ((struct socket *)0);
}

/*
 * Should use sonewconn1 whenever the 'connstatus'
 * (i.e. the missing second argument) is not 0.
 * Example:
 * - calling 'sonewconn(head)' is equal
 *  	to calling 'sonewconn1(head, 0)'
 */
struct socket *
sonewconn1(head, connstatus)
	register struct socket *head;
	int connstatus;
{
	struct socket	*so;
	int		soqueue;

	soqueue = connstatus ? 1 : 0;
	if (soqueue != 0) {
		soconnstatus = 1;
	} else {
		soconnstatus = 0;
	}
	so = sonewconn(head);
	if (connstatus) {
		sorwakeup(head);
		wakeup((caddr_t) &head->so_timeo);
		so->so_state |= connstatus;
	}
	return (so);
}

void
soqinsque(head, so, q)
	register struct socket *head, *so;
	int q;
{
	KASSERT(so->so_onq == NULL);

	so->so_head = head;
	if (q == 0) {
		head->so_q0len++;
		so->so_onq = &head->so_q0;
	} else {
		head->so_qlen++;
		so->so_onq = &head->so_q;
	}
	TAILQ_INSERT_TAIL(so->so_onq, so, so_qe);
}

int
soqremque(so, q)
	register struct socket *so;
	int q;
{
	register struct socket *head;

	head = so->so_head;
	if (q == 0) {
		if (so->so_onq != &head->so_q0)
			return (0);
		head->so_q0len--;
	} else {
		if (so->so_onq != &head->so_q)
			return (0);
		head->so_qlen--;
	}
	TAILQ_REMOVE(so->so_onq, so, so_qe);
	so->so_onq = NULL;
	so->so_head = NULL;
	return (1);
}

/*
 * Socantsendmore indicates that no more data will be sent on the
 * socket; it would normally be applied to a socket when the user
 * informs the system that no more data is to be sent, by the protocol
 * code (in case PRU_SHUTDOWN).  Socantrcvmore indicates that no more data
 * will be received, and will normally be applied to the socket by a
 * protocol when it detects that the peer will send no more data.
 * Data queued for reading in the socket may yet be read.
 */
void
socantsendmore(so)
	struct socket *so;
{

	so->so_state |= SS_CANTSENDMORE;
	sowwakeup(so);
}

void
socantrcvmore(so)
	struct socket *so;
{

	so->so_state |= SS_CANTRCVMORE;
	sorwakeup(so);
}

/*
 * Socket select/wakeup routines.
 */

/*
 * Queue a process for a select on a socket buffer.
 */
void
sbselqueue(sb)
	register struct sockbuf *sb;
{
	register struct proc *p;
	extern int selwait;

	p = pfind(sb->sb_sel.si_pid);
	if(p != NULL) {
		goto select;
	} else {
		p = u.u_procp;
		goto select;
	}

select:
	if ((p->p_wchan == (caddr_t)&selwait)) {
		selrecord(p, &sb->sb_sel);
		sb->sb_flags |= SB_COLL;
	}
}

/*
 * Wait for data to arrive at/drain from a socket buffer.
 */
void
sbwait(sb)
	register struct sockbuf *sb;
{
	sb->sb_flags |= SB_WAIT;
	sleep((caddr_t)&sb->sb_cc, PZERO+1);
}

/*
 * Wakeup processes waiting on a socket buffer.
 */
void
sbwakeup(sb)
	register struct sockbuf *sb;
{

	if (&sb->sb_sel != NULL) {
		selwakeup1(&sb->sb_sel);
		sb->sb_flags &= ~SB_COLL;
	}
	if (sb->sb_flags & SB_WAIT) {
		sb->sb_flags &= ~SB_WAIT;
		wakeup((caddr_t)&sb->sb_cc);
	}
}

/*
 * Wakeup socket readers and writers.
 * Do asynchronous notification via SIGIO
 * if the socket has the SS_ASYNC flag set.
 */
void
sowakeup(so, sb)
	register struct socket *so;
	struct sockbuf *sb;
{
	register struct proc *p;

	sbwakeup(sb);
	if (so->so_state & SS_ASYNC) {
		if (so->so_pgrp < 0)
			gsignal(-so->so_pgrp, SIGIO);
		else if (so->so_pgrp > 0 &&
		    (p = (struct proc *)netpfind(so->so_pgrp)) != 0)
			netpsignal(p, SIGIO);
	}
}

/*
 * Socket buffer (struct sockbuf) utility routines.
 *
 * Each socket contains two socket buffers: one for sending data and
 * one for receiving data.  Each buffer contains a queue of mbufs,
 * information about the number of mbufs and amount of data in the
 * queue, and other fields allowing select() statements and notification
 * on data availability to be implemented.
 *
 * Data stored in a socket buffer is maintained as a list of records.
 * Each record is a list of mbufs chained together with the m_next
 * field.  Records are chained together with the m_act field. The upper
 * level routine soreceive() expects the following conventions to be
 * observed when placing information in the receive buffer:
 *
 * 1. If the protocol requires each message be preceded by the sender's
 *    name, then a record containing that name must be present before
 *    any associated data (mbuf's must be of type MT_SONAME).
 * 2. If the protocol supports the exchange of ``access rights'' (really
 *    just additional data associated with the message), and there are
 *    ``rights'' to be received, then a record containing this data
 *    should be present (mbuf's must be of type MT_RIGHTS).
 * 3. If a name or rights record exists, then it must be followed by
 *    a data record, perhaps of zero length.
 *
 * Before using a new socket structure it is first necessary to reserve
 * buffer space to the socket, by calling sbreserve().  This should commit
 * some of the available buffer space in the system buffer pool for the
 * socket (currently, it does nothing but enforce limits).  The space
 * should be released by calling sbrelease() when the socket is destroyed.
 */

int
soreserve(so, sndcc, rcvcc)
	register struct socket *so;
	int sndcc, rcvcc;
{

	if (sbreserve(&so->so_snd, sndcc) == 0)
		goto bad;
	if (sbreserve(&so->so_rcv, rcvcc) == 0)
		goto bad2;
	return (0);
bad2:
	sbrelease(&so->so_snd);
bad:
	return (ENOBUFS);
}

/*
 * Allot mbufs to a sockbuf.
 * Attempt to scale cc so that mbcnt doesn't become limiting
 * if buffering efficiency is near the normal case.
 */
int
sbreserve(sb, cc)
	struct sockbuf *sb;
	u_long cc;
{

#ifdef FIX_43
	if ((unsigned) cc > (unsigned)SB_MAX * CLBYTES / (2 * MSIZE + CLBYTES))
		return (0);
#else
	if ((unsigned) cc > (unsigned)SB_MAX)
		return (0);
#endif
	sb->sb_hiwat = cc;
	sb->sb_mbmax = MIN(cc * 2, SB_MAX);
	return (1);
}

/*
 * Free mbufs held by a socket, and reserved mbuf space.
 */
void
sbrelease(sb)
	struct sockbuf *sb;
{

	sbflush(sb);
	sb->sb_hiwat = sb->sb_mbmax = 0;
}

/*
 * Routines to add and remove
 * data from an mbuf queue.
 *
 * The routines sbappend() or sbappendrecord() are normally called to
 * append new mbufs to a socket buffer, after checking that adequate
 * space is available, comparing the function sbspace() with the amount
 * of data to be added.  sbappendrecord() differs from sbappend() in
 * that data supplied is treated as the beginning of a new record.
 * To place a sender's address, optional access rights, and data in a
 * socket receive buffer, sbappendaddr() should be used.  To place
 * access rights and data in a socket receive buffer, sbappendrights()
 * should be used.  In either case, the new data begins a new record.
 * Note that unlike sbappend() and sbappendrecord(), these routines check
 * for the caller that there will be enough space to store the data.
 * Each fails if there is not enough space, or if it cannot find mbufs
 * to store additional information in.
 *
 * Reliable protocols may use the socket send buffer to hold data
 * awaiting acknowledgement.  Data is normally copied from a socket
 * send buffer in a protocol with m_copy for output to a peer,
 * and then removing the data from the socket buffer with sbdrop()
 * or sbdroprecord() when the data is acknowledged by the peer.
 */

/*
 * Append mbuf chain m to the last record in the
 * socket buffer sb.  The additional space associated
 * the mbuf chain is recorded in sb.  Empty mbufs are
 * discarded and mbufs are compacted where possible.
 */
void
sbappend(sb, m)
	struct sockbuf *sb;
	struct mbuf *m;
{
	register struct mbuf *n;

	if (m == 0)
		return;
	if (n == sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		while (n->m_next)
			n = n->m_next;
	}
	sbcompress(sb, m, n);
}

/*
 * As above, except the mbuf chain
 * begins a new record.
 */
void
sbappendrecord(sb, m0)
	register struct sockbuf *sb;
	register struct mbuf *m0;
{
	register struct mbuf *m;

	if (m0 == 0)
		return;
	if (m == sb->sb_mb)
		while (m->m_act)
			m = m->m_act;
	/*
	 * Put the first mbuf on the queue.
	 * Note this permits zero length records.
	 */
	sballoc(sb, m0);
	if (m)
		m->m_act = m0;
	else
		sb->sb_mb = m0;
	m = m0->m_next;
	m0->m_next = 0;
	sbcompress(sb, m, m0);
}

/*
 * Append address and data, and optionally, rights
 * to the receive queue of a socket.  Return 0 if
 * no space in sockbuf or insufficient mbufs.
 */
int
sbappendaddr(sb, asa, m0, rights0)
	register struct sockbuf *sb;
	struct sockaddr *asa;
	struct mbuf *rights0, *m0;
{
	register struct mbuf *m, *n;
	int space = sizeof (*asa);

	for (m = m0; m; m = m->m_next)
		space += m->m_len;
	if (rights0)
		space += rights0->m_len;
	if (space > sbspace(sb))
		return (0);
	MGET(m, M_DONTWAIT, MT_SONAME);
	if (m == 0)
		return (0);
	*mtod(m, struct sockaddr *) = *asa;
	m->m_len = sizeof (*asa);
	if (rights0 && rights0->m_len) {
		m->m_next = m_copy(rights0, 0, rights0->m_len);
		if (m->m_next == 0) {
			m_freem(m);
			return (0);
		}
		sballoc(sb, m->m_next);
	}
	sballoc(sb, m);
	if (n == sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		n->m_act = m;
	} else
		sb->sb_mb = m;
	if (m->m_next)
		m = m->m_next;
	if (m0)
		sbcompress(sb, m0, m);
	return (1);
}

#define	SBLINKRECORDCHAIN(sb, m0, mlast)			\
	do {											\
		if ((sb)->sb_lastrecord != NULL) {			\
			(sb)->sb_lastrecord->m_nextpkt = (m0);	\
		} else {									\
			(sb)->sb_mb = (m0);						\
		} 											\
	(sb)->sb_lastrecord = (mlast);					\
} while (/*CONSTCOND*/0)

#define	SBLINKRECORD(sb, m0)						\
	SBLINKRECORDCHAIN(sb, m0, m0)

/*
 * Helper for sbappendchainaddr: prepend a struct sockaddr* to
 * an mbuf chain.
 */
static __inline struct mbuf *
m_prepend_sockaddr(sb, m0, asa)
	struct sockbuf *sb;
	struct mbuf *m0;
	const struct sockaddr *asa;
{
	struct mbuf *m;
	const int salen = asa->sa_len;

	/* only the first in each chain need be a pkthdr */
	MGETHDR(m, M_DONTWAIT, MT_SONAME);
	if (m == 0) {
		return (0);
	}

	KASSERT(salen <= MHLEN);

	m->m_len = salen;
	bcopy((caddr_t)asa, mtod(m, caddr_t), salen);
	m->m_next = m0;
	m->m_pkthdr.len = salen + m0->m_pkthdr.len;

	return m;
}

int
sbappendaddrchain(sb, asa, m0, sbprio)
	struct sockbuf *sb;
	const struct sockaddr *asa;
	struct mbuf *m0;
	int sbprio;
{
	struct mbuf *m, *n, *n0, *nlast;
	int space;
	int error;

	(void)sbprio;

	if (m0 && (m0->m_flags & M_PKTHDR) == 0) {
		panic("sbappendaddrchain");
	}

	space = sbspace(sb);
	n0 = NULL;
	nlast = NULL;
	for (m = m0; m; m = m->m_nextpkt) {
		struct mbuf *np;

		/* Prepend sockaddr to this record (m) of input chain m0 */
		n = m_prepend_sockaddr(sb, m, asa);
		if (n == NULL) {
			error = ENOBUFS;
			goto bad;
		}

		/* Append record (asa+m) to end of new chain n0 */
		if (n0 == NULL) {
			n0 = n;
		} else {
			nlast->m_nextpkt = n;
		}
		/* Keep track of last record on new chain */
		nlast = n;

		for (np = n; np; np = np->m_next) {
			sballoc(sb, np);
		}
	}

	SBLASTRECORDCHK(sb, "sbappendaddrchain 1");

	/* Drop the entire chain of (asa+m) records onto the socket */
	SBLINKRECORDCHAIN(sb, n0, nlast);

	SBLASTRECORDCHK(sb, "sbappendaddrchain 2");

	for (m = nlast; m->m_next; m = m->m_next)
		;
	sb->sb_mbtail = m;
	SBLASTMBUFCHK(sb, "sbappendaddrchain");

	return (1);
bad:
	/*
	 * On error, free the prepended addreseses. For consistency
	 * with sbappendaddr(), leave it to our caller to free
	 * the input record chain passed to us as m0.
	 */
	while ((n = n0) != NULL) {
		struct mbuf *np;

		/* Undo the sballoc() of this record */
		for (np = n; np; np = np->m_next)
			sbfree(sb, np);

		n0 = n->m_nextpkt; 	/* iterate at next prepended address */
		MFREE(n, np); 		/* free prepended address (not data) */
	}
	return 0;
}

int
sbappendrights(sb, m0, rights)
	struct sockbuf *sb;
	struct mbuf *rights, *m0;
{
	register struct mbuf *m, *n;
	int space = 0;

	if (rights == 0)
		panic("sbappendrights");
	for (m = m0; m; m = m->m_next)
		space += m->m_len;
	space += rights->m_len;
	if (space > sbspace(sb))
		return (0);
	m = m_copy(rights, 0, rights->m_len);
	if (m == 0)
		return (0);
	sballoc(sb, m);
	if (n == sb->sb_mb) {
		while (n->m_act)
			n = n->m_act;
		n->m_act = m;
	} else
		sb->sb_mb = m;
	if (m0)
		sbcompress(sb, m0, m);
	return (1);
}

void
sbappendstream(sb, m)
	struct sockbuf *sb;
	struct mbuf *m;
{
	KDASSERT(m->m_nextpkt == NULL);
	KASSERT(sb->sb_mb == sb->sb_lastrecord);

	SBLASTMBUFCHK(sb, __func__);

	sbcompress(sb, m, sb->sb_mbtail);

	sb->sb_lastrecord = sb->sb_mb;
	SBLASTRECORDCHK(sb, __func__);
}

/*
 * Compress mbuf chain m into the socket
 * buffer sb following mbuf n.  If n
 * is null, the buffer is presumed empty.
 */
void
sbcompress(sb, m, n)
	register struct sockbuf *sb;
	register struct mbuf *m, *n;
{

	while (m) {
		if (m->m_len == 0) {
			m = m_free(m);
			continue;
		}
		if (n && n->m_off <= MMAXOFF && m->m_off <= MMAXOFF
				&& (n->m_off + n->m_len + m->m_len) <= MMAXOFF
				&& n->m_type == m->m_type) {
			bcopy(mtod(m, caddr_t), mtod(n, caddr_t) + n->m_len,
					(unsigned) m->m_len);
			n->m_len += m->m_len;
			sb->sb_cc += m->m_len;
			m = m_free(m);
			continue;
		}
		sballoc(sb, m);
		if (n)
			n->m_next = m;
		else
			sb->sb_mb = m;
		n = m;
		m = m->m_next;
		n->m_next = 0;
	}
}

/*
 * Free all mbufs in a sockbuf.
 * Check that all resources are reclaimed.
 */
void
sbflush(sb)
	register struct sockbuf *sb;
{

	if (sb->sb_flags & SB_LOCK)
		panic("sbflush");
	while (sb->sb_mbcnt)
		sbdrop(sb, (int)sb->sb_cc);
	if (sb->sb_cc || sb->sb_mbcnt || sb->sb_mb)
		panic("sbflush 2");
}

/*
 * Drop data from (the front of) a sockbuf.
 */
void
sbdrop(sb, len)
	register struct sockbuf *sb;
	register int len;
{
	register struct mbuf *m, *mn;
	struct mbuf *next;

	next = (m == sb->sb_mb) ? m->m_act : 0;
	while (len > 0) {
		if (m == 0) {
			if (next == 0)
				panic("sbdrop");
			m = next;
			next = m->m_act;
			continue;
		}
		if (m->m_len > len) {
			m->m_len -= len;
			m->m_off += len;
			sb->sb_cc -= len;
			break;
		}
		len -= m->m_len;
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	while (m && m->m_len == 0) {
		sbfree(sb, m);
		MFREE(m, mn);
		m = mn;
	}
	if (m) {
		sb->sb_mb = m;
		m->m_act = next;
	} else
		sb->sb_mb = next;
}

/*
 * Drop a record off the front of a sockbuf
 * and move the next record to the front.
 */
void
sbdroprecord(sb)
	register struct sockbuf *sb;
{
	register struct mbuf *m, *mn;

	m = sb->sb_mb;
	if (m) {
		sb->sb_mb = m->m_act;
		do {
			sbfree(sb, m);
			MFREE(m, mn);
		} while (m == mn);
	}
}


/*
 * Create a "control" mbuf containing the specified data
 * with the specified type for presentation on a socket buffer.
 */
struct mbuf *
sbcreatecontrol(p, size, type, level)
	caddr_t p;
	int size, type, level;
{
	struct cmsghdr	*cp;
	struct mbuf	*m;

	if (CMSG_SPACE(size) > MCLBYTES) {
		printf("sbcreatecontrol: message too large %d\n", size);
		return NULL;
	}

	if ((m = m_get(M_DONTWAIT, MT_CONTROL)) == NULL)
		return ((struct mbuf *) NULL);
	if (CMSG_SPACE(size) > MLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return NULL;
		}
	}
	cp = mtod(m, struct cmsghdr *);
	memcpy(CMSG_DATA(cp), p, size);
	m->m_len = CMSG_SPACE(size);
	cp->cmsg_len = CMSG_LEN(size);
	cp->cmsg_level = level;
	cp->cmsg_type = type;
	return (m);
}

static int
sodopoll(so, events)
	struct socket *so;
	int events;
{
	int revents;

	revents = 0;

	if (events & (POLLIN | POLLRDNORM))
		if (soreadable(so))
			revents |= events & (POLLIN | POLLRDNORM);

	if (events & (POLLOUT | POLLWRNORM))
		if (sowriteable(so))
			revents |= events & (POLLOUT | POLLWRNORM);

	if (events & (POLLPRI | POLLRDBAND))
		if (so->so_state & POLLRDBAND)
			revents |= events & (POLLPRI | POLLRDBAND);

	return revents;
}

int
sopoll(so, events)
	struct socket *so;
	int events;
{
	int revents = 0;

#ifndef DIAGNOSTIC
	/*
	 * Do a quick, unlocked check in expectation that the socket
	 * will be ready for I/O.  Don't do this check if DIAGNOSTIC,
	 * as the solocked() assertions will fail.
	 */
	if ((revents = sodopoll(so, events)) != 0)
		return revents;
#endif

	//solock(so);
	if ((revents = sodopoll(so, events)) == 0) {
		if (events & (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND)) {
			selrecord(curproc, &so->so_rcv.sb_sel);
			so->so_rcv.sb_flags |= SB_NOTIFY;
		}

		if (events & (POLLOUT | POLLWRNORM)) {
			selrecord(curproc, &so->so_snd.sb_sel);
			so->so_snd.sb_flags |= SB_NOTIFY;
		}
	}
	//sounlock(so);

	return revents;
}

static void
filt_sordetach(kn)
	struct knote *kn;
{
	struct socket	*so;

	so = (struct socket *)kn->kn_fp->f_data;
	SIMPLEQ_REMOVE(&so->so_rcv.sb_sel.si_klist, kn, knote, kn_selnext);
	if (SIMPLEQ_EMPTY(&so->so_rcv.sb_sel.si_klist))
		so->so_rcv.sb_flags &= ~SB_KNOTE;
}

/*ARGSUSED*/
static int
filt_soread(kn, hint)
	struct knote *kn;
	long hint;
{
	struct socket	*so;

	so = (struct socket*) kn->kn_fp->f_data;
	kn->kn_data = so->so_rcv.sb_cc;
	if (so->so_state & SS_CANTRCVMORE) {
		kn->kn_flags |= EV_EOF;
		kn->kn_fflags = so->so_error;
		return (1);
	}
	if (so->so_error) /* temporary udp error */
		return (1);
	if (kn->kn_sfflags & NOTE_LOWAT)
		return (kn->kn_data >= kn->kn_sdata);
	return (kn->kn_data >= so->so_rcv.sb_lowat);
}

static void
filt_sowdetach(kn)
	struct knote *kn;
{
	struct socket	*so;

	so = (struct socket*) kn->kn_fp->f_data;
	SIMPLEQ_REMOVE(&so->so_snd.sb_sel.si_klist, kn, knote, kn_selnext);
	if (SIMPLEQ_EMPTY(&so->so_snd.sb_sel.si_klist))
		so->so_snd.sb_flags &= ~SB_KNOTE;
}

/*ARGSUSED*/
static int
filt_sowrite(kn, hint)
	struct knote *kn;
	long hint;
{
	struct socket	*so;

	so = (struct socket*) kn->kn_fp->f_data;
	kn->kn_data = sbspace(&so->so_snd);
	if (so->so_state & SS_CANTSENDMORE) {
		kn->kn_flags |= EV_EOF;
		kn->kn_fflags = so->so_error;
		return (1);
	}
	if (so->so_error) /* temporary udp error */
		return (1);
	if (((so->so_state & SS_ISCONNECTED) == 0)
			&& (so->so_proto->pr_flags & PR_CONNREQUIRED))
		return (0);
	if (kn->kn_sfflags & NOTE_LOWAT)
		return (kn->kn_data >= kn->kn_sdata);
	return (kn->kn_data >= so->so_snd.sb_lowat);
}

/*ARGSUSED*/
static int
filt_solisten(kn, hint)
	struct knote *kn;
	long hint;
{
	struct socket	*so;

	so = (struct socket *)kn->kn_fp->f_data;

	/*
	 * Set kn_data to number of incoming connections, not
	 * counting partial (incomplete) connections.
	 */
	kn->kn_data = so->so_qlen;
	return (kn->kn_data > 0);
}

static const struct filterops solisten_filtops =
	{ 1, NULL, filt_sordetach, filt_solisten };
static const struct filterops soread_filtops =
	{ 1, NULL, filt_sordetach, filt_soread };
static const struct filterops sowrite_filtops =
	{ 1, NULL, filt_sowdetach, filt_sowrite };

int
sokqfilter(fp, kn)
	struct file *fp;
	struct knote *kn;
{
	struct socket	*so;
	struct sockbuf	*sb;

	so = (struct socket*) kn->kn_fp->f_data;
	switch (kn->kn_filter) {
	case EVFILT_READ:
		if (so->so_options & SO_ACCEPTCONN)
			kn->kn_fop = &solisten_filtops;
		else
			kn->kn_fop = &soread_filtops;
		sb = &so->so_rcv;
		break;
	case EVFILT_WRITE:
		kn->kn_fop = &sowrite_filtops;
		sb = &so->so_snd;
		break;
	default:
		return (1);
	}
	SIMPLEQ_INSERT_HEAD(&sb->sb_sel.si_klist, kn, kn_selnext);
	sb->sb_flags |= SB_KNOTE;
	return (0);
}


/*
 * Routines to add and remove
 * data from an mbuf queue.
 *
 * The routines sbappend() or sbappendrecord() are normally called to
 * append new mbufs to a socket buffer, after checking that adequate
 * space is available, comparing the function sbspace() with the amount
 * of data to be added.  sbappendrecord() differs from sbappend() in
 * that data supplied is treated as the beginning of a new record.
 * To place a sender's address, optional access rights, and data in a
 * socket receive buffer, sbappendaddr() should be used.  To place
 * access rights and data in a socket receive buffer, sbappendrights()
 * should be used.  In either case, the new data begins a new record.
 * Note that unlike sbappend() and sbappendrecord(), these routines check
 * for the caller that there will be enough space to store the data.
 * Each fails if there is not enough space, or if it cannot find mbufs
 * to store additional information in.
 *
 * Reliable protocols may use the socket send buffer to hold data
 * awaiting acknowledgement.  Data is normally copied from a socket
 * send buffer in a protocol with m_copy for output to a peer,
 * and then removing the data from the socket buffer with sbdrop()
 * or sbdroprecord() when the data is acknowledged by the peer.
 */

#ifdef SOCKBUF_DEBUG
void
sbcheck(sb)
	struct sockbuf *sb;
{
	struct mbuf	*m;
	u_long		len, mbcnt;

	len = 0;
	mbcnt = 0;
	for (m = sb->sb_mb; m; m = m->m_next) {
		len += m->m_len;
		mbcnt += MSIZE;
		if (m->m_flags & M_EXT)
			mbcnt += m->m_ext.ext_size;
		if (m->m_nextpkt)
			panic("sbcheck nextpkt");
	}
	if (len != sb->sb_cc || mbcnt != sb->sb_mbcnt) {
		printf("cc %lu != %lu || mbcnt %lu != %lu\n", len, sb->sb_cc,
		    mbcnt, sb->sb_mbcnt);
		panic("sbcheck");
	}
}

void
sblastrecordchk(sb, where)
	struct sockbuf *sb;
	const char *where;
{
	struct mbuf *m;

	m = sb->sb_mb;
	while (m && m->m_nextpkt)
		m = m->m_nextpkt;

	if (m != sb->sb_lastrecord) {
		printf("sblastrecordchk: sb_mb %p sb_lastrecord %p last %p\n",
				sb->sb_mb, sb->sb_lastrecord, m);
		printf("packet chain:\n");
		for (m = sb->sb_mb; m != NULL; m = m->m_nextpkt)
			printf("\t%p\n", m);
		panic("sblastrecordchk from %s", where);
	}
}

void
sblastmbufchk(sb, where)
	struct sockbuf *sb;
	const char *where;
{
	struct mbuf *m, *n;

	m = sb->sb_mb;
	while (m && m->m_nextpkt)
		m = m->m_nextpkt;

	while (m && m->m_next)
		m = m->m_next;

	if (m != sb->sb_mbtail) {
		printf("sblastmbufchk: sb_mb %p sb_mbtail %p last %p\n", sb->sb_mb,
				sb->sb_mbtail, m);
		printf("packet tree:\n");
		for (m = sb->sb_mb; m != NULL; m = m->m_nextpkt) {
			printf("\t");
			for (n = m; n != NULL; n = n->m_next)
				printf("%p ", n);
			printf("\n");
		}
		panic("sblastmbufchk from %s", where);
	}
}
#endif /* SOCKBUF_DEBUG */
