/*	$NetBSD: bootparam.h,v 1.3 1998/01/05 19:19:41 perry Exp $	*/

#ifndef _LIBSA_BOOTPARAM_H_
#define _LIBSA_BOOTPARAM_H_

int bp_whoami(int);
int bp_getfile(int, char *, struct in_addr *, char *);

#endif /* _LIBSA_BOOTPARAM_H_ */
