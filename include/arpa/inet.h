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

#ifndef _ARPA_INET_H_
#define	_ARPA_INET_H_

/*
 * External definitions for
 * functions in inet(3N)
 */

#include <sys/ansi.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <netinet/in.h>
#if (_POSIX_C_SOURCE - 0) >= 200112L || (_XOPEN_SOURCE - 0) >= 520 || \
    defined(__BSD_VISIBLE)
#ifndef	_SOCKLEN_T_DEFINED_
#define	_SOCKLEN_T_DEFINED_
typedef __socklen_t	socklen_t;
#define socklen_t	__socklen_t
#endif
#endif /* _POSIX_C_SOURCE >= 200112 || XOPEN_SOURCE >= 520 || __BSD_VISIBLE */

__BEGIN_DECLS
unsigned long 	inet_addr(const char *);
char			*inet_ntoa(struct in_addr);
struct in_addr inet_makeaddr(long , long);
unsigned long 	inet_network(const char *);
unsigned long 	inet_netof(struct in_addr);
unsigned long 	inet_lnaof(struct in_addr);
const char		*inet_ntop(int, const void * __restrict, char * __restrict, socklen_t);
int		 		inet_pton(int, const char * __restrict, void * __restrict);

#if __BSD_VISIBLE
int		 		inet_aton(const char *, struct in_addr *);
char 			*inet_neta(u_long, char *, size_t);
char			*inet_net_ntop(int, const void *, int, char *, size_t);
int		 		inet_net_pton(int, const char *, void *, size_t);
char			*inet_cidr_ntop(int, const void *, int, char *, size_t);
int		 		inet_cidr_pton(int, const char *, void *, int *);
u_int			inet_nsap_addr(const char *, u_char *, int);
char			*inet_nsap_ntoa(int, const u_char *, char *);
#endif
__END_DECLS

#endif /* !_ARPA_INET_H_ */
