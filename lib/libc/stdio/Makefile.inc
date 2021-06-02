#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4.1 (2.11BSD) 1997/7/29
#
# Several routines have been rewritten in assembly language for the VAX and
# PDP. If you are not running on a VAX or PDP, you should use the original
# C sources which are included in this directory. Regretably we do not have
# a C version of doprnt().
#

# stdio sources
.PATH: ${.CURDIR}/stdio

CPPFLAGS+=-DWIDE_DOUBLE

SRCS+=	clrerr.c doscan.c dprintf.c exit.c fdopen.c fgetc.c fgetln.c fgets.c 	\
		filbuf.c findiop.c flsbuf.c fopen.c fparseln.c	fprintf.c fpurge.c 		\
		fputc.c fputs.c fread.c freopen.c fseek.c ftell.c fwrite.c 				\
		getchar.c getline.c gets.c gettemp.c getw.c 							\
		printf.c putchar.c putw.c rew.c scanf.c setbuf.c setbuffer.c setvbuf.c 	\
		snprintf.c sprintf.c strout.c ungetc.c vfprintf.c vprintf.c vsprintf.c
