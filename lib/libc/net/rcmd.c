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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef _SELECT_DECLARED
int rcmd_select(int, int);
#else
int rcmd_poll(int, int);
#endif

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
			perror("select: setting up stderr");
		} else {
			fprintf(stderr, "select: protocol failure in circuit setup.\n");
		}
		(void) close(s2);
		return (-1);
	}
	return (0);
}
#endif

int
rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;
	u_short rport;
	char *locuser, *remuser, *cmd;
	int *fd2p;
{
	int s, timo = 1, pid;
	sigset_t oldmask, nmask;
	struct sockaddr_in sin, sin2, from;
	char c;
	int lport = IPPORT_RESERVED - 1;
	struct hostent *hp;

	pid = getpid();
	hp = gethostbyname(*ahost);
	if (hp == 0) {
		herror(*ahost);
		return (-1);
	}
	*ahost = hp->h_name;
	sigemptyset(&nmask);
	sigaddset(&nmask, SIGURG);
	(void)sigprocmask(SIG_BLOCK, &nmask, &oldmask);

	for (;;) {
		s = rresvport(&lport);
		if (s < 0) {
			if (errno == EAGAIN)
				fprintf(stderr, "socket: All ports in use\n");
			else
				perror("rcmd: socket");
			sigprocmask(SIG_SETMASK, &oldmask, NULL);
			return (-1);
		}
		fcntl(s, F_SETOWN, pid);
		sin.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr, hp->h_length);
		sin.sin_port = rport;
		if (connect(s, (struct sockaddr *)&sin, sizeof (sin), 0) >= 0)
			break;
		(void) close(s);
		if (errno == EADDRINUSE) {
			lport--;
			continue;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			continue;
		}
		if (hp->h_addr_list[1] != NULL) {
			int oerrno = errno;

			fprintf(stderr,
			    "connect to address %s: ", inet_ntoa(sin.sin_addr));
			errno = oerrno;
			perror(0);
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr,
			    hp->h_length);
			fprintf(stderr, "Trying %s...\n",
				inet_ntoa(sin.sin_addr));
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
		int len = sizeof (from);

		if (s2 < 0)
			goto bad;
		listen(s2, 1);
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num)+1) != strlen(num)+1) {
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
		s3 = accept(s2, (struct sockaddr *)&from, &len);
		(void) close(s2);
		if (s3 < 0) {
			perror("accept");
			lport = 0;
			goto bad;
		}
		*fd2p = s3;
        switch (from.sin_family) {
            case AF_INET:
#ifdef INET6
		    case AF_INET6:
#endif
            from.sin_port = ntohs((u_short)from.sin_port);
            if (from.sin_port != atoi(num)) {
                from.sin_port = atoi(num);
            }
			if (getnameinfo((struct sockaddr *)(void *)&from, len,
			    NULL, 0, num, (socklen_t)sizeof(num),
			    NI_NUMERICSERV) != 0 ||
			    (from.sin_port >= IPPORT_RESERVED ||
			     from.sin_port < IPPORT_RESERVED / 2)) {
				fprintf(stderr,
			    "socket: protocol failure in circuit setup.\n");
				goto bad2;
			}
			break;
		default:
			break;
		}
	}
	(void) write(s, locuser, strlen(locuser)+1);
	(void) write(s, remuser, strlen(remuser)+1);
	(void) write(s, cmd, strlen(cmd)+1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad2;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(STDERR_FILENO, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
	return (s);
bad2:
	if (lport)
		(void) close(*fd2p);
bad:
	(void) close(s);
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
	return (-1);
}

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
		portp = &((struct sockaddr_in *)sa)->sin_port;
		break;
#ifdef INET6
    case AF_INET6:
#endif
		sa->sa_len = sizeof(struct sockaddr_in6);
		portp = &((struct sockaddr_in6 *)sa)->sin6_port;
		break;
    default:
   		errno = EPFNOSUPPORT;
		return (-1);
    }
    sa->sa_family = af;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		portp = htons((u_short)*alport);
		if (bind(s, sa, sa->sa_len) >= 0)
			return (s);
		if (errno != EADDRINUSE) {
			(void) close(s);
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED/2) {
			(void) close(s);
			errno = EAGAIN;		/* close */
			return (-1);
		}
	}
}

