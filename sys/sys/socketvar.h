/*	$NetBSD: socketvar.h,v 1.69.2.1 2004/05/30 07:02:37 tron Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)socketvar.h	8.3 (Berkeley) 2/19/95
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
 *	@(#)socketvar.h	7.3.1 (2.11BSD GTE) 12/31/93
 */

#ifndef _SYS_SOCKETVAR_H_
#define _SYS_SOCKETVAR_H_

#include <sys/stat.h>			/* for struct stat */
#include <sys/filedesc.h>		/* for struct filedesc */
#include <sys/mbuf.h>			/* for struct mbuf */
#include <sys/protosw.h>		/* for struct protosw */
#include <sys/select.h>			/* for struct selinfo */
#include <sys/queue.h>

TAILQ_HEAD(soqhead, socket);

/*
 * Kernel structure per socket.
 * Contains send and receive buffer queues,
 * handle on protocol and pointer to protocol
 * private data and error information.
 */
struct socket {
	short				so_type;	/* generic type, see socket.h */
	short				so_options;	/* from socket call, see socket.h */
	short				so_linger;	/* time to linger while closing */
	short				so_state;	/* internal state flags SS_*, below */
	void	  			*so_pcb;		/* protocol control block */
	struct	protosw 	*so_proto;	/* protocol handle */
/*
 * Variables for connection queueing.
 * Socket where accepts occur is so_head in all subsidiary sockets.
 * If so_head is 0, socket is not related to an accept.
 * For head socket so_q0 queues partially completed connections,
 * while so_q is a queue of connections ready to be accepted.
 * If a connection is aborted and it has so_head set, then
 * it has to be pulled out of either so_q0 or so_q.
 * We allow connections to queue up based on current queue lengths
 * and limit on number of queued connections for this socket.
 */
	struct	socket 	 	*so_head;	/* back pointer to accept socket */
	struct	soqhead		*so_onq;	/* queue (q or q0) that we're on */
	struct	soqhead	 	so_q0;		/* queue of partial connections */
	struct	soqhead	 	so_q;		/* queue of incoming connections */
	TAILQ_ENTRY(socket) so_qe;		/* our queue entry (q or q0) */
	short				so_q0len;	/* partials on so_q0 */
	short				so_qlen;	/* number of connections on so_q */
	short				so_qlimit;	/* max number queued connections */
	short				so_timeo;	/* connection timeout */
	u_short				so_error;	/* error affecting connection */
	pid_t				so_pgrp;	/* pgrp for signals */
	u_short				so_oobmark;	/* chars to oob mark */
	/*
	 * Variables for socket buffering.
	 */
	struct sockbuf {
		u_short			sb_cc;		/* actual chars in buffer */
		u_short			sb_hiwat;	/* max actual char count */
		u_short			sb_mbcnt;	/* chars of mbufs used */
		u_short			sb_mbmax;	/* max chars of mbufs to use */
		u_short			sb_lowat;	/* low water mark (not used yet) */
		struct mbuf 	*sb_mb;		/* the mbuf chain */
		struct mbuf 	*sb_mbtail;		/* the last mbuf in the chain */
		struct mbuf 	*sb_lastrecord;	/* first mbuf of last record in */
		struct selinfo 	sb_sel;		/* process selecting read/write */
		short			sb_timeo;	/* timeout (not used yet) */
		short			sb_flags;	/* flags, see below */
	} so_rcv, so_snd;

	caddr_t				so_tpcb;	/* Misc. protocol control block XXX */
	void				(*so_upcall)(struct socket *so, caddr_t arg, int waitf);
	caddr_t				so_upcallarg;/* Arg for above */
	uid_t		        so_uid;		/* who opened the socket */
};

/*
 * Socket buf bits.
 */
#define	SB_MAX			(256*1024)	/* max chars in sockbuf */
#define	SB_LOCK			0x01		/* lock on data queue (so_rcv only) */
#define	SB_WANT			0x02		/* someone is waiting to lock */
#define	SB_WAIT			0x04		/* someone is waiting for data/space */
#define	SB_SEL			0x08		/* buffer is selected */
#define	SB_ASYNC		0x10		/* ASYNC I/O, need signals */
#define	SB_COLL			0x20		/* collision selecting */
#define	SB_NOTIFY		(SB_WAIT|SB_SEL|SB_ASYNC)
#define	SB_NOINTR		0x40		/* operations not interruptible */
#define	SB_KNOTE		0x80		/* kernel note attached */

/*
 * Socket state bits.
 */
#define	SS_NOFDREF			0x001	/* no file table ref any more */
#define	SS_ISCONNECTED		0x002	/* socket connected to a peer */
#define	SS_ISCONNECTING		0x004	/* in process of connecting to peer */
#define	SS_ISDISCONNECTED	0x008	/* socket connected to a peer */
#define	SS_ISDISCONNECTING	0x010	/* in process of disconnecting */
#define	SS_CANTSENDMORE		0x020	/* can't send more data to peer */
#define	SS_CANTRCVMORE		0x040	/* can't receive more data from peer */
#define	SS_RCVATMARK		0x080	/* at mark on input */
#define	SS_MORETOCOME		0x400	/* hint from sosend to lower layer; more data coming */

