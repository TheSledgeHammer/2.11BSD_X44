#
# Copyright (c) 1988 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.4.2 (2.11BSD) 1996/12/1
#

USE_FORT?= yes	# setuid

PROG= 	passwd
#SRCS=	passwd.c
SRCS=	passwd_v2.c
SRCS+=	local_passwd.c pwd_conf.c pwd_gensalt.c
MAN=	passwd.1

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

BINOWN=	root
BINMODE=4555

LDADD+= -lcrypt -lutil
DPADD+= ${LIBCRYPT} ${LIBUTIL}

#.if (${USE_YP} != "no")
#SRCS+=	yp_passwd.c
#CPPFLAGS+=-DUSE_YP
#LDADD+=	-lrpcsvc
#DPADD+=	${LIBRPCSVC}
#LINKS+=	${BINDIR}/passwd ${BINDIR}/yppasswd
#MAN+=	yppasswd.1
#.endif

#.if (${USE_PAM} != "no")
#CPPFLAGS+=-DUSE_PAM
#SRCS+= pam_passwd.c
#LDADD+=-lpam ${PAM_STATIC_LDADD}
#DPADD+=${LIBPAM} ${PAM_STATIC_DPADD}
#.endif

.include <bsd.prog.mk>
