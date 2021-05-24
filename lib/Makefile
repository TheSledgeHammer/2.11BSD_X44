#	$NetBSD: Makefile,v 1.292 2021/04/25 23:43:48 christos Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4.1 (2.11BSD) 1996/1/11
#

# Programs that live in subdirectories, and have makefiles of their own.
# 
# To stay in keeping with 2.11BSD: All libs are in usr.lib
# Excluding csu, libarch, libc, libcompat & libkern

.include <bsd.own.mk>

SUBDIR=		csu .WAIT

SUBDIR+=	libc
SUBDIR+=	.WAIT

#
# The SUBDIRs above are included here for completeness but should be built
# and installed prior to make(dependall) in this file, as libraries listed
# below will depend on versions from DESTDIR only.
#

SUBDIR+=	libarch libcompat

.if !defined(BSD_MK_COMPAT_FILE)
SUBDIR+=	libkern
.endif

SUBDIR+=	../contrib/elftoolchain/lib

.include <bsd.buildinstall.mk>
.include <bsd.subdir.mk>