#	$NetBSD: Makefile,v 1.47 2018/04/01 04:35:03 ryo Exp $

# For now, we install the machine and arch includes, and symlink 'machine'
# to the location of the machine includes (usually).
#
# Eventually, we should install everything.

.include <bsd.own.mk>

ARCHSUBDIR= ${MACHINE_CPU}

SUBDIR=	${MACHINE}

.if ${MACHINE} != ${ARCHSUBDIR}
.if exists(${ARCHSUBDIR})
SUBDIR+= ${ARCHSUBDIR}
.endif
.endif

INCSYMLINKS= ${MACHINE} /usr/include/machine

#INCSYMLINKS+= machine/float.h /usr/include/float.h

.include <bsd.kinc.mk>