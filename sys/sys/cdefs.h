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
 * GCC2 provides __extension__ to suppress warnings for various GNU C
 * language extensions under "-ansi -pedantic".
 */
#if !__GNUC_PREREQ__(2, 0)
#define	__extension__		/* delete __extension__ if non-gcc or gcc1 */
#endif

/*
 * GCC1 and some versions of GCC2 declare dead (non-returning) and
 * pure (no side effects) functions using "volatile" and "const";
 * unfortunately, these then cause warnings under "-ansi -pedantic".
 * GCC2 uses a new, peculiar __attribute__((attrs)) style.  All of
 * these work for GNU C++ (modulo a slight glitch in the C++ grammar
 * in the distribution version of 2.5.5).
 */
#if !__GNUC_PREREQ__(2, 0) && !defined(__lint__)
#define __attribute__(x)
#endif

#if __GNUC_PREREQ__(2, 5) || defined(__lint__)
#define	__dead		__attribute__((__noreturn__))
#elif defined(__GNUC__)
#define	__dead		__volatile
#else
#define	__dead
#endif

#if __GNUC_PREREQ__(2, 96) || defined(__lint__)
#define	__pure		__attribute__((__pure__))
#elif defined(__GNUC__)
#define	__pure		__const
#else
#define	__pure
#endif

#if __GNUC_PREREQ__(4, 0) || defined(__lint__)
#define	__null_sentinel	__attribute__((__sentinel__))
#else
#define	__null_sentinel	/* nothing */
#endif

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
#if __GNUC_PREREQ__(3, 1) || defined(__lint__)
#define	__used		__attribute__((__used__))
#else
#define	__used		__unused
#endif

/*
 * __diagused: Note that item is used in diagnostic code, but may be
 * unused in non-diagnostic code.
 */
#if (defined(_KERNEL) && defined(DIAGNOSTIC)) \
 || (!defined(_KERNEL) && !defined(NDEBUG))
#define	__diagused	/* empty */
#else
#define	__diagused	__unused
#endif

/*
 * To be used when an empty body is required like:
 *
 * #ifdef DEBUG
 * # define dprintf(a) printf(a)
 * #else
 * # define dprintf(a) __nothing
 * #endif
 *
 * We use ((void)0) instead of do {} while (0) so that it
 * works on , expressions.
 */
#define __nothing	(/*LINTED*/(void)0)

#if defined(__cplusplus)
#define	__BEGIN_EXTERN_C	extern "C" {
#define	__END_EXTERN_C		}
#define	__static_cast(x,y)	static_cast<x>(y)
#else
#define	__BEGIN_EXTERN_C
#define	__END_EXTERN_C
#define	__static_cast(x,y)	(x)y
#endif

#if __GNUC_PREREQ__(4, 0) || defined(__lint__)
#  define __dso_public	__attribute__((__visibility__("default")))
#  define __dso_hidden	__attribute__((__visibility__("hidden")))
#  define __BEGIN_PUBLIC_DECLS	\
	_Pragma("GCC visibility push(default)") __BEGIN_EXTERN_C
#  define __END_PUBLIC_DECLS	__END_EXTERN_C _Pragma("GCC visibility pop")
#  define __BEGIN_HIDDEN_DECLS	\
	_Pragma("GCC visibility push(hidden)") __BEGIN_EXTERN_C
#  define __END_HIDDEN_DECLS	__END_EXTERN_C _Pragma("GCC visibility pop")
#else
#  define __dso_public
#  define __dso_hidden
#  define __BEGIN_PUBLIC_DECLS	__BEGIN_EXTERN_C
#  define __END_PUBLIC_DECLS	__END_EXTERN_C
#  define __BEGIN_HIDDEN_DECLS	__BEGIN_EXTERN_C
#  define __END_HIDDEN_DECLS	__END_EXTERN_C
#endif
#if __GNUC_PREREQ__(4, 2) || defined(__lint__)
#  define __dso_protected	__attribute__((__visibility__("protected")))
#else
#  define __dso_protected
#endif

/*
 * Non-static C99 inline functions are optional bodies.  They don't
 * create global symbols if not used, but can be replaced if desirable.
 * This differs from the behavior of GCC before version 4.3.  The nearest
 * equivalent for older GCC is `extern inline'.  For newer GCC, use the
 * gnu_inline attribute additionally to get the old behavior.
 *
 * For C99 compilers other than GCC, the C99 behavior is expected.
 */
#if defined(__lint__)
#define __thread		/* delete */
#define	__packed		__packed
#define	__aligned(x)	_Alignas((x))
#define	__section(x)	/* delete */
#elif __GNUC_PREREQ__(2, 7) || defined(__PCC__) || defined(__lint__)
#define	__packed		__attribute__((__packed__))
#define	__aligned(x)	__attribute__((__aligned__(x)))
#define	__section(x)	__attribute__((__section__(x)))
#elif defined(_MSC_VER)
#define	__packed		/* ignore */
#else
#define	__packed		error: no __packed for this compiler
#define	__aligned(x)	error: no __aligned for this compiler
#define	__section(x)	error: no __section for this compiler
#endif

