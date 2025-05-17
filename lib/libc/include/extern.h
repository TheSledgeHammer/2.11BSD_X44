/*	$NetBSD: extern.h,v 1.9 2003/09/27 03:14:59 matt Exp $	*/

/*
 * Copyright (c) 1997 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_LIBC_EXTERN_H_
#define	_LIBC_EXTERN_H_

#include <sys/cdefs.h>
#include <ucontext.h>

__BEGIN_DECLS
#if __SSP_FORTIFY_LEVEL > 0
char 		*__getcwd(char *, size_t);
#endif
const char 	*__strerror(int , char *, size_t);	/* TODO: Resolve  */
const char 	*__strsignal(int , char *, size_t); /* TODO: Resolve  */
char 		*__dtoa(double, int, int, int *, int *, char **);
int 		__sysctl(int *, unsigned int, void *, size_t *, void *, size_t);

int 		__getlogin(char *, u_int);
void 		_resumecontext(void);
int 		_swapcontext(ucontext_t *, const ucontext_t *);

//extern char *__minbrk;
//int 		__setlogin(const char *);
//struct sigaction;
//int 		__sigaction_sigtramp(int, const struct sigaction *, struct sigaction *, const void *, int);
__END_DECLS

#endif	/* _LIBC_EXTERN_H_ */
