#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $

# 
# Notes/Info:
# 
# libutf: 
#	- Plan 9 UTF7 library
# libfmt:
# 	- Plan 9 format library
# libbio:
# 	- Plan 9 buffered io library
# libregexp:
# 	- Plan 9 regular expression library
# mk:
# 	- Plan 9 make program
#

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
