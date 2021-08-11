
.PATH: ${.CURDIR}/ingres/source/monitor

LIBS= 	$(IUTIL) $(GUTIL)

SRCS+=	branch.c buf.c clear.c clrline.c edit.c eval.c expr.c getch.c 		\
		getfilename.c go.c include.c mac.c main.c mcall.c monitor.c			\
		newdirec.c print.c prompt.c quit.c shell.c trapquery.c writeout.c 	\
		xwait.c
