/*
 * Portions Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1996-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 1985, 1989, 1993
 *    The Regents of the University of California.  All rights reserved.
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
 */

/*
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Copyright (c) 2005 by Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)res_send.c	6.19.1 (Berkeley) 6/27/94";
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Send query to name server and wait for reply.
 */

#include "namespace.h"
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>

#ifdef __SELECT_DECLARED
#include <sys/select.h>
#else
#include <sys/poll.h>
#endif

#include <netinet/in.h>
#include <arpa/nameser.h>

#include <errno.h>
#include <resolv.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "res_private.h"

#if 0
#ifdef __weak_alias
__weak_alias(res_nsend,__res_nsend)
#endif
#endif

#define EXT(res) ((res)->u)

static struct sockaddr no_addr;

#ifdef __SELECT_DECLARED

#ifndef FD_SET
#define	NFDBITS			32
#define	FD_SETSIZE		32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)		bzero((char *)(p), sizeof(*(p)))
#endif
static int res_select(int, fd_set *, struct timeval *);
#else
static int res_poll(int, struct pollfd *, struct timeval *);
#endif
static socklen_t get_salen(const struct sockaddr *);
static struct sockaddr *get_nsaddr(res_state, size_t);
static int sock_eq(struct sockaddr *, struct sockaddr *);

#define KEEPOPEN 		(RES_USEVC | RES_STAYOPEN)

