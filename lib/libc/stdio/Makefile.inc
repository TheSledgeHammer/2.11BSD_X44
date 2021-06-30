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

SRCS+=	clrerr.c doprnt.c exit.c fclose.c fdopen.c feof.c fflush.c fgetc.c fgetln.c 	\
		fgetpos.c fgets.c fopen.c fprintf.c fpurge.c fputc.c fputs.c fread.c freopen.c fseek.c 	\
		fsetpos.c ftell.c funopen.cfvwrite.c 	\
		fwrite.c getc.c getchar.c gets.c getw.c mktemp.c perror.c printf.c putc.c putchar.c	\
		puts.c putw.c remove.c rew.c rget.c scanf.c setbuf.c setbuffer.c setvbuf.c snprintf.c sprintf.c 		\
		strout.c ungetc.c vfprintf.c vfscanf.c vprintf.c vsnprintf.c vsprintf.c wbuf.c

MAN+=	fgetln.3 fgets.3 fopen.3 fputs.3 fread.3 fseek.3 print.3 mktemp.3 scanf.3 setbuf.3 		\
		ungetc.3

