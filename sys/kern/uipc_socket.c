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
 *	@(#)uipc_socket.c	8.6 (Berkeley) 5/2/95
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
 *	@(#)uipc_socket.c	7.8.1 (2.11BSD GTE) 11/26/94
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
//#ifdef INET
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/uio.h>


/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 *
 * TODO:
 *	test socketpair
 *	clean up async
 *	out-of-band is a kludge
 */
/*ARGSUSED*/
int
socreate(dom, aso, type, proto)
	int dom;
	struct socket **aso;
	register int type;
	int proto;
{
	register struct protosw *prp;
	register struct socket *so;
	register struct mbuf *m;
	register int error;

	if (proto)
		prp = pffindproto(dom, proto, type);
	else
		prp = pffindtype(dom, type);
	if (prp == 0)
		return (EPROTONOSUPPORT);
	if (prp->pr_type != type)
		return (EPROTOTYPE);
	m = m_getclr(M_WAIT, MT_SOCKET);
	so = mtod(m, struct socket *);
	TAILQ_INIT(&so->so_q0);
	TAILQ_INIT(&so->so_q);
	so->so_options = 0;
	so->so_state = 0;
	so->so_type = type;
	if (u.u_uid == 0)
		so->so_state = SS_PRIV;
	so->so_proto = prp;
/*	if (u.u_procp != 0) {
		so->so_uid = u.u_ucred->cr_uid;
	}*/
	error = (*prp->pr_usrreq)(so, PRU_ATTACH, (struct mbuf*) 0,
			(struct mbuf*) (long) proto, (struct mbuf*) 0, u.u_procp);
	if (error) {
		so->so_state |= SS_NOFDREF;
		sofree(so);
		return (error);
	}
	*aso = so;
	return (0);
}

int
sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s ,error;

	s = splnet();
	error = (*so->so_proto->pr_usrreq)(so, PRU_BIND, (struct mbuf*) 0, nam,
			(struct mbuf*) 0, u.u_procp);
	splx(s);
	return (error);
}

int
solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	int s, error;

	s = splnet();
	error = (*so->so_proto->pr_usrreq)(so, PRU_LISTEN, (struct mbuf*) 0,
			(struct mbuf*) 0, (struct mbuf*) 0, u.u_procp);
	if (error) {
		splx(s);
		return (error);
	}
	if (TAILQ_EMPTY(&so->so_q)) {
		so->so_options |= SO_ACCEPTCONN;
	}
	if (backlog < 0)
		backlog = 0;
	so->so_qlimit = MIN(backlog, SOMAXCONN);
	splx(s);
	return (0);
}

void
sofree(so)
	register struct socket *so;
{

	if (so->so_pcb || (so->so_state & SS_NOFDREF) == 0)
		return;
	if (so->so_head) {
		if (!soqremque(so, 0) && !soqremque(so, 1)) {
			panic("sofree dq");
		}
		so->so_head = 0;
	}
	sbrelease(&so->so_snd);
	sorflush(so);
	(void) m_free(dtom(so));
}

/*
 * Close a socket on last file table reference removal.
 * Initiate disconnect if connected.
 * Free socket when disconnect complete.
 */
int
soclose(so)
	register struct socket *so;
{
	struct socket *so2;
	int s;		/* conservative */
	int error;

	error = 0;
	s = splnet();
	if (so->so_options & SO_ACCEPTCONN) {
		while ((so2 = TAILQ_FIRST(&so->so_q0)) != 0) {
			(void) soqremque(so2, 0);
			(void) soabort(so2);
		}
		while ((so2 = TAILQ_FIRST(&so->so_q)) != 0) {
			(void) soqremque(so2, 1);
			(void) soabort(so2);
		}
	}
	if (so->so_pcb == 0)
		goto discard;
	if (so->so_state & SS_ISCONNECTED) {
		if ((so->so_state & SS_ISDISCONNECTING) == 0) {
			error = sodisconnect(so);
			if (error)
				goto drop;
		}
		if (so->so_options & SO_LINGER) {
			if ((so->so_state & SS_ISDISCONNECTING) && (so->so_state & SS_NBIO))
				goto drop;
			while (so->so_state & SS_ISCONNECTED) {
				error = tsleep((caddr_t) & so->so_timeo, PSOCK | PCATCH, netcls,
						so->so_linger * hz);
				if (error)
					break;
			}
		}
	}
drop:
	if (so->so_pcb) {
		int error2 = (*so->so_proto->pr_usrreq)(so, PRU_DETACH,
				(struct mbuf*) 0, (struct mbuf*) 0, (struct mbuf*) 0, u.u_procp);
		if (error == 0)
			error = error2;
	}
discard:
	if (so->so_state & SS_NOFDREF)
		panic("soclose: NOFDREF");
	so->so_state |= SS_NOFDREF;
	sofree(so);
	splx(s);
	return (error);
}