int	_check_rhosts_file = 1;

int
ruserok(rhost, superuser, ruser, luser)
	char *rhost;
	int superuser;
	char *ruser, *luser;
{
	FILE *hostf;
	char fhost[MAXHOSTNAMELEN];
	int first = 1;
	register char *sp, *p;
	int baselen = -1;

	sp = rhost;
	p = fhost;
	while (*sp) {
		if (*sp == '.') {
			if (baselen == -1)
				baselen = sp - rhost;
			*p++ = *sp++;
		} else {
			*p++ = isupper(*sp) ? tolower(*sp++) : *sp++;
		}
	}
	*p = '\0';
	hostf = superuser ? (FILE *)0 : fopen("/etc/hosts.equiv", "r");
again:
	if (hostf) {
		if (!_validuser(hostf, fhost, luser, ruser, baselen)) {
			(void) fclose(hostf);
			return(0);
		}
		(void) fclose(hostf);
	}
	if (first == 1 && (_check_rhosts_file || superuser)) {
		struct stat sbuf;
		struct passwd *pwd;
		char pbuf[MAXPATHLEN];

		first = 0;
		if ((pwd = getpwnam(luser)) == NULL)
			return(-1);
		(void)strcpy(pbuf, pwd->pw_dir);
		(void)strcat(pbuf, "/.rhosts");
		if ((hostf = fopen(pbuf, "r")) == NULL)
			return(-1);
		/*
		 * if owned by someone other than user or root or if
		 * writeable by anyone but the owner, quit
		 */
		if (fstat(fileno(hostf), &sbuf) ||
		    (sbuf.st_uid && sbuf.st_uid != pwd->pw_uid) ||
		    (sbuf.st_mode&022)) {
			fclose(hostf);
			return(-1);
		}
		goto again;
	}
	return (-1);
}

/* don't make static, used by lpd(8) */
_validuser(hostf, rhost, luser, ruser, baselen)
	char *rhost, *luser, *ruser;
	FILE *hostf;
	int baselen;
{
	char *user;
	char ahost[MAXHOSTNAMELEN];
	register char *p;

	while (fgets(ahost, sizeof (ahost), hostf)) {
		p = ahost;
		while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
			*p = isupper(*p) ? tolower(*p) : *p;
			p++;
		}
		if (*p == ' ' || *p == '\t') {
			*p++ = '\0';
			while (*p == ' ' || *p == '\t')
				p++;
			user = p;
			while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0')
				p++;
		} else
			user = p;
		*p = '\0';
		if (_checkhost(rhost, ahost, baselen) &&
		    !strcmp(ruser, *user ? user : luser)) {
			return (0);
		}
	}
	return (-1);
}

static int
_checkhost(rhost, lhost, len)
	char *rhost, *lhost;
	int len;
{
	static char ldomain[MAXHOSTNAMELEN + 1];
	static char *domainp = NULL;
	static int nodomain = 0;
	register char *cp;

	if (len == -1)
		return(!strcmp(rhost, lhost));
	if (strncmp(rhost, lhost, len))
		return(0);
	if (!strcmp(rhost, lhost))
		return(1);
	if (*(lhost + len) != '\0')
		return(0);
	if (nodomain)
		return(0);
	if (!domainp) {
		if (gethostname(ldomain, sizeof(ldomain)) == -1) {
			nodomain = 1;
			return(0);
		}
		ldomain[MAXHOSTNAMELEN] = NULL;
		if ((domainp = index(ldomain, '.')) == (char *)NULL) {
			nodomain = 1;
			return(0);
		}
		for (cp = ++domainp; *cp; ++cp)
			if (isupper(*cp))
				*cp = tolower(*cp);
	}
	return(!strcmp(domainp, rhost + len +1));
}
