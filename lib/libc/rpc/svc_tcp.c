/*	$NetBSD: svc_tcp.c,v 1.22 1999/03/25 01:16:11 lukem Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char *sccsid = "@(#)svc_tcp.c 1.21 87/08/11 Copyr 1984 Sun Micro";
static char *sccsid = "@(#)svc_tcp.c	2.2 88/08/01 4.0 RPCSRC";
#else
__RCSID("$NetBSD: svc_tcp.c,v 1.22 1999/03/25 01:16:11 lukem Exp $");
#endif
#endif

/*
 * svc_tcp.c, Server side for TCP/IP based RPC. 
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * Actually implements two flavors of transporter -
 * a tcp rendezvouser (a listner and connection establisher)
 * and a record/tcp stream.
 */

#include "namespace.h"
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>

#ifdef __weak_alias
__weak_alias(svcfd_create,_svcfd_create);
__weak_alias(svctcp_create,_svctcp_create);
#endif

/*
 * Ops vector for TCP/IP based rpc service handle
 */

static SVCXPRT *makefd_xprt(int, u_int, u_int);
static bool_t rendezvous_request(SVCXPRT *, struct rpc_msg *);
static enum xprt_stat rendezvous_stat(SVCXPRT *);
static void svctcp_destroy(SVCXPRT *);
static int readtcp(caddr_t, caddr_t, int);
static int writetcp(caddr_t, caddr_t, int);
static enum xprt_stat svctcp_stat(SVCXPRT *);
static bool_t svctcp_recv(SVCXPRT *, struct rpc_msg *);
static bool_t svctcp_getargs(SVCXPRT *, xdrproc_t, caddr_t);
static bool_t svctcp_freeargs(SVCXPRT *, xdrproc_t, caddr_t);
static bool_t svctcp_reply(SVCXPRT *, struct rpc_msg *);

static const struct xp_ops svctcp_op = {
		svctcp_recv,
		svctcp_stat,
		svctcp_getargs,
		svctcp_reply,
		svctcp_freeargs,
		svctcp_destroy
};

/*
 * Ops vector for TCP/IP rendezvous handler
 */

static const struct xp_ops svctcp_rendezvous_op = {
	rendezvous_request,
	rendezvous_stat,
	(bool_t	(*)(SVCXPRT *, xdrproc_t, caddr_t)) abort,
	(bool_t	(*)(SVCXPRT *, struct rpc_msg *)) abort,
	(bool_t	(*)(SVCXPRT *, xdrproc_t, caddr_t)) abort,
	svctcp_destroy
};

struct tcp_rendezvous { /* kept in xprt->xp_p1 */
	u_int sendsize;
	u_int recvsize;
};

struct tcp_conn {  /* kept in xprt->xp_p1 */
	enum xprt_stat strm_stat;
	u_int32_t x_id;
	XDR xdrs;
	char verf_body[MAX_AUTH_BYTES];
};

/*
 * Usage:
 *	xprt = svctcp_create(sock, send_buf_size, recv_buf_size);
 *
 * Creates, registers, and returns a (rpc) tcp based transporter.
 * Once *xprt is initialized, it is registered as a transporter
 * see (svc.h, xprt_register).  This routine returns
 * a NULL if a problem occurred.
 *
 * If sock<0 then a socket is created, else sock is used.
 * If the socket, sock is not bound to a port then svctcp_create
 * binds it to an arbitrary port.  The routine then starts a tcp
 * listener on the socket's associated port.  In any (successful) case,
 * xprt->xp_sock is the registered socket number and xprt->xp_port is the
 * associated port number.
 *
 * Since tcp streams do buffered io similar to stdio, the caller can specify
 * how big the send and receive buffers are via the second and third parms;
 * 0 => use the system default.
 */