/*
 * Must be called at splnet...
 */
int
soabort(so)
	struct socket *so;
{
	return ((*so->so_proto->pr_usrreq)(so, PRU_ABORT, (struct mbuf*) 0,
			(struct mbuf*) 0, (struct mbuf*) 0, u.u_procp));
}

int
soaccept(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s;
	int error;

	s = splnet();
	if ((so->so_state & SS_NOFDREF) == 0)
		panic("soaccept: !NOFDREF");
	so->so_state &= ~SS_NOFDREF;
	if ((so->so_state & SS_ISDISCONNECTED) == 0) {
		error = (*so->so_proto->pr_usrreq)(so, PRU_ACCEPT, (struct mbuf*) 0, nam,
					(struct mbuf*) 0, u.u_procp);
	} else {
		error = ECONNABORTED;
	}

	splx(s);
	return (error);
}

int
soconnect(so, nam)
	register struct socket *so;
	struct mbuf *nam;
{
	int s;
	int error;

	/*
	 * this is done here in supervisor mode since the kernel can't access the
	 * socket or its options.
	 */
	if (so->so_options & SO_ACCEPTCONN)
		return (EOPNOTSUPP);
	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING))
		return (EALREADY);
	s = splnet();
	/*
	 * If protocol is connection-based, can only connect once.
	 * Otherwise, if connected, try to disconnect first.
	 * This allows user to disconnect by connecting to, e.g.,
	 * a null address.
	 */
	if ((so->so_state & (SS_ISCONNECTED | SS_ISCONNECTING))
			&& ((so->so_proto->pr_flags & PR_CONNREQUIRED) || (error =
					sodisconnect(so))))
		error = EISCONN;
	else
		error = (*so->so_proto->pr_usrreq)(so, PRU_CONNECT, (struct mbuf*) 0,
				nam, (struct mbuf*) 0, u.u_procp);
	/*
	 * this is done here because the kernel mode can't get at this info without
	 * a lot of trouble.
	 */
	if (!error) {
		if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING))
			error = EINPROGRESS;
	} else
		so->so_state &= ~SS_ISCONNECTING;
	splx(s);
	return (error);
}

int
soconnect2(so1, so2)
	register struct socket *so1;
	struct socket *so2;
{
	int error, s;

	s = splnet();
	error = (*so1->so_proto->pr_usrreq)(so1, PRU_CONNECT2, (struct mbuf*) 0,
			(struct mbuf*) so2, (struct mbuf*) 0, u.u_procp);
	splx(s);
	return (error);
}

int
sodisconnect(so)
	register struct socket *so;
{
	int error, s;

	s = splnet();
	if ((so->so_state & SS_ISCONNECTED) == 0) {
		error = ENOTCONN;
		goto bad;
	}
	if (so->so_state & SS_ISDISCONNECTING) {
		error = EALREADY;
		goto bad;
	}
	error = (*so->so_proto->pr_usrreq)(so, PRU_DISCONNECT, (struct mbuf*) 0,
			(struct mbuf*) 0, (struct mbuf*) 0, u.u_procp);
bad:
	splx(s);
	return (error);
}

#define	SBLOCKWAIT(f)	(((f) & MSG_DONTWAIT) ? M_NOWAIT : M_WAITOK)

/*
 * Send on a socket.
 * If send must go all at once and message is larger than
 * send buffering, then hard error.
 * Lock against other senders.
 * If must go all at once and not enough room now, then
 * inform user that this would block and do nothing.
 * Otherwise, if nonblocking, send as much as possible.
 */
