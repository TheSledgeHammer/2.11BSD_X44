/*	$NetBSD: rcmd.c,v 1.73 2024/01/20 14:52:48 christos Exp $	*/

/*
 * Copyright (c) 1983, 1993, 1994
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
 */
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)rcmd.c	5.20.1 (2.11BSD) 1999/10/24";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#ifdef _SELECT_DECLARED
#include <sys/select.h>
#else
#include <sys/poll.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netgroup.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int rcmd_af(char **, u_short, const char *, const char *, const char *, int *, int);
#ifdef _SELECT_DECLARED
int rcmd_select(int, int);
#else
int rcmd_poll(int, int);
#endif
int rresvport_af(int *, int);
int __iruserok(const void *, int, const char *, int, const char *, const char *);
int __ivaliduser(FILE *, struct sockaddr *, socklen_t, const char *, const char *, const char *, int);
static int __icheckhost(struct sockaddr *, socklen_t, const char *, const char *, int);
static char *__gethostloop(struct sockaddr *, socklen_t);

int
rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;
	u_short rport;
	const char *locuser, *remuser, *cmd;
	int *fd2p;
{
	return (rcmd_af(ahost, rport, locuser, remuser, cmd, fd2p, AF_INET));
}

int
rcmd_af(ahost, rport, locuser, remuser, cmd, fd2p, af)
	char **ahost;
	u_short rport;
	const char *locuser, *remuser, *cmd;
	int *fd2p;
	int af;
{
	char buf[MAXHOSTNAMELEN];
	char pbuf[NI_MAXSERV];
	sigset_t oldmask, nmask;
	pid_t pid;
	struct addrinfo hints, *res, *r;
	int error;
	struct sockaddr_storage from;
	int lport, s, timo;
	char c;
	struct hostent *hp;
	struct pollfd reads[2];

	timo = 1;
	lport = IPPORT_RESERVED - 1;
	pid = getpid();
	hp = gethostbyname(*ahost);
	if (hp == NULL) {
		herror(*ahost);
		return (-1);
	}
	*ahost = hp->h_name;

	snprintf(pbuf, sizeof(pbuf), "%u", ntohs(rport));
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = af;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;
	error = getaddrinfo(hp->h_name, pbuf, &hints, &res);
	if (error) {
		fprintf(stderr, "%s: %s", hp->h_name, gai_strerror(error));
		return (-1);
	}
	if (res->ai_canonname) {
		strlcpy(buf, res->ai_canonname, sizeof(buf));
		hp->h_name = buf;
		*ahost = hp->h_name;
	}
	r = res;
	sigemptyset(&nmask);
	sigaddset(&nmask, SIGURG);
	(void) sigprocmask(SIG_BLOCK, &nmask, &oldmask);
	for (;;) {
		s = rresvport(&lport);
		if (s < 0) {
			if (errno == EAGAIN) {
				fprintf(stderr, "socket: All ports in use\n");
			} else {
				perror("rcmd: socket");
			}
			if (r->ai_next) {
				r = r->ai_next;
				continue;
			} else {
				sigprocmask(SIG_SETMASK, &oldmask, NULL);
				freeaddrinfo(res);
				return (-1);
			}
		}
		fcntl(s, F_SETOWN, pid);
		if (connect(s, r->ai_addr, r->ai_addrlen) >= 0) {
			break;
		}
		(void) close(s);
		if (errno == EADDRINUSE) {
			lport--;
			continue;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			r = res;
			continue;
		}
		if (r->ai_next && hp->h_addr_list[1] != NULL) {
			int oerrno = errno;
			char hbuf[NI_MAXHOST];
			const int niflags = NI_NUMERICHOST;

			hbuf[0] = '\0';
			if (getnameinfo(r->ai_addr, r->ai_addrlen, hbuf,
					(socklen_t) sizeof(hbuf), NULL, 0, niflags) != 0) {
				strlcpy(hbuf, "(invalid)", sizeof(hbuf));
			}
			fprintf(stderr, "connect to address %s: ", hbuf);
			errno = oerrno;
			perror(0);
			r = r->ai_next;
			hp->h_addr_list++;
			hbuf[0] = '\0';
			if (getnameinfo(r->ai_addr, r->ai_addrlen, hbuf,
					(socklen_t) sizeof(hbuf), NULL, 0, niflags) != 0) {
				strlcpy(hbuf, "(invalid)", sizeof(hbuf));
			}
			fprintf(stderr, "Trying %s...\n", hbuf);
			continue;
		}
		perror(hp->h_name);
		sigprocmask(SIG_SETMASK, &oldmask, NULL);
		return (-1);
	}
	lport--;
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 = rresvport(&lport), s3;
		int len = sizeof(from);

		if (s2 < 0) {
			goto bad;
		}
		listen(s2, 1);
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num) + 1) != strlen(num) + 1) {
			perror("write: setting up stderr");
			(void) close(s2);
			goto bad;
		}
