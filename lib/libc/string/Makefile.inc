#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1996/3/20
#
# This is the Makefile for the 'string' functions.  New routines ported from 
# 4.4BSD's libc/string directory go here but existing libc/gen files are 
# being left where they are.
#
# string sources
.PATH: ${ARCHDIR}/string ${.CURDIR}/string

SRCS+=	bcmp.c bcopy.c bzero.c explicit_bzero.c ffs.c index.c rindex.c 		\
		stpcpy.c stpncpy.c strcasestr.c strcasecmp.c strcat.c strcmp.c      \
        strcoll.c strcpy.c strcspn.c strdup.c strerror.c strlcat.c strlcpy.c \
        strlen.c strmode.c strncat.c strncmp.c strncpy.c strndup.c strnlen.c \
		strpbrk.c strsep.c strsignal.c strspn.c strstr.c strtok.c strxfrm.c \
		swab.c strchrnul.c

# wide char
SRCS+=	wcscat.c wcschr.c wcscmp.c wcscpy.c wcscspn.c wcslcat.c wcslcpy.c \
		wcslen.c wcsncat.c \
		wcsncmp.c wcsncpy.c wcspbrk.c wcsrchr.c wcsspn.c wcsstr.c wcstok.c \
		wcswcs.c wcswidth.c \
		wmemchr.c wmemcmp.c wmemcpy.c wmemmove.c wmemset.c

.include "${ARCHDIR}/string/Makefile.inc"

MAN+= 	bcmp.3 bcopy.3 bzero.3 ffs.3 rindex.3 strcasecmp.3 strcat.3 		\
		strcmp.3 strcoll.3 strcpy.3 strcspn.3 strdup.3 strerror.3 strlcpy.3 \
		strlen.3 strmode.3 strpbrk.3 strsep.3 strsignal.3 strspn.3 strstr.3 	\
		strtok.3 strxfrm.3 swab.3 wcstok.3 wmemchr.3

MLINKS+=strcasecmp.3 strncasecmp.3
MLINKS+=strcat.3 strncat.3
MLINKS+=strcmp.3 strncmp.3
MLINKS+=strcpy.3 strncpy.3
MLINKS+=strlcpy.3 strlcat.3
MLINKS+=strstr.3 strcasestr.3
MLINKS+=strtok.3 strtok_r.3
MLINKS+=strerror.3 strerror_r.3 strerror.3 perror.3 \
		strerror.3 strerror_l.3 \
		strerror.3 sys_errlist.3 strerror.3 sys_nerr.3
MLINKS+=wmemchr.3 wmemcmp.3 wmemchr.3 wmemcpy.3 \
		wmemchr.3 wmemmove.3 wmemchr.3 wmemset.3 \
		wmemchr.3 wcscat.3 wmemchr.3 wcschr.3 \
		wmemchr.3 wcscmp.3 wmemchr.3 wcscpy.3 \
		wmemchr.3 wcscspn.3 wmemchr.3 wcslcat.3 \
		wmemchr.3 wcslcpy.3 wmemchr.3 wcslen.3 \
		wmemchr.3 wcsncat.3 wmemchr.3 wcsncmp.3 \
		wmemchr.3 wcsncpy.3 wmemchr.3 wcspbrk.3 \
		wmemchr.3 wcsrchr.3 wmemchr.3 wcsspn.3 \
		wmemchr.3 wcsstr.3
