/*	$NetBSD: types.h,v 1.13.14.1 1997/11/05 04:39:04 thorpej Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)types.h	7.5 (Berkeley) 3/9/91
 */

#ifndef	_I386_MACHTYPES_H_
#define	_I386_MACHTYPES_H_

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
typedef struct _physadr {
	int 	r[1];
} *physadr;

typedef struct label_t {
	int 	val[6];
} label_t;
#endif

/* VM system types */
typedef	unsigned long					vm_offset_t;
typedef	unsigned long					vm_size_t;

typedef unsigned long					__vaddr_t;
typedef unsigned long					__paddr_t;
typedef unsigned long					__vsize_t;
typedef unsigned long					__psize_t;

/*
 * Basic integral types.  Omit the typedef if
 * not possible for a machine/compiler combination.
 */
/*
 * 7.18.1 Integer types
 */

/* 7.18.1.1 Exact-width integer types */

typedef	signed char			 			__int8_t;
typedef	unsigned char					__uint8_t;
typedef	short int						__int16_t;
typedef	unsigned short int     			__uint16_t;
typedef	int								__int32_t;
typedef	unsigned int	       			__uint32_t;
#ifdef __LP64__
typedef	__COMPILER_INT64__				__int64_t;
typedef	__COMPILER_UINT64__    			__uint64_t;
#else
/* LONGLONG */
typedef	long long int					__int64_t;
/* LONGLONG */
typedef	unsigned long long int 			__uint64_t;
#endif

/* 7.18.1.2 Minimum-width integer types */

typedef	signed char		  				int_least8_t;
typedef	unsigned char		 			uint_least8_t;
typedef	short int		 				int_least16_t;
typedef	unsigned short int				uint_least16_t;
typedef	int			 					int_least32_t;
typedef	unsigned int					uint_least32_t;
#ifdef __LP64__
typedef	__COMPILER_INT64__	 			int_least64_t;
typedef	__COMPILER_UINT64__				uint_least64_t;
#else
/* LONGLONG */
typedef	long long int		 			int_least64_t;
/* LONGLONG */
typedef	unsigned long long int			uint_least64_t;
#endif

/* 7.18.1.3 Fastest minimum-width integer types */

typedef	int			   					int_fast8_t;
typedef	unsigned int		 	 		uint_fast8_t;
typedef	int			  					int_fast16_t;
typedef	unsigned int		 			uint_fast16_t;
typedef	int			  					int_fast32_t;
typedef	unsigned int		 			uint_fast32_t;
#ifdef __LP64__
typedef	__COMPILER_INT64__	  			int_fast64_t;
typedef	__COMPILER_UINT64__	 			uint_fast64_t;
#else
/* LONGLONG */
typedef	long long int		  			int_fast64_t;
/* LONGLONG */
typedef	unsigned long long int	 		uint_fast64_t;
#endif

/* 7.18.1.4 Integer types capable of holding object pointers */

typedef	long							__intptr_t;
typedef	unsigned long					__uintptr_t;

/* 7.18.1.5 Greatest-width integer types */

#ifdef __LP64__
typedef	__COMPILER_INT64__	      		intmax_t;
typedef	unsigned __COMPILER_INT64__ 	uintmax_t;
#else
/* LONGLONG */
typedef	long long int		      		intmax_t;
/* LONGLONG */
typedef	unsigned long long int	  		uintmax_t;
#endif

#define	__BIT_TYPES_DEFINED__

#ifdef __LP64__
/* 64-bit */
typedef	__int64_t						__ptrdiff_t;
typedef	__int64_t						__segsz_t;
typedef	__uint64_t						__size_t;
typedef	__int64_t						__ssize_t;
typedef	__int64_t						__time_t;
#else
/* 32-bit */
typedef	__int32_t						__ptrdiff_t;
typedef	__int32_t						__segsz_t;
typedef	__uint32_t						__size_t;
typedef	__int32_t						__ssize_t;
typedef	__int32_t						__time_t;
#endif

/* Register size */
typedef long							__register_t;
typedef	unsigned long					__uregister_t;

/* Wide character support types */
typedef	__uint32_t						__wchar_t;
typedef __uint32_t						__wint_t;
typedef	__uint32_t						__rune_t;
typedef	void 							*__wctrans_t;
typedef	void 							*__wctype_t;

/*
 * 7.18.2 Limits of specified-width integer types.
 *
 * The following object-like macros specify the minimum and maximum limits
 * of integer types corresponding to the typedef names defined above.
 */

/* 7.18.2.1 Limits of exact-width integer types */
#define	INT8_MIN			(-0x7f - 1)
#define	INT16_MIN			(-0x7fff - 1)
#define	INT32_MIN			(-0x7fffffff - 1)
#define	INT64_MIN			(-0x7fffffffffffffffLL - 1)

#define	INT8_MAX			0x7f
#define	INT16_MAX			0x7fff
#define	INT32_MAX			0x7fffffff
#define	INT64_MAX			0x7fffffffffffffffLL

