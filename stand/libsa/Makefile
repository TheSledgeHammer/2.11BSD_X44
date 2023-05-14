#	$211BSD: Makefile,v 1.0 2021/05/25 23:59:27 Exp $

.include <bsd.obj.mk>		# Pull in OBJDIR name rules.
.include <bsd.own.mk>

LIBISPRIVATE?=	yes
LIB=			sa

SA_INCLUDE_NET?= 		YES		# Netboot via TFTP, NFS (No NFS Support)
SA_USE_LOADFILE?= 		YES		# Generic executable loading support

S= ${NETBSDSRCDIR}/sys
CPPFLAGS+= -nostdinc -I${.OBJDIR} -I${S}
CPPFLAGS+= -D_STANDALONE

COPTS+=	-ffreestanding
COPTS+=	-fno-stack-protector
COPTS+=	-fno-unwind-tables
CWARNFLAGS+= -Werror
CWARNFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes -Wpointer-arith

### find out what to use for libsa
SA_AS=	library
SADIR=	${S}/lib/libsa

.include "${SADIR}/Makefile.libsa"

.include <bsd.lib.mk>
