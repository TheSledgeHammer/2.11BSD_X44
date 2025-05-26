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

SRCS+=	ethers.c getifaddrs.c getnbyaddr.c getnbyname.c getnent.c 	\
	getpent_r.c getpname.c getproto.c getsbyname.c getsbyport.c 	\
	getsent_r.c ip6opt.c linkaddr.c rcmd.c recv.c rexec.c 		\
	ruserpass.c send.c sockatmark.c
SRCS+=	nsdispatch.c nslexer.l nsparser.y

.if (${MKHESIOD} != "no")
SRCS+=	hesiod.c
.endif

# sub-directories net sources
#.include "${.CURDIR}/net/hosttable/Makefile.inc"
.include "${.CURDIR}/net/named/Makefile.inc"

SRCS+=	getaddrinfo.c getnameinfo.c
.if (${USE_INET6} != "no")
SRCS+=	ip6opt.c rthdr.c
.endif
SRCS+=	vars6.c
SRCS+= 	if_indextoname.c if_nameindex.c if_nametoindex.c

LPREFIX=_nsyy
YPREFIX=_nsyy
YHEADER=1

CLEANFILES+=nsparser.c nslexer.c nsparser.h

nslexer.c: nslexer.l nsparser.h
	${_MKTARGET_LEX}
	${LEX.l} -t ${.IMPSRC} | sed -e '/YY_BUF_SIZE/s/16384/1024/' >${.TARGET}

.include "${ARCHDIR}/net/Makefile.inc"

MAN+=	ethers.3 gai_strerror.3 getaddrinfo.3 getifaddrs.3 getnameinfo.3 \
		if_indextoname.3 linkaddr.3 nsdispatch.3 sockatmark.3

