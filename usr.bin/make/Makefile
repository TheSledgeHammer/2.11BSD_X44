#	from: @(#)Makefile	5.2 (Berkeley) 12/28/90
#	$Id: Makefile,v 1.6 1994/06/30 05:33:39 cgd Exp $

PROG=	make
SRCS=	arch.c buf.c compat.c cond.c dir.c for.c hash.c job.c main.c 		\
		make.c parse.c str.c suff.c targ.c var.c
SRCS+=	lstAppend.c lstAtEnd.c lstAtFront.c lstClose.c lstConcat.c 			\
		lstDatum.c lstDeQueue.c lstDestroy.c lstDupl.c lstEnQueue.c 		\
		lstFind.c lstFindFrom.c lstFirst.c lstForEach.c lstForEachFrom.c 	\
		lstInit.c lstInsert.c lstIsAtEnd.c lstIsEmpty.c lstLast.c 			\
		lstMember.c lstNext.c lstOpen.c lstRemove.c lstReplace.c lstSucc.c
.PATH:	${.CURDIR}/lst.lib

.if make(install)
SUBDIR+= PSD.doc
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>

# For -DCLEANUP and similar feature toggles.
CPPFLAGS+=	${USER_CPPFLAGS}
# For overriding -std=gnu99 or similar options.
CFLAGS+=	${USER_CFLAGS}
.if defined(LATE_CC)
CC=			${LATE_CC}
.endif