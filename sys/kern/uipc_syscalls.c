/*
 * Copyright (c) 1982, 1986, 1989, 1990, 1993
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
 *	@(#)uipc_syscalls.c	8.6 (Berkeley) 2/14/95
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uipc_syscalls.c	7.1.3 (2.11BSD) 1999/9/13
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mpx.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/domain.h>
#include <sys/mount.h>
#include <sys/sysdecl.h>

#include <machine/setjmp.h>

static int pipe1(int *, int);

static void
MBZAP(m, len, type)
	register struct mbuf *m;
	int len, type;
{
	m->m_next = NULL;
	m->m_off = MMINOFF;
	m->m_len = len;
	m->m_type = type;
	m->m_act = NULL;
}

/*
 * System call interface to the socket abstraction.
 */
extern int netoff;
extern struct fileops socketops;

int
socket()
{
	register struct socket_args {
		syscallarg(int)	domain;
		syscallarg(int)	type;
		syscallarg(int)	protocol;
	} *uap = (struct socket_args *)u.u_ap;
	struct socket *so;
	register struct file *fp;

	if (netoff)
		return (u.u_error = ENETDOWN);
	if ((fp = falloc()) == NULL)
		return (u.u_error);
	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_SOCKET;
	fp->f_ops = &socketops;
	u.u_error = socreate(SCARG(uap, domain), &so, SCARG(uap, type), SCARG(uap, protocol));
	if (u.u_error) {
		goto bad;
	}
	fp->f_data = (caddr_t)so;
	fp->f_socket = so;
	return (u.u_error);
	
bad:
	u.u_ofile[u.u_r.r_val1] = 0;
	fp->f_count = 0;
	return (u.u_error);
}

int
bind()
{
	register struct bind_args {
		syscallarg(int)	s;
		syscallarg(caddr_t)	name;
		syscallarg(u_int) namelen;
	} *uap = (struct bind_args *)u.u_ap;
	register struct file *fp;
	struct mbuf *nam;
	int error;

	if (netoff)
		return (u.u_error = ENETDOWN);
	fp = gtsockf(SCARG(uap, s));
	if (fp == 0)
		return (u.u_error);
	error = sockargs(&nam, SCARG(uap, name), SCARG(uap, namelen), MT_SONAME);
	if (u.u_error) {
		return (u.u_error);
	}
	MBZAP(nam, SCARG(uap, namelen), MT_SONAME);
	if (SCARG(uap, namelen) > MLEN)
		return (u.u_error = EINVAL);
	u.u_error = copyin(SCARG(uap, name), mtod(nam, caddr_t), SCARG(uap, namelen));
	if (u.u_error)
		return (u.u_error);
	u.u_error = sobind((struct socket *)fp->f_socket, nam);
	m_free(nam);
	return (u.u_error);
}

int
listen()
{
	register struct listen_args {
		syscallarg(int)	s;
		syscallarg(int)	backlog;
	} *uap = (struct listen_args *)u.u_ap;
	register struct file *fp;

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(SCARG(uap, s));
	if (fp == 0)
		return (u.u_error);
	u.u_error = solisten((struct socket *)fp->f_socket, SCARG(uap, backlog));
	return (u.u_error);
}

