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

/*
 * Data types.
 */
#include <sys/ansi.h>
#ifndef sa_family_t
typedef __sa_family_t	sa_family_t;
#define sa_family_t		__sa_family_t
#endif

#ifndef socklen_t
typedef __socklen_t		socklen_t;
#define socklen_t		__socklen_t
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
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG		0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	SO_LINGER		0x0080		/* linger on close if data present */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */

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
#define	AF_NBS			7		/* nbs protocols */
#define	AF_ECMA			8		/* european computer manufacturers */
#define	AF_DATAKIT		9		/* datakit protocols */
#define	AF_CCITT		10		/* CCITT protocols, X.25 etc */
#define	AF_SNA			11		/* IBM SNA */
#define AF_DECnet		12		/* DECnet */
#define AF_DLI			13		/* Direct data link interface */
#define AF_LAT			14		/* LAT */
#define	AF_HYLINK		15		/* NSC Hyperchannel */
#define	AF_APPLETALK		16		/* Apple Talk */
#define	AF_ROUTE	        17		/* Internal Routing Protocol */
#define	AF_LINK		        18		/* Link layer interface */
#define	AF_INET6			19		/* IP version 6 */
#define AF_ARP				20		/* (rev.) addr. res. prot. (RFC 826) */
#define pseudo_AF_KEY		21		/* Internal key management protocol  */
#define	pseudo_AF_HDRCMPLT 	22		/* Used by BPF to not rewrite hdrs
					          in interface output routine */

#define	AF_MAX			23

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
	u_short			sa_len;				/* total length */
	sa_family_t		sa_family;			/* address family */
	char			sa_data[14];		/* up to 14 bytes of direct address */
};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	u_short			sp_family;			/* address family */
	u_short			sp_protocol;		/* protocol */
};

/*
 * RFC 2553: protocol-independent placeholder for socket addresses
 */
#define _SS_MAXSIZE		128
#define _SS_ALIGNSIZE	(sizeof(__int64_t))
#define _SS_PAD1SIZE	(_SS_ALIGNSIZE - 2)
#define _SS_PAD2SIZE	(_SS_MAXSIZE - 2 - _SS_PAD1SIZE - _SS_ALIGNSIZE)

struct sockaddr_storage {
	__uint8_t		ss_len;		/* address length */
	sa_family_t		ss_family;	/* address family */
	char			__ss_pad1[_SS_PAD1SIZE];
	__int64_t   	__ss_align;/* force desired structure storage alignment */
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
#define	PF_NBS			AF_NBS
#define	PF_ECMA			AF_ECMA
#define	PF_DATAKIT		AF_DATAKIT
#define	PF_CCITT		AF_CCITT
#define	PF_SNA			AF_SNA
#define PF_DECnet		AF_DECnet
#define PF_DLI			AF_DLI
#define PF_LAT			AF_LAT
#define	PF_HYLINK		AF_HYLINK
#define	PF_APPLETALK	AF_APPLETALK
#define	PF_ROUTE	AF_ROUTE

#define	PF_MAX			AF_MAX

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
	u_int	cmsg_len;		/* data byte count, including hdr */
	int		cmsg_level;		/* originating protocol */
	int		cmsg_type;		/* protocol-specific type */
	/* followed by	u_char  cmsg_data[]; */
};

/* given pointer to struct cmsghdr, return pointer to data */
#define	CMSG_DATA(cmsg)		((u_char *)((cmsg) + 1))

/* given pointer to struct cmsghdr, return pointer to next cmsghdr */
#define	CMSG_NXTHDR(mhdr, cmsg)										\
	(((caddr_t)(cmsg) + (cmsg)->cmsg_len + sizeof(struct cmsghdr) > \
	    (mhdr)->msg_control + (mhdr)->msg_controllen) ? 			\
	    (struct cmsghdr *)NULL : 									\
	    (struct cmsghdr *)((caddr_t)(cmsg) + ALIGN((cmsg)->cmsg_len)))

#define	CMSG_FIRSTHDR(mhdr)	((struct cmsghdr *)(mhdr)->msg_control)

/* "Socket"-level control message types: */
#define	SCM_RIGHTS	0x01		/* access rights (array of int) */
#endif	/* _SYS_SOCKET_H_ */
