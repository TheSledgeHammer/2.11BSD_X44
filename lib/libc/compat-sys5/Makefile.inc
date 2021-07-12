#	Makefile	1.6	1996/1/11
#

# compat-sys5 sources
.PATH: 	${ARCHDIR}/compat-sys5 ${.CURDIR}/compat-sys5
SRCS+=	memccpy.c memchr.c memcmp.c memcpy.c memset.c strchr.c strrchr.c tmpnam.c

MAN+= 	memccpy.3 memchr.3 memcmp.3 memcpy.3 memset.3 strchr.3 strrchr.3 tmpnam.3

