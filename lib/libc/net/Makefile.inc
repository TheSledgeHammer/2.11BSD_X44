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
.PATH:	${.CURDIR}/net
SRCS+=	getnbyaddr.c getnbyname.c getnent.c getpent.c getpname.c	\
		getproto.c getsbyname.c getsbyport.c getsent.c herror.c 	\
		rcmd.c res_comp.c res_debug.c res_init.c res_mkquery.c		\
		res_query.c res_send.c	\
		rexec.c ruserpass.c

# sub-directories net sources
.include "${.CURDIR}/net/hosttable/Makefile.inc"
.include "${.CURDIR}/net/named/Makefile.inc"
