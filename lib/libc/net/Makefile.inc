#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	5.16 (Berkeley) 3/14/88
#
# machine-independent net sources
.PATH:	${ARCHDIR}/net ${.CURDIR}/net

SRCS+=	getnbyaddr.c getnbyname.c getnent.c getpent.c getpname.c	\
		getproto.c getsbyname.c getsbyport.c getsent.c rcmd.c 		\
		rexec.c ruserpass.c

.if (${MKHESIOD} != "no")
SRCS+=	hesiod.c
.endif

# sub-directories net sources
.include "${.CURDIR}/net/hosttable/Makefile.inc"
.include "${.CURDIR}/net/named/Makefile.inc"

.if (${USE_INET6} != "no")
SRCS+=	ip6opt.c rthdr.c inet6_scopeid.c
.endif

.include "${ARCHDIR}/net/Makefile.inc"
