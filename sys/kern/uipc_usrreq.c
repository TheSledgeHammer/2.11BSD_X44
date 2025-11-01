/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
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
 *	@(#)uipc_usrreq.c	8.9 (Berkeley) 5/14/95
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uipc_usrreq.c	7.1.2 (2.11BSD GTE) 1997/1/18
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/filedesc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/unpcb.h>
#include <sys/un.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/stat.h>

/*
 * Unix communications domain.
 *
 * TODO:
 *	SEQPACKET, RDM
 *	rethink name space problems
 *	need a proper out-of-band
 */
struct sockaddr sun_noname = { AF_UNIX };
ino_t 			unp_ino;					/* prototype for fake inode numbers */

/*ARGSUSED*/
int
uipc_usrreq(so, req, m, nam, rights, p)
	struct socket *so;
	int req;
	struct mbuf *m, *nam, *rights;
	struct proc *p;
{
	struct unpcb *unp = sotounpcb(so);
	register struct socket *so2;
	int error = 0;

	if (req == PRU_CONTROL)
		return (EOPNOTSUPP);
	if (req != PRU_SEND && rights && rights->m_len) {
		error = EOPNOTSUPP;
		goto release;
	}
	if (unp == 0 && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}
	switch (req) {

	case PRU_ATTACH:
		if (unp) {
			error = EISCONN;
			break;
		}
		error = unp_attach(so);
		break;

	case PRU_DETACH:
		unp_detach(unp);
		break;

	case PRU_BIND:
		error = unp_bind(unp, nam);
		break;

	case PRU_LISTEN:
		if (unp->unp_vnode == 0)
			error = EINVAL;
		break;

	case PRU_CONNECT:
		error = unp_connect(so, nam);
		break;

	case PRU_CONNECT2:
		error = unp_connect2(so, (struct socket *)nam);
		break;

	case PRU_DISCONNECT:
		unp_disconnect(unp);
		break;

	case PRU_ACCEPT:
		/*
		 * Pass back name of connected socket,
		 * if it was bound and we are still connected
		 * (our peer may have closed already!).
		 */
		if (unp->unp_conn && unp->unp_conn->unp_addr) {
			nam->m_len = unp->unp_conn->unp_addr->m_len;
			bcopy(mtod(unp->unp_conn->unp_addr, caddr_t),
			    mtod(nam, caddr_t), (unsigned)nam->m_len);
		} else {
			nam->m_len = sizeof(sun_noname);
			*(mtod(nam, struct sockaddr *)) = sun_noname;
		}
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		unp_shutdown(unp);
		break;

	case PRU_RCVD:
		switch (so->so_type) {

		case SOCK_DGRAM:
			panic("uipc 1");
			break;
			/*NOTREACHED*/

		case SOCK_STREAM:
#define	rcv (&so->so_rcv)
#define snd (&so2->so_snd)
			if (unp->unp_conn == 0)
				break;
			so2 = unp->unp_conn->unp_socket;
			/*
			 * Adjust backpressure on sender
			 * and wakeup any waiting to write.
			 */
			snd->sb_mbmax += unp->unp_mbcnt - rcv->sb_mbcnt;
			unp->unp_mbcnt = rcv->sb_mbcnt;
			snd->sb_hiwat += unp->unp_cc - rcv->sb_cc;
			unp->unp_cc = rcv->sb_cc;
			sowwakeup(so2);
#undef snd
#undef rcv
			break;

		default:
			panic("uipc 2");
		}
		break;

	case PRU_SEND:
		if (rights) {
			error = unp_internalize(rights);
			if (error)
				break;
		}
		switch (so->so_type) {

		case SOCK_DGRAM: {
			struct sockaddr *from;

			if (nam) {
				if (unp->unp_conn) {
					error = EISCONN;
					break;
				}
				error = unp_connect(so, nam);
				if (error)
					m_freem(rights);
					m_freem(m);
					break;
			} else {
				if (unp->unp_conn == 0) {
					error = ENOTCONN;
					break;
				}
			}
			so2 = unp->unp_conn->unp_socket;
			if (unp->unp_addr)
				from = mtod(unp->unp_addr, struct sockaddr*);
			else
				from = &sun_noname;
			if (sbspace(&so2->so_rcv) > 0
					&& sbappendaddr(&so2->so_rcv, from, m, rights)) {
				sorwakeup(so2);
				m_freem(rights);
				m_freem(m);
			} else
				error = ENOBUFS;
			if (nam)
				unp_disconnect(unp);
			break;
		}

		case SOCK_STREAM:
#define	rcv (&so2->so_rcv)
#define	snd (&so->so_snd)
			if (so->so_state & SS_CANTSENDMORE) {
				error = EPIPE;
				break;
			}
			if (unp->unp_conn == 0)
				panic("uipc 3");
			so2 = unp->unp_conn->unp_socket;
			/*
			 * Send to paired receive port, and then reduce
			 * send buffer hiwater marks to maintain backpressure.
			 * Wake up readers.
			 */
			if (rights) {
				(void)sbappendrights(rcv, m, rights);
				m_freem(rights);
			} else {
				sbappend(rcv, m);
			}
			snd->sb_mbmax -=
			    rcv->sb_mbcnt - unp->unp_conn->unp_mbcnt;
			unp->unp_conn->unp_mbcnt = rcv->sb_mbcnt;
			snd->sb_hiwat -= rcv->sb_cc - unp->unp_conn->unp_cc;
			unp->unp_conn->unp_cc = rcv->sb_cc;
			sorwakeup(so2);
#undef snd
#undef rcv
			break;

		default:
			panic("uipc 4");
		}
		break;

	case PRU_ABORT:
		unp_drop(unp, ECONNABORTED);
		break;

	case PRU_SENSE:
		((struct stat *) m)->st_blksize = so->so_snd.sb_hiwat;
		if (so->so_type == SOCK_STREAM && unp->unp_conn != 0) {
			so2 = unp->unp_conn->unp_socket;
			((struct stat *) m)->st_blksize += so2->so_rcv.sb_cc;
		}
		((struct stat *) m)->st_dev = NODEV;
		if (unp->unp_ino == 0)
			unp->unp_ino = unp_ino++;
		((struct stat *) m)->st_atime =
		    ((struct stat *) m)->st_mtime =
		    ((struct stat *) m)->st_ctime = unp->unp_ctime;
		((struct stat *) m)->st_ino = unp->unp_ino;
		return (0);

	case PRU_RCVOOB:
		return (EOPNOTSUPP);

	case PRU_SENDOOB:
		error = EOPNOTSUPP;
		break;

	case PRU_SOCKADDR:
		break;

	case PRU_PEERADDR:
		if (unp->unp_conn && unp->unp_conn->unp_addr) {
			nam->m_len = unp->unp_conn->unp_addr->m_len;
			bcopy(mtod(unp->unp_conn->unp_addr, caddr_t), mtod(nam, caddr_t),
					(unsigned) nam->m_len);
		}
		break;

	case PRU_SLOWTIMO:
		break;

	default:
		panic("piusrreq");
	}
release:
	if (rights) {
		m_freem(rights);
	}
	if (m) {
		m_freem(m);
	}
	return (error);
}

/*
 * Both send and receive buffers are allocated PIPSIZ bytes of buffering
 * for stream sockets, although the total for sender and receiver is
 * actually only PIPSIZ.
 * Datagram sockets really use the sendspace as the maximum datagram size,
 * and don't really want to reserve the sendspace.  Their recvspace should
 * be large enough for at least one max-size datagram plus address.
 */
#define	PIPSIZ	8192
int	unpst_sendspace = PIPSIZ;
int	unpst_recvspace = PIPSIZ;
int	unpsq_sendspace = PIPSIZ;
int	unpsq_recvspace = PIPSIZ;
int	unpdg_sendspace = 2*1024;	/* really max datagram size */
int	unpdg_recvspace = 4*1024;

int	unp_rights;					/* file descriptors in flight */

int
unp_attach(so)
	struct socket *so;
{
	struct unpcb *unp;
	struct timeval tv;
	int error;

	if (so->so_pcb)
		return EISCONN;
	if (so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
		switch (so->so_type) {

		case SOCK_STREAM:
			error = soreserve(so, unpst_sendspace, unpst_recvspace);
			break;

		case SOCK_SEQPACKET:
			error = soreserve(so, unpsq_sendspace, unpsq_recvspace);
			break;

		case SOCK_DGRAM:
			error = soreserve(so, unpdg_sendspace, unpdg_recvspace);
			break;

		default:
			panic("unp_attach");
		}
		if (error)
			return (error);
	}
	unp = (struct unpcb *)malloc(sizeof(*unp), M_PCB, M_NOWAIT);
	if (unp == NULL) {
		return (ENOBUFS);
	}
	bzero((caddr_t)unp, sizeof(*unp));
	so->so_pcb = (caddr_t)unp;
	unp->unp_socket = so;
	microtime(&tv);
	TIMEVAL_TO_TIMESPEC(&tv, &unp->unp_ctime);
	return (0);
}

void
unp_detach(unp)
	register struct unpcb *unp;
{
	if (unp->unp_vnode) {
		unpdet(unp->unp_vnode);
		unp->unp_vnode = 0;
	}
	if (unp->unp_conn)
		unp_disconnect(unp);
	while (unp->unp_refs)
		unp_drop(unp->unp_refs, ECONNRESET);
	soisdisconnected(unp->unp_socket);
	unp->unp_socket->so_pcb = 0;
	m_freem(unp->unp_addr);
	(void) m_free(dtom(unp));
	if (unp_rights) {
		sorflush(unp->unp_socket);
		unp_gc();
	}
}

int
unp_bind(unp, nam)
	struct unpcb *unp;
	struct mbuf *nam;
{
	struct sockaddr_un *soun;
	struct vnode *vp;
	int error;

	soun = mtod(nam, struct sockaddr_un *);
	if (unp->unp_vnode != NULL || nam->m_len >= MLEN) {
		return (EINVAL);
	}
	if (nam->m_len == MLEN) {
		if (*(mtod(nam, caddr_t) + nam->m_len - 1) != 0)
			return (EINVAL);
	} else {
		*(mtod(nam, caddr_t) + nam->m_len) = 0;
	}
	if(soun == NULL) {
		soun = malloc(nam->m_len + 1, M_SONAME, M_WAITOK);
	}
	m_copyback(nam, 0, nam->m_len, (caddr_t)soun);
	*(mtod(nam, caddr_t) + nam->m_len) = 0;
	error = unpbind(soun->sun_path, nam->m_len, &vp, unp->unp_socket);
	if (error)
		return (error);
	if (!vp)
		panic("unp_bind");
	unp->unp_addr = m_copy(nam, 0, (int)M_COPYALL);
	return (0);
}

int
unp_connect(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	register struct sockaddr_un *soun;
	struct vnode *vp;
	struct socket *so2, *so3;
	struct unpcb *unp2, *unp3;
	int error;

	soun = mtod(nam, struct sockaddr_un *);
	if (nam->m_len + (nam->m_off - MMINOFF) == MLEN) {
		return (EMSGSIZE);
	}
	if (nam->m_data + nam->m_len == &nam->m_dat[MLEN]) {	/* XXX */
		if (*(mtod(nam, caddr_t) + nam->m_len - 1) != 0) {
			return (EMSGSIZE);
		}
	} else {
		*(mtod(nam, caddr_t) + nam->m_len) = 0;
	}
	if(soun == NULL) {
		soun = malloc(nam->m_len + 1, M_SONAME, M_WAITOK);
	}
	m_copyback(nam, 0, nam->m_len, (caddr_t)soun);
	*(mtod(nam, caddr_t) + nam->m_len) = 0;

	error = unpconn(soun->sun_path, nam->m_len, &so2, &vp);
	if (error || !so2 || !vp)
		goto bad;
	if (so->so_type != so2->so_type) {
		error = EPROTOTYPE;
		goto bad;
	}
	if (so->so_proto->pr_flags & PR_CONNREQUIRED) {
		if (((so2->so_options & SO_ACCEPTCONN) == 0
				|| (so3 = sonewconn(so2)) == 0)) {
			error = ECONNREFUSED;
			goto bad;
		}
		unp2 = sotounpcb(so2);
		unp3 = sotounpcb(so3);
		if (unp2->unp_addr) {
			unp3->unp_addr = m_copy(unp2->unp_addr, 0, (int)M_COPYALL);
		}
		unp3->unp_flags = unp2->unp_flags;
		so2 = so3;
	}

	error = unp_connect2(so, so2);
bad:
	if (vp)
		vput(vp);
	return (error);
}

int
unp_connect2(so, so2)
	register struct socket *so;
	register struct socket *so2;
{
	register struct unpcb *unp = sotounpcb(so);
	register struct unpcb *unp2;

	if (so2->so_type != so->so_type)
		return (EPROTOTYPE);
	unp2 = sotounpcb(so2);
	unp->unp_conn = unp2;
	switch (so->so_type) {

	case SOCK_DGRAM:
		unp->unp_nextref = unp2->unp_refs;
		unp2->unp_refs = unp;
		soisconnected(so);
		break;

	case SOCK_STREAM:
		unp2->unp_conn = unp;
		soisconnected(so2);
		soisconnected(so);
		break;

	default:
		panic("unp_connect2");
	}
	return (0);
}

void
unp_disconnect(unp)
	struct unpcb *unp;
{
	register struct unpcb *unp2 = unp->unp_conn;

	if (unp2 == 0)
		return;
	unp->unp_conn = 0;
	switch (unp->unp_socket->so_type) {

	case SOCK_DGRAM:
		if (unp2->unp_refs == unp)
			unp2->unp_refs = unp->unp_nextref;
		else {
			unp2 = unp2->unp_refs;
			for (;;) {
				if (unp2 == 0)
					panic("unp_disconnect");
				if (unp2->unp_nextref == unp)
					break;
				unp2 = unp2->unp_nextref;
			}
			unp2->unp_nextref = unp->unp_nextref;
		}
		unp->unp_nextref = 0;
		unp->unp_socket->so_state &= ~SS_ISCONNECTED;
		break;

	case SOCK_STREAM:
		soisdisconnected(unp->unp_socket);
		unp2->unp_conn = 0;
		soisdisconnected(unp2->unp_socket);
		break;
	}
}

#ifdef notdef
unp_abort(unp)
	struct unpcb *unp;
{

	unp_detach(unp);
}


/*ARGSUSED*/
unp_usrclosed(unp)
	struct unpcb *unp;
{

}
#endif

void
unp_shutdown(unp)
	struct unpcb *unp;
{
	struct socket *so;

	if (unp->unp_socket->so_type == SOCK_STREAM && unp->unp_conn
			&& (so = unp->unp_conn->unp_socket))
		socantrcvmore(so);
}

void
unp_drop(unp, errno)
	struct unpcb *unp;
	int errno;
{
	struct socket *so = unp->unp_socket;

	so->so_error = errno;
	unp_disconnect(unp);
	if (so->so_head) {
		so->so_pcb = (caddr_t) 0;
		m_freem(unp->unp_addr);
		(void) m_free(dtom(unp));
		sofree(so);
	}
}

#ifdef notdef
unp_drain()
{

}
#endif

int
unp_externalize(rights)
	struct mbuf *rights;
{
	register int i;
	register struct cmsghdr *cm = mtod(rights, struct cmsghdr *);
	register struct file **rp = (struct file **)(cm + 1);
	register struct file *fp;
	int newfds = (cm->cmsg_len - sizeof(*cm)) / sizeof (int);
	int f;

	printf("externalize rights");
	if (!fdavail(newfds)) {
		for (i = 0; i < newfds; i++) {
			fp = *rp;
			unp_discard(fp);
			*rp++ = 0;
		}
		return (EMSGSIZE);
	}
	for (i = 0; i < newfds; i++) {
		if (ufalloc(0, &f))
			panic("unp_externalize");
		fp = *rp;
		u.u_ofile[f] = fp;
		fp->f_msgcount--;
		unp_rights--;
		*(int *)rp++ = f;
	}
	return (0);
}

int
unp_internalize(rights)
	struct mbuf *rights;
{
	register struct cmsghdr *cm = mtod(rights, struct cmsghdr *);
	register struct file **rp;
	register struct file *fp;
	register int i;
	int oldfds;

	printf("unp_internalize");
	if (cm->cmsg_type != SCM_RIGHTS || cm->cmsg_level != SOL_SOCKET ||
		    cm->cmsg_len != rights->m_len) {
		return (EINVAL);
	}
	oldfds = (cm->cmsg_len - sizeof (*cm)) / sizeof (int);
	rp = (struct file **)(cm + 1);
	for (i = 0; i < oldfds; i++) {
		GETF(fp,  *(int *)rp++);
	}
	rp = (struct file **)(cm + 1);
	for (i = 0; i < oldfds; i++) {
		GETF(fp, *(int *)rp);
		*rp++ = fp;
		fp->f_count++;
		fp->f_msgcount++;
		unp_rights++;
	}
	return (0);
}

int	unp_defer, unp_gcing;
extern	struct domain unixdomain;

/*
 * What I did to the next routine isn't pretty, feel free to redo it, but
 * I doubt it'd be worth it since this isn't used very much.  SMS
 */
void
unp_gc(void)
{
	register struct file *fp;
	register struct socket *so;
	struct file xf;

	if (unp_gcing)
		return;
	unp_gcing = 1;

restart:
	unp_defer = 0;
	/* get limits AND clear FMARK|FDEFER in all file table entries */
	/*
	LIST_FOREACH(fp, &filehead, f_list) {
		fp->f_flag &= ~(FMARK|FDEFER);
	}
	*/
	unpgc1(&fp);
	do {
		LIST_FOREACH(fp, &filehead, f_list) {
			/* get file table entry, the return value is f_count */
			if (fpfetch(fp, &xf) == 0) {
				continue;
			}
			if (xf.f_flag & FDEFER) {
				fpflags(fp, 0, FDEFER);
				unp_defer--;
			} else {
				if (xf.f_flag & FMARK)
					continue;
				if (xf.f_count == xf.f_msgcount)
					continue;
				fpflags(fp, FMARK, 0);
			}
			if (xf.f_type != DTYPE_SOCKET) {
				continue;
			}
			so = xf.f_socket;
			if (so->so_proto->pr_domain != &unixdomain
					|| (so->so_proto->pr_flags & PR_RIGHTS) == 0) {
				continue;
			}
			if (so->so_rcv.sb_flags & SB_LOCK) {
				sbwait(&so->so_rcv);
				goto restart;
			}
			unp_scan(so->so_rcv.sb_mb, unp_mark);
		}
	} while (unp_defer);
	LIST_FOREACH(fp, &filehead, f_list) {
		if (fpfetch(fp, &xf) == 0) {
			continue;
		}
		if (xf.f_count == xf.f_msgcount && (xf.f_flag & FMARK) == 0) {
			while (fpfetch(fp, &xf) && xf.f_msgcount) {
				unp_discard(fp);
			}
		}
	}
	unp_gcing = 0;
}

int
unp_dispose(m)
	struct mbuf *m;
{
	if (m)
		unp_scan(m, unp_discard);
	return (0);
}

void
unp_scan(m0, op)
	register struct mbuf *m0;
	void (*op)(struct file *);
{
	register struct mbuf *m;
	register struct file **rp;
	register struct cmsghdr *cm;
	register int i;
	int qfds;

	while (m0) {
		for (m = m0; m; m = m->m_next)
			if (m->m_type == MT_RIGHTS && m->m_len) {
				cm = mtod(m, struct cmsghdr *);
				if (cm->cmsg_level != SOL_SOCKET || cm->cmsg_type != SCM_RIGHTS) {
					continue;
				}
				qfds = (cm->cmsg_len - sizeof *cm) / sizeof (struct file *);
				rp = (struct file **)(cm + 1);
				for (i = 0; i < qfds; i++)
					(*op)(*rp++);
				break; /* XXX, but saves time */
			}
		m0 = m0->m_act;
	}
}

void
unp_mark(fp)
	struct file *fp;
{
	struct file xf;

	fpfetch(fp, &xf);
	if (xf.f_flag & FMARK)
		return;
	unp_defer++;
	fpflags(fp, FMARK|FDEFER, 0);
}

void
unp_discard(fp)
	struct file *fp;
{
	unp_rights--;
	(void)unpdisc(fp);
}
