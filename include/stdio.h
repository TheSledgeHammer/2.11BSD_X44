/*	$NetBSD: stdio.h,v 1.99 2020/03/20 01:08:42 joerg Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stdio.h	8.5 (Berkeley) 4/29/95
 */
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
#include <sys/ansi.h>

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#include <sys/null.h>
/*
 * This is fairly grotesque, but pure ANSI code must not inspect the
 * innards of an fpos_t anyway.  The library internally uses off_t,
 * which we assume is exactly as big as eight chars.
 */
#if (!defined(_ANSI_SOURCE) && !defined(__STRICT_ANSI__)) || defined(_LIBC)
typedef __off_t fpos_t;
#else
typedef struct __sfpos {
	__off_t 		_pos;
	__mbstate_t 	_mbstate_in, _mbstate_out;
} fpos_t;
#endif

#define	_FSTDIO			/* Define for new stdio with functions. */

/*
 * NB: to fit things in six character monocase externals, the stdio
 * code uses the prefix `__s' for stdio objects, typically followed
 * by a three-character attempt at a mnemonic.
 */

/* stdio buffers */
struct __sbuf {
	unsigned char 	*_base;
	int				_size;
};

/*
 * stdio state variables.
 *
 * The following always hold:
 *
 *	if (_flags&(__SLBF|__SWR)) == (__SLBF|__SWR),
 *		_lbfsize is -_bf._size, else _lbfsize is 0
 *	if _flags&__SRD, _w is 0
 *	if _flags&__SWR, _r is 0
 *
 * This ensures that the getc and putc macros (or inline functions) never
 * try to write or read from a file that is in `read' or `write' mode.
 * (Moreover, they can, and do, automatically switch from read mode to
 * write mode, and back, on "r+" and "w+" files.)
 *
 * _lbfsize is used only to make the inline line-buffered output stream
 * code as compact as possible.
 *
 * _ub, _up, and _ur are used when ungetc() pushes back more characters
 * than fit in the current _bf, or when ungetc() pushes back a character
 * that does not match the previous one in _bf.  When this happens,
 * _ub._base becomes non-nil (i.e., a stream has ungetc() data iff
 * _ub._base!=NULL) and _up and _ur save the current values of _p and _r.
 *
 * NB: see WARNING above before changing the layout of this structure!
 */

struct _siobuf {
	unsigned char 	*_p;		/* current position in (some) buffer */

	int				_r;			/* read space left for getc() */
	int				_w;			/* write space left for putc() */
	unsigned short 	_flags;		/* flags, below; this FILE is free if 0 */
	short			_file;		/* fileno, if Unix descriptor, else -1 */
	struct	__sbuf 	_bf;		/* the buffer (at least 1 byte, if !NULL) */
	int				_lbfsize;	/* 0 or -_bf._size, for inline putc */

	/* operations */
	void			*_cookie;	/* cookie passed to io functions */
	int				(*_close)(void *);
	int				(*_read)(void *, char *, int);
	fpos_t			(*_seek)(void *, fpos_t, int);
	int				(*_write)(void *, const char *, int);

	/* file extension */
	struct	__sbuf 	_ext;

	/* separate buffer for long sequences of ungetc() */
	struct	__sbuf 	_ub;		/* ungetc buffer */
	unsigned char 	*_up;		/* saved _p when _p is doing ungetc data */
	int				_ur;		/* saved _r when _r is counting ungetc data */

	/* tricks to meet minimum requirements even when malloc() fails */
	unsigned char 	_ubuf[3];	/* guarantee an ungetc() buffer */
	unsigned char 	_nbuf[1];	/* guarantee a getc() buffer */

	int				(*_flush)(void *);
	/* separate buffer for fgetln() when line crosses buffer boundary */
	struct	__sbuf 	_lb;		/* buffer for fgetln() */

	/* Unix stdio files get aligned to block boundaries on fseek() */
	int				_blksize;	/* stat.st_blksize (may be != _bf._size) */
	__off_t			_offset;	/* current lseek offset */
};
typedef struct _siobuf FILE;

__BEGIN_DECLS
extern FILE __iob[3];
extern FILE __sF[3];
__END_DECLS

#define	__SLBF		0x0001		/* line buffered */
#define	__SNBF		0x0002		/* unbuffered */
#define	__SRD		0x0004		/* OK to read */
#define	__SWR		0x0008		/* OK to write */
	/* RD and WR are never simultaneously asserted */
#define	__SRW		0x0010		/* open for reading & writing */
#define	__SEOF		0x0020		/* found EOF */
#define	__SERR		0x0040		/* found error */
#define	__SMBF		0x0080		/* _buf is from malloc */
#define	__SAPP		0x0100		/* fdopen()ed in append mode */
#define	__SSTR		0x0200		/* this is an sprintf/snprintf string */
#define	__SOPT		0x0400		/* do fseek() optimization */
#define	__SNPT		0x0800		/* do not do fseek() optimization */
#define	__SOFF		0x1000		/* set iff _offset is in fact correct */
#define	__SMOD		0x2000		/* true => fgetln modified _p text */
#define	__SALC		0x4000		/* allocate string space dynamically */

