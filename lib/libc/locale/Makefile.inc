#	@(#)Makefile.inc	8.1 (Berkeley) 6/4/93

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

SRCS+=	ansi.c ctype.c euc.c frune.c isctype.c lconv.c localeconv.c 	\
		mbrune.c none.c rune.c setlocale.c table.c utf2.c utf8.c

MAN+=	ctype.3 isalnum.3 isalpha.3 isascii.3 isblank.3 iscntrl.3 		\
		isdigit.3 isgraph.3 islower.3 isprint.3 ispunct.3 isspace.3 	\
		isupper.3 isxdigit.3 mbrune.3 multibyte.3 rune.3 setlocale.3 	\
		toascii.3 tolower.3 toupper.3
MAN+=	euc.4 utf2.4
MAN+=	utf8.5
