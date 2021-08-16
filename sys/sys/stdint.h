/*	$NetBSD: stdint.h,v 1.8 2018/11/06 16:26:44 maya Exp $	*/

/*-
 * Copyright (c) 2001, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_STDINT_H_
#define _SYS_STDINT_H_

#include <sys/cdefs.h>

#include <machine/machtypes.h>

#ifndef	_BSD_INT8_T_
typedef	__int8_t			int8_t;
#define	_BSD_INT8_T_
#endif

#ifndef	_BSD_UINT8_T_
typedef	__uint8_t			uint8_t;
#define	_BSD_UINT8_T_
#endif

#ifndef	_BSD_INT16_T_
typedef	__int16_t			int16_t;
#define	_BSD_INT16_T_
#endif

#ifndef	_BSD_UINT16_T_
typedef	__uint16_t			uint16_t;
#define	_BSD_UINT16_T_
#endif

#ifndef	_BSD_INT32_T_
typedef	__int32_t			int32_t;
#define	_BSD_INT32_T_
#endif

#ifndef	_BSD_UINT32_T_
typedef	__uint32_t			uint32_t;
#define	_BSD_UINT32_T_
#endif

#ifndef	_BSD_INT64_T_
typedef	__int64_t			int64_t;
#define	_BSD_INT64_T_
#endif

#ifndef	_BSD_UINT64_T_
typedef	__uint64_t			uint64_t;
#define	_BSD_UINT64_T_
#endif

#ifndef	_BSD_INTPTR_T_
typedef	__intptr_t			intptr_t;
#define	_BSD_INTPTR_T_
#endif

#ifndef	_BSD_UINTPTR_T_
typedef	__uintptr_t			uintptr_t;
#define	_BSD_UINTPTR_T_
#endif

#ifndef _BSD_REGISTER_T_
typedef	__register_t 		register_t;
#define _BSD_REGISTER_T_
#endif

#ifndef _BSD_UREGISTER_T_
typedef	__uregister_t 		uregister_t;
#define _BSD_UREGISTER_T_
#endif

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

#endif /* !_SYS_STDINT_H_ */