#ifdef _SELECT_DECLARED
		if (rcmd_select(s, s2) != 0) {
			goto bad;
		}
#else
		if (rcmd_poll(s, s2) != 0) {
			goto bad;
		}
#endif
		s3 = accept(s2, (struct sockaddr*) &from, &len);
		(void) close(s2);
		if (s3 < 0) {
			perror("accept");
			lport = 0;
			goto bad;
		}
		*fd2p = s3;
		switch (from.ss_family) {
		case AF_INET:
#ifdef INET6
		case AF_INET6:
#endif
			if (getnameinfo((struct sockaddr*) (void*) &from, len,
			NULL, 0, num, (socklen_t) sizeof(num), NI_NUMERICSERV) != 0
					|| (atoi(num) >= IPPORT_RESERVED
							|| atoi(num) < IPPORT_RESERVED / 2)) {
				fprintf(stderr, "socket: protocol failure in circuit setup.\n");
				goto bad2;
			}
			break;
		default:
			break;
		}
	}
	(void) write(s, locuser, strlen(locuser) + 1);
	(void) write(s, remuser, strlen(remuser) + 1);
	(void) write(s, cmd, strlen(cmd) + 1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad2;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(STDERR_FILENO, &c, 1);
			if (c == '\n') {
				break;
			}
		}
		goto bad2;
	}
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
	return (s);
bad2:
	if (lport) {
		(void) close(*fd2p);
	}
bad:
	(void) close(s);
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
	return (-1);
}

#ifdef _SELECT_DECLARED
int
rcmd_select(s, s2)
	int s, s2;
{
	fd_set reads;

	FD_ZERO(&reads);
	FD_SET(s, &reads);
	FD_SET(s2, &reads);
	errno = 0;
	if (select(32, &reads, 0, 0, 0) < 1 || !FD_ISSET(s2, &reads)) {
		if (errno != 0) {
			perror("select: setting up stderr");
		} else {
			fprintf(stderr, "select: protocol failure in circuit setup.\n");
		}
		(void) close(s2);
		return (-1);
	}
	return (0);
}

#else

int
rcmd_poll(s, s2)
	int s, s2;
{
	struct pollfd reads[2];

	reads[0].fd = s;
	reads[0].events = POLLIN;
	reads[1].fd = s2;
	reads[1].events = POLLIN;
	errno = 0;
	if (poll(reads, 2, INFTIM) < 1 || (reads[1].revents & POLLIN) == 0) {
		if (errno != 0) {
			perror("poll: setting up stderr");
		} else {
			fprintf(stderr, "poll: protocol failure in circuit setup.\n");
		}
		(void) close(s2);
		return (-1);
	}
	return (0);
}
#endif

int
rresvport(alport)
    int *alport;
{
    return (rresvport_af(alport, AF_INET));
}

int
rresvport_af(alport, af)
	int *alport, af;
{
	struct sockaddr_storage ss;
	struct sockaddr *sa;
	u_int16_t *portp;
	int s;

	bzero(&ss, sizeof ss);
	sa = (struct sockaddr *)&ss;

	switch (af) {
	case AF_INET:
		sa->sa_len = sizeof(struct sockaddr_in);
		portp = &((struct sockaddr_in*) sa)->sin_port;
		break;
#ifdef INET6
    case AF_INET6:
#endif
		sa->sa_len = sizeof(struct sockaddr_in6);
		portp = &((struct sockaddr_in6*) sa)->sin6_port;
		break;
	default:
		errno = EPFNOSUPPORT;
		return (-1);
	}
	sa->sa_family = af;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		return (-1);
	}

	for (;;) {
		*portp = htons(*alport);
		if (bind(s, sa, sa->sa_len) >= 0)
			return (s);
		if (errno != EADDRINUSE) {
			(void) close(s);
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED / 2) {
			(void) close(s);
			errno = EAGAIN; /* close */
			return (-1);
		}
	}
	*portp = 0;
	sa->sa_family = af;
	if (bindresvport_sa(s, sa) == -1) {
		(void) close(s);
		return (-1);
	}
	*alport = ntohs(*portp);
	return (s);
}

