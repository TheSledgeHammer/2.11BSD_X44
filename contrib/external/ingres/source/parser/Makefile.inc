
.PATH: ${.CURDIR}/ingres/source/parser

LIBS=	$(SCANNER) $(ACCESS) $(GUTIL) $(IUTIL) -lm

SRCS+=	attfcn.c xdot.c main.c rngfcn.c tree.c norml.c call_p.c call_tree.c \
		control.c format.c p_util.c getscr.c
