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
 * $Id: libkern.h,v 1.3 1994/08/30 18:19:47 davidg Exp $
 */

#include <sys/types.h>

static __inline int imax(int a, int b) { return (a > b ? a : b); }
static __inline int imin(int a, int b) { return (a < b ? a : b); }
static __inline long lmax(long a, long b) { return (a > b ? a : b); }
static __inline long lmin(long a, long b) { return (a < b ? a : b); }
static __inline u_int max(u_int a, u_int b) { return (a > b ? a : b); }
static __inline u_int min(u_int a, u_int b) { return (a < b ? a : b); }
static __inline quad_t qmax(quad_t a, quad_t b) { return (a > b ? a : b); }
static __inline quad_t qmin(quad_t a, quad_t b) { return (a < b ? a : b); }
static __inline u_long ulmax(u_long a, u_long b) { return (a > b ? a : b); }
static __inline u_long ulmin(u_long a, u_long b) { return (a < b ? a : b); }

/* hash_prospector.c Functions */
extern uint32_t prospector32(uint32_t x);
extern uint32_t lowbias32(uint32_t x);
extern uint32_t lowbias32_r(uint32_t x);
extern uint32_t triple32(uint32_t x);
extern uint32_t triple32_r(uint32_t x);
extern uint32_t hash32(uint32_t x);
extern uint32_t murmurhash32_mix32(uint32_t x);

/* Prototypes for non-quad routines. */
int	 	bcmp(const void *, const void *, size_t);
int	 	ffs(int);
int	 	locc(int, char *, u_int);
u_long	random(void);
char	*rindex(const char *, int);
int	 	scanc(u_int, u_char *, u_char *, int);
int	 	skpc(int, int, char *);
char	*strcat(char *, const char *);
char	*strcpy(char *, const char *);
size_t	 strlen(const char *);
char	*strncpy(char *, const char *, size_t);

void	__assert(const char *, const char *, int, const char *);
void	kern_assert(const char *, ...);

#define __KASSERTSTR  "kernel %sassertion \"%s\" failed: file \"%s\", line %d "
#ifdef NDEBUG						/* tradition! */
#define	assert(e)	((void)0)
#else
#define	assert(e)	(__predict_true((e)) ? (void)0 :		    \
		kern_assert(__KASSERTSTR, "", #e, __FILE__, __LINE__))
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

#ifndef DIAGNOSTIC
#define _DIAGASSERT(a)	(void)0
#ifdef lint
#define	KASSERTMSG(e, msg, ...)	/* NOTHING */
#define	KASSERT(e)				/* NOTHING */
#else /* !lint */
#define	KASSERTMSG(e, msg, ...)	((void)0)
#define	KASSERT(e)				((void)0)
#endif /* !lint */
#else /* DIAGNOSTIC */
#define _DIAGASSERT(a)	assert(a)
#define	KASSERTMSG(e, msg, ...)		\
			(__predict_true((e)) ? (void)0 :		    \
			    kern_assert(__KASSERTSTR msg, "diagnostic ", #e,	    \
				__FILE__, __LINE__, ## __VA_ARGS__))

#define	KASSERT(e)	(__predict_true((e)) ? (void)0 :		    \
			    kern_assert(__KASSERTSTR, "diagnostic ", #e,	    \
				__FILE__, __LINE__))
#endif
