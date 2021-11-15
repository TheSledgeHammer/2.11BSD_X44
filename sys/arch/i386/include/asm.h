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

#ifndef _I386_ASM_H_
#define	_I386_ASM_H_

#include <sys/cdefs.h>

#define	NOP				\
	inb 	$0x84, %al;	\
	inb 	$0x84, %al

#define	FASTER_NOP		\
	pushl 	%eax; 		\
	inb 	$0x84,%al; 	\
	popl 	%eax

#define	LCALL(x,y)		\
	.byte 	0x9a;		\
	.long 	y; 			\
	.word 	x

#ifdef PIC
#define	PIC_PROLOGUE								\
	pushl	%ebx;									\
	call	1f;										\
1:													\
	popl	%ebx;									\
	addl	$_GLOBAL_OFFSET_TABLE_+[.-1b],%ebx
#define	PIC_EPILOGUE								\
	popl	%ebx
#define	PIC_PLT(x)		x@PLT
#define	PIC_GOT(x)		x@GOT(%ebx)
#define	PIC_GOTOFF(x)	x@GOTOFF(%ebx)
#else
#define	PIC_PROLOGUE
#define	PIC_EPILOGUE
#define	PIC_PLT(x)		x
#define	PIC_GOTOFF(x)	x
#endif

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
#if !defined(_ALIGN_TEXT) && !defined(_KERNEL)
# ifdef _STANDALONE
#  define _ALIGN_TEXT .align 1
# elif defined __ELF__
#  define _ALIGN_TEXT .align 16
# else
#  define _ALIGN_TEXT .align 4
# endif
#endif

/*
 * STRONG_ALIAS, WEAK_ALIAS
 *	Create a strong or weak alias.
 */
#define STRONG_ALIAS(alias,sym) 					\
	.global alias; 									\
	alias = sym
#define WEAK_ALIAS(alias,sym) 						\
	.weak alias; 									\
	alias = sym

#define _START_ENTRY								\
	.text; _ALIGN_TEXT

#ifdef __STDC__
#define	ENTRY(name) 								\
	_START_ENTRY;									\
	.globl _ ## name; _ ## name:					\
	.type x,@function; x:

#define	ALTENTRY(name) 								\
	.globl _ ## name; _ ## name:					\

#define	IDTVEC(name) 								\
	ALIGN_TEXT; .globl _X ## name; 					\
	.type _X ## name,@function; _X ## name:

#define	IDTVEC_END(name) 							\
	.size _X ## name, . - _X ## name

#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;						\
	.stabs __STRING(_C_LABEL(sym)) ## ,1,0,0,0
#else
#define	ENTRY(name) 								\
	_START_ENTRY;									\
	.globl _/**/name; _/**/name:					\
	.type x,@function; x:

#define	ALTENTRY(name) 								\
	.globl _/**/name; _/**/name:					\

#define	IDTVEC(name) 								\
	ALIGN_TEXT; .globl X/**/name; 					\
	.type X/**/name,@function; X/**/name:

#define	IDTVEC_END(name) 							\
	.size X/**/name, . - X/**/name

#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;							\
	.stabs __STRING(_/**/sym),1,0,0,0
#endif /* __STDC__ */

#ifdef _KERNEL
#ifdef _STANDALONE
#define ALIGN_DATA		.align	4
#define ALIGN_TEXT		.align	4	/* 4-byte boundaries */
#define SUPERALIGN_TEXT	.align	16	/* 15-byte boundaries */
#elif defined __ELF__
#define ALIGN_DATA		.align	4
#define ALIGN_TEXT		.align	16	/* 16-byte boundaries */
#define SUPERALIGN_TEXT	.align	16	/* 16-byte boundaries */
#else
#define ALIGN_DATA		.align	2
#define ALIGN_TEXT		.align	4	/* 16-byte boundaries */
#define SUPERALIGN_TEXT	.align	4	/* 16-byte boundaries */
#endif /* __ELF__ */

#define _ALIGN_TEXT ALIGN_TEXT
#endif /* _KERNEL */
#endif /* _I386_ASM_H_ */
