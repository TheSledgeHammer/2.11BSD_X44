#	$NetBSD: Makefile,v 1.85 2019/10/13 07:28:11 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
# @(#)Makefile	5.4.1 (2.11BSD GTE) 1995/04/13
#

USE_SHLIBDIR=	yes

.include <bsd.own.mk>

WARNS?=	5
LIB=	util
CPPFLAGS+=-DLIBC_SCCS -I${.CURDIR}
LINTFLAGS+=-w
SRCS+=	dkcksum.c login.c ogin_tty.c logout.c logwtmp.c	\
		pty.c

MAN=	disklabel_dkcksum.3 login.3

COPTS.logwtmp.c+=	${GCC_NO_STRINGOP_TRUNCATION}

YPREFIX=__pd
.PATH:	${NETBSDSRCDIR}/lib/libc/gen

MLINKS+=login.3 logout.3
MLINKS+=login.3 logwtmp.3

.include <bsd.lib.mk>