int
accept()
{
	register struct accept_args {
		syscallarg(int)	s;
		syscallarg(caddr_t)	name;
		syscallarg(int	*) anamelen;
	} *uap = (struct accept_args *)u.u_ap;

	register struct file *fp;
	struct mbuf *nam;
	int namelen;
	int s;
	register struct socket *so;

	if (netoff)
		return(u.u_error = ENETDOWN);
	if (SCARG(uap, name) == 0)
		goto noname;
	u.u_error = copyin((caddr_t)SCARG(uap, anamelen), (caddr_t)&namelen, sizeof(namelen));
	if (u.u_error)
		return (u.u_error);
#ifndef pdp11
	if (useracc((caddr_t)SCARG(uap, name), (u_int)namelen, B_WRITE) == 0) {
		u.u_error = EFAULT;
		return (u.u_error);
	}
#endif
noname:
	fp = gtsockf(SCARG(uap, s));
	if (fp == 0)
		return (u.u_error);
	s = splnet();
	so = fp->f_socket;
	if (soacc1(so)) {
		splx(s);
		return (u.u_error);
	}
	fp = falloc();
	if (fp == 0) {
		u.u_ofile[u.u_r.r_val1] = 0;
		splx(s);
		return (u.u_error);
	}
	if (!(so = asoqremque(so, 1))) {/* deQ in super */
		panic(">accept>");
	}
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = FREAD|FWRITE;
	fp->f_socket = so;
	nam = m_get(M_WAIT, MT_SONAME);
	MBZAP(nam, 0, MT_SONAME);
	u.u_error = soaccept(so, nam);
	if (SCARG(uap, name)) {
		if (namelen > nam->m_len)
			namelen = nam->m_len;
		/* SHOULD COPY OUT A CHAIN HERE */
		(void) copyout(mtod(nam, caddr_t), (caddr_t)SCARG(uap, name), (u_int)namelen);
		(void) copyout((caddr_t)&namelen, (caddr_t)SCARG(uap, anamelen), sizeof(*SCARG(uap, anamelen)));
	}
	m_freem(nam);
	splx(s);
	return (u.u_error);
}

int
connect()
{
	register struct connect_args {
		syscallarg(int) s;
		syscallarg(caddr_t)	name;
		syscallarg(u_int) namelen;
	} *uap = (struct connect_args *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *nam;
	int s;
	struct	socket	kcopy;

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(SCARG(uap, s));
	if (fp == 0) {
		return (u.u_error);
	}
	if (SCARG(uap, namelen) > MLEN) {
		return (u.u_error = EINVAL);
	}
	so = fp->f_socket;
	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
	    return (EALREADY);
	}
	u.u_error = sockargs(&nam, SCARG(uap, name), SCARG(uap, namelen), MT_SONAME);
	if (u.u_error) {
		return (u.u_error);
	}
	MBZAP(nam, 0, MT_SONAME);
	u.u_error = copyin(SCARG(uap, name), mtod(nam, caddr_t), SCARG(uap, namelen));
	if (u.u_error)
		return (u.u_error);
		
	/*
	 * soconnect was modified to clear the isconnecting bit on errors.
	 * also, it was changed to return the EINPROGRESS error if
	 * nonblocking, etc.
	 */
	u.u_error = soconnect(so, nam);
	if (u.u_error) {
	    goto bad;
	}
		
	if ((so->so_state & SS_NBIO) && (so->so_state & SS_ISCONNECTING)) {
		m_freem(nam);
		return (EINPROGRESS);
	}
	/*
	 * i don't think the setjmp stuff works too hot in supervisor mode,
	 * so what is done instead is do the setjmp here and then go back
	 * to supervisor mode to do the >while (isconnecting && !error)
	 * sleep()> loop.
	 */
	s = splnet();
	if (setjmp(&u.u_qsave)) {
		u.u_error = EINTR;
		goto bad2;
	}
	u.u_error = connwhile(so);
	splx(s);
	
bad:
    so->so_state &= ~SS_ISCONNECTING;
	m_freem(nam);
	if (u.u_error == ERESTART)
		u.u_error = EINTR;
	return (u.u_error);
	
bad2:
	splx(s);
	return (u.u_error);
}