/*
 * The following three definitions are for ANSI C, which took them
 * from System V, which brilliantly took internal interface macros and
 * made them official arguments to setvbuf(), without renaming them.
 * Hence, these ugly _IOxxx names are *supposed* to appear in user code.
 *
 * Although numbered as their counterparts above, the implementation
 * does not rely on this.
 */
#define	_IOFBF		0			/* setvbuf should set fully buffered */
#define	_IOREAD		1
#define	_IOWRT		2
#define	_IONBF		4			/* setvbuf should set unbuffered */
#define	_IOMYBUF	10
#define	_IOEOF		20
#define	_IOERR		40
#define	_IOSTRG		100
#define	_IOLBF		200			/* setvbuf should set line buffered */
#define	_IORW		400

#define	BUFSIZ		1024		/* size of buffer used by setbuf */
#define	EOF			(-1)

/*
 * FOPEN_MAX is a minimum maximum, and is the number of streams that
 * stdio can provide without attempting to allocate further resources
 * (which could fail).  Do not use this for anything.
 */
								/* must be == _POSIX_STREAM_MAX <limits.h> */
#define	FOPEN_MAX		20		/* must be <= OPEN_MAX <sys/syslimits.h> */
#define	FILENAME_MAX	1024	/* must be <= PATH_MAX <sys/syslimits.h> */

/* System V/ANSI C; this is the wrong way to do this, do *not* use these. */
#if __BSD_VISIBLE || __XPG_VISIBLE
#define	P_tmpdir	"/var/tmp/"
#endif
#define	L_tmpnam	1024	/* XXX must be == PATH_MAX */
/* Always ensure that this is consistent with <limits.h> */
#ifndef TMP_MAX
#define TMP_MAX		308915776	/* Legacy */
#endif

/* Always ensure that these are consistent with <fcntl.h> and <unistd.h>! */
#ifndef SEEK_SET
#define	SEEK_SET	0		/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1		/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2		/* set file offset to EOF plus offset */
#endif

#define	stdin		(&_iob[0])
#define	stdout		(&_iob[1])
#define	stderr		(&_iob[2])


#define	FPARSELN_UNESCESC	0x01
#define	FPARSELN_UNESCCONT	0x02
#define	FPARSELN_UNESCCOMM	0x04
#define	FPARSELN_UNESCREST	0x08
#define	FPARSELN_UNESCALL	0x0f

/*
 * Functions defined in ANSI C standard.
 */
__BEGIN_DECLS
void 	clearerr (FILE *);
int	 	fclose (FILE *);
int	 	feof (FILE *);
int	 	ferror (FILE *);
int	 	fflush (FILE *);
int	 	fgetc (FILE *);
int	    fgetpos(FILE * __restrict, fpos_t * __restrict);
char	*fgets(char * __restrict, int, FILE * __restrict);
FILE	*fopen(const char * __restrict , const char * __restrict);
int	    fprintf(FILE * __restrict, const char * __restrict, ...);
int	 	fputc (int, FILE *);
int	    fputs(const char * __restrict, FILE * __restrict);
size_t	fread(void * __restrict, size_t, size_t, FILE * __restrict);
FILE	*freopen(const char * __restrict, const char * __restrict, FILE * __restrict);
int	    fscanf(FILE * __restrict, const char * __restrict, ...);
int	 	fseek (FILE *, long, int);
int	 	fsetpos (FILE *, const fpos_t *);
long 	ftell (FILE *);
size_t	fwrite (const void * __restrict, size_t, size_t, FILE * __restrict);
int	 	getc (FILE *);
int	 	getchar (void);
char 	*gets (char *);
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
extern int sys_nerr;			/* perror(3) external variables */
extern __const char *__const sys_errlist[];
#endif
void 	perror (const char *);
int	 	printf (const char * __restrict, ...);
int	 	putc (int, FILE *);
int	 	putchar (int);
int	 	puts (const char *);
int	 	remove (const char *);
int	 	rename  (const char *, const char *);
void 	rewind (FILE *);
int	 	scanf (const char * __restrict, ...);
void	setbuf(FILE * __restrict, char * __restrict);
int	    setvbuf(FILE * __restrict, char * __restrict, int, size_t);
int	    sscanf(const char * __restrict, const char * __restrict, ...);
int	    sprintf(char * __restrict, const char * __restrict, ...);
FILE 	*tmpfile (void);
char 	*tmpnam (char *);
int	 	ungetc (int, FILE *);
int	    vfprintf(FILE * __restrict, const char * __restrict, __va_list);
int	    vprintf(const char * __restrict, __va_list);
int	    vsprintf(char * __restrict, const char * __restrict, __va_list);
__END_DECLS

/*
 * Functions defined in POSIX 1003.1.
 */
