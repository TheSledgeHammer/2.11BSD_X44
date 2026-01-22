/*	$NetBSD: socket.h,v 1.68 2003/08/07 16:34:14 agc Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1985, 1986, 1988, 1993, 1994
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
 *	@(#)socket.h	8.6 (Berkeley) 5/3/95
 */
/*
 * Copyright (c) 1982, 1985, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)socket.h	7.2.2 (2.11BSD GTE) 1995/10/11
 */

#ifndef	_SYS_SOCKET_H_
#define	_SYS_SOCKET_H_

/*
 * Definitions related to sockets: types, address families, options.
 */

#include <sys/types.h>			/* for off_t, uid_t, and gid_t */

/*
 * Data types.
 */
#include <sys/ansi.h>
#ifndef sa_family_t
typedef __sa_family_t	sa_family_t;
#define sa_family_t	__sa_family_t
#endif

#ifndef socklen_t
typedef __socklen_t	socklen_t;
#define socklen_t	__socklen_t
#endif

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_SSIZE_T_
typedef	_BSD_SSIZE_T_	ssize_t;
#undef	_BSD_SSIZE_T_
#endif

#include <sys/uio.h>

/*
 * Types
 */
#define	SOCK_STREAM		1		/* stream socket */
#define	SOCK_DGRAM		2		/* datagram socket */
#define	SOCK_RAW		3		/* raw-protocol interface */
#define	SOCK_RDM		4		/* reliably-delivered message */
#define	SOCK_SEQPACKET		5		/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG		0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN		0x0002		/* socket has had listen() */
#define	SO_REUSEADDR		0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE		0x0008		/* keep connections alive */
#define	SO_DONTROUTE		0x0010		/* just use interface addresses */
#define	SO_BROADCAST		0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK		0x0040		/* bypass hardware when possible */
#define	SO_LINGER		0x0080		/* linger on close if data present */
#define	SO_OOBINLINE		0x0100		/* leave received OOB data in line */
#define	SO_REUSEPORT		0x0200		/* allow local address & port reuse */
#define	SO_TIMESTAMP		0x2000		/* timestamp received dgram traffic */

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF		0x1001		/* send buffer size */
#define SO_RCVBUF		0x1002		/* receive buffer size */
#define SO_SNDLOWAT		0x1003		/* send low-water mark */
#define SO_RCVLOWAT		0x1004		/* receive low-water mark */
#define SO_SNDTIMEO		0x1005		/* send timeout */
#define SO_RCVTIMEO		0x1006		/* receive timeout */
#define	SO_ERROR		0x1007		/* get error status and clear */
#define	SO_TYPE			0x1008		/* get socket type */

/*
 * Structure used for manipulating linger option.
 */
struct	linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET		0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	AF_UNSPEC		0		/* unspecified */
#define	AF_UNIX			1		/* local to host (pipes, portals) */
#define	AF_LOCAL		AF_UNIX		/* draft POSIX compatibility */
#define	AF_INET			2		/* internetwork: UDP, TCP, etc. */
#define	AF_IMPLINK		3		/* arpanet imp addresses */
#define	AF_PUP			4		/* pup protocols: e.g. BSP */
#define	AF_CHAOS		5		/* mit CHAOS protocols */
#define	AF_NS			6		/* XEROX NS protocols */
#define	AF_ISO			7		/* ISO protocols */
#define	AF_OSI			AF_ISO
#define	AF_NBS			8		/* nbs protocols */
#define	AF_ECMA			9		/* european computer manufacturers */
#define	AF_DATAKIT		10		/* datakit protocols */
#define	AF_CCITT		11		/* CCITT protocols, X.25 etc */
#define	AF_SNA			12		/* IBM SNA */
#define AF_DECnet		13		/* DECnet */
#define AF_DLI			14		/* Direct data link interface */
#define AF_LAT			15		/* LAT */
#define	AF_HYLINK		16		/* NSC Hyperchannel */
#define	AF_APPLETALK		17		/* Apple Talk */
#define	AF_ROUTE	        18		/* Internal Routing Protocol */
#define	AF_IPX			19		/* Novell Internet Protocol */
#define	AF_LINK		        20		/* Link layer interface */
#define	AF_INET6		21		/* IP version 6 */
#define AF_NATM			22		/* native ATM access */
#define AF_ARP			23		/* (rev.) addr. res. prot. (RFC 826) */
#define pseudo_AF_KEY		24		/* Internal key management protocol  */
#define	pseudo_AF_HDRCMPLT 	25		/* Used by BPF to not rewrite hdrs
				         	  	  in interface output routine */
#define	AF_MPLS			26		/* MultiProtocol Label Switching */

