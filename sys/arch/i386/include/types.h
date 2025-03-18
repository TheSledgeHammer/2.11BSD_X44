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

/* Machine-dependent: global defines */
//#define	__HAVE_GENERIC_SOFT_INTERRUPTS

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

typedef unsigned long					__paddr_t;
typedef unsigned long					__psize_t;
typedef unsigned long					__vaddr_t;
typedef unsigned long					__vsize_t;

/* Lock Machdep */
typedef	__volatile int		    		__cpu_simple_lock_t;

#define __SIMPLELOCK_LOCKED		1
#define __SIMPLELOCK_UNLOCKED	0

/*
 * Basic integral types.  Omit the typedef if
 * not possible for a machine/compiler combination.
 */
/*
 * 7.18.1 Integer types
 */

/* 7.18.1.1 Exact-width integer types */

typedef	signed char			 		__int8_t;
typedef	unsigned char					__uint8_t;
typedef	short int					__int16_t;
typedef	unsigned short int     				__uint16_t;
typedef	int						__int32_t;
typedef	unsigned int	       				__uint32_t;
#ifdef __LP64__
typedef	__COMPILER_INT64__				__int64_t;
typedef	__COMPILER_UINT64__    				__uint64_t;
#else
/* LONGLONG */
typedef	long long int					__int64_t;
/* LONGLONG */
typedef	unsigned long long int 				__uint64_t;
#endif

/* 7.18.1.2 Minimum-width integer types */

typedef	signed char		  			int_least8_t;
typedef	unsigned char		 			uint_least8_t;
typedef	short int		 			int_least16_t;
typedef	unsigned short int				uint_least16_t;
typedef	int			 			int_least32_t;
typedef	unsigned int					uint_least32_t;
#ifdef __LP64__
typedef	__COMPILER_INT64__	 			int_least64_t;
typedef	__COMPILER_UINT64__				uint_least64_t;
#else
/* LONGLONG */
typedef	long long int		 			int_least64_t;
/* LONGLONG */
typedef	unsigned long long int				uint_least64_t;
#endif

/* 7.18.1.3 Fastest minimum-width integer types */

typedef	int			   			int_fast8_t;
typedef	unsigned int		 	 		uint_fast8_t;
typedef	int			  			int_fast16_t;
typedef	unsigned int		 			uint_fast16_t;
typedef	int			  			int_fast32_t;
typedef	unsigned int		 			uint_fast32_t;
#ifdef __LP64__
typedef	__COMPILER_INT64__	  			int_fast64_t;
typedef	__COMPILER_UINT64__	 			uint_fast64_t;
#else
/* LONGLONG */
typedef	long long int		  			int_fast64_t;
/* LONGLONG */
typedef	unsigned long long int	 			uint_fast64_t;
#endif

/* 7.18.1.4 Integer types capable of holding object pointers */

typedef	long						__intptr_t;
typedef	unsigned long					__uintptr_t;

/* 7.18.1.5 Greatest-width integer types */

#ifdef __LP64__
typedef	__COMPILER_INT64__	      			intmax_t;
typedef	unsigned __COMPILER_INT64__ 			uintmax_t;
#else
/* LONGLONG */
typedef	long long int		      			intmax_t;
/* LONGLONG */
typedef	unsigned long long int	  			uintmax_t;
#endif

#define	__BIT_TYPES_DEFINED__

#ifdef __LP64__
/* 64-bit */
typedef	__int64_t					__ptrdiff_t;
typedef	__int64_t					__segsz_t;
typedef	__uint64_t					__size_t;
typedef	__int64_t					__ssize_t;
typedef	__int64_t					__time_t;
#else
/* 32-bit */
typedef	__int32_t					__ptrdiff_t;
typedef	__int32_t					__segsz_t;
typedef	__uint32_t					__size_t;
typedef	__int32_t					__ssize_t;
typedef	__int32_t					__time_t;
#endif

