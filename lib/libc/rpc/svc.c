/*	$NetBSD: svc.c,v 1.16 1999/01/20 11:37:38 lukem Exp $	*/

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
static char *sccsid = "@(#)svc.c 1.44 88/02/08 Copyr 1984 Sun Micro";
static char *sccsid = "@(#)svc.c	2.4 88/08/11 4.0 RPCSRC";
#else
__RCSID("$NetBSD: svc.c,v 1.16 1999/01/20 11:37:38 lukem Exp $");
#endif
#endif

/*
 * svc.c, Server-side remote procedure call interface.
 *
 * There are two sets of procedures here.  The xprt routines are
 * for handling transport handles.  The svc routines handle the
 * list of service routines.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include "namespace.h"

#ifdef __SELECT_DECLARED
#include <sys/select.h>
#else
#include <sys/poll.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

#ifdef __weak_alias
__weak_alias(svc_getreq,_svc_getreq);
__weak_alias(svc_getreqset,_svc_getreqset);
__weak_alias(svc_register,_svc_register);
__weak_alias(svc_sendreply,_svc_sendreply);
__weak_alias(svc_unregister,_svc_unregister);
__weak_alias(svcerr_auth,_svcerr_auth);
__weak_alias(svcerr_decode,_svcerr_decode);
__weak_alias(svcerr_noproc,_svcerr_noproc);
__weak_alias(svcerr_noprog,_svcerr_noprog);
__weak_alias(svcerr_progvers,_svcerr_progvers);
__weak_alias(svcerr_systemerr,_svcerr_systemerr);
__weak_alias(svcerr_weakauth,_svcerr_weakauth);
__weak_alias(xprt_register,_xprt_register);
__weak_alias(xprt_unregister,_xprt_unregister);
#endif

static SVCXPRT **xports;

#define NULL_SVC ((struct svc_callout *)0)
#define	RQCRED_SIZE	400		/* this size is excessive */

#define max(a, b) (a > b ? a : b)

/*
 * The services list
 * Each entry represents a set of procedures (an rpc program).
 * The dispatch routine takes request structs and runs the
 * apropriate procedure.
 */
static struct svc_callout {
	struct svc_callout *sc_next;
	u_long		    sc_prog;
	u_long		    sc_vers;
	void		    (*sc_dispatch)(struct svc_req *, SVCXPRT *);
} *svc_head;

static struct svc_callout *svc_find(u_long, u_long, struct svc_callout **);
static int svc_fd_insert(struct pollfd *, fd_set *, int);
static int svc_fd_remove(struct pollfd *, int);
static void svc_getreqset_cred(struct rpc_msg *, struct svc_req *);
static void svc_getreqset_common(int, struct rpc_msg *, struct svc_req *);

int __svc_fdsetsize = FD_SETSIZE;
static int svc_pollfd_size;			/* number of slots in svc_pollfd */
static int svc_used_pollfd;			/* number of used slots in svc_pollfd */
static int *svc_pollfd_freelist;		/* svc_pollfd free list */
static int svc_max_free;			/* number of used slots in free list */

/* ***************  SVCXPRT related stuff **************** */

/*
 * Activate a transport handle.
 */
void
xprt_register(xprt)
	SVCXPRT *xprt;
{
	int sock = xprt->xp_sock;

	if (xports == NULL) {
		xports = (SVCXPRT **)
			mem_alloc(FD_SETSIZE * sizeof(SVCXPRT *));
		if (xports == NULL) {
			return;
		}
		memset(xports, '\0', FD_SETSIZE * sizeof(SVCXPRT *));
	}
	if (svc_fd_insert(&svc_pollfd, &svc_fdset, sock)) {
		return;
	}
	if ((sock < FD_SETSIZE) || (sock < __svc_fdsetsize)) {
		xports[sock] = xprt;
		FD_SET(sock, &svc_fdset);
		svc_maxfd = max(svc_maxfd, sock);
	}
}

/*
 * De-activate a transport handle. 
 */
