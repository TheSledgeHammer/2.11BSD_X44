/*
 * asm.h
 *
 *  Created on: 3 Mar 2020
 *      Author: marti
 */

#ifndef _MACHINE_ASM_H_
#define	_MACHINE_ASM_H_

#include <sys/cdefs.h>

#ifdef __ELF__
# define _C_LABEL(x)	x
#else
# ifdef __STDC__
#  define _C_LABEL(x)	_ ## x
# else
#  define _C_LABEL(x)	_/**/x
# endif
#endif
#define	_ASM_LABEL(x)	x

#ifdef __STDC__
# define __CONCAT(x,y)	x ## y
# define __STRING(x)	#x
#else
# define __CONCAT(x,y)	x/**/y
# define __STRING(x)	"x"
#endif

/* let kernels and others override entrypoint alignment */
#ifndef _ALIGN_TEXT
# ifdef __ELF__
#  define _ALIGN_TEXT .align 4
# else
#  define _ALIGN_TEXT .align 2
# endif
#endif

#define	ENTRY(name) 	\
	.globl _/**/name; _/**/name:

#define	ALTENTRY(name) 	\
	.globl _/**/name; _/**/name:

/* XXX Can't use __CONCAT() here, as it would be evaluated incorrectly. */
#ifdef __ELF__
#ifdef __STDC__
#define	IDTVEC(name) \
	ALIGN_TEXT; .globl X ## name; .type X ## name,@function; X ## name:
#define	IDTVEC_END(name) \
	.size X ## name, . - X ## name
#else
#define	IDTVEC(name) \
	ALIGN_TEXT; .globl X/**/name; .type X/**/name,@function; X/**/name:
#define	IDTVEC_END(name) \
	.size X/**/name, . - X/**/name
#endif /* __STDC__ */
#else
#ifdef __STDC__
#define	IDTVEC(name) \
	ALIGN_TEXT; .globl _X ## name; .type _X ## name,@function; _X ## name:
#define	IDTVEC_END(name) \
	.size _X ## name, . - _X ## name
#else
#define	IDTVEC(name) \
	ALIGN_TEXT; .globl _X/**/name; .type _X/**/name,@function; _X/**/name:
#define	IDTVEC_END(name) \
	.size _X/**/name, . - _X/**/name
#endif /* __STDC__ */
#endif /* __ELF__ */


#ifdef __ELF__
#define	WEAK_ALIAS(alias,sym)						\
	.weak alias;									\
	alias = sym
#endif

#ifdef __STDC__
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg ## ,30,0,0,0 ;						\
	.stabs __STRING(_C_LABEL(sym)) ## ,1,0,0,0
#elif defined(__ELF__)
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;							\
	.stabs __STRING(sym),1,0,0,0
#else
#define	WARN_REFERENCES(sym,msg)					\
	.stabs msg,30,0,0,0 ;							\
	.stabs __STRING(_/**/sym),1,0,0,0
#endif /* __STDC__ */
#endif /* _MACHINE_ASM_H_ */
