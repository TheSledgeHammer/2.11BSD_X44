#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $

LIB= utf
MAN= rune.3 runestrcat.3 utf.7
VERSION=2.0

.include <bsd.own.mk>

.include "../Makefile.inc"

CPPFLAGS+= -I${PLAN9_INCLUDE}
CPPFLAGS+= -I${PLAN9_LIBUTF}

.PATH: ${PLAN9_INCLUDE}
.PATH: ${PLAN9_LIBUTF}

CFILES=\
	rune.c\
	runestrcat.c\
	runestrchr.c\
	runestrcmp.c\
	runestrcpy.c\
	runestrdup.c\
	runestrlen.c\
	runestrecpy.c\
	runestrncat.c\
	runestrncmp.c\
	runestrncpy.c\
	runestrrchr.c\
	runestrstr.c\
	runetype.c\
	utfecpy.c\
	utflen.c\
	utfnlen.c\
	utfrrune.c\
	utfrune.c\
	utfutf.c\

HFILES=\
	lib9.h\
	utf.h
	
SRCS= ${CFILES}

INCS+=    ${HFILES}
INCSDIR=  /usr/include

.include <bsd.lib.mk>
