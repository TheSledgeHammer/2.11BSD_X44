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
 *	@(#)inet.h	5.2.2 (2.11BSD GTE) 12/31/93
 */

#ifndef _INET_H_
#define	_INET_H_

/*
 * External definitions for
 * functions in inet(3N)
 */

#include <sys/cdefs.h>

__BEGIN_DECLS
unsigned long 	inet_addr(const char *);
char			*inet_ntoa(struct in_addr);
struct	in_addr inet_makeaddr(long , long);
unsigned long 	inet_network(const char *);
unsigned long 	inet_netof(struct in_addr);
unsigned long 	inet_lnaof(struct in_addr);
__END_DECLS

#endif /* !_INET_H_ */