#define	AF_MAX			26

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
	unsigned short	sa_len;				/* total length */
	sa_family_t		sa_family;			/* address family */
	char			sa_data[14];		/* up to 14 bytes of direct address */
};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	unsigned short		sp_family;			/* address family */
	unsigned short		sp_protocol;		/* protocol */
};

/*
 * RFC 2553: protocol-independent placeholder for socket addresses
 */
#define _SS_MAXSIZE		128
#define _SS_ALIGNSIZE		(sizeof(__int64_t))
#define _SS_PAD1SIZE		(_SS_ALIGNSIZE - 2)
#define _SS_PAD2SIZE		(_SS_MAXSIZE - 2 - _SS_PAD1SIZE - _SS_ALIGNSIZE)

struct sockaddr_storage {
	__uint8_t		ss_len;		/* address length */
	sa_family_t		ss_family;	/* address family */
	char			__ss_pad1[_SS_PAD1SIZE];
	__int64_t   		__ss_align;	/* force desired structure storage alignment */
	char			__ss_pad2[_SS_PAD2SIZE];
};

/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC		AF_UNSPEC
#define	PF_UNIX			AF_UNIX
#define	PF_INET			AF_INET
#define	PF_IMPLINK		AF_IMPLINK
#define	PF_PUP			AF_PUP
#define	PF_CHAOS		AF_CHAOS
#define	PF_NS			AF_NS
#define	PF_ISO			AF_ISO
#define	PF_OSI			AF_ISO
#define	PF_NBS			AF_NBS
#define	PF_ECMA			AF_ECMA
#define	PF_DATAKIT		AF_DATAKIT
#define	PF_CCITT		AF_CCITT
#define	PF_SNA			AF_SNA
#define PF_DECnet		AF_DECnet
#define PF_DLI			AF_DLI
#define PF_LAT			AF_LAT
#define	PF_HYLINK		AF_HYLINK
#define	PF_APPLETALK		AF_APPLETALK
#define	PF_ROUTE		AF_ROUTE
#define	PF_IPX 			AF_IPX
#define	PF_LINK			AF_LINK
#define	PF_INET6		AF_INET6
#define PF_NATM 		AF_NATM
#define PF_ARP			AF_ARP
#define PF_KEY 		    	pseudo_AF_KEY	/* like PF_ROUTE, only for key mgmt */
#define	PF_MPLS			AF_MPLS
#define	PF_MAX			AF_MAX

/*
 * These are the valid values for the "how" field used by shutdown(2).
 */
#define	SHUT_RD		0
#define	SHUT_WR		1
#define	SHUT_RDWR	2

/*
 * PF_ROUTE - Routing table
 *
 * Three additional levels are defined:
 *	Fourth: address family, 0 is wildcard
 *	Fifth: type of info, defined below
 *	Sixth: flag(s) to mask with for NET_RT_FLAGS
 */
#define	NET_RT_DUMP			1	/* dump; may limit to a.f. */
#define	NET_RT_FLAGS		2	/* by flags, e.g. RESOLVING */
#define	NET_RT_IFLIST		3	/* survey interface list */

/*
 * Definitions for network related sysctl, CTL_NET.
 *
 * Second level is protocol family.
 * Third level is protocol number.
 *
 * Further levels are defined by the individual families below.
 */
#define NET_MAXID	AF_MAX

#ifndef	_KERNEL
#define CTL_NET_NAMES { 			\
	{ 0, 0 }, 						\
	{ "unix", CTLTYPE_NODE }, 		\
	{ "inet", CTLTYPE_NODE }, 		\
	{ "implink", CTLTYPE_NODE }, 	\
	{ "pup", CTLTYPE_NODE }, 		\
	{ "chaos", CTLTYPE_NODE }, 		\
	{ "xerox_ns", CTLTYPE_NODE }, 	\
	{ "iso", CTLTYPE_NODE }, 		\
	{ "emca", CTLTYPE_NODE }, 		\
	{ "datakit", CTLTYPE_NODE }, 	\
	{ "ccitt", CTLTYPE_NODE }, 		\
	{ "ibm_sna", CTLTYPE_NODE }, 	\
	{ "decnet", CTLTYPE_NODE }, 	\
	{ "dec_dli", CTLTYPE_NODE }, 	\
	{ "lat", CTLTYPE_NODE }, 		\
	{ "hylink", CTLTYPE_NODE }, 	\
	{ "appletalk", CTLTYPE_NODE }, 	\
	{ "route", CTLTYPE_NODE }, 		\
	{ "link_layer", CTLTYPE_NODE }, \
	{ "inet6", CTLTYPE_NODE }, 		\
	{ "arp", CTLTYPE_NODE }, 		\
	{ "key", CTLTYPE_NODE }, 		\
}
#endif

/*
 * Maximum queue length specifiable by listen.
 */
