#	@(#)Makefile	8.1 (Berkeley) 6/4/93
# Notes: 
# In 2.11BSD gethnamadr.c is gethostnamadr.c

.include <bsd.own.mk>

LIBC=	${NETBSDSRCDIR}/lib/libc

LIB=resolv
CPPFLAGS+=-I${LIBC}/include -I${LIBC}/resolv -D_LIBRESOLV

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH: ${LIBC}/net ${LIBC}/net/named ${LIBC}/resolv

SRCS= gethnamadr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
