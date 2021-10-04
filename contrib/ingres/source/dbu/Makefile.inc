.include <ingres.mk>

.PATH: ${.CURDIR}/ingres/source/dbu

LIBS=	$(ACCESS) $(IUTIL) $(GUTIL) -lm

SRCS+=	main.c index.c create.c modify.c modupdate.c destroy.c userdestroy.c 	\
		purgetup.c print.c help.c copy.c save.c rupdate.c update.c convert.c 	\
		secupdate.c rdwrbatch.c resetrel.c get_seq_atts.c dbuproctab.c 			\
		display.c pr_tree.c readtree.c pr_prot.c rmqm.c get_p_tid.c
