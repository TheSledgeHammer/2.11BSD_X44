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
.PATH: ${.CURDIR}/resolv

SRCS+=	herror.c res_comp.c res_debug.c res_init.c res_mkquery.c		\
		res_query.c res_send.c											\

COPTS.res_query.c+=	${CC_WNO_STRINGOP_OVERFLOW}
