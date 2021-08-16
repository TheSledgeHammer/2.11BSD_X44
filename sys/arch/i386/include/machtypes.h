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
	int 	r[1];
} *physadr;

typedef struct label_t {
	int 	val[6];
} label_t;
#endif

typedef	unsigned long					vm_offset_t;
typedef	unsigned long					vm_size_t;

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
#ifdef __COMPILER_INT64__
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
#ifdef __COMPILER_INT64__
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
#ifdef __COMPILER_INT64__
typedef	__COMPILER_INT64__	  			int_fast64_t;
typedef	__COMPILER_UINT64__	 			uint_fast64_t;
#else
/* LONGLONG */
typedef	long long int		  			int_fast64_t;
/* LONGLONG */
typedef	unsigned long long int	 		uint_fast64_t;
#endif

/* 7.18.1.4 Integer types capable of holding object pointers */

typedef	int		             			__intptr_t;
typedef	unsigned int	      			__uintptr_t;

typedef	long							__intptr_t;
typedef	unsigned long					__uintptr_t;

/* 7.18.1.5 Greatest-width integer types */

#ifdef __COMPILER_INT64__
typedef	__COMPILER_INT64__	      		intmax_t;
typedef	unsigned __COMPILER_INT64__ 	uintmax_t;
#else
/* LONGLONG */
typedef	long long int		      		intmax_t;
/* LONGLONG */
typedef	unsigned long long int	  		uintmax_t;
#endif

#define	__BIT_TYPES_DEFINED__

/* Register size */
typedef long							__register_t;
typedef	unsigned long					__uregister_t;

/* VM system types */
typedef unsigned long					__vaddr_t;
typedef unsigned long					__paddr_t;
typedef unsigned long					__vsize_t;
typedef unsigned long					__psize_t;

typedef	__int64_t						__ptrdiff_t;	/* ptr1 - ptr2 */
typedef	__int64_t						__register_t;
typedef	__int64_t						__segsz_t;		/* segment size (in pages) */
typedef	__uint64_t						__size_t;		/* sizeof() */
typedef	__int64_t						__ssize_t;		/* byte count or error */
typedef	__int64_t						__time_t;		/* time()... */
typedef	__uint64_t						__uintfptr_t;
typedef	__uint64_t						__uintptr_t;

typedef	__int32_t						__ptrdiff_t;
typedef	__int32_t						__register_t;
typedef	__int32_t						__segsz_t;
typedef	__uint32_t						__size_t;
typedef	__int32_t						__ssize_t;
typedef	__int32_t						__time_t;
typedef	__uint32_t						__uintfptr_t;
typedef	__uint32_t						__uintptr_t;

typedef	int								__wchar_t;
typedef int								__wint_t;
typedef	int								__rune_t;
typedef	void *							__wctrans_t;
typedef	void *							__wctype_t;

/*
 * 7.18.4 Macros for integer constants
 */

/* 7.18.4.1 Macros for minimum-width integer constants */

#define	INT8_C(c)		c
#define	INT16_C(c)		c
#define	INT32_C(c)		c
#define	INT64_C(c)		c ## LL

#define	UINT8_C(c)		c
#define	UINT16_C(c)		c
#define	UINT32_C(c)		c ## U
#define	UINT64_C(c)		c ## ULL

/* 7.18.4.2 Macros for greatest-width integer constants */

#define	INTMAX_C(c)		c ## LL
#define	UINTMAX_C(c)	c ## ULL

#endif /* _MACHTYPES_H_ */