/* Register size */
typedef long						__register_t;
typedef	unsigned long					__uregister_t;

/* Wide character support types */
typedef	__int32_t					__wchar_t;
typedef __int32_t					__wint_t;
typedef	__int32_t					__rune_t;
typedef	void 						*__wctrans_t;
typedef	void 						*__wctype_t;

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
#define	SIG_ATOMIC_MIN			INT32_MIN
#define	SIG_ATOMIC_MAX			INT32_MAX

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

/*
 * 7.8.1 Macros for format specifiers
 */

/* fprintf macros for signed integers */

#define	PRId8		"d"	/* int8_t		*/
#define	PRId16		"d"	/* int16_t		*/
#define	PRId32		"d"	/* int32_t		*/
#define	PRId64		"lld"	/* int64_t		*/
#define	PRIdLEAST8	"d"	/* int_least8_t		*/
#define	PRIdLEAST16	"d"	/* int_least16_t	*/
#define	PRIdLEAST32	"d"	/* int_least32_t	*/
#define	PRIdLEAST64	"lld"	/* int_least64_t	*/
#define	PRIdFAST8	"d"	/* int_fast8_t		*/
#define	PRIdFAST16	"d"	/* int_fast16_t		*/
#define	PRIdFAST32	"d"	/* int_fast32_t		*/
#define	PRIdFAST64	"lld"	/* int_fast64_t		*/
#define	PRIdMAX		"lld"	/* intmax_t		*/
#define	PRIdPTR		"d"	/* intptr_t		*/

#define	PRIi8		"i"	/* int8_t		*/
#define	PRIi16		"i"	/* int16_t		*/
#define	PRIi32		"i"	/* int32_t		*/
#define	PRIi64		"lli"	/* int64_t		*/
#define	PRIiLEAST8	"i"	/* int_least8_t		*/
#define	PRIiLEAST16	"i"	/* int_least16_t	*/
#define	PRIiLEAST32	"i"	/* int_least32_t	*/
#define	PRIiLEAST64	"lli"	/* int_least64_t	*/
#define	PRIiFAST8	"i"	/* int_fast8_t		*/
#define	PRIiFAST16	"i"	/* int_fast16_t		*/
#define	PRIiFAST32	"i"	/* int_fast32_t		*/
#define	PRIiFAST64	"lli"	/* int_fast64_t		*/
#define	PRIiMAX		"lli"	/* intmax_t		*/
#define	PRIiPTR		"i"	/* intptr_t		*/

/* fprintf macros for unsigned integers */

#define	PRIo8		"o"	/* uint8_t		*/
#define	PRIo16		"o"	/* uint16_t		*/
#define	PRIo32		"o"	/* uint32_t		*/
#define	PRIo64		"llo"	/* uint64_t		*/
#define	PRIoLEAST8	"o"	/* uint_least8_t	*/
#define	PRIoLEAST16	"o"	/* uint_least16_t	*/
#define	PRIoLEAST32	"o"	/* uint_least32_t	*/
#define	PRIoLEAST64	"llo"	/* uint_least64_t	*/
#define	PRIoFAST8	"o"	/* uint_fast8_t		*/
#define	PRIoFAST16	"o"	/* uint_fast16_t	*/
#define	PRIoFAST32	"o"	/* uint_fast32_t	*/
#define	PRIoFAST64	"llo"	/* uint_fast64_t	*/
#define	PRIoMAX		"llo"	/* uintmax_t		*/
#define	PRIoPTR		"o"	/* uintptr_t		*/

