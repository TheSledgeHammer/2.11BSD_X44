/*-
 * Copyright (c) 1990, 1993
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
 *	@(#)stdlib.h	8.3.2 (2.11BSD) 1996/1/12
 *
 * Adapted from the 4.4-Lite CD.  The odds of a ANSI C compiler for 2.11BSD
 * being slipped under the door are not distinguishable from 0 - so the 
 * prototypes and ANSI ifdefs have been removed from this file. 
 *
 * Some functions (strtoul for example) do not exist yet but are retained in
 * this file because additions to libc.a are anticipated shortly.
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <machine/ansi.h>
#include <sys/cdefs.h>


#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#if defined(_BSD_WCHAR_T_) && !defined(__cplusplus)
typedef	_BSD_WCHAR_T_	wchar_t;
#undef	_BSD_WCHAR_T_
#endif

#ifdef	notyet
typedef struct {
	int quot;		/* quotient */
	int rem;		/* remainder */
} div_t;

typedef struct {
	long quot;		/* quotient */
	long rem;		/* remainder */
} ldiv_t;
#endif

#ifndef NULL
#define	NULL	0
#endif

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

#define	RAND_MAX	0x7fff

void			abort();
int				abs();
int				atexit();
double			atof();
int				atoi();
long			atol();
void			*calloc();
void			exit();
void			free();
char			*getenv();
void			*malloc();
void			qsort();
int				rand();
void			*realloc();
void			srand();
double			strtod();
long			strtol();
unsigned long 	strtoul();
int				system();

int				putenv();
int				setenv();

void			*alloca(size_t);
char			*getbsize (int *, long *);
char			*cgetcap (char *, char *, int);
int	 			cgetclose (void);
int	 			cgetent (char **, char **, char *);
int	 			cgetfirst (char **, char **);
int	 			cgetmatch (char *, char *);
int	 			cgetnext (char **, char **);
int	 			cgetnum (char *, char *, long *);
int	 			cgetset (char *);
int	 			cgetstr (char *, char *, char **);
int	 			cgetustr (char *, char *, char **);

int				daemon(int, int);
char			*devname(int, int);
int				getloadavg(double[], int);

int	 			heapsort (void *, size_t, size_t, int (*)(const void *, const void *));
char			*initstate (unsigned long, char *, long);
int		 		mergesort (void *, size_t, size_t, int (*)(const void *, const void *));
int	 			radixsort (const unsigned char **, int, const unsigned char *, unsigned);
int	 			sradixsort (const unsigned char **, int, const unsigned char *, unsigned);

extern char *optarg;			/* getopt(3) external variables */
extern int opterr, optind, optopt;
int				getopt();

extern char *suboptarg;			/* getsubopt(3) external variable */
int				getsubopt();

long			random();
char			*setstate();
void			srandom();
void			unsetenv();

long long	 	strtoq (const char *, char **, int);
unsigned long long
				strtouq (const char *, char **, int);

#endif /* _STDLIB_H_ */