#define	SS_PRIV				0x100	/* privileged for broadcast, raw... */
#define	SS_NBIO				0x200	/* non-blocking ops */
#define	SS_ASYNC			0x400	/* async i/o notify */

/*
 * Macros for sockets and socket buffering.
 */

/* how much space is there in a socket buffer (so->so_snd or so->so_rcv) */
static __inline u_long
sbspace(sb)
	struct sockbuf *sb;
{
	return (MIN((int)(sb->sb_hiwat - sb->sb_cc), (int)(sb->sb_mbmax - sb->sb_mbcnt)));
}

/* do we have to send all at once on a socket? */
static __inline int
sosendallatonce(so)
	struct socket *so;
{
	return (so->so_proto->pr_flags & PR_ATOMIC);
}

/* can we read something from so? */
static __inline int
soreadable(so)
	struct socket *so;
{
	 return (so->so_rcv.sb_cc || (so->so_state & SS_CANTRCVMORE) || so->so_qlen || so->so_error);
}

/* can we write something to so? */
static __inline int
sowriteable(so)
	struct socket *so;
{
	int check = (sbspace(&so->so_snd) > 0 || sbspace(&so->so_snd) >= so->so_snd.sb_lowat);
	return ((check && ((so->so_state & SS_ISCONNECTED) ||
			(so->so_proto->pr_flags & PR_CONNREQUIRED)==0)) ||
			(so->so_state & SS_CANTSENDMORE) || so->so_error);
}

/* adjust counters in sb reflecting allocation of m */
static __inline void
sballoc(sb, m)
	struct sockbuf *sb;
	struct mbuf *m;
{
	sb->sb_cc += m->m_len;
	sb->sb_mbcnt += MSIZE;
	if (m->m_off > MMAXOFF | M_EXT) {
		sb->sb_mbcnt += CLBYTES;
	}
}

/* adjust counters in sb reflecting freeing of m */
static __inline void
sbfree(sb, m)
	struct sockbuf *sb;
	struct mbuf *m;
{
	sb->sb_cc -= m->m_len;
	sb->sb_mbcnt -= MSIZE;
	if (m->m_off > MMAXOFF | M_EXT) {
		sb->sb_mbcnt -= CLBYTES;
	}
}

/* set lock on sockbuf sb */
static __inline void
sblock(sb)
	struct sockbuf *sb;
{
	while (sb->sb_flags & SB_LOCK) {
		sb->sb_flags |= SB_WANT;
		sleep((caddr_t) & sb->sb_flags, PZERO+1);
	}
	sb->sb_flags |= SB_LOCK;
}

/* release lock on sockbuf sb */
static __inline void
sbunlock(sb)
	struct sockbuf *sb;
{
	sb->sb_flags &= ~SB_LOCK;
	if (sb->sb_flags & SB_WANT) {
		sb->sb_flags &= ~SB_WANT;
		wakeup((caddr_t) & sb->sb_flags);
	}
}

#define	sorwakeup(so)	sowakeup((so), &(so)->so_rcv)
#define	sowwakeup(so)	sowakeup((so), &(so)->so_snd)

#ifdef SOCKBUF_DEBUG
/*
 * SBLASTRECORDCHK: check sb->sb_lastrecord is maintained correctly.
 * SBLASTMBUFCHK: check sb->sb_mbtail is maintained correctly.
 *
 * => panic if the socket buffer is inconsistent.
 * => 'where' is used for a panic message.
 */
void	sblastrecordchk(struct sockbuf *, const char *);
#define	SBLASTRECORDCHK(sb, where)	sblastrecordchk((sb), (where))

void	sblastmbufchk(struct sockbuf *, const char *);
#define	SBLASTMBUFCHK(sb, where)	sblastmbufchk((sb), (where))
void	sbcheck(struct sockbuf *);
#define	SBCHECK(sb)					sbcheck(sb)
#else
#define	SBLASTRECORDCHK(sb, where)	/* nothing */
#define	SBLASTMBUFCHK(sb, where)	/* nothing */
#define	SBCHECK(sb)					/* nothing */
#endif /* SOCKBUF_DEBUG */

#ifdef _KERNEL
struct msghdr;

/* to catch callers missing new second argument to sonewconn: */
extern u_long	sb_max;

/* strings for sleep message: */
extern	char netio[], netcon[], netcls[];

/* File Operations on sockets */
int		soo_ioctl(struct file *, u_long, void *, struct proc *);
int		soo_stat(struct socket *, struct stat *, struct proc *);
int		soo_fstat(struct file *, struct stat *, struct proc *);
int		soo_rw(struct file *, struct uio *, struct ucred *);
int		soo_read(struct file *, struct uio *, struct ucred *);
int		soo_write(struct file *, struct uio *, struct ucred *);
int 	soo_close(struct file *, struct proc *);
int		soo_poll(struct file *, int, struct proc *);
int		soo_kqfilter(struct file *, struct knote *);