int
res_nsend(statp, buf, buflen, answer, anslen)
	res_state statp;
	const u_char *buf;
	int buflen;
	u_char *answer;
	int anslen;
{
	register int n, s = 0;
	int retry, v_circuit, resplen, ns;
	int gotsomewhere = 0, connected = 0;
	int needclose = 0, connreset = 0;
	u_short id, len;
	u_char *cp;
	struct timeval timeout;
	const HEADER *hp = (const HEADER *) buf;
	HEADER *anhp = (HEADER *) answer;
	u_int badns;		/* XXX NSMAX can't exceed #/bits per this */
	struct iovec iov[2];
	struct sockaddr_storage peer;
	int peerlen;
	int terrno = ETIMEDOUT;
	char junk[16];
#ifdef __SELECT_DECLARED
	fd_set dsmask;
#else
	struct pollfd pollfd;
#endif

#ifdef DEBUG
	if (statp->options & RES_DEBUG) {
		printf("res_send()\n");
		p_query(buf);
	}
#endif /* DEBUG */
	if (!(statp->options & RES_INIT)) {
		if (res_ninit(statp) == -1) {
			return (-1);
		}
	}
	if (statp->nscount == 0 || EXT(statp).ext == NULL) {
		errno = ESRCH;
		return (-1);
	}
	if (anslen < HFIXEDSZ) {
		errno = EINVAL;
		return (-1);
	}
	v_circuit = (statp->options & RES_USEVC) || buflen > PACKETSZ;
	id = hp->id;
	badns = 0;

	/*
	 * If the ns_addr_list in the resolver context has changed, then
	 * invalidate our cached copy and the associated timing data.
	 */
	if (EXT(statp).nscount != 0) {
		if (EXT(statp).nscount != statp->nscount) {
			needclose++;
		} else {
			for (ns = 0; ns < statp->nscount; ns++) {
				if (statp->nsaddr_list[ns].sin_family
						&& !sock_eq(
								(struct sockaddr*) (void*) &statp->nsaddr_list[ns],
								(struct sockaddr*) (void*) &EXT(statp).ext->nsaddr_list[ns])) {
					needclose++;
					break;
				}
				if (EXT(statp).nssocks[ns] == -1) {
					continue;
				}
				peerlen = sizeof(peer);
				if (getpeername(EXT(statp).nssocks[ns],
						(struct sockaddr*) (void*) &peer, &peerlen) < 0) {
					needclose++;
					break;
				}
				if (!sock_eq((struct sockaddr*) (void*) &peer,
						get_nsaddr(statp, (size_t) ns))) {
					needclose++;
					break;
				}
			}
		}
		if (needclose) {
			res_nclose(statp);
			EXT(statp).nscount = 0;
		}
	}

	/*
	 * Maybe initialize our private copy of the ns_addr_list.
	 */
	if (EXT(statp).nscount == 0) {
		for (ns = 0; ns < statp->nscount; ns++) {
			EXT(statp).nstimes[ns] = RES_MAXTIME;
			EXT(statp).nssocks[ns] = -1;
			if (!statp->nsaddr_list[ns].sin_family) {
				continue;
			}
			EXT(statp).ext->nsaddr_list[ns].sin = statp->nsaddr_list[ns];
		}
		EXT(statp).nscount = statp->nscount;
	}

	/*
	 * Some resolvers want to even out the load on their nameservers.
	 * Note that RES_BLAST overrides RES_ROTATE.
	 */
	if ((statp->options & RES_ROTATE) != 0U
			&& (statp->options & RES_BLAST) == 0U) {
		union res_sockaddr_union inu;
		struct sockaddr_in ina;
		int lastns = statp->nscount - 1;
		int fd;
		u_int16_t nstime;

		if (EXT(statp).ext != NULL) {
			inu = EXT(statp).ext->nsaddr_list[0];
		}
		ina = statp->nsaddr_list[0];
		fd = EXT(statp).nssocks[0];
		nstime = EXT(statp).nstimes[0];
		for (ns = 0; ns < lastns; ns++) {
			if (EXT(statp).ext != NULL) {
				EXT(statp).ext->nsaddr_list[ns] =
				EXT(statp).ext->nsaddr_list[ns + 1];
			}
			statp->nsaddr_list[ns] = statp->nsaddr_list[ns + 1];
			EXT(statp).nssocks[ns] = EXT(statp).nssocks[ns + 1];
			EXT(statp).nstimes[ns] = EXT(statp).nstimes[ns + 1];
		}
		if (EXT(statp).ext != NULL) {
			EXT(statp).ext->nsaddr_list[lastns] = inu;
		}
		statp->nsaddr_list[lastns] = ina;
		EXT(statp).nssocks[lastns] = fd;
		EXT(statp).nstimes[lastns] = nstime;
	}

	/*
	 * Send request, RETRY times, or until successful
	 */
	for (retry = statp->retry; retry > 0; retry--) {
		for (ns = 0; ns < statp->nscount; ns++) {
			struct sockaddr *nsap;
			socklen_t nsaplen;

			nsap = get_nsaddr(statp, (size_t) ns);
			nsaplen = get_salen(nsap);
			statp->_flags &= ~RES_F_LASTMASK;
			statp->_flags |= (ns << RES_F_LASTSHIFT);
			if (badns & (1 << ns)) {
				continue;
			}
#ifdef DEBUG
			if (statp->options & RES_DEBUG)
				printf("Querying server (# %d) address = %s\n", ns + 1,
						inet_ntoa(statp->nsaddr_list[ns].sin_addr));
#endif /* DEBUG */

			if (v_circuit) {
				int truncated = 0;

				/*
				 * Use virtual circuit.
				 */
				if (statp->_vcsock >= 0 && (statp->_flags & RES_F_VC) != 0) {
					peerlen = sizeof(peer);
					if (getpeername(statp->_vcsock,
							(struct sockaddr*) (void*) &peer, &peerlen) < 0
							|| !sock_eq((struct sockaddr*) (void*) &peer,
									nsap)) {
						res_nclose(statp);
						statp->_flags &= ~RES_F_VC;
					}
				}
				if (statp->_vcsock < 0 || (statp->_flags & RES_F_VC) == 0) {
					if (statp->_vcsock >= 0) {
						res_nclose(statp);
					}
					statp->_vcsock = socket(nsap->sa_family, SOCK_STREAM, 0);
					if (statp->_vcsock < 0) {
						terrno = errno;
#ifdef DEBUG
						if (statp->options & RES_DEBUG)
							perror("socket failed");
#endif /* DEBUG */
						continue;
					}
					if (connect(statp->_vcsock, nsap, nsaplen) < 0) {
						terrno = errno;
#ifdef DEBUG
						if (statp->options & RES_DEBUG)
							perror("connect failed");
#endif /* DEBUG */
						res_nclose(statp);
						continue;
					}
					statp->_flags |= RES_F_VC;
				}

				/*
				 * Send length & message
				 */
				len = htons((u_short) buflen);
				iov[0].iov_base = (caddr_t) &len;
				iov[0].iov_len = sizeof(len);
				iov[1].iov_base = __UNCONST(buf);
				iov[1].iov_len = buflen;
				if (writev(statp->_vcsock, iov, 2) != sizeof(len) + buflen) {
					terrno = errno;
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						perror("write failed");
#endif /* DEBUG */
					res_nclose(statp);
					continue;
				}

				/*
				 * Receive length & response
				 */
				cp = answer;
				len = sizeof(short);
				while (len != 0 && (n = read(statp->_vcsock, (char*) cp, (int) len)) > 0) {
					cp += n;
					len -= n;
				}
				if (n <= 0) {
					terrno = errno;
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						perror("read failed");
#endif /* DEBUG */
					res_nclose(statp);
					/*
					 * A long running process might get its TCP
					 * connection reset if the remote server was
					 * restarted.  Requery the server instead of
					 * trying a new one.  When there is only one
					 * server, this means that a query might work
					 * instead of failing.  We only allow one reset
					 * per query to prevent looping.
					 */
					if (terrno == ECONNRESET && !connreset) {
						connreset = 1;
						ns--;
					}
					continue;
				}
				cp = answer;
				if ((resplen = ntohs(*(u_short*) cp)) > anslen) {
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						fprintf(stderr, "response truncated\n");
#endif /* DEBUG */
					len = anslen;
					truncated = 1;
				} else
					len = resplen;
				while (len != 0 && (n = read(s, (char*) cp, (int) len)) > 0) {
					cp += n;
					len -= n;
				}
				if (n <= 0) {
					terrno = errno;
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						perror("read failed");
#endif /* DEBUG */
					res_nclose(statp);
					continue;
				}
				if (truncated) {
					/*
					 * Flush rest of answer
					 * so connection stays in synch.
					 */
					anhp->tc = 1;
					len = resplen - anslen;
					while (len != 0) {
						n = (len > sizeof(junk) ? sizeof(junk) : len);
						if ((n = read(statp->_vcsock, junk, n)) > 0)
							len -= n;
						else
							break;
					}
				}
			} else {
				/*
				 * Use datagrams.
				 */
				if (EXT(statp).nssocks[ns] == -1) {
					EXT(statp).nssocks[ns] = socket(nsap->sa_family, SOCK_DGRAM, 0);
					if (EXT(statp).nssocks[ns] < 0) {
						terrno = errno;
#ifdef DEBUG
						if (_res.options & RES_DEBUG)
							perror("socket (dg) failed");
#endif /* DEBUG */
						continue;
					}
				}
				s = EXT(statp).nssocks[ns];
//#if	BSD >= 43
				if (statp->nscount == 1 || (retry == statp->retry && ns == 0)) {
					/*
					 * Don't use connect if we might
					 * still receive a response
					 * from another server.
					 */
					if (connected == 0) {
						if (connect(EXT(statp).nssocks[ns], nsap, nsaplen) < 0) {
#ifdef DEBUG
							if (statp->options & RES_DEBUG)
								perror("connect");
#endif /* DEBUG */
							continue;
						}
						connected = 1;
					}
					if (send(s, buf, buflen, 0) != buflen) {
#ifdef DEBUG
						if (statp->options & RES_DEBUG)
							perror("send");
#endif /* DEBUG */
						continue;
					}
				} else {
					/*
					 * Disconnect if we want to listen
					 * for responses from more than one server.
					 */
					if (statp->nscount > 1 && connected) {
						(void) connect(s, &no_addr, sizeof(no_addr));
						connected = 0;
					}
				}
//#endif BSD
				if (sendto(s, buf, buflen, 0, nsap, nsaplen) != buflen) {
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						perror("sendto");
#endif /* DEBUG */
					continue;
				}

				/*
				 * Wait for reply
				 */
				timeout.tv_sec = (statp->retrans << (statp->retry - retry))
						/ statp->nscount;
				if (timeout.tv_sec <= 0)
					timeout.tv_sec = 1;
				timeout.tv_usec = 0;
wait:
#ifdef __SELECT_DECLARED
				n = res_select(s, &dsmask, &timeout);
#else
				n = res_poll(1, &pollfd, &timeout);
#endif
				if (n < 0) {
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						perror("select");
#endif /* DEBUG */
					continue;
				}
				if (n == 0) {
					/*
					 * timeout
					 */
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						printf("timeout\n");
#endif /* DEBUG */
					gotsomewhere = 1;
					continue;
				}
				if ((resplen = recv(s, answer, anslen, 0)) <= 0) {
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						perror("recvfrom");
#endif /* DEBUG */
					continue;
				}
				gotsomewhere = 1;
				if (id != anhp->id) {
					/*
					 * response from old query, ignore it
					 */
#ifdef DEBUG
					if (statp->options & RES_DEBUG
							|| (statp->pfcode & RES_PRF_REPLY)) {
						printf("old answer:\n");
						p_query(answer);
					}
#endif /* DEBUG */
					goto wait;
				}
				if (anhp->rcode == SERVFAIL || anhp->rcode == NOTIMP
						|| anhp->rcode == REFUSED) {
#ifdef DEBUG
					if (statp->options & RES_DEBUG) {
						printf("server rejected query:\n");
						p_query(answer);
					}
#endif /* DEBUG */
					badns |= (1 << ns);
					continue;
				}
				if (!(statp->options & RES_IGNTC) && anhp->tc) {
					/*
					 * get rest of answer
					 */
#ifdef DEBUG
					if (statp->options & RES_DEBUG)
						printf("truncated answer\n");
#endif /* DEBUG */
					res_nclose(statp);
					/*
					 * retry decremented on continue
					 * to desired starting value
					 */
					retry = statp->retry + 1;
					v_circuit = 1;
					continue;
				}
			}
#ifdef DEBUG
			if (statp->options & RES_DEBUG) {
				printf("got answer:\n");
				p_query(answer);
			}
			if ((statp->options & RES_DEBUG) || (statp->pfcode & RES_PRF_REPLY)) {
				p_query(answer);
			}
#endif /* DEBUG */
			/*
			 * We are going to assume that the first server is preferred
			 * over the rest (i.e. it is on the local machine) and only
			 * keep that one open.
			 */
			if ((statp->options & KEEPOPEN) == KEEPOPEN && ns == 0) {
				return (resplen);
			} else {
				res_nclose(statp);
				return (resplen);
			}
		}
	}
	if (statp->_vcsock >= 0 || s >= 0) {
		res_nclose(statp);
	}
	if (v_circuit == 0)
		if (gotsomewhere == 0)
			errno = ECONNREFUSED;
		else
			errno = ETIMEDOUT;
	else
		errno = terrno;
	return (-1);
}

