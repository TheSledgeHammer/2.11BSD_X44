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
#define	MAXDNSRCH			3		/* max # default domain levels to try */
#define	LOCALDOMAINPARTS 	2		/* min levels in name that is "local" */

#define	RES_TIMEOUT			4		/* seconds between retries */

struct __res_state {
	int					retrans;	 			/* retransmition time interval */
	int					retry;					/* number of times to retransmit */
	long				options;				/* option flags - see below. */
	int					nscount;				/* number of name servers */
	struct	sockaddr_in nsaddr_list[MAXNS];		/* address of name server */
#define	nsaddr			nsaddr_list[0]			/* for backward compatibility */
	u_short				id;						/* current packet id */
	char				defdname[MAXDNAME];		/* default domain */
	char				*dnsrch[MAXDNSRCH+1];	/* components of domain to search */
	long				pfcode;					/* RES_PRF_ flags - see below. */
};

typedef struct __res_state *res_state;

/*
 * Resolver options
 */
#define RES_INIT		0x0001		/* address initialized */
#define RES_DEBUG		0x0002		/* print debug messages */
#define RES_AAONLY		0x0004		/* authoritative answers only */
#define RES_USEVC		0x0008		/* use virtual circuit */
#define RES_PRIMARY		0x0010		/* query primary server only */
#define RES_IGNTC		0x0020		/* ignore trucation errors */
#define RES_RECURSE		0x0040		/* recursion desired */
#define RES_DEFNAMES	0x0080		/* use default domain name */
#define RES_STAYOPEN	0x0100		/* Keep TCP socket open */
#define RES_DNSRCH		0x0200		/* search up local domain tree */

#define RES_DEFAULT	(RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)

/*
 * Resolver "pfcode" values.  Used by dig.
 */
#define RES_PRF_STATS	0x0001
/*						0x0002	*/
#define RES_PRF_CLASS   0x0004
#define RES_PRF_CMD		0x0008
#define RES_PRF_QUES	0x0010
#define RES_PRF_ANS		0x0020
#define RES_PRF_AUTH	0x0040
#define RES_PRF_ADD		0x0080
#define RES_PRF_HEAD1	0x0100
#define RES_PRF_HEAD2	0x0200
#define RES_PRF_TTLID	0x0400
#define RES_PRF_HEADX	0x0800
#define RES_PRF_QUERY	0x1000
#define RES_PRF_REPLY	0x2000
#define RES_PRF_INIT    0x4000
/*						0x8000	*/

extern struct __res_state _res;

/* Private routines shared between libc/net, named, nslookup and others. */
#define	__dn_skipname	dn_skipname
#define	__fp_query		fp_query
#define	__hostalias		hostalias
#define	__putlong		putlong
#define	__putshort		putshort
#define __p_query		p_query
#define __p_cdname		p_cdname
#define __p_rr			p_rr
#define __p_class		p_class
#define __p_type		p_type

__BEGIN_DECLS
int	 dn_skipname(const u_char *, const u_char *);
//void fp_resstat(struct state *, FILE *);
void fp_query(char *, FILE *);
char *hostalias(const char *);
void putlong(u_long, u_char *);
void putshort(u_short, u_char *);
void p_query(char *);
char *p_cdname(char *, char *, FILE *);
char *p_rr(char *, char *, FILE *);
char *p_type(int);
char *p_class(int);

int	 dn_comp(const u_char *, u_char *, int, u_char **, u_char **);
int	 dn_expand(const u_char *, const u_char *, const u_char *, u_char *, int);

int	 res_init(void);
int	 res_mkquery(int, const char *, int, int, const char *, int, const char *, char *, int);
int	 res_send(const char *, int, char *, int);
__END_DECLS
#endif /* !_RESOLV_H_ */
