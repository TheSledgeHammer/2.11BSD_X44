#	$FreeBSD$

.include <bsd.own.mk>

PROG=	makefs
SRCS=	ffs.c
SRCS+=  makefs.c walk.c
MAN=	makefs.8

MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree

CPPFLAGS+=	-I${.CURDIR} -I${MKNODSRC} -I${MTREESRC}
.PATH:		${MKNODSRC} ${MTREESRC}

#.include "${.CURDIR}/cd9660/Makefile.inc"
.include "${.CURDIR}/ffs/Makefile.inc"
#.include "${.CURDIR}/msdos/Makefile.inc"

.if !defined(HOSTPROG)
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

.include <bsd.prog.mk>