/* Private */
#ifdef __SELECT_DECLARED
static int
res_select(s, dsmask, timeout)
	int s;
	fd_set *dsmask;
	struct timeval *timeout;
{
	FD_ZERO(dsmask);
	FD_SET(s, dsmask);
	return (select(s + 1, dsmask, (fd_set*) NULL, (fd_set*) NULL, timeout));
}

#else

static int
res_poll(s, pollfd, timeout)
	int s;
	struct pollfd   *pollfd;
	struct timeval *timeout;
{
	struct timespec timespec;
	int polltimeout;

	TIMEVAL_TO_TIMESPEC(timeout, &timespec);
	polltimeout = 1000 * (int) timespec.tv_sec
			+ (int) timespec.tv_nsec / 1000000;
	pollfd->fd = s;
	pollfd->events = POLLRDNORM;
	return (poll(pollfd, s, polltimeout));
}
#endif

static socklen_t
get_salen(sa)
	const struct sockaddr *sa;
{
#ifdef HAVE_SA_LEN
	/* There are people do not set sa_len.  Be forgiving to them. */
	if (sa->sa_len) {
		return ((socklen_t)sa->sa_len);
	}
#endif

	if (sa->sa_family == AF_INET) {
		return ((socklen_t) sizeof(struct sockaddr_in));
	} else if (sa->sa_family == AF_INET6) {
		return ((socklen_t) sizeof(struct sockaddr_in6));
	} else {
		return (0); /*%< unknown, die on connect */
	}
}