void
xprt_unregister(xprt) 
	SVCXPRT *xprt;
{ 
	int sock = xprt->xp_sock;

	if (((sock < FD_SETSIZE) || (sock < __svc_fdsetsize)) && (xports[sock] == xprt)) {
		xports[sock] = (SVCXPRT *)NULL;
		(void)svc_fd_remove(&svc_pollfd, sock);
		FD_CLR(sock, &svc_fdset);
		if (sock == svc_maxfd) {
			for (svc_maxfd--; svc_maxfd >= 0; svc_maxfd--) {
				if (xports[svc_maxfd]) {
					break;
				}
			}
		}
	}
}

/*
 * Insert a socket into svc_pollfd, svc_fdset.
 * If we are out of space, we allocate ~128 more slots than we
 * need now for future expansion.
 * We try to keep svc_pollfd well packed (no holes) as possible
 * so that poll(2) is efficient.
 */
static int
svc_fd_insert(struct pollfd *readpfd, fd_set *readfds, int sock)
{
	int slot;

	/*
	 * Find a slot for sock in svc_pollfd; four possible cases:
	 *  1) need to allocate more space for svc_pollfd
	 *  2) there is an entry on the free list
	 *  3) the free list is empty (svc_used_pollfd is the next slot)
	 */
	if (readpfd == NULL || svc_used_pollfd == svc_pollfd_size) {
		struct pollfd *pfd;
		int new_size, *new_freelist;

		new_size = readpfd ? svc_pollfd_size + 128 : FD_SETSIZE;
		pfd = reallocarray(readpfd, new_size, sizeof(*readpfd));
		if (pfd == NULL) {
			return (0);			/* no changes */
		}
		new_freelist = realloc(svc_pollfd_freelist, new_size / 2);
		if (new_freelist == NULL) {
			free(pfd);
			return (0);			/* no changes */
		}
		readpfd = pfd;
		svc_pollfd_size = new_size;
		svc_pollfd_freelist = new_freelist;
		for (slot = svc_used_pollfd; slot < svc_pollfd_size; slot++) {
			readpfd[slot].fd = -1;
			readpfd[slot].events = 0;
			readpfd[slot].revents = 0;
		}
		slot = svc_used_pollfd;
	}  else if (svc_max_free != 0) {
		/* there is an entry on the free list, use it */
		slot = svc_pollfd_freelist[--svc_max_free];
	} else {
		/* nothing on the free list but we have room to grow */
		slot = svc_used_pollfd;
	}
	if (sock + 1 > __svc_fdsetsize) {
		fd_set *fds;
		size_t bytes;

		bytes = howmany(sock + 128, NFDBITS) * sizeof(fd_mask);
		/* realloc() would be nicer but it gets tricky... */
		fds = (fd_set*)mem_alloc(bytes);
		if (fds != NULL) {
			memset(fds, 0, bytes);
			memcpy(fds, readfds, howmany(__svc_fdsetsize, NFDBITS) * sizeof(fd_mask));
			readfds = fds;
			__svc_fdsetsize = bytes / sizeof(fd_mask) * NFDBITS;
		}
	}
	readpfd[slot].fd = sock;
	readpfd[slot].events = POLLIN;
	svc_used_pollfd++;
	if (svc_max_pollfd < slot + 1) {
		svc_max_pollfd = slot + 1;
	}
	return (1);
}


/*
 * Remove a socket from svc_pollfd.
 * Freed slots are placed on the free list.  If the free list fills
 * up, we compact svc_pollfd (free list size == svc_pollfd_size /2).
 */
