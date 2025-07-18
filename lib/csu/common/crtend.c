/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>

#include <dot_init.h>

typedef void (*fptr_t)(void);
typedef void (*func_t)(void *);

#ifdef HAVE_CTORS
static fptr_t __CTOR_LIST__[1] __section(".ctors") __used = { (fptr_t)-1 };
static fptr_t __CTOR_END__[] __section(".ctors") __used = { (fptr_t)0 };
#endif
#if defined(JCR) && defined(__GNUC__)
static fptr_t __JCR_LIST__[] __section(".jcr") __used = { (fptr_t)0 };
extern void _Jv_RegisterClasses(void *) __attribute__((weak));
#endif

#ifndef MD_CALL_STATIC_FUNCTION
#if defined(__GNUC__)
#define	MD_CALL_STATIC_FUNCTION(section, func)		\
static void __attribute__((__unused__))				\
__call_##func(void)									\
{													\
	__asm volatile (".section " #section);			\
	func();											\
	__asm volatile (".previous");					\
}
#else
#error Need MD_CALL_STATIC_FUNCTION
#endif
#endif /* ! MD_CALL_STATIC_FUNCTION */

/*
 * On some architectures and toolchains we may need to call the .dtors.
 * These are called in the order they are in the ELF file.
 */
#ifdef HAVE_CTORS

static void __ctors(void) __used;
static void __do_global_ctors_aux(void) __used;

static inline void
common_ctors(fptr_t *list, fptr_t *end)
{
	fptr_t *p;
	for (p = end; p > list + 1;) {
		(*(*--p))();
	}
}

static inline void
common_init(fptr_t *jcr, func_t regclass, fptr_t *list, fptr_t *end)
{
	static int initialized;

	if (initialized) {
		return;
	}

	initialized = 1;

#if defined(JCR) && defined(__GNUC__)
	if (jcr[0] != NULL && regclass != NULL) {
		regclass(jcr);
	}
#endif

	/* Call global constructors. */
	common_ctors(list, end);
}

static void
__ctors(void)
{
	common_ctors(__CTOR_LIST__, __CTOR_END__);
}

static void
__do_global_ctors_aux(void)
{
    common_init(__JCR_LIST__, _Jv_RegisterClasses, __CTOR_LIST__, __CTOR_END__);
}

MD_CALL_STATIC_FUNCTION(.init, __do_global_ctors_aux);

#endif
