/*	$NetBSD: ifconfig.c,v 1.141.4.2 2005/07/24 01:58:38 snj Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1983, 1993
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)ifconfig.c	8.2 (Berkeley) 2/16/94";
#else
__RCSID("$NetBSD: ifconfig.c,v 1.141.4.2 2005/07/24 01:58:38 snj Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include <netdb.h>

#include <sys/protosw.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <util.h>

#include "ifconfig.h"

struct	ifreq		ifr, ridreq;
struct	ifaliasreq	addreq __attribute__((aligned(4)));
struct	sockaddr_in	netmask;

char	name[30];
u_short	flags;
int	setaddr, setipdst, doalias;
u_long	metric, mtu;
int	clearaddr, s;
int	newaddr = -1;
int	conflicting = 0;
int af;
int	aflag, bflag, Cflag, dflag, lflag, mflag, sflag, uflag, vflag, zflag;
#ifdef INET6
int	Lflag;
#endif
int	explicit_prefix = 0;

struct ifcapreq g_ifcr;
int	g_ifcr_updated;

struct afswtch *afp;
struct cmd *cmdlist = NULL;
struct afswtch *aflist = NULL;

void 	notealias(const char *, int);
void 	notrailers(const char *, int);
void 	setifaddr(const char *, int);
void 	setifdstaddr(const char *, int);
void 	setifflags(const char *, int);
void	setifcaps(const char *, int);
void 	setifbroadaddr(const char *, int);
void 	setifipdst(const char *, int);
void 	setifmetric(const char *, int);
void 	setifmtu(const char *, int);

void 	setifnetmask(const char *, int);
void	setifprefixlen(const char *, int);
void	clone_create(const char *, int);
void	clone_destroy(const char *, int);

void    if_init(void);
void	cmds_init(void);
int		main(int, char *[]);

int		actions;			/* Actions performed */

struct cmd if_cmds[] = {
		{ "up",		IFF_UP,		0,		setifflags } ,
		{ "down",	-IFF_UP,	0,		setifflags },
		{ "trailers",	-1,		0,		notrailers },
		{ "-trailers",	1,		0,		notrailers },
		{ "arp",	-IFF_NOARP,	0,		setifflags },
		{ "-arp",	IFF_NOARP,	0,		setifflags },
		{ "debug",	IFF_DEBUG,	0,		setifflags },
		{ "-debug",	-IFF_DEBUG,	0,		setifflags },
		{ "alias",	IFF_UP,		0,		notealias },
		{ "-alias",	-IFF_UP,	0,		notealias },
		{ "delete",	-IFF_UP,	0,		notealias },
		{ "netmask",	NEXTARG,	0,		setifnetmask },
		{ "metric",	NEXTARG,	0,		setifmetric },
		{ "mtu",	NEXTARG,	0,		setifmtu },
		{ "broadcast",	NEXTARG,	0,		setifbroadaddr },
		{ "ipdst",	NEXTARG,	0,		setifipdst },
		{ "prefixlen",	NEXTARG,	0,		setifprefixlen},
#if 0
		/* XXX `create' special-cased below */
		{ "create",	0,		0,		clone_create } ,
#endif
		{ "destroy",	0,		0,		clone_destroy } ,
		{ "link0",	IFF_LINK0,	0,		setifflags } ,
		{ "-link0",	-IFF_LINK0,	0,		setifflags } ,
		{ "link1",	IFF_LINK1,	0,		setifflags } ,
		{ "-link1",	-IFF_LINK1,	0,		setifflags } ,
		{ "link2",	IFF_LINK2,	0,		setifflags } ,
		{ "-link2",	-IFF_LINK2,	0,		setifflags } ,
		{ "ip4csum",	IFCAP_CSUM_IPv4,0,		setifcaps },
		{ "-ip4csum",	-IFCAP_CSUM_IPv4,0,		setifcaps },
		{ "tcp4csum",	IFCAP_CSUM_TCPv4,0,		setifcaps },
		{ "-tcp4csum",	-IFCAP_CSUM_TCPv4,0,		setifcaps },
		{ "udp4csum",	IFCAP_CSUM_UDPv4,0,		setifcaps },
		{ "-udp4csum",	-IFCAP_CSUM_UDPv4,0,		setifcaps },
		{ "tcp6csum",	IFCAP_CSUM_TCPv6,0,		setifcaps },
		{ "-tcp6csum",	-IFCAP_CSUM_TCPv6,0,		setifcaps },
		{ "udp6csum",	IFCAP_CSUM_UDPv6,0,		setifcaps },
		{ "-udp6csum",	-IFCAP_CSUM_UDPv6,0,		setifcaps },
		{ "tcp4csum-rx",IFCAP_CSUM_TCPv4_Rx,0,		setifcaps },
		{ "-tcp4csum-rx",-IFCAP_CSUM_TCPv4_Rx,0,	setifcaps },
		{ "udp4csum-rx",IFCAP_CSUM_UDPv4_Rx,0,		setifcaps },
		{ "-udp4csum-rx",-IFCAP_CSUM_UDPv4_Rx,0,	setifcaps },
		{ 0,		0,		0,		setifaddr },
		{ 0,		0,		0,		setifdstaddr },
};