#define	PRIu8		"u"	/* uint8_t		*/
#define	PRIu16		"u"	/* uint16_t		*/
#define	PRIu32		"u"	/* uint32_t		*/
#define	PRIu64		"llu"	/* uint64_t		*/
#define	PRIuLEAST8	"u"	/* uint_least8_t	*/
#define	PRIuLEAST16	"u"	/* uint_least16_t	*/
#define	PRIuLEAST32	"u"	/* uint_least32_t	*/
#define	PRIuLEAST64	"llu"	/* uint_least64_t	*/
#define	PRIuFAST8	"u"	/* uint_fast8_t		*/
#define	PRIuFAST16	"u"	/* uint_fast16_t	*/
#define	PRIuFAST32	"u"	/* uint_fast32_t	*/
#define	PRIuFAST64	"llu"	/* uint_fast64_t	*/
#define	PRIuMAX		"llu"	/* uintmax_t		*/
#define	PRIuPTR		"u"	/* uintptr_t		*/

#define	PRIx8		"x"	/* uint8_t		*/
#define	PRIx16		"x"	/* uint16_t		*/
#define	PRIx32		"x"	/* uint32_t		*/
#define	PRIx64		"llx"	/* uint64_t		*/
#define	PRIxLEAST8	"x"	/* uint_least8_t	*/
#define	PRIxLEAST16	"x"	/* uint_least16_t	*/
#define	PRIxLEAST32	"x"	/* uint_least32_t	*/
#define	PRIxLEAST64	"llx"	/* uint_least64_t	*/
#define	PRIxFAST8	"x"	/* uint_fast8_t		*/
#define	PRIxFAST16	"x"	/* uint_fast16_t	*/
#define	PRIxFAST32	"x"	/* uint_fast32_t	*/
#define	PRIxFAST64	"llx"	/* uint_fast64_t	*/
#define	PRIxMAX		"llx"	/* uintmax_t		*/
#define	PRIxPTR		"x"	/* uintptr_t		*/

#define	PRIX8		"X"	/* uint8_t		*/
#define	PRIX16		"X"	/* uint16_t		*/
#define	PRIX32		"X"	/* uint32_t		*/
#define	PRIX64		"llX"	/* uint64_t		*/
#define	PRIXLEAST8	"X"	/* uint_least8_t	*/
#define	PRIXLEAST16	"X"	/* uint_least16_t	*/
#define	PRIXLEAST32	"X"	/* uint_least32_t	*/
#define	PRIXLEAST64	"llX"	/* uint_least64_t	*/
#define	PRIXFAST8	"X"	/* uint_fast8_t		*/
#define	PRIXFAST16	"X"	/* uint_fast16_t	*/
#define	PRIXFAST32	"X"	/* uint_fast32_t	*/
#define	PRIXFAST64	"llX"	/* uint_fast64_t	*/
#define	PRIXMAX		"llX"	/* uintmax_t		*/
#define	PRIXPTR		"X"	/* uintptr_t		*/

/* fscanf macros for signed integers */

#define	SCNd8		"hhd"	/* int8_t		*/
#define	SCNd16		"hd"	/* int16_t		*/
#define	SCNd32		"d"	/* int32_t		*/
#define	SCNd64		"lld"	/* int64_t		*/
#define	SCNdLEAST8	"hhd"	/* int_least8_t		*/
#define	SCNdLEAST16	"hd"	/* int_least16_t	*/
#define	SCNdLEAST32	"d"	/* int_least32_t	*/
#define	SCNdLEAST64	"lld"	/* int_least64_t	*/
#define	SCNdFAST8	"d"	/* int_fast8_t		*/
#define	SCNdFAST16	"d"	/* int_fast16_t		*/
#define	SCNdFAST32	"d"	/* int_fast32_t		*/
#define	SCNdFAST64	"lld"	/* int_fast64_t		*/
#define	SCNdMAX		"lld"	/* intmax_t		*/
#define	SCNdPTR		"d"	/* intptr_t		*/

