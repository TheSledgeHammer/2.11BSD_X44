#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4 (Berkeley) 9/5/85
#

# machine-independent net sources
.PATH: 	${.CURDIR}/inet

SRCS+=	inet_addr.c inet_cidr_ntop.c inet_cidr_pton.c \
		inet_lnaof.c inet_maddr.c inet_net_ntop.c \
		inet_net_pton.c inet_neta.c inet_ntop.c \
		inet_pton.c inet_netof.c inet_network.c \
		inet_ntoa.c nsap_addr.c

# sub-directories inet sources
.include "${.CURDIR}/inet/iso/Makefile.inc"
.include "${.CURDIR}/inet/ns/Makefile.inc"

MAN+=		inet.3 inet_net.3
MLINKS+=	inet_net.3 inet_net_ntop.3 inet_net.3 inet_net_pton.3
MLINKS+=	inet.3 addr.3 inet.3 inet_addr.3 inet.3 inet_aton.3 			\
			inet.3 inet_lnaof.3 inet.3 inet_makeaddr.3 inet.3 inet_netof.3 	\
			inet.3 inet_network.3 inet.3 inet_ntoa.3 inet.3 network.3 		\
			inet.3 ntoa.3 inet.3 inet_ntop.3 inet.3 inet_pton.3

# IPv6
MAN+=		inet6_option_space.3 inet6_rthdr_space.3
MLINKS+=	inet6_option_space.3 inet6_option_init.3 	\
			inet6_option_space.3 inet6_option_append.3 	\
			inet6_option_space.3 inet6_option_alloc.3 	\
			inet6_option_space.3 inet6_option_next.3 	\
			inet6_option_space.3 inet6_option_find.3 	\
			inet6_rthdr_space.3 inet6_rthdr_init.3 		\
			inet6_rthdr_space.3 inet6_rthdr_add.3 		\
			inet6_rthdr_space.3 inet6_rthdr_lasthop.3 	\
			inet6_rthdr_space.3 inet6_rthdr_reverse.3 	\
			inet6_rthdr_space.3 inet6_rthdr_segments.3 	\
			inet6_rthdr_space.3 inet6_rthdr_getaddr.3 	\
			inet6_rthdr_space.3 inet6_rthdr_getflags.3
