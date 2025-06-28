#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $

LIB= fmt
MAN= fmtinstall.3 print.3
VERSION=2.0

.include <bsd.own.mk>

.include "../Makefile.inc"

CPPFLAGS+= -I${PLAN9_INCLUDE}
CPPFLAGS+= -I${PLAN9_LIBUTF}
CPPFLAGS+= -I${PLAN9_LIBFMT}

.PATH: ${PLAN9_INCLUDE}
.PATH: ${PLAN9_LIBUTF}
.PATH: ${PLAN9_LIBFMT}

NAN=\
    	nan64.c

NUM=\
	charstod.c\
	pow10.c\

CFILES=\
	dofmt.c\
	dorfmt.c\
	errfmt.c\
	fltfmt.c\
	fmt.c\
	fmtfd.c\
	fmtfdflush.c\
	fmtlocale.c\
	fmtlock.c\
	fmtnull.c\
	fmtprint.c\
	fmtquote.c\
	fmtrune.c\
	fmtstr.c\
	fmtvprint.c\
	fprint.c\
	print.c\
	runefmtstr.c\
	runeseprint.c\
	runesmprint.c\
	runesnprint.c\
	runesprint.c\
	runevseprint.c\
	runevsmprint.c\
	runevsnprint.c\
	seprint.c\
	smprint.c\
	snprint.c\
	sprint.c\
	strtod.c\
	vfprint.c\
	vseprint.c\
	vsmprint.c\
	vsnprint.c\
	
HFILES=\
	fmt.h

SRCS= ${CFILES} ${NUM} ${NAN}

INCS+=    ${HFILES}
INCSDIR=  /usr/include

.include <bsd.lib.mk>
