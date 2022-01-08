/*	$NetBSD: libkern.h,v 1.50 2003/08/13 11:34:24 ragge Exp $	*/
/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)libkern.h	8.1 (Berkeley) 6/10/93
 * 	$Id: libkern.h,v 1.3 1994/08/30 18:19:47 davidg Exp $
 */
#include <sys/types.h>
#include <sys/stddef.h>
#include <sys/inttypes.h>
#include <sys/null.h>

/* BCD conversions. */
extern u_char const	bcd2bin_data[];
extern u_char const	bin2bcd_data[];
extern char const	hex2ascii_data[];

#define	bcd2bin(bcd)	(bcd2bin_data[bcd])
#define	bin2bcd(bin)	(bin2bcd_data[bin])
#define	hex2ascii(hex)	(hex2ascii_data[hex])

int 	imax(int, int);
int 	imin(int, int);
u_int 	max(u_int, u_int);
u_int 	min(u_int, u_int);
long 	lmax(long, long);
long 	lmin(long, long);
u_long 	ulmax(u_long, u_long);
u_long 	ulmin(u_long, u_long);
int 	abs(int);

int 	isspace(int);
int 	isascii(int);
int 	isupper(int);
int 	islower(int);
int 	isalpha(int);
int 	isdigit(int);
int 	isxdigit(int);
int 	toupper(int);
int 	tolower(int);

/* hash Functions */
uint32_t 	prospector32(uint32_t);
uint32_t 	lowbias32(uint32_t);
uint32_t 	lowbias32_r(uint32_t);
uint32_t 	triple32(uint32_t);
uint32_t 	triple32_r(uint32_t);
uint32_t 	hash32(uint32_t);
uint32_t 	murmurhash32_mix32(uint32_t);
uint32_t 	murmur3_32_hash(const void *, size_t, uint32_t);
uint32_t 	murmur3_32_hash32(const uint32_t *, size_t, uint32_t);

/* Prototypes for non-quad routines. */
/* XXX notyet #ifdef _STANDALONE */
int	 		bcmp (const void *, const void *, size_t);
void	 	bzero (void *, size_t);
/* #endif */
/* Prototypes for which GCC built-ins exist. */
void		*memcpy (void *, const void *, size_t);
int	 		memcmp (const void *, const void *, size_t);
void		*memset (void *, int, size_t);
#if __GNUC_PREREQ__(2, 95) && !defined(__vax__)
#define	memcpy(d, s, l)		__builtin_memcpy(d, s, l)
#define	memcmp(a, b, l)		__builtin_memcmp(a, b, l)
#define	memset(d, v, l)		__builtin_memset(d, v, l)
#endif

char		*strcpy (char *, const char *);
int	 		strcmp (const char *, const char *);
size_t	 	strlen (const char *);
#if __GNUC_PREREQ__(2, 95)
#define	strcpy(d, s)		__builtin_strcpy(d, s)
#define	strcmp(a, b)		__builtin_strcmp(a, b)
#define	strlen(a)			__builtin_strlen(a)
#endif

/* Functions for which we always use built-ins. */
#ifdef __GNUC__
#define	alloca(s)		__builtin_alloca(s)
#endif

/* These exist in GCC 3.x, but we don't bother. */
char		*strcat (char *, const char *);
char		*strncpy (char *, const char *, size_t);
int	 		strncmp (const char *, const char *, size_t);
char		*strchr (const char *, int);
char		*strrchr (const char *, int);
char		*strstr (const char *, const char *);

/*
 * ffs is an instruction on vax.
 */
int	 		ffs(int);
#if __GNUC_PREREQ__(2, 95) && (!defined(__vax__) || __GNUC_PREREQ__(4,1))
#define	ffs(x)	__builtin_ffs(x)
#endif
int	 		ffsl(long);

u_int16_t	bswap16(u_int16_t);
u_int32_t	bswap32(u_int32_t);
u_int64_t	bswap64(u_int64_t);

//void	 	__assert (const char *, const char *, int, const char *) __attribute__((__noreturn__));
void		kern_assert (const char *, ...);
u_int32_t 	inet_addr (const char *);
int	 		locc (int, char *, u_int);
char		*intoa (u_int32_t);
#define inet_ntoa(a) intoa((a).s_addr)
void		*memchr (const void *, int, size_t);
void		*memmove (void *, const void *, size_t);
int	 		pmatch (const char *, const char *, const char **);
u_int32_t 	arc4random (void);
void	 	arc4randbytes (void *, size_t);
void		qsort(void *, size_t, size_t, int (*)(const void *, const void *));
void		qsort_r(void *, size_t, size_t, void *, int (*)(void *, const void *, const void *));
u_long	 	random (void);
char		*rindex (const char *, int);
int	 		scanc (u_int, const u_char *, const u_char *, int);
int	 		skpc (int, size_t, u_char *);
int	 		strcasecmp (const char *, const char *);
size_t	 	strlcpy (char *, const char *, size_t);
size_t	 	strlcat (char *, const char *, size_t);
int	 		strncasecmp (const char *, const char *, size_t);
u_long	 	strtoul (const char *, char **, int);
void	 	hexdump(void (*)(const char *, ...), const char *, const void *, size_t);

#define __KASSERTSTR  "Kernel assertion failed: (%s), function %s, file %s, line %d."

#ifdef __COVERITY__
#ifndef DIAGNOSTIC
#define DIAGNOSTIC
#endif
#endif

#ifndef	CTASSERT
#define	CTASSERT(x)				__CTASSERT(x)
#endif
#ifndef	CTASSERT_SIGNED
#define	CTASSERT_SIGNED(x)		__CTASSERT(((typeof(x))-1) < 0)
#endif
#ifndef	CTASSERT_UNSIGNED
#define	CTASSERT_UNSIGNED(x)	__CTASSERT(((typeof(x))-1) >= 0)
#endif