int
socketpair()
{
	register struct socketpair_args {
		syscallarg(int)	domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(int *) rsv;
	} *uap = (struct socketpair_args *)u.u_ap;
	register struct file *fp1, *fp2;
	struct socket *so1, *so2;
	int sv[2];

	if	(netoff)
		return (u.u_error = ENETDOWN);
	u.u_error = socreate(SCARG(uap, domain), &so1, SCARG(uap, type), SCARG(uap, protocol));
	if	(u.u_error)
		return (u.u_error);;
	u.u_error = socreate(SCARG(uap, domain), &so2, SCARG(uap, type), SCARG(uap, protocol));
	if	(u.u_error)
		goto free;
	fp1 = falloc();
	if (fp1 == NULL)
		goto free2;
	sv[0] = u.u_r.r_val1;
	fp1->f_flag = FREAD|FWRITE;
	fp1->f_type = DTYPE_SOCKET;
	fp1->f_socket = so1;
	fp2 = falloc();
	if (fp2 == NULL)
		goto free3;
	fp2->f_flag = FREAD|FWRITE;
	fp2->f_type = DTYPE_SOCKET;
	fp2->f_socket = so2;
	sv[1] = u.u_r.r_val1;
	u.u_error = soconnect2(so1, so2);
	if (u.u_error)
		goto free4;
	if (SCARG(uap, type) == SOCK_DGRAM) {
		/*
		 * Datagram socket connection is asymmetric.
		 */
		 u.u_error = soconnect2(so2, so1);
		 if (u.u_error)
			goto free4;
	}
	u.u_r.r_val1 = 0;
	(void) copyout((caddr_t)sv, (caddr_t)SCARG(uap, rsv), 2 * sizeof (int));
	return (u.u_error);;
free4:
	fp2->f_count = 0;
	u.u_ofile[sv[1]] = 0;
free3:
	fp1->f_count = 0;
	u.u_ofile[sv[0]] = 0;
free2:
	(void)soclose(so2);
free:
	(void)soclose(so1);
	return (u.u_error);
}

int
sendto()
{
	register struct sendto_args {
		syscallarg(int) s;
		syscallarg(caddr_t) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
		syscallarg(caddr_t) to;
		syscallarg(int) tolen;
	} *uap = (struct sendto_args *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = SCARG(uap, to);
	msg.msg_namelen = SCARG(uap, tolen);
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, len);
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;

	return (sendit(SCARG(uap, s), &msg, SCARG(uap, flags)));
}

int
send()
{
	register struct send_args {
		syscallarg(int)	s;
		syscallarg(caddr_t)	buf;
		syscallarg(int) len;
		syscallarg(int)	flags;
	} *uap = (struct send_args *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, len);
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	return (sendit(SCARG(uap, s), &msg, SCARG(uap, flags)));
}

int
sendmsg()
{
	register struct sendmsg_args {
		syscallarg(int)	s;
		syscallarg(caddr_t)	msg;
		syscallarg(int)	flags;
	} *uap = (struct sendmsg_args *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[UIO_SMALLIOV], *iov;

	u.u_error = copyin(SCARG(uap, msg), (caddr_t)&msg, sizeof (msg));
	if (u.u_error) {
		return (u.u_error);
	}
	if ((u_int)msg.msg_iovlen >= UIO_SMALLIOV) {
		if ((u_int)msg.msg_iovlen >= UIO_MAXIOV) {
			u.u_error = EMSGSIZE;
			return (EMSGSIZE);
		}
		MALLOC(iov, struct iovec *, sizeof(struct iovec) * (u_int)msg.msg_iovlen, M_IOV, M_WAITOK);
	} else {
		iov = aiov;
	}
	u.u_error = copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof (aiov[0])));
	if (u.u_error) {
		return (u.u_error);
	}
	msg.msg_iov = aiov;
	return (sendit(SCARG(uap, s), &msg, SCARG(uap, flags)));
}

int
sendit(s, mp, flags)
	int s;
	register struct msghdr *mp;
	int flags;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *to, *rights;
	int len, error;

	if (netoff)
		u.u_error = ENETDOWN;
		return (ENETDOWN);
	fp = gtsockf(s);
	if (fp == NULL)
		return (ENOTSOCK);
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_offset = 0; /* XXX */
	auio.uio_resid = 0;
	auio.uio_rw = UIO_WRITE;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
		if (iov->iov_len == 0)
			continue;
		auio.uio_resid += iov->iov_len;
	}
	if (mp->msg_name) {
		error = sockargs(&to, mp->msg_name, mp->msg_namelen, MT_SONAME);
		if (error) {
			return (error);
		}
		MBZAP(to, mp->msg_namelen, MT_SONAME);
		u.u_error = copyin(mp->msg_name, mtod(to, caddr_t), mp->msg_namelen);
		if (u.u_error)
			return (u.u_error);
	} else {
		to = 0;
	}
	if (mp->msg_accrights) {
		error = sockargs(&rights, mp->msg_accrights, mp->msg_accrightslen,
		MT_RIGHTS);
		if (error) {
			return (error);
		}
		MBZAP(rights, mp->msg_accrightslen, MT_RIGHTS);
		if (mp->msg_accrightslen > MLEN)
			u.u_error = EINVAL;
		return (u.u_error);
		u.u_error = copyin(mp->msg_accrights, mtod(rights, caddr_t),
				mp->msg_accrightslen);
		if (u.u_error) {
			return (u.u_error);
		}
	} else {
		rights = 0;
	}
	len = auio.uio_resid;
	if (setjmp(&u.u_qsave)) {
		if (auio.uio_resid == len)
			return (u.u_error);
		else
			u.u_error = 0;
	} else
		u.u_error = sosend(fp->f_socket, to, &auio, flags, rights);
	u.u_r.r_val1 = len - auio.uio_resid;
	return (u.u_error);
}