void		printall(const char *);
void		list_cloners(void);
void 		status(const struct sockaddr_dl *);
void 		usage(void);
const char 	*get_string(const char *, const char *, u_int8_t *, int *);
void		print_string(const u_int8_t *, int);
struct afswtch *lookup_af(const char *);

int
main(int argc, char *argv[])
 {
	int ch;

	cmds_init();
	/* Parse command-line options */
	aflag = mflag = vflag = zflag = 0;
	while ((ch = getopt(argc, argv, "AabCdlmsuvz"
#ifdef INET6
					"L"
#endif
			)) != -1) {
		switch (ch) {
		case 'A':
			warnx("-A is deprecated");
			break;

		case 'a':
			aflag = 1;
			break;

		case 'b':
			bflag = 1;
			break;

		case 'C':
			Cflag = 1;
			break;

		case 'd':
			dflag = 1;
			break;

#ifdef INET6
		case 'L':
			Lflag = 1;
			break;
#endif

		case 'l':
			lflag = 1;
			break;

		case 'm':
			mflag = 1;
			break;

		case 's':
			sflag = 1;
			break;

		case 'u':
			uflag = 1;
			break;

		case 'v':
			vflag = 1;
			break;

		case 'z':
			zflag = 1;
			break;

		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * -l means "list all interfaces", and is mutally exclusive with
	 * all other flags/commands.
	 *
	 * -C means "list all names of cloners", and it mutually exclusive
	 * with all other flags/commands.
	 *
	 * -a means "print status of all interfaces".
	 */
	if ((lflag || Cflag) && (aflag || mflag || vflag || argc || zflag)) {
		usage();
	}
#ifdef INET6
	if ((lflag || Cflag) && Lflag) {
		usage();
	}
#endif
	if (lflag && Cflag) {
		usage();
	}
	if (Cflag) {
		if (argc) {
			usage();
		}
		list_cloners();
		exit(0);
	}
	if (aflag || lflag) {
		if (argc > 1) {
			usage();
		} else if (argc == 1) {
			afp = lookup_af(argv[0]);
			if (afp == NULL) {
				usage();
			}
		}
		if (afp) {
			af = ifr.ifr_addr.sa_family = afp->af_af;
		}
		printall(NULL);
		exit(0);
	}

	/* Make sure there's an interface name. */
	if (argc < 1) {
		usage();
	}
	if (strlcpy(name, argv[0], sizeof(name)) >= sizeof(name)) {
		errx(1, "interface name '%s' too long", argv[0]);
	}
	argc--;
	argv++;

	/*
	 * NOTE:  We must special-case the `create' command right
	 * here as we would otherwise fail in getinfo().
	 */
	if (argc > 0 && strcmp(argv[0], "create") == 0) {
		clone_create(argv[0], 0);
		argc--, argv++;
		if (argc == 0) {
			exit(0);
		}
	}

	/* Check for address family. */
	afp = NULL;
	if (argc > 0) {
		afp = lookup_af(argv[0]);
		if (afp != NULL) {
			argv++;
			argc--;
		}
	}

	/* Initialize af, just for use in getinfo(). */
	if (afp) {
		af = afp->af_af;
	}

	/* Get information about the interface. */
	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (getinfo(&ifr) < 0) {
		exit(1);
	}

	if (sflag) {
		if (argc != 0) {
			usage();
		} else {
			exit(carrier());
		}
	}

	/* No more arguments means interface status. */
	if (argc == 0) {
		printall(name);
		exit(0);
	}

	/* The following operations assume inet family as the default. */
	af = ifr.ifr_addr.sa_family = afp->af_af;

#ifdef INET6
	/* initialization */
	in6_init();
#endif

	/* Process commands. */
	while (argc > 0) {
		const struct cmd *p;

		for (p = cmdlist; p != NULL; p = p->c_next) {
			if (strcmp(argv[0], p->c_name) == 0) {
				break;
			}
		}
		if (p->c_name == 0 && setaddr) {
			if ((flags & IFF_POINTOPOINT) == 0) {
				errx(EXIT_FAILURE, "can't set destination address %s",
						"on non-point-to-point link");
			}
			p++; /* got src, do dst */
		}
		if (p->c_func != NULL || p->c_func2 != NULL) {
			if (p->c_parameter == NEXTARG) {
				if (argc < 2) {
					errx(EXIT_FAILURE, "'%s' requires argument", p->c_name);
				}
				(*p->c_func)(argv[1], 0);
				argc--, argv++;
			} else if (p->c_parameter == NEXTARG2) {
				if (argc < 3) {
					errx(EXIT_FAILURE, "'%s' requires 2 arguments", p->c_name);
				}
				(*p->c_func2)(argv[1], argv[2]);
				argc -= 2, argv += 2;
			} else {
				(*p->c_func)(argv[0], p->c_parameter);
			}
			actions |= p->c_action;
		}
		argc--, argv++;
	}
	/*
	 * See if multiple alias, -alias, or delete commands were
	 * specified. More than one constitutes an invalid command line
	 */

	if (conflicting > 1) {
		errx(EXIT_FAILURE, "Only one use of alias, -alias or delete is valid.");
	}

	/* Process any media commands that may have been issued. */
	process_media_commands();

	if (af == AF_INET6 && explicit_prefix == 0) {
		/*
		 * Aggregatable address architecture defines all prefixes
		 * are 64. So, it is convenient to set prefixlen to 64 if
		 * it is not specified.
		 */
		setifprefixlen("64", 0);
		/* in6_getprefix("64", MASK) if MASK is available here... */
	}

#ifndef INET_ONLY

	xns_nsip(setipdst, AF_NS);

#endif	/* INET_ONLY */

	if (clearaddr) {
		(void) strncpy(afp->af_ridreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, afp->af_difaddr, afp->af_ridreq) == -1) {
			err(EXIT_FAILURE, "SIOCDIFADDR");
		}
	}
	if (newaddr > 0) {
		(void) strncpy(afp->af_addreq, name, sizeof ifr.ifr_name);
		if (ioctl(s, afp->af_aifaddr, afp->af_addreq) == -1) {
			warn("SIOCAIFADDR");
		}
	}

	if (g_ifcr_updated) {
		(void) strncpy(g_ifcr.ifcr_name, name, sizeof(g_ifcr.ifcr_name));
		if (ioctl(s, SIOCSIFCAP, &g_ifcr) == -1) {
			err(EXIT_FAILURE, "SIOCSIFCAP");
		}
	}
	exit(0);
}

void
if_init(void)
{
	size_t i;

	for (i = 0; i < nitems(if_cmds);  i++) {
		cmd_register(&if_cmds[i]);
	}
}

void
cmds_init(void)
{
	if_init();
	inet_init();
#ifdef INET6
	inet6_init();
#endif
	media_init();
	ieee80211_init();
#ifndef INET_ONLY
	carp_init();
#endif
	tunnel_init();
	xns_init();
	vlan_init();
}

void
af_register(struct afswtch *p)
{
	p->af_next = aflist;
	aflist = p;
}

struct afswtch *
af_getbyname(const char *afname)
{
	struct afswtch *a;

	for (a = aflist; a !=  NULL; a = a->af_next) {
		if (strcmp(a->af_name, afname) == 0) {
			return (a);
		}
	}
	return (NULL);
}

struct afswtch *
af_getbyfamily(int affamily)
{
	struct afswtch *a;

	for (a = aflist; a != NULL; a = a->af_next) {
		if (afp->af_af == affamily) {
			return (a);
		}
	}
	return (NULL);
}

struct afswtch *
lookup_af(const char *cp)
{
	struct afswtch *a;

	a = af_getbyname(cp);
	return (a);
}

void
cmd_register(struct cmd *p)
{
	p->c_next = cmdlist;
	cmdlist = p;
}

struct cmd *
cmd_lookup(const char *cmdname)
{
	struct cmd *p;

	for (p = cmdlist; p != NULL; p = p->c_next) {
		if (strcmp(cmdname, p->c_name) == 0) {
			return (p);
		}
	}
	return (NULL);
}

void
getsock(int naf)
{
	static int oaf = -1;

	if (oaf == naf)
		return;
	if (oaf != -1)
		close(s);
	s = socket(naf, SOCK_DGRAM, 0);
	if (s < 0)
		oaf = -1;
	else
		oaf = naf;
}

int
getinfo(struct ifreq *giifr)
{
	getsock(af);
	if (s < 0)
		err(EXIT_FAILURE, "socket");
	if (ioctl(s, SIOCGIFFLAGS, giifr) == -1) {
		warn("SIOCGIFFLAGS %s", giifr->ifr_name);
		return (-1);
	}
	flags = giifr->ifr_flags;
	if (ioctl(s, SIOCGIFMETRIC, giifr) == -1) {
		warn("SIOCGIFMETRIC %s", giifr->ifr_name);
		metric = 0;
	} else
		metric = giifr->ifr_metric;
	if (ioctl(s, SIOCGIFMTU, giifr) == -1)
		mtu = 0;
	else
		mtu = giifr->ifr_mtu;

	memset(&g_ifcr, 0, sizeof(g_ifcr));
	strcpy(g_ifcr.ifcr_name, giifr->ifr_name);
	(void) ioctl(s, SIOCGIFCAP, &g_ifcr);

	return (0);
}

void
printall(const char *ifname)
{
	struct ifaddrs *ifap, *ifa;
	struct ifreq paifr;
	const struct sockaddr_dl *sdl = NULL;
	int idx;
	char *p;

	if (getifaddrs(&ifap) != 0)
		err(EXIT_FAILURE, "getifaddrs");
	p = NULL;
	idx = 0;
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		memset(&paifr, 0, sizeof(paifr));
		strncpy(paifr.ifr_name, ifa->ifa_name, sizeof(paifr.ifr_name));
		if (sizeof(paifr.ifr_addr) >= ifa->ifa_addr->sa_len) {
			memcpy(&paifr.ifr_addr, ifa->ifa_addr,
			    ifa->ifa_addr->sa_len);
		}

		if (ifname && strcmp(ifname, ifa->ifa_name) != 0)
			continue;
		if (ifa->ifa_addr->sa_family == AF_LINK)
			sdl = (const struct sockaddr_dl *) ifa->ifa_addr;
		if (p && strcmp(p, ifa->ifa_name) == 0)
			continue;
		if (strlcpy(name, ifa->ifa_name, sizeof(name)) >= sizeof(name))
			continue;
		p = ifa->ifa_name;

		if (getinfo(&paifr) < 0)
			continue;
		if (bflag && (ifa->ifa_flags & IFF_BROADCAST) == 0)
			continue;
		if (dflag && (ifa->ifa_flags & IFF_UP) != 0)
			continue;
		if (uflag && (ifa->ifa_flags & IFF_UP) == 0)
			continue;

		if (sflag && carrier())
			continue;
		idx++;
		/*
		 * Are we just listing the interfaces?
		 */
		if (lflag) {
			if (idx > 1)
				printf(" ");
			fputs(name, stdout);
			continue;
		}

		status(sdl);
		sdl = NULL;
	}
	if (lflag)
		printf("\n");
	freeifaddrs(ifap);
}