static int
svc_fd_remove(struct pollfd *readpfd, int sock)
{
	int slot;

	if (readpfd == NULL) {
		return (0);
	}

	for (slot = 0; slot < svc_max_pollfd; slot++) {
		if (readpfd[slot].fd == sock) {
			readpfd[slot].fd = -1;
			readpfd[slot].events = 0;
			readpfd[slot].revents = 0;
			svc_used_pollfd--;
			if (svc_max_free == svc_pollfd_size / 2) {
				int i, j;

				/*
				 * Out of space in the free list; this means
				 * that svc_pollfd is half full.  Pack things
				 * such that svc_max_pollfd == svc_used_pollfd
				 * and svc_pollfd_freelist is empty.
				 */
				for (i = svc_used_pollfd, j = 0;
						i < svc_max_pollfd && j < svc_max_free; i++) {
					if (readpfd[i].fd == -1) {
						continue;
					}
					/* be sure to use a low-numbered slot */
					while (svc_pollfd_freelist[j] >= svc_used_pollfd) {
						j++;
					}
					readpfd[svc_pollfd_freelist[j++]] = readpfd[i];
					readpfd[i].fd = -1;
					readpfd[i].events = 0;
					readpfd[i].revents = 0;
				}
				svc_max_pollfd = svc_used_pollfd;
				svc_max_free = 0;
				/* could realloc if svc_pollfd_size is big */
			} else {
				/* trim svc_max_pollfd from the end */
				while (svc_max_pollfd > 0
						&& readpfd[svc_max_pollfd - 1].fd == -1) {
					svc_max_pollfd--;
				}
			}
			svc_pollfd_freelist[svc_max_free++] = slot;

			return (1);
		}
	}
	return (0);		/* not found, shouldn't happen */
}


/* ********************** CALLOUT list related stuff ************* */

/*
 * Add a service program to the callout list.
 * The dispatch routine will be called when a rpc request for this
 * program number comes in.
 */
bool_t
svc_register(xprt, prog, vers, dispatch, protocol)
	SVCXPRT *xprt;
	u_long prog;
	u_long vers;
	void (*dispatch)(struct svc_req *, SVCXPRT *);
	int protocol;
{
	struct svc_callout *prev;
	struct svc_callout *s;

	if ((s = svc_find(prog, vers, &prev)) != NULL_SVC) {
		if (s->sc_dispatch == dispatch)
			goto pmap_it;  /* he is registering another xptr */
		return (FALSE);
	}
	s = (struct svc_callout *)mem_alloc(sizeof(struct svc_callout));
	if (s == (struct svc_callout *)0) {
		return (FALSE);
	}
	s->sc_prog = prog;
	s->sc_vers = vers;
	s->sc_dispatch = dispatch;
	s->sc_next = svc_head;
	svc_head = s;
pmap_it:
	/* now register the information with the local binder service */
	if (protocol) {
		return (pmap_set(prog, vers, protocol, xprt->xp_port));
	}
	return (TRUE);
}

/*
 * Remove a service program from the callout list.
 */
void
svc_unregister(prog, vers)
	u_long prog;
	u_long vers;
{
	struct svc_callout *prev;
	struct svc_callout *s;

	if ((s = svc_find(prog, vers, &prev)) == NULL_SVC)
		return;
	if (prev == NULL_SVC) {
		svc_head = s->sc_next;
	} else {
		prev->sc_next = s->sc_next;
	}
	s->sc_next = NULL_SVC;
	mem_free(s, sizeof(struct svc_callout));
	/* now unregister the information with the local binder service */
	(void)pmap_unset(prog, vers);
}

/*
 * Search the callout list for a program number, return the callout
 * struct.
 */
static struct svc_callout *
svc_find(prog, vers, prev)
	u_long prog;
	u_long vers;
	struct svc_callout **prev;
{
	struct svc_callout *s, *p;

	p = NULL_SVC;
	for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
		if ((s->sc_prog == prog) && (s->sc_vers == vers))
			goto done;
		p = s;
	}
done:
	*prev = p;
	return (s);
}

/* ******************* REPLY GENERATION ROUTINES  ************ */

/*
 * Send a reply to an rpc request
 */
bool_t
svc_sendreply(xprt, xdr_results, xdr_location)
	SVCXPRT *xprt;
	xdrproc_t xdr_results;
	caddr_t xdr_location;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY;  
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf; 
	rply.acpted_rply.ar_stat = SUCCESS;
	rply.acpted_rply.ar_results.where = xdr_location;
	rply.acpted_rply.ar_results.proc = xdr_results;
	return (SVC_REPLY(xprt, &rply)); 
}