int
recvfrom()
{
	register struct recvfrom_args {
		syscallarg(int)	s;
		syscallarg(caddr_t)	buf;
		syscallarg(int)	len;
		syscallarg(int)	flags;
		syscallarg(caddr_t)	from;
		syscallarg(int *) fromlenaddr;
	} *uap = (struct recvfrom_args *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;
	int len;

	u.u_error = copyin((caddr_t)SCARG(uap, fromlenaddr), (caddr_t)&len, sizeof(len));
	if (u.u_error)
		return (u.u_error);
	msg.msg_name = SCARG(uap, from);
	msg.msg_namelen = len;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, len);
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	return (recvit(SCARG(uap, s), &msg, SCARG(uap, flags), (caddr_t)SCARG(uap, fromlenaddr), (caddr_t)0));
}

int
recv()
{
	register struct recv_args {
		syscallarg(int)	s;
		syscallarg(caddr_t)	buf;
		syscallarg(int)	len;
		syscallarg(int)	flags;
	} *uap = (struct recv_args *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, len);
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	return (recvit(SCARG(uap, s), &msg, SCARG(uap, flags), (caddr_t)0, (caddr_t)0));
}

int
recvmsg()
{
	register struct recvmsg_args {
		syscallarg(int)	s;
		syscallarg(struct msghdr *) msg;
		syscallarg(int)	flags;
	} *uap = (struct recvmsg_args *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[UIO_SMALLIOV], *uiov, *iov;

	u.u_error = copyin((caddr_t)SCARG(uap, msg), (caddr_t)&msg, sizeof (msg));
	if (u.u_error) {
		return (u.u_error);
	}
	if ((u_int)msg.msg_iovlen >= UIO_SMALLIOV) {
		if ((u_int)msg.msg_iovlen >= UIO_MAXIOV) {
			u.u_error = EMSGSIZE;
			return (u.u_error);
		}
		MALLOC(iov, struct iovec *, sizeof(struct iovec) * (u_int)msg.msg_iovlen, M_IOV, M_WAITOK);
	} else {
		iov = aiov;
	}
	uiov = msg.msg_iov;
	msg.msg_iov = iov;
	u.u_error = copyin((caddr_t)msg.msg_iov, (caddr_t)aiov, (unsigned)(msg.msg_iovlen * sizeof(aiov[0])));
	if (u.u_error) {
		goto done;
	}
	return (recvit(SCARG(uap, s), &msg, SCARG(uap, flags), (caddr_t) & SCARG(uap, msg)->msg_namelen, (caddr_t) & SCARG(uap, msg)->msg_accrightslen));

done:
	if (iov != aiov) {
		FREE(iov, M_IOV);
	}
	return (u.u_error);
}

int
recvit(s, mp, flags, namelenp, rightslenp)
	int s;
	register struct msghdr *mp;
	int flags;
	caddr_t namelenp, rightslenp;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *from, *rights;
	int len;

	if (netoff)
		return (u.u_error = ENETDOWN);
	fp = gtsockf(s);
	if (fp == 0)
		return (ENOTSOCK);
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_offset = 0; /* XXX */
	auio.uio_resid = 0;
	auio.uio_rw = UIO_READ;
	iov = mp->msg_iov;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
		if (iov->iov_len == 0)
			continue;
		auio.uio_resid += iov->iov_len;
	}
	len = auio.uio_resid;
	if (setjmp(&u.u_qsave)) {
		if (auio.uio_resid == len)
			return (u.u_error);
		else
			u.u_error = 0;
	} else
		u.u_error = soreceive((struct socket*)fp->f_socket, &from, &auio, flags, &rights);
	if (u.u_error)
		return (u.u_error);
	u.u_r.r_val1 = len - auio.uio_resid;
	if (mp->msg_name) {
		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else
			(void) netcopyout(from, mp->msg_name, &len);
		(void) copyout((caddr_t) & len, namelenp, sizeof(int));
	}
	if (mp->msg_accrights) {
		len = mp->msg_accrightslen;
		if (len <= 0 || rights == 0)
			len = 0;
		else
			(void) netcopyout(rights, mp->msg_accrights, &len);
		(void) copyout((caddr_t) & len, rightslenp, sizeof(int));
	}
	if (rights)
		m_freem(rights);
	if (from)
		m_freem(from);
	return (u.u_error);
}