int
ruserok(rhost, superuser, ruser, luser)
	const char *rhost;
	int superuser;
	const char *ruser, *luser;
{
	struct addrinfo hints, *res, *r;
	int error;

	error = getaddrinfo(rhost, "0", &hints, &res);
	if (error) {
		return (-1);
	}
	for (r = res; r; r = r->ai_next) {
		if (__iruserok(r->ai_addr, (int) r->ai_addrlen, rhost, superuser, ruser,
				luser) == 0) {
			freeaddrinfo(res);
			return (0);
		}
	}
	freeaddrinfo(res);
	return (-1);
}

int	_check_rhosts_file = 1;

int
__iruserok(raddr, rlen, rhost, superuser, ruser, luser)
	const void *raddr;
	const char *rhost;
	int rlen, superuser;
	const char *ruser, *luser;
{
	struct sockaddr *sa;
	FILE *hostf;
	struct stat sbuf;
	struct passwd *pwd;
	const char fhost[MAXHOSTNAMELEN];
	int first = 1;
	char *sp, *p;
	int baselen = -1;
	char pbuf[MAXPATHLEN];
	uid_t uid;
	gid_t gid;

	sa = __UNCONST(raddr);
	sp = __UNCONST(rhost);
	p = __UNCONST(fhost);
	while (*sp) {
		if (*sp == '.') {
			if (baselen == -1) {
				baselen = sp - rhost;
			}
			*p++ = *sp++;
		} else {
			*p++ = isupper(*sp) ? tolower(*sp++) : *sp++;
		}
	}
	*p = '\0';
	hostf = superuser ? NULL : fopen(_PATH_HEQUIV, "r");
again:
	if (hostf) {
		if (!__ivaliduser(hostf, sa, (socklen_t) rlen, fhost, luser, ruser,
				baselen)) {
			(void) fclose(hostf);
			return (0);
		}
		(void) fclose(hostf);
	}
	if (first == 1 && (_check_rhosts_file || superuser)) {
		first = 0;
		if ((pwd = getpwnam(__UNCONST(luser))) == NULL) {
			return (-1);
		}
		uid = geteuid();
		gid = getegid();
		(void) seteuid(pwd->pw_uid);
		(void) strcpy(pbuf, pwd->pw_dir);
		(void) strcat(pbuf, "/.rhosts");
		hostf = fopen(pbuf, "re");
		(void) seteuid(uid);
		(void) setegid(gid);

		if (hostf == NULL) {
			return (-1);
		}

		/*
		 * if owned by someone other than user or root or if
		 * writeable by anyone but the owner, quit
		 */
		if (fstat(fileno(hostf), &sbuf)
				|| (sbuf.st_uid && sbuf.st_uid != pwd->pw_uid)
				|| (sbuf.st_mode & (S_IWGRP | S_IWOTH))) {
			fclose(hostf);
			return (-1);
		}
		goto again;
	}
	return (-1);
}

/* don't make static, used by lpd(8) */
int
__ivaliduser(hostf, raddr, salen, rhost, luser, ruser, baselen)
	FILE *hostf;
	struct sockaddr *raddr;
	socklen_t salen;
	const char *rhost, *luser, *ruser;
	int baselen;
{
	char *user;
	char buf[MAXHOSTNAMELEN + 128];		/* host + login */
	const char *auser, *ahost;
	int firsttime = 1;
	int hostok, userok;
	char domain[MAXHOSTNAMELEN];
	char *p;

	(void)getdomainname(domain, sizeof(domain));

	while (fgets(__UNCONST(ahost), sizeof(ahost), hostf)) {
		p = __UNCONST(ahost);
		while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
			*p = isupper(*p) ? tolower(*p) : *p;
			p++;
		}
		if (*p == ' ' || *p == '\t') {
			*p++ = '\0';
			while (*p == ' ' || *p == '\t') {
				p++;
			}
			user = p;
			while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
				p++;
			}
		} else {
			user = p;
		}
		*p = '\0';

		if (p == buf) {
			continue;
		}

		auser = *user ? user : luser;
		ahost = buf;

		if (ahost[0] == '+') {
			switch (ahost[1]) {
			case '\0':
				hostok = 1;
				break;
			case '@':
				if (firsttime) {
					rhost = __gethostloop(raddr, salen);
					firsttime = 0;
				}
				if (rhost) {
					hostok = innetgr(&ahost[2], rhost, NULL, domain);
				} else {
					hostok = 0;
				}
				break;
			default:
				hostok = __icheckhost(raddr, salen, rhost, &ahost[1], baselen);
				break;
			}
		} else if (ahost[0] == '-') {
			switch (ahost[1]) {
			case '\0':
				hostok = -1;
				break;
			case '@':
				if (firsttime) {
					rhost = __gethostloop(raddr, salen);
					firsttime = 0;
				}
				if (rhost) {
					hostok = innetgr(&ahost[2], rhost, NULL, domain);
				} else {
					hostok = 0;
				}
				break;
			default:
				hostok = -__icheckhost(raddr, salen, rhost, &ahost[1], baselen);
				break;
			}
		} else {
			hostok = __icheckhost(raddr, salen, rhost, ahost, baselen);
		}

		if (auser[0] == '+') {
			switch (auser[1]) {
			case '\0':
				userok = 1;
				break;
			case '@':
				userok = innetgr(&auser[2], NULL, ruser, domain);
				break;
			default:
				userok = strcmp(ruser, &auser[1]) == 0;
				break;
			}
		} else if (auser[0] == '-') {
			switch (auser[1]) {
			case '\0':
				userok = -1;
				break;
			case '@':
				userok = -innetgr(&auser[2], NULL, ruser, domain);
				break;
			default:
				userok = -(strcmp(ruser, &auser[1]) == 0 ? 1 : 0);
				break;
			}
		} else {
			userok = strcmp(ruser, auser) == 0;
		}

		/* Check if one component did not match */
		if (hostok == 0 || userok == 0) {
			continue;
		}

		/* Check if we got a forbidden pair */
		if (userok == -1 || hostok == -1) {
			return (-1);
		}

		/* Check if we got a valid pair */
		if (hostok == 1 && userok == 1) {
			return (0);
		}
	}
	return (-1);
}