#ifdef NDEBUG						/* tradition! */
#define	assert(e)	((void)0)
#else
#ifdef __STDC__
#define	assert(e)	(__predict_true((e)) ? (void)0 :		    	\
			    __assert("diagnostic ", __FILE__, __LINE__, #e))
#else
#define	assert(e)	(__predict_true((e)) ? (void)0 :		   	 	\
			    __assert("diagnostic ", __FILE__, __LINE__, "e"))
#endif
#endif

#ifndef DIAGNOSTIC
#define _DIAGASSERT(a)			(void)0
#ifdef lint
#define	KASSERTMSG(e, msg, ...)	/* NOTHING */
#define	KASSERT(e)				/* NOTHING */
#else /* !lint */
#define	KASSERTMSG(e, msg, ...)	((void)0)
#define	KASSERT(e)				((void)0)
#endif /* !lint */
#else /* DIAGNOSTIC */
#define _DIAGASSERT(a)			assert(a)
#define	KASSERTMSG(e, msg, ...)	(__predict_true((e)) ? (void)0 :	\
			    __assert(__KASSERTSTR msg, "diagnostic ", #e, __FILE__, __LINE__, ## __VA_ARGS__))

#define	KASSERT(e)	(__predict_true((e)) ? (void)0 :		    	\
			    __assert(__KASSERTSTR, "diagnostic ", #e, __FILE__, __LINE__))
#endif

#ifndef DEBUG
#ifdef lint
#define	KDASSERTMSG(e,msg, ...)	/* NOTHING */
#define	KDASSERT(e)				/* NOTHING */
#else /* lint */
#define	KDASSERTMSG(e,msg, ...)	((void)0)
#define	KDASSERT(e)				((void)0)
#endif /* lint */
#else
#define	KASSERTMSG(e, msg, ...)	(__predict_true((e)) ? (void)0 :	\
			    __assert(__KASSERTSTR msg, "debugging ", #e, __FILE__, __LINE__, ## __VA_ARGS__))

#define	KASSERT(e)	(__predict_true((e)) ? (void)0 :		    	\
			    __assert(__KASSERTSTR, "debugging ", #e, __FILE__, __LINE__))
#endif


/*
static __inline int 	imax (int, int) __attribute__ ((unused));
static __inline int 	imin (int, int) __attribute__ ((unused));
static __inline u_int 	max (u_int, u_int) __attribute__ ((unused));
static __inline u_int 	min (u_int, u_int) __attribute__ ((unused));
static __inline long 	lmax (long, long) __attribute__ ((unused));
static __inline long 	lmin (long, long) __attribute__ ((unused));
static __inline u_long 	ulmax (u_long, u_long) __attribute__ ((unused));
static __inline u_long 	ulmin (u_long, u_long) __attribute__ ((unused));
static __inline int 	abs (int) __attribute__ ((unused));

static __inline int 	isspace (int) __attribute__((__unused__));
static __inline int 	isascii (int) __attribute__((__unused__));
static __inline int 	isupper (int) __attribute__((__unused__));
static __inline int 	islower (int) __attribute__((__unused__));
static __inline int 	isalpha (int) __attribute__((__unused__));
static __inline int 	isdigit (int) __attribute__((__unused__));
static __inline int 	isxdigit (int) __attribute__((__unused__));
static __inline int 	toupper (int) __attribute__((__unused__));
static __inline int 	tolower (int) __attribute__((__unused__));

static __inline int
imax(int a, int b)
{
	return (a > b ? a : b);
}

static __inline int
imin(int a, int b)
{
	return (a < b ? a : b);
}

static __inline long
lmax(long a, long b)
{
	return (a > b ? a : b);
}

static __inline long
lmin(long a, long b)
{
	return (a < b ? a : b);
}

static __inline u_int
max(u_int a, u_int b)
{
	return (a > b ? a : b);
}

static __inline u_int
min(u_int a, u_int b)
{
	return (a < b ? a : b);
}

static __inline quad_t
qmax(quad_t a, quad_t b)
{
	return (a > b ? a : b);
}

static __inline quad_t
qmin(quad_t a, quad_t b)
{
	return (a < b ? a : b);
}

static __inline u_long
ulmax(u_long a, u_long b)
{
	return (a > b ? a : b);
}

static __inline u_long
ulmin(u_long a, u_long b)
{
	return (a < b ? a : b);
}

static __inline int
abs(int j)
{
	return(j < 0 ? -j : j);
}

static __inline int
isspace(int ch)
{
	return (ch == ' ' || (ch >= '\t' && ch <= '\r'));
}

static __inline int
isascii(int ch)
{
	return ((ch & ~0x7f) == 0);
}

static __inline int
isupper(int ch)
{
	return (ch >= 'A' && ch <= 'Z');
}

static __inline int
islower(int ch)
{
	return (ch >= 'a' && ch <= 'z');
}

static __inline int
isalpha(int ch)
{
	return (isupper(ch) || islower(ch));
}

static __inline int
isdigit(int ch)
{
	return (ch >= '0' && ch <= '9');
}

static __inline int
isxdigit(int ch)
{
	return (isdigit(ch) ||
	    (ch >= 'A' && ch <= 'F') ||
	    (ch >= 'a' && ch <= 'f'));
}

static __inline int
toupper(int ch)
{
	if (islower(ch))
		return (ch - 0x20);
	return (ch);
}

static __inline int
tolower(int ch)
{
	if (isupper(ch))
		return (ch + 0x20);
	return (ch);
}
*/
