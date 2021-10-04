.include <ingres.mk>

.PATH: ${.CURDIR}/ingres/source/decomp

LIB40=	$(DBU) $(ACCESS) $(IUTIL) $(GUTIL)

LIB70=	$(OVQP) $(DBU) $(ACCESS) $(IUTIL) $(GUTIL)

SRCS+=	main.c ageval.c aggregate.c byeval.c 									\
		decision.c decomp.c error.c exec_sq.c lockit.c 							\
		makenode.c mapvar.c mklist.c openrs.c printree.c prlinks.c 				\
		pull_const.c pull_sq.c qryproc.c rangetable.c readq.c reduction.c 		\
		reformat.c reinit.c ret_unique.c rnum.c selectv.c setvar.c tempvar.c 	\
		usubr.c writeback.c writenod.c