#define	SCNi8		"hhi"	/* int8_t		*/
#define	SCNi16		"hi"	/* int16_t		*/
#define	SCNi32		"i"	/* int32_t		*/
#define	SCNi64		"lli"	/* int64_t		*/
#define	SCNiLEAST8	"hhi"	/* int_least8_t		*/
#define	SCNiLEAST16	"hi"	/* int_least16_t	*/
#define	SCNiLEAST32	"i"	/* int_least32_t	*/
#define	SCNiLEAST64	"lli"	/* int_least64_t	*/
#define	SCNiFAST8	"i"	/* int_fast8_t		*/
#define	SCNiFAST16	"i"	/* int_fast16_t		*/
#define	SCNiFAST32	"i"	/* int_fast32_t		*/
#define	SCNiFAST64	"lli"	/* int_fast64_t		*/
#define	SCNiMAX		"lli"	/* intmax_t		*/
#define	SCNiPTR		"i"	/* intptr_t		*/

/* fscanf macros for unsigned integers */

#define	SCNo8		"hho"	/* uint8_t		*/
#define	SCNo16		"ho"	/* uint16_t		*/
#define	SCNo32		"o"	/* uint32_t		*/
#define	SCNo64		"llo"	/* uint64_t		*/
#define	SCNoLEAST8	"hho"	/* uint_least8_t	*/
#define	SCNoLEAST16	"ho"	/* uint_least16_t	*/
#define	SCNoLEAST32	"o"	/* uint_least32_t	*/
#define	SCNoLEAST64	"llo"	/* uint_least64_t	*/
#define	SCNoFAST8	"o"	/* uint_fast8_t		*/
#define	SCNoFAST16	"o"	/* uint_fast16_t	*/
#define	SCNoFAST32	"o"	/* uint_fast32_t	*/
#define	SCNoFAST64	"llo"	/* uint_fast64_t	*/
#define	SCNoMAX		"llo"	/* uintmax_t		*/
#define	SCNoPTR		"o"	/* uintptr_t		*/

#define	SCNu8		"hhu"	/* uint8_t		*/
#define	SCNu16		"hu"	/* uint16_t		*/
#define	SCNu32		"u"	/* uint32_t		*/
#define	SCNu64		"llu"	/* uint64_t		*/
#define	SCNuLEAST8	"hhu"	/* uint_least8_t	*/
#define	SCNuLEAST16	"hu"	/* uint_least16_t	*/
#define	SCNuLEAST32	"u"	/* uint_least32_t	*/
#define	SCNuLEAST64	"llu"	/* uint_least64_t	*/
#define	SCNuFAST8	"u"	/* uint_fast8_t		*/
#define	SCNuFAST16	"u"	/* uint_fast16_t	*/
#define	SCNuFAST32	"u"	/* uint_fast32_t	*/
#define	SCNuFAST64	"llu"	/* uint_fast64_t	*/
#define	SCNuMAX		"llu"	/* uintmax_t		*/
#define	SCNuPTR		"u"	/* uintptr_t		*/

#define	SCNx8		"hhx"	/* uint8_t		*/
#define	SCNx16		"hx"	/* uint16_t		*/
#define	SCNx32		"x"	/* uint32_t		*/
#define	SCNx64		"llx"	/* uint64_t		*/
#define	SCNxLEAST8	"hhx"	/* uint_least8_t	*/
#define	SCNxLEAST16	"hx"	/* uint_least16_t	*/
#define	SCNxLEAST32	"x"	/* uint_least32_t	*/
#define	SCNxLEAST64	"llx"	/* uint_least64_t	*/
#define	SCNxFAST8	"x"	/* uint_fast8_t		*/
#define	SCNxFAST16	"x"	/* uint_fast16_t	*/
#define	SCNxFAST32	"x"	/* uint_fast32_t	*/
#define	SCNxFAST64	"llx"	/* uint_fast64_t	*/
#define	SCNxMAX		"llx"	/* uintmax_t		*/
#define	SCNxPTR		"x"	/* uintptr_t		*/

#endif /* _I386_MACHTYPES_H_ */