void
list_cloners(void)
{
	struct if_clonereq ifcr;
	char *cp, *buf;
	int idx;

	memset(&ifcr, 0, sizeof(ifcr));

	getsock(AF_INET);

	if (ioctl(s, SIOCIFGCLONERS, &ifcr) == -1)
		err(EXIT_FAILURE, "SIOCIFGCLONERS for count");

	buf = malloc(ifcr.ifcr_total * IFNAMSIZ);
	if (buf == NULL)
		err(EXIT_FAILURE, "unable to allocate cloner name buffer");

	ifcr.ifcr_count = ifcr.ifcr_total;
	ifcr.ifcr_buffer = buf;

	if (ioctl(s, SIOCIFGCLONERS, &ifcr) == -1)
		err(EXIT_FAILURE, "SIOCIFGCLONERS for names");

	/*
	 * In case some disappeared in the mean time, clamp it down.
	 */
	if (ifcr.ifcr_count > ifcr.ifcr_total)
		ifcr.ifcr_count = ifcr.ifcr_total;

	for (cp = buf, idx = 0; idx < ifcr.ifcr_count; idx++, cp += IFNAMSIZ) {
		if (idx > 0)
			printf(" ");
		printf("%s", cp);
	}

	printf("\n");
	free(buf);
	return;
}

