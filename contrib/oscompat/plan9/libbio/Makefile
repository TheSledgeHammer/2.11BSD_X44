#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $

LIB= bio
MAN= bio.3
VERSION=2.0

.include <bsd.own.mk>

.include "../Makefile.inc"

CPPFLAGS+= -I${PLAN9_INCLUDE}
CPPFLAGS+= -I${PLAN9_LIBBIO}
CPPFLAGS+= -I${PLAN9_LIBUTF}
CPPFLAGS+= -I${PLAN9_LIBFMT}

.PATH: ${PLAN9_INCLUDE}
.PATH: ${PLAN9_LIBBIO}
.PATH: ${PLAN9_LIBUTF}
.PATH: ${PLAN9_LIBFMT}

CFILES=\
	bbuffered.c\
	bfildes.c\
	bflush.c\
	bgetc.c\
	bgetd.c\
	bgetrune.c\
	binit.c\
	boffset.c\
	bprint.c\
	bvprint.c\
	bputc.c\
	bputrune.c\
	brdline.c\
	brdstr.c\
	bread.c\
	bseek.c\
	bwrite.c\

SRCS= ${CFILES}

INCS+=    bio.h
INCSDIR=  /usr/include

.include <bsd.lib.mk>
