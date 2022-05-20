/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Berkeley Software Design, Inc.
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
 *	@(#)cdefs.h	8.8 (Berkeley) 1/9/95
 */

#ifndef	_SYS_CDEFS_H_
#define	_SYS_CDEFS_H_

/*
 * Macro to test if we're using a GNU C compiler of a specific vintage
 * or later, for e.g. features that appeared in a particular version
 * of GNU C.  Usage:
 *
 *	#if __GNUC_PREREQ__(major, minor)
 *	...cool feature...
 *	#else
 *	...delete feature...
 *	#endif
 */
#ifdef __GNUC__
#define	__GNUC_PREREQ__(x, y)								\
	((__GNUC__ == (x) && __GNUC_MINOR__ >= (y)) ||			\
	 (__GNUC__ > (x)))
#else
#define	__GNUC_PREREQ__(x, y)	0
#endif

/*
 * Macros to test Clang/LLVM features.
 * Testing against Clang-specific extensions.
 */
#ifndef	__has_attribute
#define	__has_attribute(x)	0
#endif
#ifndef	__has_extension
#define	__has_extension		__has_feature
#endif
#ifndef	__has_feature
#define	__has_feature(x)	0
#endif
#ifndef	__has_include
#define	__has_include(x)	0
#endif
#ifndef	__has_builtin
#define	__has_builtin(x)	0
#endif

#ifdef __ELF__
#include <sys/cdefs_elf.h>
#else
#include <sys/cdefs_aout.h>
#endif

#if defined(__cplusplus)
#define	__BEGIN_DECLS	extern "C" {
#define	__END_DECLS		};
#else
#define	__BEGIN_DECLS
#define	__END_DECLS
#endif

#ifdef __GNUC__
#define	__strict_weak_alias(alias,sym)								\
	__unused static __typeof__(alias) *__weak_alias_##alias = &sym;	\
	__weak_alias(alias,sym)
#else
#define	__strict_weak_alias(alias,sym) __weak_alias(alias,sym)
#endif

/*
 * The __CONCAT macro is used to concatenate parts of symbol names, e.g.
 * with "#define OLD(foo) __CONCAT(old,foo)", OLD(foo) produces oldfoo.
 * The __CONCAT macro is a bit tricky -- make sure you don't put spaces
 * in between its arguments.  __CONCAT can also concatenate double-quoted
 * strings produced by the __STRING macro, but this only works with ANSI C.
 */

#define	___STRING(x)	__STRING(x)
#define	___CONCAT(x,y)	__CONCAT(x,y)

#if defined(__STDC__) || defined(__cplusplus)
#define	__P(protos)		protos		/* full-blown ANSI C */
#define	__CONCAT(x,y)	x##y
#define	__STRING(x)		#x

#define	__const			const		/* define reserved names to standard */
#define	__signed		signed
#define	__volatile		volatile

#define	__CONCAT3(a,b,c)			a ## b ## c
#define	__CONCAT4(a,b,c,d)			a ## b ## c ## d
#define	__CONCAT5(a,b,c,d,e)		a ## b ## c ## d ## e
#define	__CONCAT6(a,b,c,d,e,f)		a ## b ## c ## d ## e ## f
#define	__CONCAT7(a,b,c,d,e,f,g)	a ## b ## c ## d ## e ## f ## g
#define	__CONCAT8(a,b,c,d,e,f,g,h)	a ## b ## c ## d ## e ## f ## g ## h

#if defined(__cplusplus) || defined(__PCC__)
#define	__inline	inline			/* convert to C++/C99 keyword */
#else
#if !defined(__GNUC__) && !defined(__lint__)
#define	__inline					/* delete GCC keyword */
#endif /* !__GNUC__  && !__lint__ */
#endif /* !__cplusplus */

#else	/* !(__STDC__ || __cplusplus) */
#define	__P(protos)		()			/* traditional C preprocessor */
#define	__CONCAT(x,y)	x/**/y
#define	__STRING(x)		"x"

#ifndef __GNUC__
#define	__const						/* delete pseudo-ANSI C keywords */
#define	__inline
#define	__signed
#define	__volatile
#endif	/* !__GNUC__ */

/*
 * In non-ANSI C environments, new programs will want ANSI-only C keywords
 * deleted from the program and old programs will want them left alone.
 * When using a compiler other than gcc, programs using the ANSI C keywords
 * const, inline etc. as normal identifiers should define -DNO_ANSI_KEYWORDS.
 * When using "gcc -traditional", we assume that this is the intent; if
 * __GNUC__ is defined but __STDC__ is not, we leave the new keywords alone.
 */
#ifndef	NO_ANSI_KEYWORDS
#define	const		__const			/* convert ANSI C keywords */
#define	inline		__inline
#define	signed		__signed
#define	volatile	__volatile
#endif /* !NO_ANSI_KEYWORDS */
#endif	/* !(__STDC__ || __cplusplus) */

/*
 * Used for internal auditing of the NetBSD source tree.
 */
#ifdef __AUDIT__
#define	__aconst	__const
#else
#define	__aconst
#endif

/*
 * The following macro is used to remove const cast-away warnings
 * from gcc -Wcast-qual; it should be used with caution because it
 * can hide valid errors; in particular most valid uses are in
 * situations where the API requires it, not to cast away string
 * constants. We don't use *intptr_t on purpose here and we are
 * explicit about unsigned long so that we don't have additional
 * dependencies.
 */
#define __UNCONST(a)	((void *)(unsigned long)(const void *)(a))

/*
 * Compile Time Assertion.
 */
#ifdef __COUNTER__
#define	__CTASSERT(x)			__CTASSERT0(x, __ctassert, __COUNTER__)
#else
#define	__CTASSERT(x)			__CTASSERT99(x, __INCLUDE_LEVEL__, __LINE__)
#define	__CTASSERT99(x, a, b)	__CTASSERT0(x, __CONCAT(__ctassert,a), \
					       	   	   	   	   	   __CONCAT(_,b))
