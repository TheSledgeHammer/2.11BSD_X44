/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include <sys/cdefs.h>
#include <sys/param.h>

#include <dot_init.h>

typedef void (*fptr_t)(void);
typedef void (*func_t)(void *);

#ifdef HAVE_CTORS
static fptr_t __DTOR_LIST__[1] __section(".dtors") __used = { (fptr_t)-1 };
static fptr_t __DTOR_END__[] __section(".dtors") __used = { (fptr_t)0 };
#endif

#ifdef SHARED
void *__dso_handle = &__dso_handle;
#else
void *__dso_handle = NULL;
#endif
__asm(".hidden  __dso_handle");

#ifdef SHARED
extern void __cxa_finalize(void *)  __attribute__((weak));
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

static void __dtors(void) __used;
static void __do_global_dtors_aux(void) __used;

static inline void
common_dtors(fptr_t *list, fptr_t *end)
{
	fptr_t *p;
	for (p = list + 1; p < end;) {
		(*(*--p))();
	}
}

static inline void
common_fini(func_t final, void *handle, fptr_t *list, fptr_t *end)
{
	static int finished;

	if (finished) {
		return;
	}

	finished = 1;

#ifdef SHARED
	/*
	 * Call __cxa_finalize with the dso handle in shared objects.
	 * When we have ctors/dtors call from the dtor handler before calling
	 * any dtors, otherwise use a destructor.
	 */
	if (final != NULL) {
		final(handle);
	}
#endif

	/* Call global destructors.	*/
	common_dtors(list, end);
}

static void
__dtors(void)
{
	common_dtors(__DTOR_LIST__, __DTOR_END__);
}

static void
__do_global_dtors_aux(void)
{
#ifdef SHARED
	common_fini(__cxa_finalize, __dso_handle, __DTOR_LIST__, __DTOR_END__);
#else
	common_fini(NULL, __dso_handle, __DTOR_LIST__, __DTOR_END__);
#endif
}

MD_CALL_STATIC_FUNCTION(.fini, __do_global_dtors_aux);
#endif