int		socreate(int, struct socket **, int, int);
int		sobind(struct socket *, struct mbuf *);
int		solisten(struct socket *, int);
void	sofree(struct socket *);
int		soclose(struct socket *);
int		soabort(struct socket *);
int		soaccept(struct socket *, struct mbuf *);
int     soconnect(struct socket *, struct mbuf *);
int		soconnect2(struct socket *, struct socket *);
int		sodisconnect(struct socket *);
int		sosend(struct socket *, struct mbuf *, struct uio *, int, struct mbuf *);
int		soreceive(struct socket *, struct mbuf **, struct uio *, int, struct mbuf **);
int		soshutdown(struct socket *, int);
void	sorflush(struct socket *);
int		sosetopt(struct socket *, int, int, struct mbuf *);
int		sogetopt(struct socket *, int, int, struct mbuf *);
void	sohasoutofband(struct socket *);
int		soacc1(struct socket *);
struct socket *asoqremque(struct socket *, int);
int		connwhile(struct socket *);
int		sogetnam(struct socket *, struct mbuf *);
int		sogetpeer(struct socket *, struct mbuf *);
void	soisconnecting(struct socket *);
void	soisconnected(struct socket *);
void	soisdisconnecting(struct socket *);
void	soisdisconnected(struct socket *);
struct socket *sonewconn(struct socket *);
/* use sonewconn1 when appending the second argument (i.e 'connstatus') */
struct socket *sonewconn1(struct socket *, int);
void	soqinsque(struct socket *, struct socket *, int);
int		soqremque(struct socket *, int);
void	socantsendmore(struct socket *);
void	socantrcvmore(struct socket *);
int		sopoll(struct socket *, int);
int		sokqfilter(struct file *, struct knote *);
void	sbselqueue(struct sockbuf *);
void	sbwait(struct sockbuf *);
void	sbwakeup(struct sockbuf *);
void	sowakeup(struct socket *, struct sockbuf *);
int		soreserve(struct socket *, int, int);
int		sbreserve(struct sockbuf *, u_long);
void	sbrelease(struct sockbuf *);
void	sbappend(struct sockbuf *, struct mbuf *);
void	sbappendrecord(struct sockbuf *, struct mbuf *);
int		sbappendaddr(struct sockbuf *, struct sockaddr *, struct mbuf *, struct mbuf *);
int		sbappendaddrchain(struct sockbuf *, const struct sockaddr *, struct mbuf *, int);
int		sbappendrights(struct sockbuf *, struct mbuf *, struct mbuf *);
void	sbappendstream(struct sockbuf *, struct mbuf *);
void	sbcheck(struct sockbuf *);
void	sbcompress(struct sockbuf *, struct mbuf *, struct mbuf *);
struct mbuf *sbcreatecontrol(caddr_t, int, int, int);
void	sbflush(struct sockbuf *);
void	sbdrop(struct sockbuf *, int);
void	sbdroprecord(struct sockbuf *);
int     sendit(int, struct msghdr *, int);
int     recvit(int, struct msghdr *, int, caddr_t, caddr_t);
struct file *gtsockf(int);
int     sockargs(struct mbuf **, caddr_t, u_int, int);
int     netcopyout(struct mbuf *, char *, int *);

/* sys_kern.c */
void	netcrash();
void	netpsignal(struct proc *, int);
struct proc *netpfind(int);
void	fpflags(struct file *, int, int);
void	fadjust(struct file *, int, int);
int		fpfetch(struct file *, struct file *);
void	unpdet(struct vnode *);
int		unpbind(char *, int, struct vnode **, struct socket *);
int		unpconn(char *, int, struct socket **, struct vnode **);
void	unpgc1(struct file **, struct file **);
int		unpdisc(struct file *);

/*
 * Values for socket-buffer-append priority argument to sbappendaddrchain().
 * The following flags are reserved for future implementation:
 *
 *  SB_PRIO_NONE:  honour normal socket-buffer limits.
 *
 *  SB_PRIO_ONESHOT_OVERFLOW:  if the socket has any space,
 *	deliver the entire chain. Intended for large requests
 *      that should be delivered in their entirety, or not at all.
 *
 * SB_PRIO_OVERDRAFT:  allow a small (2*MLEN) overflow, over and
 *	aboce normal socket limits. Intended messages indicating
 *      buffer overflow in earlier normal/lower-priority messages .
 *
 * SB_PRIO_BESTEFFORT: Ignore  limits entirely.  Intended only for
 * 	kernel-generated messages to specially-marked scokets which
 *	require "reliable" delivery, nd where the source socket/protocol
 *	message generator enforce some hard limit (but possibly well
 *	above kern.sbmax). It is entirely up to the in-kernel source to
 *	avoid complete mbuf exhaustion or DoS scenarios.
 */

#define SB_PRIO_NONE 	 			0
#define SB_PRIO_ONESHOT_OVERFLOW 	1
#define SB_PRIO_OVERDRAFT			2
#define SB_PRIO_BESTEFFORT			3

#endif
#endif /* _SYS_SOCKETVAR_H_ */
