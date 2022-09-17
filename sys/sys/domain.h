/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)domain.h	7.2 (Berkeley) 12/30/87
 */

#ifndef _SYS_DOMAIN_H_
#define	_SYS_DOMAIN_H_

/*
 * Structure per communications domain.
 */
struct	mbuf;

struct	domain {
	int		dom_family;							/* AF_xxx */
	char	*dom_name;
	int		(*dom_init)(void);					/* initialize domain data structures */
	int		(*dom_externalize)(struct mbuf *);	/* externalize access rights */
	int		(*dom_dispose)(struct mbuf *);		/* dispose of internalized rights */
	struct	protosw *dom_protosw, *dom_protoswNPROTOSW;
	struct	domain *dom_next;
	int		(*dom_rtattach)(void **, int);		/* initialize routing table */
	int		dom_rtoffset;						/* an arg to rtattach, in bits */
	int		dom_maxrtkey;						/* for routing layer */
};

#ifdef _KERNEL
extern struct domain *domains;
void 	domaininit(void);
#endif
#endif	/* !_SYS_DOMAIN_H_ */
