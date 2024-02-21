/*	$NetBSD: installboot.h,v 1.4 1998/07/28 20:10:54 drochner Exp $	*/

#ifndef _INSTALLBOOT_INSTALLBOOT_H_
#define _INSTALLBOOT_INSTALLBOOT_H_

unsigned long createfileondev(char *, char *, char *, unsigned int);
void cleanupfileondev(char *, char *, int);

char *getmountpoint(char *);
void cleanupmount(char *);

extern int verbose;

#endif /* _INSTALLBOOT_INSTALLBOOT_H_ */