SVCXPRT *
svctcp_create(sock, sendsize, recvsize)
	int sock;
	u_int sendsize;
	u_int recvsize;
{
	bool_t madesock = FALSE;
	SVCXPRT *xprt;
	struct tcp_rendezvous *r = NULL;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);

	if (sock == RPC_ANYSOCK) {
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			warn("svctcp_create - socket creation problem");
			goto cleanup_svctcp_create;
		}
		madesock = TRUE;
	}
	memset(&addr, 0, sizeof (addr));
	addr.sin_len = sizeof(struct sockaddr_in);
	addr.sin_family = AF_INET;
	if (bindresvport(sock, &addr)) {
		addr.sin_port = 0;
		(void)bind(sock, (struct sockaddr *)(void *)&addr, len);
	}
	if (getsockname(sock, (struct sockaddr *)(void *)&addr, &len) != 0) {
		warn("svctcp_create - cannot getsockname");
		goto cleanup_svctcp_create;
	}
	if (listen(sock, SOMAXCONN) != 0) {
		warn("svctcp_create - cannot listen");
		goto cleanup_svctcp_create;
	}
	r = (struct tcp_rendezvous *)mem_alloc(sizeof(*r));
	if (r == NULL) {
		warnx("svctcp_create: out of memory");
		goto cleanup_svctcp_create;
	}
	r->sendsize = sendsize;
	r->recvsize = recvsize;
	xprt = (SVCXPRT *)mem_alloc(sizeof(SVCXPRT));
	if (xprt == NULL) {
		warnx("svctcp_create: out of memory");
		goto cleanup_svctcp_create;
	}
	xprt->xp_p2 = NULL;
	xprt->xp_p1 = (caddr_t)(void *)r;
	xprt->xp_verf = _null_auth;
	xprt->xp_ops = &svctcp_rendezvous_op;
	xprt->xp_port = ntohs(addr.sin_port);
	xprt->xp_sock = sock;
	xprt_register(xprt);
	return (xprt);
 cleanup_svctcp_create:
	if (madesock)
	       (void)close(sock);
	if (r != NULL)
		mem_free(r, sizeof(*r));
	return ((SVCXPRT *)NULL);
}

/*
 * Like svtcp_create(), except the routine takes any *open* UNIX file
 * descriptor as its first input.
 */
SVCXPRT *
svcfd_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{

	return (makefd_xprt(fd, sendsize, recvsize));
}

static SVCXPRT *
makefd_xprt(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	SVCXPRT *xprt;
	struct tcp_conn *cd;
 
	xprt = (SVCXPRT *)mem_alloc(sizeof(SVCXPRT));
	if (xprt == (SVCXPRT *)NULL) {
		warnx("svc_tcp: makefd_xprt: out of memory");
		goto done;
	}
	cd = (struct tcp_conn *)mem_alloc(sizeof(struct tcp_conn));
	if (cd == (struct tcp_conn *)NULL) {
		warnx("svc_tcp: makefd_xprt: out of memory");
		mem_free(xprt, sizeof(SVCXPRT));
		xprt = (SVCXPRT *)NULL;
		goto done;
	}
	cd->strm_stat = XPRT_IDLE;
	xdrrec_create(&(cd->xdrs), sendsize, recvsize,
	    (caddr_t)(void *)xprt, readtcp, writetcp);
	xprt->xp_p2 = NULL;
	xprt->xp_p1 = (caddr_t)(void *)cd;
	xprt->xp_verf.oa_base = cd->verf_body;
	xprt->xp_addrlen = 0;
	xprt->xp_ops = &svctcp_op;  /* truely deals with calls */
	xprt->xp_port = 0;  /* this is a connection, not a rendezvouser */
	xprt->xp_sock = fd;
	xprt_register(xprt);
    done:
	return (xprt);
}

/*ARGSUSED*/
static bool_t
rendezvous_request(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	int sock;
	struct tcp_rendezvous *r;
	struct sockaddr_in addr;
	int len;

	r = (struct tcp_rendezvous *)xprt->xp_p1;
    again:
	len = sizeof(struct sockaddr_in);
	if ((sock = accept(xprt->xp_sock, (struct sockaddr *)(void *)&addr,
	    &len)) < 0) {
		if (errno == EINTR)
			goto again;
	       return (FALSE);
	}
	/*
	 * make a new transporter (re-uses xprt)
	 */
	xprt = makefd_xprt(sock, r->sendsize, r->recvsize);
	xprt->xp_raddr = addr;
	xprt->xp_addrlen = len;
	return (FALSE); /* there is never an rpc msg to be processed */
}