int
sosend(so, nam, uio, flags, rights)
	register struct socket *so;
	struct mbuf *nam;
	register struct uio *uio;
	int flags;
	struct mbuf *rights;
{
	struct mbuf *top = 0;
	register struct mbuf *m, **mp;
	register long space, len, resid;
	int mlen, rlen = 0, error = 0, s, dontroute, first = 1;
	int atomic = sosendallatonce(so) || top;

	if (uio) {
		resid = uio->uio_resid;
	} else {
		resid = top->m_pkthdr.len;
	}

	if (resid < 0) {
		return (EINVAL);
	}

	if (sosendallatonce(so) && resid > so->so_snd.sb_hiwat) {
		return (EMSGSIZE);
	}
	dontroute = (flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0
			&& (so->so_proto->pr_flags & PR_ATOMIC);
	u.u_ru.ru_msgsnd++;
	if (rights)
		rlen = rights->m_len;

#define	snderr(errno)	{ 	\
	error = errno; 			\
	splx(s); 				\
	goto release; 			\
}

restart:
	sblock(&so->so_snd);
	do {
		s = splnet();
		if (so->so_state & SS_CANTSENDMORE)
			snderr(EPIPE);
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0; /* ??? */
			splx(s);
			goto release;
		}
		if ((so->so_state & SS_ISCONNECTED) == 0) {
			if (so->so_proto->pr_flags & PR_CONNREQUIRED)
				snderr(ENOTCONN);
			if (nam == 0)
				snderr(EDESTADDRREQ);
		}
		space = sbspace(&so->so_snd);
		if (flags & MSG_OOB) {
			space += 1024;
		} else {
			space = sbspace(&so->so_snd);
			if (space <= rlen
					|| (atomic && space < resid + rlen)
					|| (resid >= MCLBYTES && space < MCLBYTES
							&& so->so_snd.sb_cc >= MCLBYTES
							&& (so->so_state & SS_NBIO) == 0)) {
				if (so->so_state & SS_NBIO) {
					if (first)
						error = EWOULDBLOCK;
					splx(s);
					goto release;
				}
				sbunlock(&so->so_snd);
				sbwait(&so->so_snd);
				splx(s);
				goto restart;
			}
		}
		splx(s);
		mp = &top;
		space -= rlen;
		do {
			if (uio == NULL) {
				resid = 0;
				if (flags & MSG_EOR) {
					top->m_flags |= M_EOR;
				}
			} else do {
					if (top == 0) {
						MGETHDR(m, M_WAIT, MT_DATA);
						mlen = MHLEN;
						m->m_pkthdr.len = 0;
						m->m_pkthdr.rcvif = (struct ifnet*) 0;
					} else {
						MGET(m, M_WAIT, MT_DATA);
						mlen = MLEN;
					}
					if (resid >= MINCLSIZE / 2 && space >= MCLBYTES) {
						MCLGET(m, M_WAIT);
						if (m->m_len != CLBYTES || (m->m_flags & M_EXT) == 0) {
							goto nopages;
						}
						mlen = MCLBYTES;

#ifdef	MAPPED_MBUFS
						len = MIN(MCLBYTES, resid);
#else
						if (atomic && top == 0) {
							len = MIN(MCLBYTES - max_hdr, resid);
							m->m_data += max_hdr;
						} else {
							len = MIN(MCLBYTES, resid);
						}
#endif
						space -= MCLBYTES;
					} else {
nopages:
						len = MIN(MIN(MLEN, resid), space);
						space -= len;
						/*
						 * For datagram protocols, leave room
						 * for protocol headers in first mbuf.
						 */
						if (atomic && top == 0 && len < mlen) {
							MH_ALIGN(m, len);
						}
					}
					error = uiomove(mtod(m, caddr_t), len, uio);
					resid = uio->uio_resid;
					m->m_len = len;
					*mp = m;
					top->m_pkthdr.len += len;
					if (error) {
						goto release;
					}
					mp = &m->m_next;
					if (resid <= 0) {
						if (flags & MSG_EOR) {
							top->m_flags |= M_EOR;
						}
						break;
					}

				} while (space > 0 && atomic);
			if (dontroute) {
				so->so_options |= SO_DONTROUTE;
			}
			s = splnet(); /* XXX */
			error = (*so->so_proto->pr_usrreq)(so,
					(flags & MSG_OOB) ? PRU_SENDOOB : PRU_SEND, top, nam,
					rights, u.u_procp);
			splx(s);
			rights = 0;
			rlen = 0;
			top = 0;
			first = 0;
			mp = &top;
			if (error) {
				goto release;
			}
		} while (resid && space > 0);
	} while (resid);