int
shutdown()
{
	register struct shutdown_args {
		syscallarg(int)	s;
		syscallarg(int)	how;
	} *uap = (struct shutdown_args *)u.u_ap;
	register struct file *fp;

	if (netoff)
		return (u.u_error = ENETDOWN);
	fp = gtsockf(SCARG(uap, s));
	if (fp == 0)
		return (0);
	u.u_error = soshutdown(fp->f_socket, SCARG(uap, how));
	return (u.u_error);
}

int
setsockopt()
{
	register struct setsockopt_args {
		syscallarg(int)	s;
		syscallarg(int)	level;
		syscallarg(int)	name;
		syscallarg(caddr_t)	val;
		syscallarg(u_int) valsize;
	} *uap = (struct setsockopt_args *)u.u_ap;
	register struct file *fp;
	register struct mbuf *m = NULL;

	if (netoff)
		return (u.u_error = ENETDOWN);
	fp = gtsockf(SCARG(uap, s));
	if (fp == 0)
		return (0);
	if (SCARG(uap, valsize) > MLEN) {
		u.u_error = EINVAL;
		return (u.u_error);
	}
	if (SCARG(uap, val)) {
		m = m_get(M_WAIT, MT_SOOPTS);
		if (m == NULL) {
			return (ENOBUFS);
		}
		MBZAP(m, SCARG(uap, valsize), MT_SOOPTS);
		u.u_error = copyin(SCARG(uap, val), mtod(m, caddr_t), (u_int)SCARG(uap, valsize));
		if (u.u_error) {
			(void) m_free(m);
			return (u.u_error);
		}
	}
	u.u_error = sosetopt(fp->f_socket, SCARG(uap, level), SCARG(uap, name), m);
	return (u.u_error);
}

int
getsockopt()
{
	register struct getsockopt_args {
		syscallarg(int)	s;
		syscallarg(int)	level;
		syscallarg(int)	name;
		syscallarg(caddr_t)	val;
		syscallarg(int *) avalsize;
	} *uap = (struct getsockopt_args *)u.u_ap;
	register struct file *fp;
	struct mbuf *m = NULL;
	int valsize;

	if (netoff)
		return (u.u_error = ENETDOWN);
	fp = gtsockf(SCARG(uap, s));
	if (fp == 0)
		return (0);
	if (SCARG(uap, val)) {
		u.u_error = copyin((caddr_t)SCARG(uap, avalsize), (caddr_t)&valsize, sizeof(valsize));
		if (u.u_error)
			return (u.u_error);
	} else
		valsize = 0;
	u.u_error = sogetopt(fp->f_socket, SCARG(uap, level), SCARG(uap, name), m);
	if (u.u_error)
		goto bad;
	if (SCARG(uap, val) && valsize && m != NULL) {
		u.u_error = netcopyout(m, SCARG(uap, val), &valsize);
		if (u.u_error)
			goto bad;
		u.u_error = copyout((caddr_t)&valsize, (caddr_t)SCARG(uap, avalsize), sizeof(valsize));
	}
bad:
	if (m != NULL)
		m_free(m);
	return (u.u_error);
}

/* pipe multiplexor */
enum pipe_ops {
	FMPX_READ,		/* read channel */
	FMPX_WRITE,		/* write channel */
	FMPX_NCHANS,		/* number of channels */
};

