.include <ingres.mk>

.PATH: ${.CURDIR}/ingres/source/scanner

SRCS+=	parser.c yylex.c inout.c name.c number.c operator.c string.c symtab.c \
		comment.c cmap.c
