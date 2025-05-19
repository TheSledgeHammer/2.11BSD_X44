/*
 * Copyright (c) 1983, 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)resolv.h	5.5 (Berkeley) 5/12/87
 */

#ifndef _RESOLV_H_
#define	_RESOLV_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include <arpa/nameser.h>

/*
 * Resolver configuration file.
 * Normally not present, but may contain the address of the
 * inital name server(s) to query and the domain search list.
 */

#ifndef _PATH_RESCONF
#define _PATH_RESCONF        "/etc/resolv.conf"
#endif

/*
 * Global defines and variables for resolver stub.
 */

#define	MAXNS				3		/* max # name servers we'll track */
#define	MAXDNSRCH			6		/* max # default domain levels to try */
#define	LOCALDOMAINPARTS 		2		/* min levels in name that is "local" */
#define MAXDNSLUS			4		/* max # of host lookup types */

#define	RES_TIMEOUT			5		/* seconds between retries */
#define	MAXRESOLVSORT			10		/* number of net to sort on */

#define	RES_MAXTIME			65535		/* Infinity, in milliseconds. */

struct __res_state {
	int				retrans;	 		/* retransmition time interval */
	int				retry;				/* number of times to retransmit */
	long				options;			/* option flags - see below. */
	int				nscount;			/* number of name servers */
	struct	sockaddr_in 		nsaddr_list[MAXNS];		/* address of name server */
#define	nsaddr				nsaddr_list[0]			/* for backward compatibility */
	u_short				id;				/* current packet id */
	char				defdname[MAXDNAME];		/* default domain */
	char				*dnsrch[MAXDNSRCH+1];		/* components of domain to search */
	long				pfcode;				/* RES_PRF_ flags - see below. */
	unsigned 			ndots:4;			/* threshold for initial abs. query */
	unsigned 			nsort:4;			/* number of elements in sort_list[] */
	char				unused[3];
	struct {
		struct in_addr		addr;
		uint32_t		mask;
	} sort_list[MAXRESOLVSORT];
	char    			lookups[MAXDNSLUS];
	int				_vcsock;			/* PRIVATE: for res_send VC i/o */
	u_int				_flags;				/* PRIVATE: see below */
	u_int				_pad;				/* make _u 64 bit aligned */
	union {
		uint16_t		nscount;
		uint16_t		nstimes[MAXNS];			/* ms. */
		int			nssocks[MAXNS];
		struct __res_state_ext	*ext;				/* extension for IPv6 */
	} u;
	u_char				*_rnd;				/* PRIVATE: random state */
};

union res_sockaddr_union {
	struct sockaddr_in		sin;
#ifdef IN6ADDR_ANY_INIT
	struct sockaddr_in6		sin6;
#endif
	char				__space[128];   		/* max size */
};

struct __res_state_ext {
	union res_sockaddr_union 	nsaddr_list[MAXNS];
	struct {
		int			af;
		union {
			struct in_addr 	ina;
			struct in6_addr in6a;
		} addr, mask;
	} sort_list[MAXRESOLVSORT];
	char 				nsuffix[64];
	char 				nsuffix2[64];
	struct timespec 		res_conf_time;
};

typedef struct __res_state 		*res_state;
typedef struct __res_state_ext 		*res_state_ext;

/*%
 * Resolver flags (used to be discrete per-module statics ints).
 */
#define	RES_F_VC		0x00000001	/* socket is TCP */
#define	RES_F_CONN		0x00000002	/* socket is connected */
#define	RES_F_EDNS0ERR		0x00000004	/* EDNS0 caused errors */
#define	RES_F__UNUSED		0x00000008	/* (unused) */
#define	RES_F_LASTMASK		0x000000F0	/* ordinal server of last res_nsend */
#define	RES_F_LASTSHIFT		4		/* bit position of LASTMASK "flag" */
#define	RES_GETLAST(res) 	(((res)._flags & RES_F_LASTMASK) >> RES_F_LASTSHIFT)

/*
 * Resolver options
 */