#endif
#define	__CTASSERT0(x, y, z)	__CTASSERT1(x, y, z)
#define	__CTASSERT1(x, y, z)	\
	typedef struct { \
		unsigned int y ## z : /*CONSTCOND*/(x) ? 1 : -1; \
	} y ## z ## _struct __unused

/*
 * GCC1 and some versions of GCC2 declare dead (non-returning) and
 * pure (no side effects) functions using "volatile" and "const";
 * unfortunately, these then cause warnings under "-ansi -pedantic".
 * GCC2 uses a new, peculiar __attribute__((attrs)) style.  All of
 * these work for GNU C++ (modulo a slight glitch in the C++ grammar
 * in the distribution version of 2.5.5).
 */
#if !defined(__GNUC__) || __GNUC__ < 2 || \
	(__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#define	__attribute__(x)	/* delete __attribute__ if non-gcc or gcc1 */
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#define	__dead		__volatile
#define	__pure		__const
#endif
#endif

/* Delete pseudo-keywords wherever they are not available or needed. */
#ifndef __dead
#define	__dead
#define	__pure
#endif

#if defined(__lint__)
#define	__packed		__packed
#define	__aligned(x)	/* delete */
#define	__section(x)	/* delete */
#elif defined(__PCC__) || defined(__lint__)
#define	__packed		__attribute__((__packed__))
#define	__aligned(x)	__attribute__((__aligned__(x)))
#define	__section(x)	__attribute__((__section__(x)))
#endif

#if defined(_KERNEL)
#if defined(NO_KERNEL_RCSIDS)
#undef __KERNEL_RCSID
#define	__KERNEL_RCSID(_n, _s)		/* nothing */
#endif /* NO_KERNEL_RCSIDS */
#endif /* _KERNEL */

/*
 * __unused: Note that item or function might be unused.
 */
#if __GNUC_PREREQ__(2, 7) || defined(__lint__)
#define	__unused	__attribute__((__unused__))
#else
#define	__unused	/* delete */
#endif

/*
 * __used: Note that item is needed, even if it appears to be unused.
 */
#if __GNUC_PREREQ__(3, 1)
#define	__used		__attribute__((__used__))
#else
#define	__used		__unused
#endif

#if __GNUC_PREREQ__(2, 96) || defined(__lint__)
#define	__predict_true(exp)		__builtin_expect((exp) != 0, 1)
#define	__predict_false(exp)	__builtin_expect((exp) != 0, 0)
#else
#define	__predict_true(exp)		(exp)
#define	__predict_false(exp)	(exp)
#endif

/*
 * Compiler-dependent macros to declare that functions take printf-like
 * or scanf-like arguments.  They are null except for versions of gcc
 * that are known to support the features properly (old versions of gcc-2
 * didn't permit keeping the keywords out of the application namespace).
 */
#if __GNUC_PREREQ__(2, 7) || defined(__lint__)
#define __printflike(fmtarg, firstvararg)	\
	    __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#ifndef __syslog_attribute__
#define __syslog__ __printf__
#endif
#define __sysloglike(fmtarg, firstvararg)	\
	    __attribute__((__format__ (__syslog__, fmtarg, firstvararg)))
#define __scanflike(fmtarg, firstvararg)	\
	    __attribute__((__format__ (__scanf__, fmtarg, firstvararg)))
#define __format_arg(fmtarg)    __attribute__((__format_arg__ (fmtarg)))
#else
#define __printflike(fmtarg, firstvararg)	/* nothing */
#define __scanflike(fmtarg, firstvararg)	/* nothing */
#define __sysloglike(fmtarg, firstvararg)	/* nothing */
#define __format_arg(fmtarg)				/* nothing */
#endif

/*
 * Finally deal with BSD-specific interfaces that are not covered
 * by any standards.  We expose these when none of the POSIX or XPG
 * macros is defined or if the user explicitly asks for them.
 */
#if !defined(_BSD_SOURCE) && \
   (defined(_ANSI_SOURCE) || defined(__XPG_VISIBLE) || defined(__POSIX_VISIBLE))
# define __BSD_VISIBLE		0
#endif

/*
 * Keywords added in C11.
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201112L

#if !defined(__cplusplus) && !__has_extension(c_atomic) && \
	!__has_extension(cxx_atomic) && !__GNUC_PREREQ__(4, 7)
/*
 * No native support for _Atomic(). Place object in structure to prevent
 * most forms of direct non-atomic access.
 */
#define	_Atomic(T)		struct { T volatile __val; }
#endif
#endif
#endif /* !_SYS_CDEFS_H_ */
