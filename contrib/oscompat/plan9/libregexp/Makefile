#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $

LIB= regexp
MAN= regexp9.3 regexp9.7
VERSION=2.0

.include <bsd.own.mk>

.include "../Makefile.inc"

CPPFLAGS+= -I${PLAN9_INCLUDE}
CPPFLAGS+= -I${PLAN9_LIBREGEXP}
CPPFLAGS+= -I${PLAN9_LIBUTF}
CPPFLAGS+= -I${PLAN9_LIBFMT}

.PATH: ${PLAN9_INCLUDE}
.PATH: ${PLAN9_LIBREGEXP}
.PATH: ${PLAN9_LIBUTF}
.PATH: ${PLAN9_LIBFMT}

CFILES=\
	regcomp.c\
	regerror.c\
	regexec.c\
	regsub.c\
	regaux.c\
	rregexec.c\
	rregsub.c\

HFILES=\
	regexp9.h
	
SRCS= ${CFILES}

INCS+=    ${HFILES}
INCSDIR=  /usr/include

.include <bsd.lib.mk>