/*
 * C99 defines the restrict type qualifier keyword, which was made available
 * in GCC 2.92.
 */
#if __STDC_VERSION__ >= 199901L
#define	__restrict	restrict
#elif __GNUC_PREREQ__(2, 92)
#define	__restrict	__restrict__
#else
#define	__restrict	/* delete __restrict when not supported */
#endif

/*
 * C99 and C++11 define __func__ predefined identifier, which was made
 * available in GCC 2.95.
 */
#if !(__STDC_VERSION__ >= 199901L) && !(__cplusplus - 0 >= 201103L)
#if __GNUC_PREREQ__(2, 4) || defined(__lint__)
#define	__func__	__FUNCTION__
#else
#define	__func__	""
#endif
#endif /* !(__STDC_VERSION__ >= 199901L) && !(__cplusplus - 0 >= 201103L) */

#if defined(_KERNEL) && defined(NO_KERNEL_RCSIDS)
#undef	__KERNEL_RCSID
#define	__KERNEL_RCSID(_n, _s)	/* nothing */
#undef	__RCSID
#define	__RCSID(_s)		/* nothing */
#endif


#if !defined(_STANDALONE) && !defined(_KERNEL)
#ifdef __GNUC__
#define	__RENAME(x)	___RENAME(x)
#else
#ifdef __lint__
#define	__RENAME(x)	__symbolrename(x)
#else
#error "No function renaming possible"
#endif /* __lint__ */
#endif /* __GNUC__ */
#else /* _STANDALONE || _KERNEL */
#define	__RENAME(x)	no renaming in kernel or standalone environment
#endif

/*
 * A barrier to stop the optimizer from moving code or assume live
 * register values. This is gcc specific, the version is more or less
 * arbitrary, might work with older compilers.
 */
#if __GNUC_PREREQ__(2, 95)
#define	__insn_barrier()	__asm __volatile("":::"memory")
#else
#define	__insn_barrier()	/* */
#endif

/*
 * GNU C version 2.96 adds explicit branch prediction so that
 * the CPU back-end can hint the processor and also so that
 * code blocks can be reordered such that the predicted path
 * sees a more linear flow, thus improving cache behavior, etc.
 *
 * The following two macros provide us with a way to use this
 * compiler feature.  Use __predict_true() if you expect the expression
 * to evaluate to true, and __predict_false() if you expect the
 * expression to evaluate to false.
 *
 * A few notes about usage:
 *
 *	* Generally, __predict_false() error condition checks (unless
 *	  you have some _strong_ reason to do otherwise, in which case
 *	  document it), and/or __predict_true() `no-error' condition
 *	  checks, assuming you want to optimize for the no-error case.
 *
 *	* Other than that, if you don't know the likelihood of a test
 *	  succeeding from empirical or other `hard' evidence, don't
 *	  make predictions.
 *
 *	* These are meant to be used in places that are run `a lot'.
 *	  It is wasteful to make predictions in code that is run
 *	  seldomly (e.g. at subsystem initialization time) as the
 *	  basic block reordering that this affects can often generate
 *	  larger code.
 */
#if __GNUC_PREREQ__(2, 96) || defined(__lint__)
#define	__predict_true(exp)	__builtin_expect((exp) != 0, 1)
#define	__predict_false(exp)	__builtin_expect((exp) != 0, 0)
#else
#define	__predict_true(exp)	(exp)
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
 * Return the natural alignment in bytes for the given type
 */
#if __GNUC_PREREQ__(4, 1) || defined(__lint__)
#define	__alignof(__t)  __alignof__(__t)
#else
#define __alignof(__t) (sizeof(struct { char __x; __t __y; }) - sizeof(__t))
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

/*
 * "The nice thing about standards is that there are so many to choose from."
 * There are a number of "feature test macros" specified by (different)
 * standards that determine which interfaces and types the header files
 * should expose.
 *
 * Because of inconsistencies in these macros, we define our own
 * set in the private name space that end in _VISIBLE.  These are
 * always defined and so headers can test their values easily.
 * Things can get tricky when multiple feature macros are defined.
 * We try to take the union of all the features requested.
 *
 * The following macros are guaranteed to have a value after cdefs.h
 * has been included:
 *	__POSIX_VISIBLE
 *	__XPG_VISIBLE
 *	__ISO_C_VISIBLE
 *	__BSD_VISIBLE
 */
/*
 * X/Open Portability Guides and Single Unix Specifications.
 * _XOPEN_SOURCE				XPG3
 * _XOPEN_SOURCE && _XOPEN_VERSION = 4		XPG4
 * _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED = 1	XPG4v2
 * _XOPEN_SOURCE == 500				XPG5
 * _XOPEN_SOURCE == 520				XPG5v2
 * _XOPEN_SOURCE == 600				POSIX 1003.1-2001 with XSI
 * _XOPEN_SOURCE == 700				POSIX 1003.1-2008 with XSI
 *
 * The XPG spec implies a specific value for _POSIX_C_SOURCE.
 */
