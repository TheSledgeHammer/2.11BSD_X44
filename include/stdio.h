/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)stdio.h	5.3 (Berkeley) 3/15/86
 */

#ifndef	_STDIO_H_
#define	_STDIO_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/null.h>

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif
#ifdef	_BSD_SSIZE_T_
typedef	_BSD_SSIZE_T_	ssize_t;
#undef	_BSD_SSIZE_T_
#endif

#if defined(_POSIX_C_SOURCE)
#ifndef __VA_LIST_DECLARED
typedef __va_list va_list;
#define __VA_LIST_DECLARED
#endif
#endif

#ifndef FILE
#define	BUFSIZ	1024
struct	_iobuf {
	int		_cnt;
	char	*_ptr;		/* should be unsigned char */
	char	*_base;		/* ditto */
	int		_bufsiz;
	short	_flag;
	char	_file;		/* should be short */
} _iob[];

#define	_IOREAD		01
#define	_IOWRT		02
#define	_IONBF		04
#define	_IOMYBUF	010
#define	_IOEOF		020
#define	_IOERR		040
#define	_IOSTRG		0100
#define	_IOLBF		0200
#define	_IORW		0400
#define	NULL		0
#define	FILE		struct _iobuf
#define	EOF			(-1)

#define	stdin		(&_iob[0])
#define	stdout		(&_iob[1])
#define	stderr		(&_iob[2])
#ifndef lint
#define	getc(p)		(--(p)->_cnt>=0? (int)(*(unsigned char *)(p)->_ptr++):_filbuf(p))
#endif not lint
#define	getchar()	getc(stdin)
#ifndef lint
#define putc(x, p)	(--(p)->_cnt >= 0 ?						\
	(int)(*(unsigned char *)(p)->_ptr++ = (x)) :			\
	(((p)->_flag & _IOLBF) && -(p)->_cnt < (p)->_bufsiz ?	\
		((*(p)->_ptr = (x)) != '\n' ?						\
			(int)(*(unsigned char *)(p)->_ptr++) :			\
			_flsbuf(*(unsigned char *)(p)->_ptr, p)) :		\
		_flsbuf((unsigned char)(x), p)))
#endif not lint
#define	putchar(x)	putc(x,stdout)
#define	feof(p)		(((p)->_flag&_IOEOF)!=0)
#define	ferror(p)	(((p)->_flag&_IOERR)!=0)
#define	fileno(p)	((p)->_file)
#define	clearerr(p)	((p)->_flag &= ~(_IOERR|_IOEOF))

FILE	*fopen();
FILE	*fdopen();
FILE	*freopen();
FILE	*popen();
long	ftell();
char	*fgets();
char	*gets();
#ifdef vax
char	*sprintf();		/* too painful to do right */
#endif
# endif

