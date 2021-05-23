#	$NetBSD: Makefile.inc,v 1.10 1996/04/13 01:17:41 cgd Exp $

SRCS+=	__main.c __assert.c 										\
		imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c 	\
		ffs.S htonl.S htons.S locc.S ntohl.S ntohs.S				\
		scanc.S setjmp.S skpc.S strcat.S strcmp.S strcpy.S strlen.S \
		strcasecmp.c strncmp.c strncpy.c strtoul.c
