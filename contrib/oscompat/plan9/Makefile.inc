#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $

.include <bsd.own.mk>

PLAN9DIR=        ${NETBSDSRCDIR}/contrib/oscompat/plan9

PLAN9_INCLUDE=   ${PLAN9DIR}/include

PLAN9_LIBUTF=    ${PLAN9DIR}/libutf
PLAN9_LIBFMT=    ${PLAN9DIR}/libfmt
PLAN9_LIBBIO=    ${PLAN9DIR}/libbio
PLAN9_LIBREGEXP= ${PLAN9DIR}/libregexp
PLAN9_MK=        ${PLAN9DIR}/mk

DPLIBS+= regexp  ${PLAN9_LIBREGEXP}
DPLIBS+= bio     ${PLAN9_LIBBIO}
DPLIBS+= fmt     ${PLAN9_LIBFMT}
DPLIBS+= utf     ${PLAN9_LIBUTF}

#CPPFLAGS+=      -I${PLAN9_INCLUDE}
#.PATH:          ${PLAN9_INCLUDE}

#INCS+=          bio.h fmt.h lib9.h regexp9.h utf.h
#INCSDIR=        /usr/include