#ifdef _XOPEN_SOURCE
# if (_XOPEN_SOURCE - 0 >= 800)
#  define __XPG_VISIBLE		800
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	202405L
# elif (_XOPEN_SOURCE - 0 >= 700)
#  define __XPG_VISIBLE		700
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	200809L
# elif (_XOPEN_SOURCE - 0 >= 600)
#  define __XPG_VISIBLE		600
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	200112L
# elif (_XOPEN_SOURCE - 0 >= 520)
#  define __XPG_VISIBLE		520
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	199506L
# elif (_XOPEN_SOURCE - 0 >= 500)
#  define __XPG_VISIBLE		500
#  undef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE	199506L
# elif (_XOPEN_SOURCE_EXTENDED - 0 == 1)
#  define __XPG_VISIBLE		420
# elif (_XOPEN_VERSION - 0 >= 4)
#  define __XPG_VISIBLE		400
# else
#  define __XPG_VISIBLE		300
# endif
#endif

/*
 * POSIX macros, these checks must follow the XOPEN ones above.
 *
 * _POSIX_SOURCE == 1		1003.1-1988 (superseded by _POSIX_C_SOURCE)
 * _POSIX_C_SOURCE == 1		1003.1-1990
 * _POSIX_C_SOURCE == 2		1003.2-1992
 * _POSIX_C_SOURCE == 199309L	1003.1b-1993
 * _POSIX_C_SOURCE == 199506L   1003.1c-1995, 1003.1i-1995,
 *				and the omnibus ISO/IEC 9945-1:1996
 * _POSIX_C_SOURCE == 200112L   1003.1-2001
 * _POSIX_C_SOURCE == 200809L   1003.1-2008
 * _POSIX_C_SOURCE == 202405L   1003.1-2024
 * 
 * The POSIX spec implies a specific value for __ISO_C_VISIBLE, though
 * this may be overridden by the _ISOC99_SOURCE macro later.
 */
#ifdef _POSIX_C_SOURCE
# if (_POSIX_C_SOURCE - 0 >= 202405)
#  define __POSIX_VISIBLE	202405
#  define __ISO_C_VISIBLE	2017
# elif (_POSIX_C_SOURCE - 0 >= 200809)
#  define __POSIX_VISIBLE	200809
#  define __ISO_C_VISIBLE	1999
# elif (_POSIX_C_SOURCE - 0 >= 200112)
#  define __POSIX_VISIBLE	200112
#  define __ISO_C_VISIBLE	1999
# elif (_POSIX_C_SOURCE - 0 >= 199506)
#  define __POSIX_VISIBLE	199506
#  define __ISO_C_VISIBLE	1990
# elif (_POSIX_C_SOURCE - 0 >= 199309)
#  define __POSIX_VISIBLE	199309
#  define __ISO_C_VISIBLE	1990
# elif (_POSIX_C_SOURCE - 0 >= 2)
#  define __POSIX_VISIBLE	199209
#  define __ISO_C_VISIBLE	1990
# else
#  define __POSIX_VISIBLE	199009
#  define __ISO_C_VISIBLE	1990
# endif
#elif defined(_POSIX_SOURCE)
# define __POSIX_VISIBLE	198808
#  define __ISO_C_VISIBLE	0
#endif

/*
 * _ANSI_SOURCE means to expose ANSI C89 interfaces only.
 * If the user defines it in addition to one of the POSIX or XOPEN
 * macros, assume the POSIX/XOPEN macro(s) should take precedence.
 */
#if defined(_ANSI_SOURCE) && !defined(__POSIX_VISIBLE) && \
    !defined(__XPG_VISIBLE)
# define __POSIX_VISIBLE	0
# define __XPG_VISIBLE		0
# define __ISO_C_VISIBLE	1990
#endif

/*
 * _ISOC99_SOURCE, _ISOC11_SOURCE, __STDC_VERSION__, and __cplusplus
 * override any of the other macros since they are non-exclusive.
 */
#if defined(_ISOC11_SOURCE) || \
    (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112) || \
    (defined(__cplusplus) && __cplusplus >= 201703)
# undef __ISO_C_VISIBLE
# define __ISO_C_VISIBLE	2011
#elif defined(_ISOC99_SOURCE) || \
    (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901) || \
    (defined(__cplusplus) && __cplusplus >= 201103)
# undef __ISO_C_VISIBLE
# define __ISO_C_VISIBLE	1999
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
 * Default values.
 */
#ifndef __XPG_VISIBLE
# define __XPG_VISIBLE		700
#endif
#ifndef __POSIX_VISIBLE
# define __POSIX_VISIBLE	200809
#endif
#ifndef __ISO_C_VISIBLE
# define __ISO_C_VISIBLE	2011
#endif
#ifndef __BSD_VISIBLE
# define __BSD_VISIBLE		1
#endif
#endif /* !_SYS_CDEFS_H_ */
