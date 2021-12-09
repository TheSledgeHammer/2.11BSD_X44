/*	$NetBSD: bootparam.h,v 1.3 1998/01/05 19:19:41 perry Exp $	*/

#ifndef LIBSA_BOOTPARAM_H_
#define LIBSA_BOOTPARAM_H_

int bp_whoami(int);
int bp_getfile(int, char *, struct in_addr *, char *);

#endif /* LIBSA_BOOTPARAM_H_ */