#define	UINT8_MAX			0xff
#define	UINT16_MAX			0xffff
#define	UINT32_MAX			0xffffffffU
#define	UINT64_MAX			0xffffffffffffffffULL

/* 7.18.2.2 Limits of minimum-width integer types */
#define	INT_LEAST8_MIN		INT8_MIN
#define	INT_LEAST16_MIN		INT16_MIN
#define	INT_LEAST32_MIN		INT32_MIN
#define	INT_LEAST64_MIN		INT64_MIN

#define	INT_LEAST8_MAX		INT8_MAX
#define	INT_LEAST16_MAX		INT16_MAX
#define	INT_LEAST32_MAX		INT32_MAX
#define	INT_LEAST64_MAX		INT64_MAX

#define	UINT_LEAST8_MAX		UINT8_MAX
#define	UINT_LEAST16_MAX	UINT16_MAX
#define	UINT_LEAST32_MAX	UINT32_MAX
#define	UINT_LEAST64_MAX	UINT64_MAX

/* 7.18.2.3 Limits of fastest minimum-width integer types */
#define	INT_FAST8_MIN		__INT_FAST8_MIN
#define	INT_FAST16_MIN		__INT_FAST16_MIN
#define	INT_FAST32_MIN		__INT_FAST32_MIN
#define	INT_FAST64_MIN		__INT_FAST64_MIN

#define	INT_FAST8_MAX		__INT_FAST8_MAX
#define	INT_FAST16_MAX		__INT_FAST16_MAX
#define	INT_FAST32_MAX		__INT_FAST32_MAX
#define	INT_FAST64_MAX		__INT_FAST64_MAX

#define	UINT_FAST8_MAX		__UINT_FAST8_MAX
#define	UINT_FAST16_MAX		__UINT_FAST16_MAX
#define	UINT_FAST32_MAX		__UINT_FAST32_MAX
#define	UINT_FAST64_MAX		__UINT_FAST64_MAX

/* 7.18.2.4 Limits of integer types capable of holding object pointers */
#ifdef __LP64__
#define	INTPTR_MIN			(-0x7fffffffffffffffL - 1)
#define	INTPTR_MAX			0x7fffffffffffffffL
#define	UINTPTR_MAX			0xffffffffffffffffUL
#else
#define	INTPTR_MIN			(-0x7fffffffL - 1)
#define	INTPTR_MAX			0x7fffffffL
#define	UINTPTR_MAX			0xffffffffUL
#endif

/* 7.18.2.5 Limits of greatest-width integer types */
#define	INTMAX_MIN			INT64_MIN
#define	INTMAX_MAX			INT64_MAX
#define	UINTMAX_MAX			UINT64_MAX

/*
 * 7.18.3 Limits of other integer types.
 *
 * The following object-like macros specify the minimum and maximum limits
 * of integer types corresponding to types specified in other standard
 * header files.
 */

/* Limits of ptrdiff_t */
#define	PTRDIFF_MIN			INTPTR_MIN
#define	PTRDIFF_MAX			INTPTR_MAX

/* Limits of sig_atomic_t */
#define	SIG_ATOMIC_MIN		INT32_MIN
#define	SIG_ATOMIC_MAX		INT32_MAX

/* Limit of size_t */
#ifndef	SIZE_MAX
#define	SIZE_MAX			UINTPTR_MAX
#endif

/* Limits of wchar_t */
#ifndef	WCHAR_MIN
#define	WCHAR_MIN			INT32_MIN
#endif
#ifndef	WCHAR_MAX
#define	WCHAR_MAX			INT32_MAX
#endif

/* Limits of wint_t */
#define	WINT_MIN			INT32_MIN
#define	WINT_MAX			INT32_MAX

/*
 * 7.18.4 Macros for integer constants.
 *
 * The following function-like macros expand to integer constants
 * suitable for initializing objects that have integer types corresponding
 * to types defined in <stdint.h>.  The argument in any instance of
 * these macros shall be a decimal, octal, or hexadecimal constant with
 * a value that does not exceed the limits for the corresponding type.
 */

/* 7.18.4.1 Macros for minimum-width integer constants. */
#define	INT8_C(_c)			(_c)
#define	INT16_C(_c)			(_c)
#define	INT32_C(_c)			(_c)
#define	INT64_C(_c)			__CONCAT(_c, LL)

#define	UINT8_C(_c)			(_c)
#define	UINT16_C(_c)		(_c)
#define	UINT32_C(_c)		__CONCAT(_c, U)
#define	UINT64_C(_c)		__CONCAT(_c, ULL)

/* 7.18.4.2 Macros for greatest-width integer constants. */
#define	INTMAX_C(_c)		__CONCAT(_c, LL)
#define	UINTMAX_C(_c)		__CONCAT(_c, ULL)

#endif /* _I386_MACHTYPES_H_ */