/*
 * No procedure error reply
 */
void
svcerr_noproc(xprt)
	SVCXPRT *xprt;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROC_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}

/*
 * Can't decode args error reply
 */
void
svcerr_decode(xprt)
	SVCXPRT *xprt;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = GARBAGE_ARGS;
	SVC_REPLY(xprt, &rply); 
}

/*
 * Some system error
 */
void
svcerr_systemerr(xprt)
	SVCXPRT *xprt;
{
	struct rpc_msg rply; 

	rply.rm_direction = REPLY; 
	rply.rm_reply.rp_stat = MSG_ACCEPTED; 
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = SYSTEM_ERR;
	SVC_REPLY(xprt, &rply); 
}

/*
 * Authentication error reply
 */
void
svcerr_auth(xprt, why)
	SVCXPRT *xprt;
	enum auth_stat why;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_DENIED;
	rply.rjcted_rply.rj_stat = AUTH_ERROR;
	rply.rjcted_rply.rj_why = why;
	SVC_REPLY(xprt, &rply);
}

/*
 * Auth too weak error reply
 */
void
svcerr_weakauth(xprt)
	SVCXPRT *xprt;
{

	svcerr_auth(xprt, AUTH_TOOWEAK);
}

/*
 * Program unavailable error reply
 */
void 
svcerr_noprog(xprt)
	SVCXPRT *xprt;
{
	struct rpc_msg rply;  

	rply.rm_direction = REPLY;   
	rply.rm_reply.rp_stat = MSG_ACCEPTED;  
	rply.acpted_rply.ar_verf = xprt->xp_verf;  
	rply.acpted_rply.ar_stat = PROG_UNAVAIL;
	SVC_REPLY(xprt, &rply);
}

/*
 * Program version mismatch error reply
 */
void  
svcerr_progvers(xprt, low_vers, high_vers)
	SVCXPRT *xprt; 
	u_long low_vers;
	u_long high_vers;
{
	struct rpc_msg rply;

	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROG_MISMATCH;
	rply.acpted_rply.ar_vers.low = (u_int32_t)low_vers;
	rply.acpted_rply.ar_vers.high = (u_int32_t)high_vers;
	SVC_REPLY(xprt, &rply);
}

/* ******************* SERVER INPUT STUFF ******************* */

/*
 * Get server side input from some transport.
 *
 * Statement of authentication parameters management:
 * This function owns and manages all authentication parameters, specifically
 * the "raw" parameters (msg.rm_call.cb_cred and msg.rm_call.cb_verf) and
 * the "cooked" credentials (rqst->rq_clntcred).
 * However, this function does not know the structure of the cooked
 * credentials, so it make the following assumptions: 
 *   a) the structure is contiguous (no pointers), and
 *   b) the cred structure size does not exceed RQCRED_SIZE bytes. 
 * In all events, all three parameters are freed upon exit from this routine.
 * The storage is trivially management on the call stack in user land, but
 * is mallocated in kernel land.
 */

void
svc_getreq(rdfds)
	int rdfds;
{
	fd_set readfds;

	FD_ZERO(&readfds);
	readfds.fds_bits[0] = rdfds;
	svc_getreqset_fdset(&readfds);
}

void
svc_getreqset(arg)
	void *arg;
{
#ifdef __SELECT_DECLARED
	fd_set *readfds = (fd_set *)arg;
	svc_getreqset_fdset(readfds);
#else
	struct pollfd *readpfd = (struct pollfd *)arg;
	svc_getreqset_poll(readpfd);
#endif
}

void
svc_getreqset_fdset(readfds)
	fd_set *readfds;
{
	struct pollfd *readpfd = &svc_pollfd;
	svc_getreqset_mix(readpfd, readfds);
}

void
svc_getreqset_poll(readpfd)
	struct pollfd *readpfd;
{
	fd_set *readfds = &svc_fdset;
	svc_getreqset_mix(readpfd, readfds);
}

