/*	$NetBSD: profile.h,v 1.18 2003/10/27 13:44:20 junyoung Exp $	*/

/*
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
 *	@(#)profile.h	8.1 (Berkeley) 6/11/93
 */

#ifndef _I386_PROFILE_H_
#define _I386_PROFILE_H_

#define	_MCOUNT_DECL static inline void _mcount

#ifdef __ELF__
#define MCOUNT_ENTRY	"__mcount"
#define MCOUNT_COMPAT	__weak_alias(mcount, __mcount)
#else
#define MCOUNT_ENTRY	"mcount"
#define MCOUNT_COMPAT	/* nothing */
#endif

#define	MCOUNT 								\
MCOUNT_COMPAT								\
extern void mcount(void) __asm__(MCOUNT_ENTRY);				\
void									\
mcount()								\
{									\
	int selfpc, frompcindex;					\
	/*								\
	 * find the return address for mcount,				\
	 * and the return address for mcount's caller.			\
	 *								\
	 * selfpc = pc pushed by mcount call				\
	 */								\
	__asm__ __volatile__("movl 4(%%ebp),%0" : "=r" (selfpc));	\
	/*								\
	 * frompcindex = pc pushed by call into self.			\
	 */								\
	__asm__ __volatile__("movl (%%ebp),%0;movl 4(%0),%0"		\
	    : "=r" (frompcindex));					\
	_mcount((u_long)frompcindex, (u_long)selfpc);			\
}

#ifdef _KERNEL
#define	MCOUNT_ENTER	(void)&s; __asm__("cli");
#define	MCOUNT_EXIT	__asm__("sti");
#endif /* _KERNEL */

#endif /* _I386_PROFILE_H_ */
