#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4 (Berkeley) 9/5/85
#

# machine-independent net sources
.PATH: 	${.CURDIR}/inet
SRCS+=	inet_addr.c inet_network.c \
		inet_netof.c inet_ntoa.c inet_lnaof.c \
		inet_maddr.c