//#define	SOMAXCONN	5
#ifndef SOMAXCONN
#define	SOMAXCONN	128
#endif

/*
 * Message header for recvmsg and sendmsg calls.
 */
struct msghdr {
	caddr_t			msg_name;			/* optional address */
	int				msg_namelen;		/* size of address */
	struct iovec 	*msg_iov;			/* scatter/gather array */
	int				msg_iovlen;			/* # elements in msg_iov */
	caddr_t			msg_accrights;		/* access rights sent/received */
	int				msg_accrightslen;
	int				msg_flags;			/* flags on received message */
#define msg_control 	msg_accrights
#define msg_controllen	msg_accrightslen
};

#define	MSG_OOB			0x1		/* process out-of-band data */
#define	MSG_PEEK		0x2		/* peek at incoming message */
#define	MSG_DONTROUTE	0x4		/* send without using routing tables */
#define	MSG_EOR			0x8		/* data completes record */
#define	MSG_TRUNC		0x10	/* data discarded before delivery */
#define	MSG_CTRUNC		0x20	/* control data lost before delivery */
#define	MSG_WAITALL		0x40	/* wait for full request or error */
#define	MSG_DONTWAIT	0x80	/* this message should be nonblocking */

#define	MSG_MAXIOVLEN	16

/*
 * Header for ancillary data objects in msg_control buffer.
 * Used for additional information with/about a datagram
 * not expressible by flags.  The format is a sequence
 * of message elements headed by cmsghdr structures.
 */
struct cmsghdr {
	unsigned int	cmsg_len;		/* data byte count, including hdr */
	int				cmsg_level;		/* originating protocol */
	int				cmsg_type;		/* protocol-specific type */
	/* followed by	u_char  cmsg_data[]; */
};

#define _CMSG_ALIGN(n)		_ALIGN(n)

/* Round len up to next alignment boundary */
#ifdef _KERNEL
#define CMSG_ALIGN(n)		_CMSG_ALIGN(n)
#endif

/* given pointer to struct cmsghdr, return pointer to data */
#define	CMSG_DATA(cmsg)		((unsigned char *)((cmsg) + _CMSG_ALIGN(sizeof(struct cmsghdr))))
#define	CCMSG_DATA(cmsg)	((const unsigned char *)((cmsg) + _CMSG_ALIGN(sizeof(struct cmsghdr))))

/* given pointer to struct cmsghdr, return pointer to next cmsghdr */
#define	CMSG_NXTHDR(mhdr, cmsg)										\
	(((caddr_t)(cmsg) + _CMSG_ALIGN((cmsg)->cmsg_len) + 			\
			_CMSG_ALIGN(sizeof(struct cmsghdr)) > 					\
	    (mhdr)->msg_control + (mhdr)->msg_controllen) ? 			\
	    (struct cmsghdr *)NULL : 									\
	    (struct cmsghdr *)((caddr_t)(cmsg) + _CMSG_ALIGN((cmsg)->cmsg_len)))

#define	CMSG_FIRSTHDR(mhdr)	((struct cmsghdr *)(mhdr)->msg_control)

/* Length of the contents of a control message of length len */
#define	CMSG_LEN(len)	(_CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))

/* Length of the space taken up by a padded control message of length len */
#define	CMSG_SPACE(len)	(_CMSG_ALIGN(sizeof(struct cmsghdr)) + _CMSG_ALIGN(len))

/* "Socket"-level control message types: */
#define	SCM_RIGHTS		0x01		/* access rights (array of int) */
#define	SCM_TIMESTAMP	0x08		/* timestamp (struct timeval) */
#define	SCM_CREDS		0x10		/* credentials (struct sockcred) */

#ifndef	_KERNEL

#include <sys/cdefs.h>

__BEGIN_DECLS
int	accept(int, struct sockaddr *, int *);
int	bind(int, const struct sockaddr *, int);
int	connect(int, const struct sockaddr *, int);
int	getpeername(int, struct sockaddr *, int *);
int	getsockname(int, struct sockaddr *, int *);
int	getsockopt(int, int, int, void *, int *);
int	listen(int, int);
ssize_t	recv(int, void *, size_t, int);
ssize_t	recvfrom(int, void *, size_t, int, struct sockaddr *, int *);
ssize_t	recvmsg(int, struct msghdr *, int);
ssize_t	send(int, const void *, size_t, int);
ssize_t	sendto(int, const void *, size_t, int, const struct sockaddr *, int);
ssize_t	sendmsg(int, const struct msghdr *, int);
int	setsockopt(int, int, int, const void *, int);
int	shutdown(int, int);
int	socket(int, int, int);
int	socketpair(int, int, int, int *);
int sockatmark(int);
__END_DECLS

#endif /* !_KERNEL */
#endif	/* _SYS_SOCKET_H_ */
