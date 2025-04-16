/*	$NetBSD: strtod.c,v 1.45.2.1 2005/04/19 13:35:54 tron Exp $	*/

#ifndef _ARITH_H_INCLUDED
#define _ARITH_H_INCLUDED

#if defined(__m68k__) || defined(__sparc__) || defined(__i386__) || \
    defined(__mips__) || defined(__ns32k__) || defined(__alpha__) || \
    defined(__powerpc__) || defined(__sh__) || defined(__x86_64__) || \
    defined(__hppa__) || defined(__aarch__) || defined(__powerpc64__) || \
	defined(__sparc64__) || defined(__riscv__) || \
    (defined(__arm__) && defined(__VFP_FP__))
#include <sys/types.h>
#include <machine/endian.h>
#if BYTE_ORDER == BIG_ENDIAN
#define IEEE_BIG_ENDIAN
#else
#define IEEE_LITTLE_ENDIAN
#endif
#endif

#if defined(__arm__) && !defined(__VFP_FP__)
/*
 * Although the CPU is little endian the FP has different
 * byte and word endianness. The byte order is still little endian
 * but the word order is big endian.
 */
#define IEEE_BIG_ENDIAN
#endif

#if defined(__alpha__) || defined(__ia64__)
#ifndef _IEEE_FP
#define Sudden_Underflow
#endif
#endif

#if defined(__vax__)
#define VAX
#define NO_HEX_FP	/* XXX */
#endif

#endif /* _ARITH_H_INCLUDED */
