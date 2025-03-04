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
 *
 *	@(#)nameser.h	5.20.1 (2.11BSD GTE) 12/31/93
*/

#ifndef _ARPA_NAMESER_H_
#define	_ARPA_NAMESER_H_

#include <sys/types.h>

/*
 * Define constants based on RFC0883, RFC1034, RFC 1035
 */
#define PACKETSZ	512		/* maximum packet size */
#define MAXDNAME	1025	/* maximum domain name */
#define MAXMSG		65535	/* maximum message size */
#define MAXCDNAME	255		/* maximum compressed domain name */
#define MAXLABEL	63		/* maximum length of domain label */
#define MAXLABELS	128		/* theoretical max #/labels per domain name */
#define MAXNNAME	256		/* maximum uncompressed (binary) domain name*/
#define	MAXPADDR	(sizeof "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff")
#define HFIXEDSZ	12		/* #/bytes of fixed data in header */
	/* Number of bytes of fixed size data in query structure */
#define QFIXEDSZ	4
	/* number of bytes of fixed size data in resource record */
#define RRFIXEDSZ	10
#define INT32SZ		4		/* #/bytes of data in a uint32_t */
#define INT16SZ		2		/* #/bytes of data in a uint16_t */
#define INT8SZ		1		/* #/bytes of data in a u_int8_t */
#define INADDRSZ	4		/* IPv4 T_A */
#define IN6ADDRSZ	16		/* IPv6 T_AAAA */
#define CMPRSFLGS	0xc0	/* Flag bits indicating name compression. */

/*
 * Internet nameserver port number
 */
#define NAMESERVER_PORT	53	/* For both TCP and UDP. */
#define DEFAULTPORT  NAMESERVER_PORT
/*
 * Currently defined opcodes
 */
#define QUERY		0x0		/* standard query */
#define IQUERY		0x1		/* inverse query */
#define STATUS		0x2		/* nameserver status query */
/*#define xxx		0x3	*/	/* 0x3 reserved */
#define NS_NOTIFY_OP 0x4	/* notify secondary of SOA change */
	/* non standard */
#define UPDATEA		0x9		/* add resource record */
#define UPDATED		0xa		/* delete a specific resource record */
#define UPDATEDA	0xb		/* delete all nemed resource record */
#define UPDATEM		0xc		/* modify a specific resource record */
#define UPDATEMA	0xd		/* modify all named resource record */

#define ZONEINIT	0xe		/* initial zone transfer */
#define ZONEREF		0xf		/* incremental zone referesh */

/*
 * Currently defined response codes
 */
#define NOERROR		0		/* no error */
#define FORMERR		1		/* format error */
#define SERVFAIL	2		/* server failure */
#define NXDOMAIN	3		/* non existent domain */
#define NOTIMP		4		/* not implemented */
#define REFUSED		5		/* query refused */
	/* non standard */
#define NOCHANGE	0xf		/* update failed to change db */

/*
 * Type values for resources and queries
 */
#define T_A			1		/* host address */
#define T_NS		2		/* authoritative server */
#define T_MD		3		/* mail destination */
#define T_MF		4		/* mail forwarder */
#define T_CNAME		5		/* connonical name */
#define T_SOA		6		/* start of authority zone */
#define T_MB		7		/* mailbox domain name */
#define T_MG		8		/* mail group member */
#define T_MR		9		/* mail rename name */
#define T_NULL		10		/* null resource record */
#define T_WKS		11		/* well known service */
#define T_PTR		12		/* domain name pointer */
#define T_HINFO		13		/* host information */
#define T_MINFO		14		/* mailbox information */
#define T_MX		15		/* mail routing information */
#define	T_TXT		16		/* text strings */
#define T_RP		17		/* responsible person */
#define T_AFSDB		18		/* AFS cell database */
#define T_X25		19		/* X_25 calling address */
#define T_ISDN		20		/* ISDN calling address */
#define T_RT		21		/* router */
#define T_NSAP		22		/* NSAP address */
#define T_NSAP_PTR	23		/* reverse NSAP lookup (deprecated) */
#define T_SIG		24		/* security signature */
#define T_KEY		25		/* security key */
#define T_PX		26		/* X.400 mail mapping */
#define T_GPOS		27		/* geographical position (withdrawn) */
#define T_AAAA		28		/* IP6 Address */
#define T_LOC		29		/* Location Information */
#define T_NXT		30		/* Next Valid Name in Zone */
#define T_EID		31		/* Endpoint identifier */
#define T_NIMLOC	32		/* Nimrod locator */
#define T_SRV		33		/* Server selection */
#define T_ATMA		34		/* ATM Address */
#define T_NAPTR		35		/* Naming Authority PoinTeR */
#define T_KX		36		/* Key Exchanger */
#define T_CERT		37		/* CERT */
#define T_A6		38		/* A6 */
#define T_DNAME		39		/* DNAME */
#define T_SINK		40		/* SINK */
#define T_OPT		41		/* OPT pseudo-RR, RFC2671 */
#define T_APL		42		/* APL */
#define T_DS		43		/* Delegation Signer */
#define T_SSHFP		44		/* SSH Key Fingerprint */
#define T_RRSIG		46		/* RRSIG */
#define T_NSEC		47		/* NSEC */
#define T_DNSKEY	48		/* DNSKEY */
	/* non standard */
#define T_UINFO		100		/* user (finger) information */
#define T_UID		101		/* user ID */
#define T_GID		102		/* group ID */
#define T_UNSPEC	103		/* Unspecified format (binary data) */
	/* Query type values which do not appear in resource records */
#define	T_TKEY		249		/* Transaction Key */
#define	T_TSIG		250		/* Transaction Signature */
#define	T_IXFR		251		/* incremental zone transfer */
#define T_AXFR		252		/* transfer zone of authority */
#define T_MAILB		253		/* transfer mailbox records */
#define T_MAILA		254		/* transfer mail agent records */
#define T_ANY		255		/* wildcard match */

/*
 * Values for class field
 */

#define C_IN		1		/* the arpa internet */
#define C_CHAOS		3		/* for chaos net at MIT */
#define	C_HS		4
	/* Query class values which do not appear in resource records */
#define C_ANY		255		/* wildcard match */

/*
 * Status return codes for T_UNSPEC conversion routines
 */
#define CONV_SUCCESS 	0
#define CONV_OVERFLOW	-1
#define CONV_BADFMT 	-2
#define CONV_BADCKSUM 	-3
#define CONV_BADBUFLEN 	-4

#ifndef BYTE_ORDER
#define	LITTLE_ENDIAN	1234	/* least-significant byte first (vax) */
#define	BIG_ENDIAN		4321	/* most-significant byte first (IBM, net) */
#define	PDP_ENDIAN		3412	/* LSB first in word, MSW first in long (pdp) */

#if defined(vax) || defined(ns32000) || defined(sun386) || \
    defined(BIT_ZERO_ON_RIGHT)
#define BYTE_ORDER	LITTLE_ENDIAN

#endif
#if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || \
    defined(is68k) || defined (tahoe) || defined (BIT_ZERO_ON_LEFT)
#define BYTE_ORDER	BIG_ENDIAN
#endif
#endif /* BYTE_ORDER */
#if defined(pdp11)
#define BYTE_ORDER	PDP_ENDIAN
#endif

#ifndef BYTE_ORDER
	/* you must determine what the correct bit order is for your compiler */
	UNDEFINED_BIT_ORDER;
#endif
/*
 * Structure for query header, the order of the fields is machine and
 * compiler dependent, in our case, the bits within a byte are assignd 
 * least significant first, while the order of transmition is most 
 * significant first.  This requires a somewhat confusing rearrangement.
 */

typedef struct {
	u_short	id;		/* query identification number */
#if BYTE_ORDER == BIG_ENDIAN
			/* fields in third byte */
	u_char	qr:1;		/* response flag */
	u_char	opcode:4;	/* purpose of message */
	u_char	aa:1;		/* authoritive answer */
	u_char	tc:1;		/* truncated message */
	u_char	rd:1;		/* recursion desired */
			/* fields in fourth byte */
	u_char	ra:1;		/* recursion available */
	u_char	pr:1;		/* primary server required (non standard) */
	u_char	unused:2;	/* unused bits */
	u_char	rcode:4;	/* response code */
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
			/* fields in third byte */
	u_char	rd:1;		/* recursion desired */
	u_char	tc:1;		/* truncated message */
	u_char	aa:1;		/* authoritive answer */
	u_char	opcode:4;	/* purpose of message */
	u_char	qr:1;		/* response flag */
			/* fields in fourth byte */
	u_char	rcode:4;	/* response code */
	u_char	unused:2;	/* unused bits */
	u_char	pr:1;		/* primary server required (non standard) */
	u_char	ra:1;		/* recursion available */
#endif
#if BYTE_ORDER == PDP_ENDIAN
	/* Bit zero on right, compiler doesn't like u_char bit fields:  PDP */
			/* fields in third byte */
	u_int	rd:1;		/* recursion desired */
	u_int	tc:1;		/* truncated message */
	u_int	aa:1;		/* authoritive answer */
	u_int	opcode:4;	/* purpose of message */
	u_int	qr:1;		/* response flag */
			/* fields in fourth byte */
	u_int	rcode:4;	/* response code */
	u_int	unused:2;	/* unused bits */
	u_int	pr:1;		/* primary server required (non standard) */
	u_int	ra:1;		/* recursion available */
#endif
			/* remaining bytes */
	u_short	qdcount;	/* number of question entries */
	u_short	ancount;	/* number of answer entries */
	u_short	nscount;	/* number of authority entries */
	u_short	arcount;	/* number of resource entries */
} HEADER;

/*
 * Defines for handling compressed domain names
 */
#define INDIR_MASK	0xc0

/*
 * Structure for passing resource records around.
 */
struct rrec {
	short	r_zone;			/* zone number */
	short	r_class;		/* class number */
	short	r_type;			/* type number */
	u_long	r_ttl;			/* time to live */
	int		r_size;			/* size of data area */
	const u_char *r_data;	/* pointer to data */
};

//extern	u_short	_getshort(u_char *);
//extern	u_long	_getlong(u_char *);

/*
 * Inline versions of get/put short/long.
 * Pointer is advanced; we assume that both arguments
 * are lvalues and will already be in registers.
 * cp MUST be u_char *.
 */
#define GETSHORT(s, cp) { \
	(s) = *(cp)++ << 8; \
	(s) |= *(cp)++; \
}

#define GETLONG(l, cp) { \
	(l) = *(cp)++ << 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; \
}


#define PUTSHORT(s, cp) { \
	*(cp)++ = (s) >> 8; \
	*(cp)++ = (s); \
}

/*
 * Warning: PUTLONG destroys its first argument.
 */
#define PUTLONG(l, cp) { \
	(cp)[3] = l; \
	(cp)[2] = (l >>= 8); \
	(cp)[1] = (l >>= 8); \
	(cp)[0] = l >> 8; \
	(cp) += sizeof(u_long); \
}

#endif /* !_ARPA_NAMESER_H_ */
