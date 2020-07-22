/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2011 Ed Schouten <ed@FreeBSD.org>
 *                    David Chisnall <theraven@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _STDATOMIC_H_
#define	_STDATOMIC_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#if __has_extension(c_atomic) || __has_extension(cxx_atomic)
#define	__CLANG_ATOMICS
#elif __GNUC_PREREQ__(4, 7)
#define	__GNUC_ATOMICS
#elif defined(__GNUC__)
#define	__SYNC_ATOMICS
#else
#error "stdatomic.h does not support your compiler"
#endif

/*
 * 7.17.1 Atomic lock-free macros.
 */

#ifdef __GCC_ATOMIC_BOOL_LOCK_FREE
#define	ATOMIC_BOOL_LOCK_FREE		__GCC_ATOMIC_BOOL_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_CHAR_LOCK_FREE
#define	ATOMIC_CHAR_LOCK_FREE		__GCC_ATOMIC_CHAR_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_CHAR16_T_LOCK_FREE
#define	ATOMIC_CHAR16_T_LOCK_FREE	__GCC_ATOMIC_CHAR16_T_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_CHAR32_T_LOCK_FREE
#define	ATOMIC_CHAR32_T_LOCK_FREE	__GCC_ATOMIC_CHAR32_T_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_WCHAR_T_LOCK_FREE
#define	ATOMIC_WCHAR_T_LOCK_FREE	__GCC_ATOMIC_WCHAR_T_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_SHORT_LOCK_FREE
#define	ATOMIC_SHORT_LOCK_FREE		__GCC_ATOMIC_SHORT_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_INT_LOCK_FREE
#define	ATOMIC_INT_LOCK_FREE		__GCC_ATOMIC_INT_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_LONG_LOCK_FREE
#define	ATOMIC_LONG_LOCK_FREE		__GCC_ATOMIC_LONG_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_LLONG_LOCK_FREE
#define	ATOMIC_LLONG_LOCK_FREE		__GCC_ATOMIC_LLONG_LOCK_FREE
#endif
#ifdef __GCC_ATOMIC_POINTER_LOCK_FREE
#define	ATOMIC_POINTER_LOCK_FREE	__GCC_ATOMIC_POINTER_LOCK_FREE
#endif

/*
 * 7.17.2 Initialization.
 */

#if defined(__CLANG_ATOMICS)
#define	ATOMIC_VAR_INIT(value)		(value)
#define	atomic_init(obj, value)		__c11_atomic_init(obj, value)
#else
#define	ATOMIC_VAR_INIT(value)		{ .__val = (value) }
#define	atomic_init(obj, value)		((void)((obj)->__val = (value)))
#endif

/*
 * Clang and recent GCC both provide predefined macros for the memory
 * orderings.  If we are using a compiler that doesn't define them, use the
 * clang values - these will be ignored in the fallback path.
 */

#ifndef __ATOMIC_RELAXED
#define __ATOMIC_RELAXED		0
#endif
#ifndef __ATOMIC_CONSUME
#define __ATOMIC_CONSUME		1
#endif
#ifndef __ATOMIC_ACQUIRE
#define __ATOMIC_ACQUIRE		2
#endif
#ifndef __ATOMIC_RELEASE
#define __ATOMIC_RELEASE		3
#endif
#ifndef __ATOMIC_ACQ_REL
#define __ATOMIC_ACQ_REL		4
#endif
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST		5
#endif

/*
 * 7.17.3 Order and consistency.
 *
 * The memory_order_* constants that denote the barrier behaviour of the
 * atomic operations.
 */

typedef enum {
	memory_order_relaxed = __ATOMIC_RELAXED,
	memory_order_consume = __ATOMIC_CONSUME,
	memory_order_acquire = __ATOMIC_ACQUIRE,
	memory_order_release = __ATOMIC_RELEASE,
	memory_order_acq_rel = __ATOMIC_ACQ_REL,
	memory_order_seq_cst = __ATOMIC_SEQ_CST
} memory_order;