void
svc_getreqset_mix(readpfd, readfds)
	struct pollfd *readpfd;
	fd_set *readfds;
{
	int pollretval;
	int32_t *maskp;

	maskp = (int32_t *)readfds->fds_bits;
	pollretval = svc_getreqset_mask(maskp, FD_SETSIZE);
	svc_getreqset_nomask(readpfd, readfds, pollretval);
}

int
svc_getreqset_mask(maskp, width)
	int32_t *maskp;
	int width;
{
	int fd, sock, bit;
	int32_t mask;

	for (sock = 0; sock < width; sock += NFDBITS) {
		for (mask = *maskp++; (bit = ffs(mask)) != 0; mask ^= (1 << (bit - 1))) {
			fd = (sock + bit - 1);
			return (fd);
		}
	}
	return (-1);
}

void
svc_getreqset_nomask(readpfd, readfds, pollretval)
	struct pollfd *readpfd;
	fd_set *readfds;
	int pollretval;
{
	struct rpc_msg msg;
	struct svc_req r;
	int i, fds_found;

	fds_found = 0;
	svc_getreqset_cred(&msg, &r);
	for (i = fds_found; fds_found < pollretval; i++) {
		if (readpfd[i].revents) {
			fds_found++;
			if (readpfd[i].revents & POLLNVAL) {
				FD_CLR(readpfd[i].fd, readfds);
			} else {
				svc_getreqset_common(readpfd[i].fd, &msg, &r);
			}
		}
	}
}

static void
svc_getreqset_cred(msg, r)
	struct rpc_msg *msg;
	struct svc_req *r;
{
	char cred_area[2*MAX_AUTH_BYTES + RQCRED_SIZE];

	msg->rm_call.cb_cred.oa_base = cred_area;
	msg->rm_call.cb_verf.oa_base = &(cred_area[MAX_AUTH_BYTES]);
	r->rq_clntcred = &(cred_area[2*MAX_AUTH_BYTES]);
}

static void
svc_getreqset_common(sock, msg, r)
	int sock;
	struct rpc_msg *msg;
	struct svc_req *r;
{
	enum xprt_stat stat;
	int prog_found;
	u_long low_vers;
	u_long high_vers;
	SVCXPRT *xprt;

	/* sock has input waiting */
	xprt = xports[sock];
	if (xprt == NULL) {
		/* But do we control sock? */
		return;
	}
	/* now receive msgs from xprtprt (support batch calls) */
	do {
		if (SVC_RECV(xprt, msg)) {

			/* now find the exported program and call it */
			struct svc_callout *s;
			enum auth_stat why;

			r->rq_xprt = xprt;
			r->rq_prog = msg->rm_call.cb_prog;
			r->rq_vers = msg->rm_call.cb_vers;
			r->rq_proc = msg->rm_call.cb_proc;
			r->rq_cred = msg->rm_call.cb_cred;
			/* first authenticate the message */
			if ((why = _authenticate(r, msg)) != AUTH_OK) {
				svcerr_auth(xprt, why);
				goto call_done;
			}
			/* now match message with a registered service*/
			prog_found = FALSE;
			low_vers = (u_long) -1L;
			high_vers = (u_long) 0L;
			for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
				if (s->sc_prog == r->rq_prog) {
					if (s->sc_vers == r->rq_vers) {
						(*s->sc_dispatch)(r, xprt);
						goto call_done;
					} /* found correct version */
					prog_found = TRUE;
					if (s->sc_vers < low_vers)
						low_vers = s->sc_vers;
					if (s->sc_vers > high_vers)
						high_vers = s->sc_vers;
				} /* found correct program */
			}
			/*
			 * if we got here, the program or version
			 * is not served ...
			 */
			if (prog_found)
				svcerr_progvers(xprt, low_vers, high_vers);
			else
				svcerr_noprog(xprt);
			/* Fall through to ... */
		}
call_done:
		if ((stat = SVC_STAT(xprt)) == XPRT_DIED) {
			SVC_DESTROY(xprt);
			break;
		}
	} while (stat == XPRT_MOREREQS);
}
