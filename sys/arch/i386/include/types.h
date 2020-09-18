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

#ifndef	_MACHTYPES_H_
#define	_MACHTYPES_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#if !defined(_ANSI_SOURCE)
typedef struct _physadr {
	int r[1];
} *physadr;

typedef struct label_t {
	int val[6];
} label_t;
#endif

typedef	unsigned long	vm_offset_t;
typedef	unsigned long	vm_size_t;

/*
 * Basic integral types.  Omit the typedef if
 * not possible for a machine/compiler combination.
 */
/* 7.18.1.1 Exact-width integer types */
typedef	signed char			__int8_t;
typedef	unsigned char		__uint8_t;
typedef	short				__int16_t;
typedef	unsigned short		__uint16_t;
typedef	int					__int32_t;
typedef	unsigned int		__uint32_t;
typedef	long long			__int64_t;
typedef	unsigned long long	__uint64_t;

/* 7.18.1.2 Minimum-width integer types */
typedef	__int8_t			__int_least8_t;
typedef	__uint8_t			__uint_least8_t;
typedef	__int16_t			__int_least16_t;
typedef	__uint16_t			__uint_least16_t;
typedef	__int32_t			__int_least32_t;
typedef	__uint32_t			__uint_least32_t;
typedef	__int64_t			__int_least64_t;
typedef	__uint64_t			__uint_least64_t;

/* 7.18.1.3 Fastest minimum-width integer types */
typedef	__int32_t			__int_fast8_t;
typedef	__uint32_t			__uint_fast8_t;
typedef	__int32_t			__int_fast16_t;
typedef	__uint32_t			__uint_fast16_t;
typedef	__int32_t			__int_fast32_t;
typedef	__uint32_t			__uint_fast32_t;
typedef	__int64_t			__int_fast64_t;
typedef	__uint64_t			__uint_fast64_t;
#define	__INT_FAST8_MIN		INT32_MIN
#define	__INT_FAST16_MIN	INT32_MIN
#define	__INT_FAST32_MIN	INT32_MIN
#define	__INT_FAST64_MIN	INT64_MIN
#define	__INT_FAST8_MAX		INT32_MAX
#define	__INT_FAST16_MAX	INT32_MAX
#define	__INT_FAST32_MAX	INT32_MAX
#define	__INT_FAST64_MAX	INT64_MAX
#define	__UINT_FAST8_MAX	UINT32_MAX
#define	__UINT_FAST16_MAX	UINT32_MAX
#define	__UINT_FAST32_MAX	UINT32_MAX
#define	__UINT_FAST64_MAX	UINT64_MAX

/* 7.18.1.4 Integer types capable of holding object pointers */
typedef	long				__intptr_t;
typedef	unsigned long		__uintptr_t;

/* 7.18.1.5 Greatest-width integer types */
typedef	__int64_t			__intmax_t;
typedef	__uint64_t			__uintmax_t;

/* Register size */
typedef long				__register_t;
typedef	unsigned long		__uregister_t;

/* VM system types */
typedef unsigned long		__vaddr_t;
typedef unsigned long		__paddr_t;
typedef unsigned long		__vsize_t;
typedef unsigned long		__psize_t;


/* 7.18.1.4 Integer types capable of holding object pointers */

typedef	int		       		__intptr_t;
typedef	unsigned int	    __uintptr_t;

typedef	__int64_t			__ptrdiff_t;	/* ptr1 - ptr2 */
typedef	__int64_t			__register_t;
typedef	__int64_t			__segsz_t;		/* segment size (in pages) */
typedef	__uint64_t			__size_t;		/* sizeof() */
typedef	__int64_t			__ssize_t;		/* byte count or error */
typedef	__int64_t			__time_t;		/* time()... */
typedef	__uint64_t			__uintfptr_t;
typedef	__uint64_t			__uintptr_t;

typedef	__int32_t			__ptrdiff_t;
typedef	__int32_t			__register_t;
typedef	__int32_t			__segsz_t;
typedef	__uint32_t			__size_t;
typedef	__int32_t			__ssize_t;
typedef	__int32_t			__time_t;
typedef	__uint32_t			__uintfptr_t;
typedef	__uint32_t			__uintptr_t;

typedef	int					___wchar_t;

#define	__WCHAR_MIN			__INT_MIN	/* min value for a wchar_t */
#define	__WCHAR_MAX			__INT_MAX	/* max value for a wchar_t */

/* Clang already provides these types as built-ins, but only in C++ mode. */
#if !defined(__clang__) || !defined(__cplusplus)
typedef	__uint_least16_t 	__char16_t;
typedef	__uint_least32_t 	__char32_t;
#endif

#endif /* _MACHTYPES_H_ */