/*ARGSUSED*/
static enum xprt_stat
rendezvous_stat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE);
}

static void
svctcp_destroy(xprt)
	SVCXPRT *xprt;
{
	struct tcp_conn *cd = (struct tcp_conn *)xprt->xp_p1;

	xprt_unregister(xprt);
	if (xprt->xp_sock != -1)
		(void)close(xprt->xp_sock);
	if (xprt->xp_port != 0) {
		/* a rendezvouser socket */
		xprt->xp_port = 0;
	} else {
		/* an actual connection socket */
		XDR_DESTROY(&(cd->xdrs));
	}
	mem_free(cd, sizeof(struct tcp_conn));
	mem_free(xprt, sizeof(SVCXPRT));
}


/*
 * reads data from the tcp conection.
 * any error is fatal and the connection is closed.
 * (And a read of zero bytes is a half closed stream => error.)
 * All read operations timeout after 35 seconds.  A timeout is
 * fatal for the connection.
 */
static int
readtcp(xprtp, buf, len)
	caddr_t xprtp;
	caddr_t buf;
	int len;
{
	SVCXPRT *xprt = (SVCXPRT *)(void *)xprtp;
	int sock = xprt->xp_sock;
	int milliseconds = 35 * 1000;
	struct pollfd pollfd;

	do {
		pollfd.fd = sock;
		pollfd.events = POLLIN;
		switch (poll(&pollfd, 1, milliseconds)) {
		case -1:
			if (errno == EINTR) {
				continue;
			}
			/*FALLTHROUGH*/
		case 0:
			goto fatal_err;

		default:
			break;
		}
	} while ((pollfd.revents & POLLIN) == 0);

	if ((len = read(sock, buf, (size_t)len)) > 0)
		return (len);

fatal_err:
	((struct tcp_conn *)(xprt->xp_p1))->strm_stat = XPRT_DIED;
	return (-1);
}

/*
 * writes data to the tcp connection.
 * Any error is fatal and the connection is closed.
 */
static int
writetcp(xprtp, buf, len)
	caddr_t xprtp;
	caddr_t buf;
	int len;
{
	SVCXPRT *xprt = (SVCXPRT *)(void *)xprtp;
	int i, cnt;

	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		if ((i = write(xprt->xp_sock, buf, (size_t)cnt)) < 0) {
			((struct tcp_conn *)(xprt->xp_p1))->strm_stat =
			    XPRT_DIED;
			return (-1);
		}
	}
	return (len);
}

static enum xprt_stat
svctcp_stat(xprt)
	SVCXPRT *xprt;
{
	struct tcp_conn *cd = (struct tcp_conn *)(xprt->xp_p1);

	if (cd->strm_stat == XPRT_DIED)
		return (XPRT_DIED);
	if (! xdrrec_eof(&(cd->xdrs)))
		return (XPRT_MOREREQS);
	return (XPRT_IDLE);
}

static bool_t
svctcp_recv(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	struct tcp_conn *cd = (struct tcp_conn *)(xprt->xp_p1);
	XDR *xdrs = &(cd->xdrs);

	xdrs->x_op = XDR_DECODE;
	(void)xdrrec_skiprecord(xdrs);
	if (xdr_callmsg(xdrs, msg)) {
		cd->x_id = msg->rm_xid;
		return (TRUE);
	}
	cd->strm_stat = XPRT_DIED;
	return (FALSE);
}

static bool_t
svctcp_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{

	return ((*xdr_args)(&(((struct tcp_conn *)(xprt->xp_p1))->xdrs),
	    args_ptr));
}

static bool_t
svctcp_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	XDR *xdrs = &(((struct tcp_conn *)(xprt->xp_p1))->xdrs);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_args)(xdrs, args_ptr));
}

static bool_t
svctcp_reply(xprt, msg)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	struct tcp_conn *cd = (struct tcp_conn *)(xprt->xp_p1);
	XDR *xdrs = &(cd->xdrs);
	bool_t stat;

	xdrs->x_op = XDR_ENCODE;
	msg->rm_xid = cd->x_id;
	stat = xdr_replymsg(xdrs, msg);
	(void)xdrrec_endofrecord(xdrs, TRUE);
	return (stat);
}
