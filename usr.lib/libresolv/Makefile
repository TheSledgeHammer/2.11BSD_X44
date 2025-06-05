#	@(#)Makefile	8.1 (Berkeley) 6/4/93
# Notes: 
# In 2.11BSD gethnamadr.c is gethostnamadr.c

.include <bsd.own.mk>

LIBC=	    ${NETBSDSRCDIR}/lib/libc
INCLUDE=    ${LIBC}/include
NET=        ${LIBC}/net
NAMED=      ${NET}/named
RESOLV=     ${LIBC}/resolv

LIB=resolv
CPPFLAGS+=-D_LIBRESOLV
CPPFLAGS+=-I${LIBC}
CPPFLAGS+=-I${INCLUDE}
CPPFLAGS+=-I${RESOLV}
CPPFLAGS+=-I${NET}
CPPFLAGS+=-I${NAMED}

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.PATH: ${RESOLV} ${NET} ${NAMED}

SRCS= gethnamadr.c res_mkquery.c res_query.c res_send.c

.include <bsd.lib.mk>