/*%
 * pick appropriate nsaddr_list for use.  see res_init() for initialization.
 */
static struct sockaddr *
get_nsaddr(statp, n)
	res_state statp;
	size_t n;
{
	if (!statp->nsaddr_list[n].sin_family && EXT(statp).ext) {
		/*
		 * - EXT(statp).ext->nsaddrs[n] holds an address that is larger
		 *   than struct sockaddr, and
		 * - user code did not update statp->nsaddr_list[n].
		 */
		return ((struct sockaddr *)&EXT(statp).ext->nsaddr_list[n]);
	} else {
		/*
		 * - user code updated statp->nsaddr_list[n], or
		 * - statp->nsaddr_list[n] has the same content as
		 *   EXT(statp).ext->nsaddrs[n].
		 */
		return ((struct sockaddr *)&statp->nsaddr_list[n]);
	}
}

static int
sock_eq(a, b)
	struct sockaddr *a, *b;
{
	struct sockaddr_in *a4, *b4;
	struct sockaddr_in6 *a6, *b6;

	if (a->sa_family != b->sa_family)
		return 0;
	switch (a->sa_family) {
	case AF_INET:
		a4 = (struct sockaddr_in *)(void *)a;
		b4 = (struct sockaddr_in *)(void *)b;
		return a4->sin_port == b4->sin_port &&
		    a4->sin_addr.s_addr == b4->sin_addr.s_addr;
	case AF_INET6:
		a6 = (struct sockaddr_in6 *)(void *)a;
		b6 = (struct sockaddr_in6 *)(void *)b;
		return a6->sin6_port == b6->sin6_port &&
#ifdef HAVE_SIN6_SCOPE_ID
		    a6->sin6_scope_id == b6->sin6_scope_id &&
#endif
		    IN6_ARE_ADDR_EQUAL(&a6->sin6_addr, &b6->sin6_addr);
	default:
		return 0;
	}
}

/*
 * This routine is for closing the socket if a virtual circuit is used and
 * the program wants to close it.  This provides support for endhostent()
 * which expects to close the socket.
 *
 * This routine is not expected to be user visible.
 */
void
res_nclose(statp)
	res_state statp;
{
	int ns;

	if (statp->_vcsock >= 0 || statp->_vcsock != -1) {
		(void) close(statp->_vcsock);
		statp->_vcsock = -1;
		statp->_flags &= ~(RES_F_VC | RES_F_CONN);
	}
	for (ns = 0; ns < EXT(statp).nscount; ns++) {
		if (EXT(statp).nssocks[ns] >= 0 || EXT(statp).nssocks[ns] != -1) {
			(void) close(EXT(statp).nssocks[ns]);
			EXT(statp).nssocks[ns] = -1;
		}
	}
}