#define RES_INIT		0x00000001		/* address initialized */
#define RES_DEBUG		0x00000002		/* print debug messages */
#define RES_AAONLY		0x00000004		/* authoritative answers only */
#define RES_USEVC		0x00000008		/* use virtual circuit */
#define RES_PRIMARY		0x00000010		/* query primary server only */
#define RES_IGNTC		0x00000020		/* ignore trucation errors */
#define RES_RECURSE		0x00000040		/* recursion desired */
#define RES_DEFNAMES		0x00000080		/* use default domain name */
#define RES_STAYOPEN		0x00000100		/* Keep TCP socket open */
#define RES_DNSRCH		0x00000200		/* search up local domain tree */

#define RES_ROTATE		0x00004000		/* rotate ns list after each query */
#define	RES_BLAST		0x00020000		/* blast all recursive servers */

#define RES_DEFAULT		(RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)

/*
 * Resolver "pfcode" values.  Used by dig.
 */
#define RES_PRF_STATS		0x0001
/*				0x0002	*/
#define RES_PRF_CLASS   	0x0004
#define RES_PRF_CMD		0x0008
#define RES_PRF_QUES		0x0010
#define RES_PRF_ANS		0x0020
#define RES_PRF_AUTH		0x0040
#define RES_PRF_ADD		0x0080
#define RES_PRF_HEAD1		0x0100
#define RES_PRF_HEAD2		0x0200
#define RES_PRF_TTLID		0x0400
#define RES_PRF_HEADX		0x0800
#define RES_PRF_QUERY		0x1000
#define RES_PRF_REPLY		0x2000
#define RES_PRF_INIT    	0x4000
/*				0x8000	*/

extern struct __res_state _res;

/* Private routines shared between libc/net, named, nslookup and others. */
#define	dn_comp	        	__dn_comp
#define	dn_skipname	    	__dn_skipname
#define fp_resstat      	__fp_resstat
#define	fp_query		__fp_query
#define	hostalias		__hostalias
#define	putlong		    	__putlong
#define	putshort		__putshort
#define p_fqname        	__p_fqname
#define p_query		    	__p_query
#define p_cdname		__p_cdname
#define p_rr			__p_rr
#define p_class		    	__p_class
#define p_type		    	__p_type
#define p_option        	__p_option

#define res_close		__res_close
#define	res_querydomain		__res_querydomain
#define res_send		__res_send

__BEGIN_DECLS
void 		fp_resstat(res_state, FILE *);
void 		fp_query(const u_char *, FILE *);
char 		*hostalias(const char *);
void 		putlong(u_long, u_char *);
void 		putshort(u_short, u_char *);
u_long 		getlong(const u_char *);
u_short		getshort(const u_char *);

void 		p_query(const u_char *);
const u_char 	*p_cdname(const u_char *, const u_char *, FILE *);
const u_char 	*p_fqname(const u_char *, const u_char *, FILE *);
const u_char 	*p_rr(const u_char *, const u_char *, FILE *);
const char	*p_type(int);
const char	*p_class(int);
const char  	*p_option(u_int);

int		dn_skipname(const u_char *, const u_char *);
int		dn_comp(const u_char *, u_char *, int, u_char **, u_char **);
int		dn_expand(const u_char *, const u_char *, const u_char *, char *, int);

int		res_hnok(const char *);
int		res_ownok(const char *);
int		res_mailok(const char *);
int		res_dnok(const char *);

res_state 	res_get_state(void);
void		res_put_state(res_state);

int	 	res_init(void);
int	 	res_mkquery(int, const char *, int, int, const u_char *, int, const u_char *, u_char *, int);
int	 	res_query(char *, int, int, u_char *, int);
int	 	res_querydomain(char *, char *, int, int, u_char *, int);
int	 	res_search(char *, int, int, u_char *, int);
int	 	res_send(const u_char *, int, u_char *, int);
void    	res_close(void);
void    	res_destroy(void);
__END_DECLS
#endif /* !_RESOLV_H_ */
