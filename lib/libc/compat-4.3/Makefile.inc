#
# Public domain 1997/8/28
#
#	@(#)Makefile	1.1 (2.11BSD) 1997/11/30
#

# compat-4.3 sources
.PATH: 	${ARCHDIR}/compat-4.3 ${.CURDIR}/compat-4.3
SRCS+= 	creat.c gethostid.c sethostid.c sigcompat.c \
		setregid.c setreuid.c setrgid.c setruid.c

MAN+= 	creat.3 setregid.3 setreuid.3 setruid.3 memset.3
