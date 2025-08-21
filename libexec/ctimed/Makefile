#
# Public Domain.  1996/6/25 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1996/6/25
#

PROG= 	ctimed
SRCS=	ctimed.c

BINDIR=	/usr/libexec
BINOWN=		root
BINMODE=	755

STUBS_DIR:= ${NETBSDSRCDIR}/contrib/oscompat/211bsd/libstubs
.PATH: ${STUBS_DIR}

CPPFLAGS+= -I${STUBS_DIR}

DPADD+=	${LIBSTUBS}
LDADD+= -lstubs

.include <bsd.prog.mk>