static struct mpx *
pipe_mpx_create(fp, idx, size, data)
	struct file *fp;
	int idx;
	u_long size;
	void *data;
{
	int error;

	if (fp->f_mpx == NULL) {
		fp->f_mpx = mpx_allocate(FMPX_NCHANS);
	}
	error = mpx_channel_create(fp->f_mpx, idx, size, data);
	if (error != 0) {
		mpx_deallocate(fp->f_mpx);
		return (NULL);
	}
	return (fp->f_mpx);
}

static int
pipe_mpx_read(fp, size, data)
	struct file *fp;
	u_long size;
	void *data;
{
	register struct mpx *rmpx;
	int error;

	rmpx = pipe_mpx_create(fp, FMPX_READ, size, data);
	if (rmpx != NULL) {
		error = mpx_channel_get(rmpx, FMPX_READ, data);
		if (error != 0) {
			return (error);
		}
		rmpx->mpx_file = fp;
	}
	return (0);
}

static int
pipe_mpx_write(fp, size, data)
	struct file *fp;
	u_long size;
	void *data;
{
	register struct mpx *wmpx;
	int error;

	wmpx = pipe_mpx_create(fp, FMPX_WRITE, size, data);
	if (wmpx != NULL) {
		error = mpx_channel_put(wmpx, FMPX_WRITE, size, data);
		if (error != 0) {
			return (error);
		}
		wmpx->mpx_file = fp;
	}
	return (0);
}

/* pipe read using mpx */
static void
pipe_read(rf, rso)
	struct file *rf;
	struct socket *rso;
{
	int error;

	error = pipe_mpx_read(rf, sizeof(rso), rso);
	if (error != 0) {
		u.u_error = error;
		return;
	}
	rf->f_flag = FREAD;
	rf->f_type = DTYPE_SOCKET;
	rf->f_ops = &socketops;
	rf->f_data = (caddr_t)rso;
}

/* pipe write using mpx */
static void
pipe_write(wf, wso)
	struct file *wf;
	struct socket *wso;
{
	int error;

	error = pipe_mpx_write(wf, sizeof(wso), wso);
	if (error != 0) {
		u.u_error = error;
		return;
	}
	wf->f_flag = FWRITE;
	wf->f_type = DTYPE_SOCKET;
	wf->f_ops = &socketops;
	wf->f_data = (caddr_t)wso;
}

/* ARGSUSED */
int
pipe()
{
	void *uap;
	register_t *retval;
	register struct filedesc *fdp;
	struct file *rf, *wf;
	struct socket *rso, *wso;
	int fd, error;

	uap = u.u_ap;
	fdp = u.u_procp->p_fd;
	error = socreate(AF_UNIX, &rso, SOCK_STREAM, 0);
	if (error) {
		u.u_error = error;
		return (error);
	}
	error = socreate(AF_UNIX, &wso, SOCK_STREAM, 0);
	if (error) {
		goto free1;
	}
	rf = falloc();
	if (rf == NULL) {
		goto free2;
	}
	pipe_read(rf, rso);
	fd = u.u_r.r_val1;
	retval[0] = fd;
	wf = falloc();
	if (wf == NULL) {
		goto free3;
	}
	pipe_write(wf, wso);
	u.u_r.r_val2 = u.u_r.r_val1;
	u.u_r.r_val1 = fd;
	retval[1] = fd;
	error = unp_connect2(wso, rso);
	if (error) {
		goto free4;
	}
	return (0);

free4:
	ffree(wf);
	fdp->fd_ofiles[retval[1]] = 0;
	ufdsync(fdp);
free3:
	ffree(rf);
	fdp->fd_ofiles[retval[0]] = 0;
	ufdsync(fdp);
free2:
	(void)soclose(wso);
free1:
	(void)soclose(rso);

	u.u_error = error;
	return (error);
}

static int
pipe1(int *fildes, int flags)
{
	if (flags &~(O_CLOEXEC | O_NONBLOCK)) {
		return (EINVAL);
	}
	return (pipe(fildes));
}

