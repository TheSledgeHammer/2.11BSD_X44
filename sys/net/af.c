/*
 * Copyright (c) 1983, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)af.c	7.3 (Berkeley) 12/30/87
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/af.h>

/*
 * Address family support routines
 */
int	null_hash(struct sockaddr *, struct afhash *);
int null_netmatch(struct sockaddr *, struct sockaddr *);

#define	AFNULL 		\
	{ null_hash, null_netmatch }

#ifdef INET
extern int inet_hash(), inet_netmatch();
#define	AFINET 			\
	{ inet_hash,	inet_netmatch }
#else
#define	AFINET	AFNULL
#endif
#ifdef NS
extern int ns_hash(), ns_netmatch();
#define	AFNS	 \
	{ ns_hash,	ns_netmatch }
#else
#define	AFNS	AFNULL
#endif

struct afswitch afswitch[AF_MAX] = {
	AFNULL,	AFNULL,	AFINET,	AFINET,	AFNULL,
	AFNULL,	AFNS,	AFNULL,	AFNULL,	AFNULL,
	AFNULL, AFNULL, AFNULL, AFNULL, AFNULL,
	AFNULL, AFNULL,					/* through 16 */
};

void
null_init(void)
{
	register struct afswitch *af;

	for (af = afswitch; af < &afswitch[AF_MAX]; af++) {
		if (af->af_hash == (int (*)())NULL) {
			af->af_hash = null_hash;
			af->af_netmatch = null_netmatch;
		}
	}
}

/*ARGSUSED*/
int
null_hash(addr, hp)
	struct sockaddr *addr;
	struct afhash *hp;
{
	hp->afh_nethash = hp->afh_hosthash = 0;

	return (0);
}

/*ARGSUSED*/
null_netmatch(a1, a2)
	struct sockaddr *a1, *a2;
{
	return (0);
}