release:
	sbunlock(&so->so_snd);
out:
	if (top)
		m_freem(top);
	if (error == EPIPE)
		m_freem(rights);
		netpsignal(u.u_procp, SIGPIPE);
	return (error);
}

/*
 * Implement receive operations on a socket.
 * We depend on the way that records are added to the sockbuf
 * by sbappend*.  In particular, each record (mbufs linked through m_next)
 * must begin with an address if the protocol so specifies,
 * followed by an optional mbuf containing access rights if supported
 * by the protocol, and then zero or more mbufs of data.
 * In order to avoid blocking network interrupts for the entire time here,
 * we splx() while doing the actual copy to user space.
 * Although the sockbuf is locked, new data may still be appended,
 * and thus we must maintain consistency of the sockbuf during that time.
 */
int
soreceive(so, aname, uio, flags, rightsp)
	register struct socket *so;
	struct mbuf **aname;
	register struct uio *uio;
	int flags;
	struct mbuf **rightsp;
{
	register struct mbuf *m;
	register int len, error, s, offset;
	struct protosw *pr;
	struct mbuf *nextrecord;
	int moff;

	error = 0;
	pr = so->so_proto;
	if (rightsp)
		*rightsp = 0;
	if (aname)
		*aname = 0;
	if (flags & MSG_OOB) {
		m = m_get(M_WAIT, MT_DATA);
		error = (*pr->pr_usrreq)(so, PRU_RCVOOB, m,
				(struct mbuf*) (flags & MSG_PEEK), (struct mbuf*) 0, u.u_procp);
		if (error)
			goto bad;
		do {
			len = uio->uio_resid;
			if (len > m->m_len)
				len = m->m_len;
			error = uiomove(mtod(m, caddr_t), (int) len, uio);
			m = m_free(m);
		} while (uio->uio_resid && error == 0 && m);
bad:
		if (m)
			m_freem(m);
		return (error);
	}

restart:
	sblock(&so->so_rcv);
	s = splnet();

