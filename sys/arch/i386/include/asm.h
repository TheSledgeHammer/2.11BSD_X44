/*	$NetBSD: asm.h,v 1.44 2020/04/25 15:26:17 bouyer Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)asm.h	5.5 (Berkeley) 5/7/91
 */

#ifndef _MACHINE_ASM_H_
#define	_MACHINE_ASM_H_

#include <sys/cdefs.h>

#ifdef __ELF__
# define _C_LABEL(x)	x
#else
# ifdef __STDC__
#  define _C_LABEL(x)	_ ## x
# else
#  define _C_LABEL(x)	_/**/x
# endif
#endif
#define	_ASM_LABEL(x)	x

#ifdef __STDC__
# define __CONCAT(x,y)	x ## y
# define __STRING(x)	#x
#else
# define __CONCAT(x,y)	x/**/y
# define __STRING(x)	"x"
#endif

/* let kernels and others override entrypoint alignment */
#ifndef _ALIGN_TEXT
# ifdef __ELF__
#  define _ALIGN_TEXT .align 4
# else
#  define _ALIGN_TEXT .align 2
# endif
#endif

#define	ENTRY(name) 								\
	.globl _/**/name; _/**/name:

#define	ALTENTRY(name) 								\
	.globl _/**/name; _/**/name:

/* XXX Can't use __CONCAT() here, as it would be evaluated incorrectly. */
#ifdef __ELF__
#ifdef __STDC__
#define	IDTVEC(name) 								\
	ALIGN_TEXT; .globl X ## name; .type X ## name,@function; X ## name:
#define	IDTVEC_END(name) 							\
	.size X ## name, . - X ## name
#else
#define	IDTVEC(name) 								\
	ALIGN_TEXT; .globl X/**/name; .type X/**/name,@function; X/**/name:
#define	IDTVEC_END(name) 							\
	.size X/**/name, . - X/**/name
#endif /* __STDC__ */
#else
#ifdef __STDC__
#define	IDTVEC(name) 								\
	ALIGN_TEXT; .globl _X ## name; .type _X ## name,@function; _X ## name:
#define	IDTVEC_END(name) 							\
	.size _X ## name, . - _X ## name
#else
#define	IDTVEC(name) 								\
	ALIGN_TEXT; .globl _X/**/name; .type _X/**/name,@function; _X/**/name:
#define	IDTVEC_END(name) 							\
	.size _X/**/name, . - _X/**/name
#endif /* __STDC__ */
#endif /* __ELF__ */


#ifdef __ELF__
#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;									\
	alias = sym
#endif

#ifdef __STDC__
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;						\
	.stabs __STRING(_C_LABEL(sym)) ## ,1,0,0,0
#elif defined(__ELF__)
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;							\
	.stabs __STRING(sym),1,0,0,0
#else
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;							\
	.stabs __STRING(_/**/sym),1,0,0,0
#endif /* __STDC__ */
#endif /* _MACHINE_ASM_H_ */