/*ARGSUSED*/
void
clone_create(const char *addr, int param)
{

	/* We're called early... */
	getsock(AF_INET);

	(void) strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCIFCREATE, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCIFCREATE");
}

/*ARGSUSED*/
void
clone_destroy(const char *addr, int param)
{

	(void)strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	if (ioctl(s, SIOCIFDESTROY, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCIFDESTROY");
}

/*ARGSUSED*/
void
setifaddr(const char *addr, int param)
{
	struct ifreq *siifr;		/* XXX */

	/*
	 * Delay the ioctl to set the interface addr until flags are all set.
	 * The address interpretation may depend on the flags,
	 * and the flags may change when the address is set.
	 */
	setaddr++;
	if (newaddr == -1)
		newaddr = 1;
	if (doalias == 0 && afp->af_gifaddr != 0) {
		siifr = (struct ifreq *)afp->af_ridreq;
		(void)strncpy(siifr->ifr_name, name, sizeof(siifr->ifr_name));
		siifr->ifr_addr.sa_family = afp->af_af;
		if (ioctl(s, afp->af_gifaddr, afp->af_ridreq) == 0) {
			clearaddr = 1;
		} else if (errno == EADDRNOTAVAIL) {
			/* No address was assigned yet. */
			;
		} else {
			err(EXIT_FAILURE, "SIOCGIFADDR");
		}
	}

	(*afp->af_getaddr)(addr, (doalias >= 0 ? ADDR : RIDADDR));
}

void
setifnetmask(const char *addr, int d)
{
	(*afp->af_getaddr)(addr, MASK);
}

void
setifbroadaddr(const char *addr, int d)
{
	(*afp->af_getaddr)(addr, DSTADDR);
}

void
setifipdst(const char *addr, int d)
{
	(*afp->af_getaddr)(addr, DSTADDR);
	setipdst++;
	clearaddr = 0;
	newaddr = 0;
}

#define rqtosa(x) (&(((struct ifreq *)(afp->x))->ifr_addr))
/*ARGSUSED*/
void
notealias(const char *addr, int param)
{
	if (setaddr && doalias == 0 && param < 0)
		(void) memcpy(rqtosa(af_ridreq), rqtosa(af_addreq),
		    rqtosa(af_addreq)->sa_len);
	doalias = param;
	if (param < 0) {
		clearaddr = 1;
		newaddr = 0;
		conflicting++;
	} else {
		clearaddr = 0;
		conflicting++;
	}
}

/*ARGSUSED*/
void
notrailers(const char *vname, int value)
{
	puts("Note: trailers are no longer sent, but always received");
}

/*ARGSUSED*/
void
setifdstaddr(const char *addr, int param)
{
	(*afp->af_getaddr)(addr, DSTADDR);
}

void
setifflags(const char *vname, int value)
{
	struct ifreq ifreq;

	(void) strncpy(ifreq.ifr_name, name, sizeof(ifreq.ifr_name));
 	if (ioctl(s, SIOCGIFFLAGS, &ifreq) == -1)
		err(EXIT_FAILURE, "SIOCGIFFLAGS");
 	flags = ifreq.ifr_flags;

	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value;
	ifreq.ifr_flags = flags;
	if (ioctl(s, SIOCSIFFLAGS, &ifreq) == -1)
		err(EXIT_FAILURE, "SIOCSIFFLAGS");
}

void
setifcaps(const char *vname, int value)
{

	if (value < 0) {
		value = -value;
		g_ifcr.ifcr_capenable &= ~value;
	} else
		g_ifcr.ifcr_capenable |= value;

	g_ifcr_updated = 1;
}

void
setifmetric(const char *val, int d)
{
	char *ep = NULL;

	(void) strncpy(ifr.ifr_name, name, sizeof (ifr.ifr_name));
	ifr.ifr_metric = strtoul(val, &ep, 10);
	if (!ep || *ep)
		errx(EXIT_FAILURE, "%s: invalid metric", val);
	if (ioctl(s, SIOCSIFMETRIC, &ifr) == -1)
		warn("SIOCSIFMETRIC");
}

void
setifmtu(const char *val, int d)
{
	char *ep = NULL;

	(void)strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = strtoul(val, &ep, 10);
	if (!ep || *ep)
		errx(EXIT_FAILURE, "%s: invalid mtu", val);
	if (ioctl(s, SIOCSIFMTU, &ifr) == -1)
		warn("SIOCSIFMTU");
}

const char *
get_string(const char *val, const char *sep, u_int8_t *buf, int *lenp)
{
	int len;
	int hexstr;
	u_int8_t *p;

	len = *lenp;
	p = buf;
	hexstr = (val[0] == '0' && tolower((u_char)val[1]) == 'x');
	if (hexstr)
		val += 2;
	for (;;) {
		if (*val == '\0')
			break;
		if (sep != NULL && strchr(sep, *val) != NULL) {
			val++;
			break;
		}
		if (hexstr) {
			if (!isxdigit((u_char)val[0]) ||
			    !isxdigit((u_char)val[1])) {
				warnx("bad hexadecimal digits");
				return NULL;
			}
		}
		if (p > buf + len) {
			if (hexstr)
				warnx("hexadecimal digits too long");
			else
				warnx("strings too long");
			return NULL;
		}
		if (hexstr) {
#define	tohex(x)	(isdigit(x) ? (x) - '0' : tolower(x) - 'a' + 10)
			*p++ = (tohex((u_char)val[0]) << 4) |
			    tohex((u_char)val[1]);
#undef tohex
			val += 2;
		} else {
			*p++ = *val++;
		}
	}
	len = p - buf;
	if (len < *lenp)
		memset(p, 0, *lenp - len);
	*lenp = len;
	return val;
}

void
print_string(const u_int8_t *buf, int len)
{
	int i;
	int hasspc;

	i = 0;
	hasspc = 0;
	if (len < 2 || buf[0] != '0' || tolower(buf[1]) != 'x') {
		for (; i < len; i++) {
			if (!isprint(buf[i]))
				break;
			if (isspace(buf[i]))
				hasspc++;
		}
	}
	if (i == len) {
		if (hasspc || len == 0)
			printf("\"%.*s\"", len, buf);
		else
			printf("%.*s", len, buf);
	} else {
		printf("0x");
		for (i = 0; i < len; i++)
			printf("%02x", buf[i]);
	}
}

#define	IFFBITS \
"\020\1UP\2BROADCAST\3DEBUG\4LOOPBACK\5POINTOPOINT\6NOTRAILERS\7RUNNING\10NOARP\
\11PROMISC\12ALLMULTI\13OACTIVE\14SIMPLEX\15LINK0\16LINK1\17LINK2\20MULTICAST"

#define	IFCAPBITS \
"\020\1IP4CSUM\2TCP4CSUM\3UDP4CSUM\4TCP6CSUM\5UDP6CSUM\6TCP4CSUM_Rx\7UDP4CSUM_Rx"

const int ifm_status_valid_list[] = IFM_STATUS_VALID_LIST;

const struct ifmedia_status_description ifm_status_descriptions[] =
    IFM_STATUS_DESCRIPTIONS;

/*
 * Print the status of the interface.  If an address family was
 * specified, show it and it only; otherwise, show them all.
 */
void
status(const struct sockaddr_dl *sdl)
{
	struct afswtch *p = afp;
	struct ifmediareq ifmr;
	struct ifdatareq ifdr;
	int *media_list, i;
	char hbuf[NI_MAXHOST];
	char fbuf[BUFSIZ];

	(void)snprintb(fbuf, sizeof(fbuf), IFFBITS, flags);
	printf("%s: flags=%s", name, &fbuf[2]);
	if (metric)
		printf(" metric %lu", metric);
	if (mtu)
		printf(" mtu %lu", mtu);
	printf("\n");

	if (g_ifcr.ifcr_capabilities) {
		(void)snprintb(fbuf, sizeof(fbuf), IFCAPBITS,
		    g_ifcr.ifcr_capabilities);
		printf("\tcapabilities=%s\n", &fbuf[2]);
		(void)snprintb(fbuf, sizeof(fbuf), IFCAPBITS,
		    g_ifcr.ifcr_capenable);
		printf("\tenabled=%s\n", &fbuf[2]);
	}

	ieee80211_status();
	vlan_status();
#ifndef INET_ONLY
	carp_status();
#endif
	tunnel_status();

	if (sdl != NULL &&
	    getnameinfo((const struct sockaddr *)sdl, sdl->sdl_len,
		hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST) == 0 &&
	    hbuf[0] != '\0')
		printf("\taddress: %s\n", hbuf);

	(void) memset(&ifmr, 0, sizeof(ifmr));
	(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

	if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1) {
		/*
		 * Interface doesn't support SIOC{G,S}IFMEDIA.
		 */
		goto iface_stats;
	}

	if (ifmr.ifm_count == 0) {
		warnx("%s: no media types?", name);
		goto iface_stats;
	}

	media_list = (int *)malloc(ifmr.ifm_count * sizeof(int));
	if (media_list == NULL)
		err(EXIT_FAILURE, "malloc");
	ifmr.ifm_ulist = media_list;

	if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1)
		err(EXIT_FAILURE, "SIOCGIFMEDIA");

	printf("\tmedia: %s ", get_media_type_string(ifmr.ifm_current));
	print_media_word(ifmr.ifm_current, " ");
	if (ifmr.ifm_active != ifmr.ifm_current) {
		printf(" (");
		print_media_word(ifmr.ifm_active, " ");
		printf(")");
	}
	printf("\n");

	if (ifmr.ifm_status & IFM_STATUS_VALID) {
		const struct ifmedia_status_description *ifms;
		int bitno, found = 0;

		printf("\tstatus: ");
		for (bitno = 0; ifm_status_valid_list[bitno] != 0; bitno++) {
			for (ifms = ifm_status_descriptions;
			     ifms->ifms_valid != 0; ifms++) {
				if (ifms->ifms_type !=
				      IFM_TYPE(ifmr.ifm_current) ||
				    ifms->ifms_valid !=
				      ifm_status_valid_list[bitno])
					continue;
				printf("%s%s", found ? ", " : "",
				    IFM_STATUS_DESC(ifms, ifmr.ifm_status));
				found = 1;

				/*
				 * For each valid indicator bit, there's
				 * only one entry for each media type, so
				 * terminate the inner loop now.
				 */
				break;
			}
		}

		if (found == 0)
			printf("unknown");
		printf("\n");
	}

	if (mflag) {
		int type, printed_type;

		for (type = IFM_NMIN; type <= IFM_NMAX; type += IFM_NMIN) {
			for (i = 0, printed_type = 0; i < ifmr.ifm_count; i++) {
				if (IFM_TYPE(media_list[i]) != type)
					continue;
				if (printed_type == 0) {
					printf("\tsupported %s media:\n",
					    get_media_type_string(type));
					printed_type = 1;
				}
				printf("\t\tmedia ");
				print_media_word(media_list[i], " mediaopt ");
				printf("\n");
			}
		}
	}

	free(media_list);

 iface_stats:
	if (!vflag && !zflag)
		goto proto_status;

	(void) strncpy(ifdr.ifdr_name, name, sizeof(ifdr.ifdr_name));

	if (ioctl(s, zflag ? SIOCZIFDATA:SIOCGIFDATA, &ifdr) == -1) {
		err(EXIT_FAILURE, zflag ? "SIOCZIFDATA" : "SIOCGIFDATA");
	} else {
		struct if_data * const ifi = &ifdr.ifdr_data;
#define	PLURAL(n)	((n) == 1 ? "" : "s")
		printf("\tinput: %llu packet%s, %llu byte%s",
		    (unsigned long long) ifi->ifi_ipackets,
		    PLURAL(ifi->ifi_ipackets),
		    (unsigned long long) ifi->ifi_ibytes,
		    PLURAL(ifi->ifi_ibytes));
		if (ifi->ifi_imcasts)
			printf(", %llu multicast%s",
			    (unsigned long long) ifi->ifi_imcasts,
			    PLURAL(ifi->ifi_imcasts));
		if (ifi->ifi_ierrors)
			printf(", %llu error%s",
			    (unsigned long long) ifi->ifi_ierrors,
			    PLURAL(ifi->ifi_ierrors));
		if (ifi->ifi_iqdrops)
			printf(", %llu queue drop%s",
			    (unsigned long long) ifi->ifi_iqdrops,
			    PLURAL(ifi->ifi_iqdrops));
		if (ifi->ifi_noproto)
			printf(", %llu unknown protocol",
			    (unsigned long long) ifi->ifi_noproto);
		printf("\n\toutput: %llu packet%s, %llu byte%s",
		    (unsigned long long) ifi->ifi_opackets,
		    PLURAL(ifi->ifi_opackets),
		    (unsigned long long) ifi->ifi_obytes,
		    PLURAL(ifi->ifi_obytes));
		if (ifi->ifi_omcasts)
			printf(", %llu multicast%s",
			    (unsigned long long) ifi->ifi_omcasts,
			    PLURAL(ifi->ifi_omcasts));
		if (ifi->ifi_oerrors)
			printf(", %llu error%s",
			    (unsigned long long) ifi->ifi_oerrors,
			    PLURAL(ifi->ifi_oerrors));
		if (ifi->ifi_collisions)
			printf(", %llu collision%s",
			    (unsigned long long) ifi->ifi_collisions,
			    PLURAL(ifi->ifi_collisions));
		printf("\n");
#undef PLURAL
	}

proto_status:
	if ((p = afp) != NULL) {
		(*p->af_status)(1);
	} else for (p = aflist; p->af_name; p++) {
		ifr.ifr_addr.sa_family = p->af_af;
		(*p->af_status)(0);
	}
}

void
setifprefixlen(const char *addr, int d)
{
	if (*afp->af_getprefix) {
		(*afp->af_getprefix)(addr, MASK);
	}
	explicit_prefix = 1;
}

void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr,
	    "usage: %s [-m] [-v] [-z] "
#ifdef INET6
		"[-L] "
#endif
		"interface\n"
		"\t[ af [ address [ dest_addr ] ] [ netmask mask ] [ prefixlen n ]\n"
		"\t\t[ alias | -alias ] ]\n"
		"\t[ up ] [ down ] [ metric n ] [ mtu n ]\n"
		"\t[ nwid network_id ] [ nwkey network_key | -nwkey ]\n"
		"\t[ powersave | -powersave ] [ powersavesleep duration ]\n"
		"\t[ [ af ] tunnel src_addr dest_addr ] [ deletetunnel ]\n"
		"\t[ arp | -arp ]\n"
		"\t[ media type ] [ mediaopt opts ] [ -mediaopt opts ] "
		"[ instance minst ]\n"
		"\t[ vlan n vlanif i ]\n"
		"\t[ anycast | -anycast ] [ deprecated | -deprecated ]\n"
		"\t[ tentative | -tentative ] [ pltime n ] [ vltime n ] [ eui64 ]\n"
		"\t[ link0 | -link0 ] [ link1 | -link1 ] [ link2 | -link2 ]\n"
		"       %s -a [-b] [-m] [-d] [-u] [-v] [-z] [ af ]\n"
		"       %s -l [-b] [-d] [-u] [-s]\n"
		"       %s -C\n"
		"       %s interface create\n"
		"       %s interface destroy\n",
		progname, progname, progname, progname, progname, progname);
	exit(1);
}