int
pipe2()
{
	register struct pipe2_args {
		syscallarg(int *) fildes;
		syscallarg(int) flags;
	} *uap = (struct pipe2_args *)u.u_ap;
	int fd[2], error;

	error = pipe1(fd, SCARG(uap, flags));
	if (error != 0) {
		u.u_error = error;
	}
	error = copyout(fd, SCARG(uap, fildes), sizeof(fd));
	if (error != 0) {
		u.u_error = error;
	}
	u.u_r.r_val1 = 0;
	return (error);
}

/*
 * Get socket name.
 */
int
getsockname()
{
	register struct getsockname_args {
		syscallarg(int)	fdes;
		syscallarg(caddr_t)	asa;
		syscallarg(int *) alen;
	} *uap = (struct getsockname_args *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *m;
	int len;

	if (netoff)
		u.u_error = ENETDOWN;
	return (u.u_error);
	fp = gtsockf(SCARG(uap, fdes));
	if (fp == 0)
		return (u.u_error);
	u.u_error = copyin((caddr_t)SCARG(uap, alen), (caddr_t)&len, sizeof (len));
	if (u.u_error)
		return (u.u_error);
	so = (struct socket *)fp->f_socket;
	m = m_getclr(M_WAIT, MT_SONAME);
	if (m == NULL) {
		return (ENOBUFS);
	}
	MBZAP(m, 0, MT_SONAME);
	u.u_error = sogetnam(fp->f_socket, m);
	if (u.u_error) {
		goto bad;
	}
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)SCARG(uap, asa), (u_int)len);
	if (u.u_error)
		return (u.u_error);
	u.u_error = copyout((caddr_t)&len, (caddr_t)SCARG(uap, alen), sizeof (len));

	return (u.u_error);
bad:
	m_freem(m);
	return (u.u_error);
}

/*
 * Get name of peer for connected socket.
 */
int
getpeername()
{
	register struct getpeername_args {
		syscallarg(int) fdes;
		syscallarg(caddr_t)	asa;
		syscallarg(int *) alen;
	} *uap = (struct getpeername_args *)u.u_ap;
	register struct file *fp;
	struct mbuf *m;
	u_int len;

	if (netoff)
		return (u.u_error = ENETDOWN);
	fp = gtsockf(SCARG(uap, fdes));
	if (fp == 0)
		return (u.u_error);
	m = m_getclr(M_WAIT, MT_SONAME);
	if (m == NULL) {
		return (ENOBUFS);
	}
	u.u_error = copyin((caddr_t) SCARG(uap, alen), (caddr_t) & len, sizeof(len));
	if (u.u_error)
		return (u.u_error);
	u.u_error = sogetpeer(fp->f_socket, m);
	if (u.u_error) {
		goto bad;
	}
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t) SCARG(uap, asa), (u_int) len);
	if (u.u_error)
		return (u.u_error);
	u.u_error = copyout((caddr_t) & len, (caddr_t) SCARG(uap, alen), sizeof(len));

	return (u.u_error);
bad:
	m_freem(m);
	return (u.u_error);
}

int
sockargs(aname, name, namelen, type)
	struct mbuf **aname;
	caddr_t name;
	u_int namelen;
	int type;
{
	register struct sockaddr *sa;
	register struct mbuf *m;
	int error;

	if (namelen > MLEN)
		return (EINVAL);
	m = m_get(M_WAIT, type);
	if (m == NULL)
		return (ENOBUFS);
	m->m_len = namelen;
	error = copyin(name, mtod(m, caddr_t), (u_int)namelen);
	if (error) {
		(void) m_free(m);
		return (error);
	} else {
		*aname = m;
	}
	if (type == MT_SONAME) {
		sa = mtod(m, struct sockaddr *);
		sa->sa_len = namelen;
	}
	return (error);
}

struct file *
gtsockf(fdes)
	int fdes;
{
	register struct file *fp;

	fp = getf(fdes);
	if (fp == NULL)
		return (NULL);
	if (fp->f_type != DTYPE_SOCKET) {
		u.u_error = ENOTSOCK;
		return (NULL);
	}
	return (fp);
}

int
netcopyout(m, to, len)
	struct mbuf *m;
	char *to;
	int *len;
{
	if (*len > m->m_len)
		*len = m->m_len;
	return (copyout(mtod(m, caddr_t), to, *len));
}
