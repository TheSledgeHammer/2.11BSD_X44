/* $NetBSD: gd_qnan.h,v 1.1 2006/01/25 15:33:28 kleink Exp $ */

#ifndef _GD_QNAN_H_
#define _GD_QNAN_H_

#include <machine/endian.h>

#if defined(__aarch__)
#define f_QNAN 		0x7fc00000
#ifdef __AARCH64EB__
#define d_QNAN0 	0x7ff80000
#define d_QNAN1 	0x0
#define ld_QNAN0 	0x7fff8000
#define ld_QNAN1 	0x0
#define ld_QNAN2 	0x0
#define ld_QNAN3 	0x0
#else
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff80000
#define ld_QNAN0 	0x0
#define ld_QNAN1 	0x0
#define ld_QNAN2 	0x0
#define ld_QNAN3 	0x7fff8000
#endif
#endif

#if defined(__alpha__)
#define f_QNAN 		0x7fc00000
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff80000
#endif

#if defined(__arm__)
#define f_QNAN 		0x7fc00000
#if BYTE_ORDER == BIG_ENDIAN
#define d_QNAN0 	0x7ff80000
#define d_QNAN1 	0x0
#else
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff80000
#endif
#endif

#if defined(__hppa__)
#define f_QNAN 		0x7fa00000
#define d_QNAN0 	0x7ff40000
#define d_QNAN1 	0x0
#ifdef _LP64
#define ld_QNAN0 	0x7fff4000
#define ld_QNAN1 	0x0
#define ld_QNAN2 	0x0
#define ld_QNAN3 	0x0
#endif
#endif

#if defined(__i386__)
#define f_QNAN 		0x7fc00000
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff80000
#define ldus_QNAN0 	0x0
#define ldus_QNAN1 	0x0
#define ldus_QNAN2 	0x0
#define ldus_QNAN3 	0xc000
#define ldus_QNAN4 	0x7fff
/* 2 bytes of tail padding follow, per i386 ABI */
#endif

#if defined(__ia64__)
#define f_QNAN 		0x7fc00000
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff80000
#endif

#if defined(__m68k__)
#define f_QNAN 		0x7fc00000
#define d_QNAN0 	0x7ff80000
#define d_QNAN1 	0x0
#ifndef __mc68010__
#define ld_QNAN0 	0x7fff0000
#define ld_QNAN1 	0x40000000
#define ld_QNAN2 	0x0
#endif
#endif

#if defined(__mips__)
#define f_QNAN 		0x7fa00000
#if BYTE_ORDER == BIG_ENDIAN
#define d_QNAN0 	0x7ff40000
#define d_QNAN1 	0x0
#define ld_QNAN0 	0x7fff8000
#define ld_QNAN1 	0x0
#define ld_QNAN2 	0x0
#define ld_QNAN3 	0x0
#else
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff40000
#define ld_QNAN0	0x0
#define ld_QNAN1 	0x0
#define ld_QNAN2 	0x0
#define ld_QNAN3 	0x7fff8000
#endif
#endif

#if defined(__powerpc__) || defined(__powerpc64__)
#define f_QNAN 		0x7fc00000
#define d_QNAN0 	0x7ff80000
#define d_QNAN1 	0x0
#endif

#if defined(__riscv__)
/*
 * The RISC-V Instruction Set Manual Volume I: User-Level ISA
 * Document Version 2.2
 *
 * 8.3 NaN Generation and Propagation
 *
 * The canonical NaN has a positive sign and all significand bits clear except
 * the MSB, aka the quiet bit.
 */

#define f_QNAN 		0x7fc00000
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff80000
#define ld_QNAN0 	0x0
#define ld_QNAN1 	0x0
#define ld_QNAN2 	0x0
#define ld_QNAN3 	0x7fff8000
#endif

#if defined(__sh__)
#define f_QNAN 		0x7fa00000
#if BYTE_ORDER == BIG_ENDIAN
#define d_QNAN0 	0x7ff40000
#define d_QNAN1 	0x0
#else
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff40000
#endif
#endif

#if defined(__sparc__) || defined(__sparc64__)
#define f_QNAN 		0x7fc00000
#define d_QNAN0 	0x7ff80000
#define d_QNAN1 	0x0
#if defined(__sparc64__)
#define ld_QNAN0 	0x7fff8000
#define ld_QNAN1 	0x0
#define ld_QNAN2 	0x0
#define ld_QNAN3 	0x0
#endif
#endif

#if defined(__x86_64__)
#define f_QNAN 		0x7fc00000
#define d_QNAN0 	0x0
#define d_QNAN1 	0x7ff80000
#define ld_QNAN0 	0x0
#define ld_QNAN1 	0xc0000000
#define ld_QNAN2 	0x7fff
#define ld_QNAN3 	0x0
#define ldus_QNAN0 	0x0
#define ldus_QNAN1 	0x0
#define ldus_QNAN2 	0x0
#define ldus_QNAN3 	0xc000
#define ldus_QNAN4 	0x7fff
/* 6 bytes of tail padding follow, per AMD64 ABI */
#endif
#endif /* _GD_QNAN_H_ */
