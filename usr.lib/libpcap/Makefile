#	$NetBSD: Makefile,v 1.27 2005/01/10 02:58:58 lukem Exp $	

.include <bsd.own.mk>

USE_FORT?= yes	# network protocol library

LIB=	pcap
MAN=	pcap.3

WARNS?=	1

CPPFLAGS+=-I. -I${.CURDIR} -I${NETBSDSRCDIR}/sys/net/pf
CPPFLAGS+=-DYYBISON
CPPFLAGS+=-DHAVE_MALLOC_H=1 -DHAVE_SYS_IOCCOM_H=1
CPPFLAGS+=-DHAVE_NETINET_IF_ETHER_H
CPPFLAGS+=-DHAVE_ETHER_HOSTTON=1 -DHAVE_STRERROR=1 -DHAVE_SOCKADDR_SA_LEN=1
CPPFLAGS+=-DHAVE_IFADDRS_H=1 -DHAVE_LIMITS_H=1
CPPFLAGS+=-DHAVE_SNPRINTF=1 -DHAVE_STRLCPY=1 -DHAVE_VSNPRINTF=1
# used in no place
#CPPFLAGS+=-DLBL_ALIGN=1
LPREFIX=pcap_
YPREFIX=pcap_
YHEADER=1

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

SRCS=	scanner.l savefile.c pcap.c pcap-bpf.c optimize.c nametoaddr.c \
inet.c grammar.y gencode.c fad-getad.c etherent.c bpf_image.c \
bpf_dump.c 
SRCS+=	bpf_filter.c version.c

.PATH:	${NETBSDSRCDIR}/sys/net

INCS=	pcap-namedb.h pcap.h
INCSDIR=/usr/include

#.include "Makefile.inc"
# XXX -dcy
scanner.c: grammar.h

.include <bsd.lib.mk>