#ifndef _ANSI_SOURCE
#define	L_cuserid	9		/* size for cuserid(); UT_NAMESIZE + 1 */
#define	L_ctermid	1024	/* size for ctermid(); PATH_MAX */

__BEGIN_DECLS
char	*ctermid (char *);
FILE	*fdopen (int, const char *);
int	 	fileno (FILE *);

/*
 * These are normally used through macros as defined below, but POSIX
 * requires functions as well.
 */
int	 	getc_unlocked(FILE *);
int	 	getchar_unlocked(void);
int	 	putc_unlocked(int, FILE *);
int	 	putchar_unlocked(int);
__END_DECLS
#endif /* not ANSI */

/*
 * Routines that are purely local.
 */
#if !defined (_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
__BEGIN_DECLS
int	    asprintf(char ** __restrict, const char * __restrict, ...);
char	*fgetln(FILE * __restrict, size_t * __restrict);
char	*fparseln(FILE *, size_t *, size_t *, const char[3], int);
int	 	fpurge (FILE *);
int	 	getw (FILE *);
int	 	pclose (FILE *);
FILE	*popen (const char *, const char *);
int	 	putw (int, FILE *);
void	setbuffer (FILE *, char *, int);
int	 	setlinebuf (FILE *);
char	*tempnam (const char *, const char *);
int	    snprintf(char * __restrict, size_t, const char * __restrict, ...);
int	    vsnprintf(char * __restrict, size_t, const char * __restrict, __va_list);
int	    vasprintf(char ** __restrict, const char * __restrict, __va_list);
int	    vscanf(const char * __restrict, __va_list);
int	    vsscanf(const char * __restrict, const char * __restrict, __va_list);

ssize_t	 getdelim(char ** __restrict, size_t * __restrict, int, FILE * __restrict);
ssize_t	 getline(char ** __restrict, size_t * __restrict, FILE * __restrict);
__END_DECLS

/*
 * This is a #define because the function is used internally and
 * (unlike vfscanf) the name __svfscanf is guaranteed not to collide
 * with a user function when _ANSI_SOURCE or _POSIX_SOURCE is defined.
 */
#define	 vfscanf	__svfscanf

/*
 * Stdio function-access interface.
 */
__BEGIN_DECLS
FILE		*funopen(const void *, int (*)(void *, char *, int), int (*)(void *, const char *, int), fpos_t (*)(void *, fpos_t, int), int (*)(void *));
__END_DECLS
#define		fropen(cookie, fn) funopen(cookie, fn, 0, 0, 0)
#define		fwopen(cookie, fn) funopen(cookie, 0, fn, 0, 0)
#endif /* !_ANSI_SOURCE && !_POSIX_SOURCE */

/*
 * Functions internal to the implementation.
 */
__BEGIN_DECLS
int		__srget(FILE *);
int		__svfscanf(FILE *, const char *, __va_list);
int		__swbuf(int, FILE *);

/* 2.11BSD Compatibility */
__inline int _doscan(FILE * iop, char *fmt, __va_list argp) {
	return (__svfscanf(iop, fmt, argp));
}
__END_DECLS

/*
 * The __sfoo macros are here so that we can
 * define function versions in the C library.
 */
#define	__sgetc(p) (--(p)->_r < 0 ? __srget(p) : (int)(*(p)->_p++))
static __inline int __sputc(int _c, FILE *_p) {
	if (--_p->_w >= 0 || (_p->_w >= _p->_lbfsize && (char)_c != '\n'))
		return (*_p->_p++ = _c);
	else
		return (__swbuf(_c, _p));
}

#define	__sfeof(p)		(((p)->_flags & __SEOF) != 0)
#define	__sferror(p)	(((p)->_flags & __SERR) != 0)
#define	__sclearerr(p)	((void)((p)->_flags &= (unsigned short)~(__SERR|__SEOF)))
#define	__sfileno(p)	((p)->_file == -1 ? -1 : (int)(unsigned short)(p)->_file)

#define	feof(p)			__sfeof(p)
#define	ferror(p)		__sferror(p)
#define	clearerr(p)		__sclearerr(p)

#ifndef _ANSI_SOURCE
#define	fileno(p)		__sfileno(p)
#endif

#ifndef lint
#define	getc(fp)		__sgetc(fp)
#define putc(x, fp)		__sputc(x, fp)
#endif /* lint */

#if __POSIX_VISIBLE >= 199506
#define	getc_unlocked(fp)	__sgetc(fp)
/*
 * The macro implementations of putc and putc_unlocked are not
 * fully POSIX compliant; they do not set errno on failure
 */
#if __BSD_VISIBLE
#define putc_unlocked(x, fp)	__sputc(x, fp)
#endif /* __BSD_VISIBLE */
#endif /* __POSIX_VISIBLE >= 199506 */

#define	getchar()			getc(stdin)
#define	putchar(x)			putc(x, stdout)
#define getchar_unlocked()	getc_unlocked(stdin)
#define putchar_unlocked(c)	putc_unlocked(c, stdout)

#endif /* _STDIO_H_ */