static int
__icheckhost(raddr, salen, rhost, lhost, len)
	struct sockaddr *raddr;
	socklen_t salen;
	const char *rhost, *lhost;
    int len;
{
	struct addrinfo hints, *res, *r;
	int error;
	char h1[NI_MAXHOST], h2[NI_MAXHOST];
	const int niflags = NI_NUMERICHOST;

	_DIAGASSERT(raddr != NULL);
	_DIAGASSERT(lhost != NULL);

	h1[0] = '\0';
	if (getnameinfo(raddr, salen, h1, (socklen_t) sizeof(h1), NULL, 0, niflags)
			!= 0) {
		return (0);
	}

	/* Resolve laddr into sockaddr */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = raddr->sa_family;
	hints.ai_socktype = SOCK_DGRAM; /*dummy*/
	res = NULL;
	error = getaddrinfo(lhost, "0", &hints, &res);
	if (error) {
		return (0);
	}

	/*
	 * Try string comparisons between raddr and laddr.
	 */
	for (r = res; r; r = r->ai_next) {
		h2[0] = '\0';
		if (getnameinfo(r->ai_addr, r->ai_addrlen, h2, (socklen_t) sizeof(h2),
				NULL, 0, niflags) != 0) {
			continue;
		}
		if (strcmp(h1, h2) == 0) {
			freeaddrinfo(res);
			return (1);
		}
	}
	/* No match. */
	freeaddrinfo(res);
	return (0);
}


/*
 * Return the hostname associated with the supplied address.
 * Do a reverse lookup as well for security. If a loop cannot
 * be found, pack the numeric IP address into the string.
 */
static char *
__gethostloop(raddr, salen)
	struct sockaddr *raddr;
	socklen_t salen;
{
	static char remotehost[NI_MAXHOST];
	char h1[NI_MAXHOST], h2[NI_MAXHOST];
	struct addrinfo hints, *res, *r;
	int error;
	const int niflags = NI_NUMERICHOST;

	_DIAGASSERT(raddr != NULL);

	h1[0] = remotehost[0] = '\0';
	if (getnameinfo(raddr, salen, remotehost, (socklen_t) sizeof(remotehost),
			NULL, 0, NI_NAMEREQD) != 0) {
		return (NULL);
	}
	if (getnameinfo(raddr, salen, h1, (socklen_t) sizeof(h1), NULL, 0, niflags)
			!= 0) {
		return (NULL);
	}

	/*
	 * Look up the name and check that the supplied
	 * address is in the list
	 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = raddr->sa_family;
	hints.ai_socktype = SOCK_DGRAM; /*dummy*/
	hints.ai_flags = AI_CANONNAME;
	res = NULL;
	error = getaddrinfo(remotehost, "0", &hints, &res);
	if (error) {
		return (NULL);
	}

	for (r = res; r; r = r->ai_next) {
		h2[0] = '\0';
		if (getnameinfo(r->ai_addr, r->ai_addrlen, h2, (socklen_t) sizeof(h2),
		NULL, 0, niflags) != 0) {
			continue;
		}
		if (strcmp(h1, h2) == 0) {
			freeaddrinfo(res);
			return remotehost;
		}
	}

	/*
	 * either the DNS administrator has made a configuration
	 * mistake, or someone has attempted to spoof us
	 */
	freeaddrinfo(res);
	return (NULL);
}
