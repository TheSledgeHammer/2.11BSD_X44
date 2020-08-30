#
# Public domain 1997/8/28
#
#	@(#)Makefile	1.1 (2.11BSD) 1997/11/30
#

# compat-43 sources
.PATH: 	${ARCHDIR}/compat-43 ${.CURDIR}/compat-43
SRCS+= 	creat.c gethostid.c sethostid.c sigcompat.c \
		setregid.c setreuid.c setrgid.c setruid.c