	if (so->so_rcv.sb_cc == 0) {
		if (so->so_error) {
			error = so->so_error;
			so->so_error = 0;
			goto release;
		}
		if (so->so_state & SS_CANTRCVMORE)
			goto release;
		if ((so->so_state & SS_ISCONNECTED) == 0 &&
		    (so->so_proto->pr_flags & PR_CONNREQUIRED)) {
			error = ENOTCONN;
			goto release;
		}
		if (uio->uio_resid == 0)
			goto release;
		if (so->so_state & SS_NBIO) {
			error = EWOULDBLOCK;
			goto release;
		}
		sbunlock(&so->so_rcv);
		sbwait(&so->so_rcv);
		splx(s);
		goto restart;
	}
	u.u_ru.ru_msgrcv++;
	m = so->so_rcv.sb_mb;
	if (m == 0)
		panic("receive 1");
	nextrecord = m->m_act;
	if (pr->pr_flags & PR_ADDR) {
		if (m->m_type != MT_SONAME)
			panic("receive 1a");
		if (flags & MSG_PEEK) {
			if (aname)
				*aname = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (aname) {
				*aname = m;
				m = m->m_next;
				(*aname)->m_next = 0;
				so->so_rcv.sb_mb = m;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
			}
			if (m)
				m->m_act = nextrecord;
		}
	}
	if (m && m->m_type == MT_RIGHTS) {
		if ((pr->pr_flags & PR_RIGHTS) == 0)
			panic("receive 2");
		if (flags & MSG_PEEK) {
			if (rightsp)
				*rightsp = m_copy(m, 0, m->m_len);
			m = m->m_next;
		} else {
			sbfree(&so->so_rcv, m);
			if (rightsp) {
				*rightsp = m;
				so->so_rcv.sb_mb = m->m_next;
				m->m_next = 0;
				m = so->so_rcv.sb_mb;
			} else {
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
			}
			if (m)
				m->m_act = nextrecord;
		}
	}
	moff = 0;
	offset = 0;
	while (m && uio->uio_resid && error == 0) {
		if (m->m_type != MT_DATA && m->m_type != MT_HEADER)
			panic("receive 3");
		len = uio->uio_resid;
		so->so_state &= ~SS_RCVATMARK;
		if (so->so_oobmark && len > so->so_oobmark - offset)
			len = so->so_oobmark - offset;
		if (len > m->m_len - moff)
			len = m->m_len - moff;
		splx(s);
		error = uiomove(mtod(m, caddr_t) + moff, (int) len, uio);
		s = splnet();
		if (len == m->m_len - moff) {
			if (flags & MSG_PEEK) {
				m = m->m_next;
				moff = 0;
			} else {
				nextrecord = m->m_act;
				sbfree(&so->so_rcv, m);
				MFREE(m, so->so_rcv.sb_mb);
				m = so->so_rcv.sb_mb;
				if (m)
					m->m_act = nextrecord;
			}
		} else {
			if (flags & MSG_PEEK)
				moff += len;
			else {
				m->m_off += len;
				m->m_len -= len;
				so->so_rcv.sb_cc -= len;
			}
		}
		if (so->so_oobmark) {
			if ((flags & MSG_PEEK) == 0) {
				so->so_oobmark -= len;
				if (so->so_oobmark == 0) {
					so->so_state |= SS_RCVATMARK;
					break;
				}
			} else
				offset += len;
		}
	}
	if ((flags & MSG_PEEK) == 0) {
		if (m == 0)
			so->so_rcv.sb_mb = nextrecord;
		else if (pr->pr_flags & PR_ATOMIC)
			(void) sbdroprecord(&so->so_rcv);
		if ((pr->pr_flags & PR_WANTRCVD) && so->so_pcb)
			(*pr->pr_usrreq)(so, PRU_RCVD, (struct mbuf*) 0, (struct mbuf*) 0,
					(struct mbuf*) 0, u.u_procp);
		if (error == 0 && rightsp && *rightsp && pr->pr_domain->dom_externalize)
			error = (*pr->pr_domain->dom_externalize)(*rightsp);
	}
release:
	sbunlock(&so->so_rcv);
	splx(s);
	return (error);
}

