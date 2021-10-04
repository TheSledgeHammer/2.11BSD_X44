.include <ingres.mk>

.PATH: ${.CURDIR}/ingres/source/equel

.include "${.CURDIR}/ingres/source/equel/runtime/Makefile.inc"

LIBS=	$(IUTIL) $(GUTIL) -lm

SRCS+=	main.c parser.c yylex.c cvar.c include.c name.c getkey.c number.c 	\
		operator.c prtout.c string.c comment.c retrieve.c display.c getch.c \
		cmap.c yyerror.c nalloc.c

