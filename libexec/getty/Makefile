#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.2 (2.11BSD) 1997/4/28
#

PROG=	getty
SRCS=	get_date.c gettytab.c main.c init.c subr.c
CPPFLAGS+= -DUSE_OLD_TTY
CPPFLAGS+= -D__SELECT_DECLARED

DPADD+=	${LIBUTIL} ${LIBTERMINFO}
LDADD+=	-lutil -lterminfo
#MAN=

.include <bsd.prog.mk>