/*
 * 7.17.4 Fences.
 */

static __inline void
atomic_thread_fence(memory_order __order __unused)
{

#ifdef __CLANG_ATOMICS
	__c11_atomic_thread_fence(__order);
#elif defined(__GNUC_ATOMICS)
	__atomic_thread_fence(__order);
#else
	__sync_synchronize();
#endif
}

static __inline void
atomic_signal_fence(memory_order __order __unused)
{

#ifdef __CLANG_ATOMICS
	__c11_atomic_signal_fence(__order);
#elif defined(__GNUC_ATOMICS)
	__atomic_signal_fence(__order);
#else
	__asm volatile ("" ::: "memory");
#endif
}

/*
 * 7.17.5 Lock-free property.
 */

#if defined(_KERNEL)
/* Atomics in kernelspace are always lock-free. */
#define	atomic_is_lock_free(obj) \
	((void)(obj), (_Bool)1)
#elif defined(__CLANG_ATOMICS) || defined(__GNUC_ATOMICS)
#define	atomic_is_lock_free(obj) \
	__atomic_is_lock_free(sizeof(*(obj)), obj)
#else
#define	atomic_is_lock_free(obj) \
	((void)(obj), sizeof((obj)->__val) <= sizeof(void *))
#endif

/*
 * 7.17.6 Atomic integer types.
 */

typedef _Atomic(_Bool)				atomic_bool;
typedef _Atomic(char)				atomic_char;
typedef _Atomic(signed char)		atomic_schar;
typedef _Atomic(unsigned char)		atomic_uchar;
typedef _Atomic(short)				atomic_short;
typedef _Atomic(unsigned short)		atomic_ushort;
typedef _Atomic(int)				atomic_int;
typedef _Atomic(unsigned int)		atomic_uint;
typedef _Atomic(long)				atomic_long;
typedef _Atomic(unsigned long)		atomic_ulong;
typedef _Atomic(long long)			atomic_llong;
typedef _Atomic(unsigned long long)	atomic_ullong;
typedef _Atomic(__char16_t)			atomic_char16_t;
typedef _Atomic(__char32_t)			atomic_char32_t;
typedef _Atomic(___wchar_t)			atomic_wchar_t;
typedef _Atomic(__int_least8_t)		atomic_int_least8_t;
typedef _Atomic(__uint_least8_t)	atomic_uint_least8_t;
typedef _Atomic(__int_least16_t)	atomic_int_least16_t;
typedef _Atomic(__uint_least16_t)	atomic_uint_least16_t;
typedef _Atomic(__int_least32_t)	atomic_int_least32_t;
typedef _Atomic(__uint_least32_t)	atomic_uint_least32_t;
typedef _Atomic(__int_least64_t)	atomic_int_least64_t;
typedef _Atomic(__uint_least64_t)	atomic_uint_least64_t;
typedef _Atomic(__int_fast8_t)		atomic_int_fast8_t;
typedef _Atomic(__uint_fast8_t)		atomic_uint_fast8_t;
typedef _Atomic(__int_fast16_t)		atomic_int_fast16_t;
typedef _Atomic(__uint_fast16_t)	atomic_uint_fast16_t;
typedef _Atomic(__int_fast32_t)		atomic_int_fast32_t;
typedef _Atomic(__uint_fast32_t)	atomic_uint_fast32_t;
typedef _Atomic(__int_fast64_t)		atomic_int_fast64_t;
typedef _Atomic(__uint_fast64_t)	atomic_uint_fast64_t;
typedef _Atomic(__intptr_t)			atomic_intptr_t;
typedef _Atomic(__uintptr_t)		atomic_uintptr_t;
typedef _Atomic(__size_t)			atomic_size_t;
typedef _Atomic(__ptrdiff_t)		atomic_ptrdiff_t;
typedef _Atomic(__intmax_t)			atomic_intmax_t;
typedef _Atomic(__uintmax_t)		atomic_uintmax_t;
