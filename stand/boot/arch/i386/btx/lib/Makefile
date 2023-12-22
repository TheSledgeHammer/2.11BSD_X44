# $FreeBSD$

.include <bsd.init.mk>
.include "${.CURDIR}/../Makefile.inc"

PROG=	crt0.o
INTERNALPROG=
SRCS=	btxcsu.S btxsys.S btxv86.S
CFLAGS+=-I${.CURDIR}/../../common
LDFLAGS+=-Wl,-r

.include <bsd.prog.mk>