int
soshutdown(so, how)
	register struct socket *so;
	register int how;
{
	register struct protosw *pr;

	pr = so->so_proto;
	how++;
	if (how & FREAD)
		sorflush(so);
	if (how & FWRITE)
		return ((*pr->pr_usrreq)(so, PRU_SHUTDOWN,
		    (struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0, u.u_procp));
	return (0);
}

void
sorflush(so)
	register struct socket *so;
{
	register struct sockbuf *sb;
	register struct protosw *pr;
	register int s;
	struct sockbuf asb;

	sb = &so->so_rcv;
	pr = so->so_proto;
	sblock(sb);
	s = splimp();
	socantrcvmore(so);
	sbunlock(sb);
	asb = *sb;
	bzero((caddr_t) sb, sizeof(*sb));
	splx(s);
	if ((pr->pr_flags & PR_RIGHTS) && pr->pr_domain->dom_dispose)
		(*pr->pr_domain->dom_dispose)(asb.sb_mb);
	sbrelease(&asb);
}

int
sosetopt(so, level, optname, m0)
	register struct socket *so;
	int level, optname;
	struct mbuf *m0;
{
	int error;
	register struct mbuf *m;

	error = 0;
	m = m0;

	/* we make a copy because the kernel is faking the m0 mbuf and we have to
	 * have something for the m_free's to work with
	 */
	if (m0)
		m = m0 = m_copy(m0, 0, M_COPYALL);
	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput)
			return ((*so->so_proto->pr_ctloutput)(PRCO_SETOPT, so, level,
					optname, &m0));
		error = ENOPROTOOPT;
	} else {
		switch (optname) {

		case SO_LINGER:
			if (m == NULL || m->m_len != sizeof(struct linger)) {
				error = EINVAL;
				goto bad;
			}
			if (mtod(m, struct linger *)->l_linger < 0 ||
					mtod(m, struct linger *)->l_linger > (INT_MAX / hz)) {
				error = EDOM;
				goto bad;
			}
			so->so_linger = mtod(m, struct linger *)->l_linger;
			break;
			/* fall thru... */

		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_DONTROUTE:
		case SO_USELOOPBACK:
		case SO_BROADCAST:
		case SO_REUSEADDR:
		case SO_OOBINLINE:
		//case SO_TIMESTAMP:
			if (m == NULL || m->m_len < sizeof(int)) {
				error = EINVAL;
				goto bad;
			}
			if (*mtod(m, int*))
				so->so_options |= optname;
			else
				so->so_options &= ~optname;
			break;

		case SO_SNDBUF:
		case SO_RCVBUF:
		case SO_SNDLOWAT:
		case SO_RCVLOWAT:
		{
			int optval;

			if (m == NULL || m->m_len < sizeof(int)) {
				error = EINVAL;
				goto bad;
			}

			/*
			 * Values < 1 make no sense for any of these
			 * options, so disallow them.
			 */
			optval = *mtod(m, int*);
			if (optval < 1) {
				error = EINVAL;
				goto bad;
			}

			switch (optname) {

			case SO_SNDBUF:
			case SO_RCVBUF:
				if (sbreserve(optname == SO_SNDBUF ? &so->so_snd : &so->so_rcv,
						(u_long) optval) == 0) {
					error = ENOBUFS;
					goto bad;
				}
				break;

				/*
				 * Make sure the low-water is never greater than
				 * the high-water.
				 */
			case SO_SNDLOWAT:
				so->so_snd.sb_lowat =
						(optval > so->so_snd.sb_hiwat) ?
								so->so_snd.sb_hiwat : optval;
				break;
			case SO_RCVLOWAT:
				so->so_rcv.sb_lowat =
						(optval > so->so_rcv.sb_hiwat) ?
								so->so_rcv.sb_hiwat : optval;
				break;
			}
			break;
		}

		case SO_SNDTIMEO:
		case SO_RCVTIMEO:
		{
			struct timeval *tv;
			short val;
			if (m == NULL || m->m_len < sizeof(*tv)) {
				error = EINVAL;
				goto bad;
			}
			tv = mtod(m, struct timeval*);
			if (tv->tv_sec > (SHRT_MAX - tv->tv_usec / tick) / hz) {
				error = EDOM;
				goto bad;
			}
			val = tv->tv_sec * hz + tv->tv_usec / tick;
			if (val == 0 && tv->tv_usec != 0) {
				val = 1;
			}

			switch (optname) {

			case SO_SNDTIMEO:
				so->so_snd.sb_timeo = val;
				break;
			case SO_RCVTIMEO:
				so->so_rcv.sb_timeo = val;
				break;
			}
			break;
		}
		default:
			error = ENOPROTOOPT;
			break;
		}
	}

	if (error == 0 && so->so_proto && so->so_proto->pr_ctloutput) {
		(void) ((*so->so_proto->pr_ctloutput)(PRCO_SETOPT, so, level, optname, &m0));
		m = NULL;	/* freed by protocol */
	}

bad:
	if (m)
		(void) m_free(m);
	return (error);
}

int
sogetopt(so, level, optname, mp)
	register struct socket *so;
	int level, optname;
	struct mbuf *mp;
{
	register struct mbuf *m;

