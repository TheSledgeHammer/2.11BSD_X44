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

SRCS+=	asprintf.c clrerr.c doprnt.c doscan.c dprintf.c fclose.c fdopen.c feof.c ferror.c fflush.c 	\
	fgetc.c fgetln.c fgetpos.c fgets.c fgetwc.c fileno.c findfp.c fopen.c fparseln.c flags.c flockfile.c     \
        fprintf.c fpurge.c fputc.c fputs.c fputwc.c fread.c freopen.c fseek.c fsetpos.c ftell.c funopen.c    	\
        fwalk.c fwide.c fvwrite.c fwrite.c getc.c getchar.c getdelim.c getline.c gets.c gettemp.c getw.c    \
        makebuf.c mktemp.c open_memstream.c open_wmemstream.c perror.c printf.c putc.c putchar.c puts.c \
	putw.c remove.c refill.c rew.c rget.c scanf.c setbuf.c setbuffer.c setvbuf.c snprintf.c 	\
	sprintf.c stdio.c strout.c ungetc.c ungetwc.c vasprintf.c vdprintf.c vfprintf.c vfscanf.c vprintf.c 	\
	vsnprintf.c vsprintf.c wbuf.c wsetup.c

MAN+=	fclose.3 ferror.3 fgetln.3 fgets.3 flockfile.3 fopen.3 fparseln.3 fputs.3 fread.3 fseek.3 	\
	funopen.3 getc.3 getwc.3 getdelim.3 mktemp.3 printf.3 putwc.3 scanf.3 setbuf.3 stdio.3 ungetc.3 \
	ungetwc.3

MLINKS+=putwc.3 fputwc.3 putwc.3 putwchar.3
MLINKS+=getwc.3 fgetwc.3 getwc.3 getwchar.3
