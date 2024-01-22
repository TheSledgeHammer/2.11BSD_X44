#	$NetBSD: Makefile,v 1.292 2021/04/25 23:43:48 christos Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4.1 (2.11BSD) 1996/1/11
#

.include <bsd.own.mk>

SUBDIR= 	libbz2 libcrypt libcurses libdbm libedit libexecinfo libform libkvm 		\
		    libm libmenu libmp libom libpanel libpcap libpci libpthread             \
            libpthread_dbg libresolv libstubs libtelnet libtermcap libutil libvmf     \
            liby libz
                        
# IPv6/IPsec
SUBDIR+=	libipsec					

# Fortran: F77, U77 and I77
SUBDIR+=    libfortran

.if (${MKSKEY} != "no")
SUBDIR+=	libskey
.endif

.include <bsd.subdir.mk>