	if (level != SOL_SOCKET) {
		if (so->so_proto && so->so_proto->pr_ctloutput) {
			return ((*so->so_proto->pr_ctloutput)(PRCO_GETOPT, so, level,
					optname, &mp));
		} else
			return (ENOPROTOOPT);
	} else {
		m = m_get(M_WAIT, MT_SOOPTS);
		m->m_len = sizeof(int);

		switch (optname) {

		case SO_LINGER:
			m->m_len = sizeof(struct linger);
			mtod(m, struct linger *)->l_onoff = so->so_options & SO_LINGER;
			mtod(m, struct linger *)->l_linger = so->so_linger;
			break;

		case SO_USELOOPBACK:
		case SO_DONTROUTE:
		case SO_DEBUG:
		case SO_KEEPALIVE:
		case SO_REUSEADDR:
		case SO_BROADCAST:
		case SO_OOBINLINE:
			*mtod(m, int *) = so->so_options & optname;
			break;

		case SO_TYPE:
			*mtod(m, int *) = so->so_type;
			break;

		case SO_ERROR:
			*mtod(m, int *) = so->so_error;
			so->so_error = 0;
			break;

		case SO_SNDBUF:
			*mtod(m, int *) = so->so_snd.sb_hiwat;
			break;

		case SO_RCVBUF:
			*mtod(m, int *) = so->so_rcv.sb_hiwat;
			break;

		case SO_SNDLOWAT:
			*mtod(m, int *) = so->so_snd.sb_lowat;
			break;

		case SO_RCVLOWAT:
			*mtod(m, int *) = so->so_rcv.sb_lowat;
			break;

		case SO_SNDTIMEO:
			*mtod(m, int *) = so->so_snd.sb_timeo;
			break;

		case SO_RCVTIMEO:
		{
			int val = (
					optname == SO_SNDTIMEO ?
							so->so_snd.sb_timeo : so->so_rcv.sb_timeo);
			m->m_len = sizeof(struct timeval);
			mtod(m, struct timeval *)->tv_sec = val / hz;
			mtod(m, struct timeval *)->tv_usec = (val % hz) * tick;
			//*mtod(m, int *) = so->so_rcv.sb_timeo;
			break;
		}

		default:
			(void)m_free(m);
			return (ENOPROTOOPT);
		}
		mp = m;
		return (0);
	}
}

void
sohasoutofband(so)
	register struct socket *so;
{
	register struct proc *p;

	if (so->so_pgrp < 0) {
		gsignal(-so->so_pgrp, SIGURG);
	} else if (so->so_pgrp > 0 && (p = (struct proc*) netpfind(so->so_pgrp)) != 0) {
		netpsignal(p, SIGURG);
	}
	selwakeup1(&so->so_rcv.sb_sel);
}

/*
 * this routine was extracted from the accept() call in uipc_sys.c to
 * do the initial accept processing in the supervisor rather than copying
 * the socket struct back and forth.
*/
int
soacc1(so)
	register struct	socket	*so;
{

	if ((so->so_options & SO_ACCEPTCONN) == 0)
		return (u.u_error = EINVAL);
	if ((so->so_state & SS_NBIO) && so->so_qlen == 0)
		return (u.u_error = EWOULDBLOCK);
	while (so->so_qlen == 0 && so->so_error == 0) {
		if (so->so_state & SS_CANTRCVMORE) {
			so->so_error = ECONNABORTED;
			break;
		}
		sleep(&so->so_timeo, PZERO + 1);
	}
	if (so->so_error) {
		u.u_error = so->so_error;
		so->so_error = 0;
		return (u.u_error);
	}
	return (0);
}

/*
 * used to dequeue a connection request.  the panic on nothing left is
 * done in the kernel when we return 0.
*/

struct socket *
asoqremque(so, n)
	struct	socket	*so;
	int	n;
{
	register struct	socket	*aso;

	aso = TAILQ_FIRST(&so->so_q);
	if (soqremque(aso, n) == 0)
		return (0);
	return (aso);
}

/* 
 * this is the while loop from connect(), the setjmp has been done in
 * kernel, so we just wait for isconnecting to go away.
*/
int
connwhile(so)
	register struct	socket	*so;
{

	while ((so->so_state & SS_ISCONNECTING) && so->so_error == 0)
		sleep(&so->so_timeo, PZERO + 1);
	u.u_error = so->so_error;
	so->so_error = 0;
	so->so_state &= ~SS_ISCONNECTING;
	return (u.u_error);
}

int
sogetnam(so, m)
	register struct	socket	*so;
	struct	mbuf	*m;
{
	return (u.u_error = (*so->so_proto->pr_usrreq)(so, PRU_SOCKADDR, 0, m, 0, u.u_procp));
}

int
sogetpeer(so, m)
	register struct socket *so;
	struct	mbuf	*m;
{

	if ((so->so_state & SS_ISCONNECTED) == 0)
		return (u.u_error = ENOTCONN);
	return (u.u_error = (*so->so_proto->pr_usrreq)(so, PRU_PEERADDR, 0, m, 0, u.u_procp));
}
//#endif
