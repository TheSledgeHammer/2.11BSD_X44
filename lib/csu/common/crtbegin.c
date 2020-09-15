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
#include <sys/exec_elf.h>
#include <stdlib.h>

#include "crt.h"

typedef void (*fptr)(void);

static void (*__DTOR_LIST__[])(void)
    __attribute__((section(".dtors"))) = { (void *)-1 };	/* XXX */

static void (*__DTOR_END__[])(void)
    __attribute__((section(".dtors"))) = { (void *)0 };		/* XXX */

#ifndef SHARED
void *__dso_handle = 0;
#else
void *__dso_handle = &__dso_handle;
void __cxa_finalize(void *) __weak_symbol;

/*
 * Call __cxa_finalize with the dso handle in shared objects.
 * When we have ctors/dtors call from the dtor handler before calling
 * any dtors, otherwise use a destructor.
 */
#ifndef HAVE_CTORS
__attribute__((destructor))
#endif
static void
run_cxa_finalize(void)
{

	if (__cxa_finalize != NULL)
		__cxa_finalize(__dso_handle);
}
#endif

/*
 * On some architectures and toolchains we may need to call the .dtors.
 * These are called in the order they are in the ELF file.
 */

#ifdef HAVE_CTORS
static void
__dtors(void)
{
	void (**p)(void);
	for (p = __DTOR_LIST__ + 1; p < __DTOR_END__; ) {
		(*(*--p))();
	}
}

static void __do_global_dtors_aux(void) __used;

static void
__do_global_dtors_aux(void)
{
	static int finished;

	if (finished)
		return;

#ifdef SHARED
	run_cxa_finalize();
#endif

	/*
	 * Call global destructors.
	 */
	__dtors();
}

asm (
    ".pushsection .fini		\n"
    "\t" INIT_CALL_SEQ(__do_global_dtors_aux) "\n"
    ".popsection		\n"
);
#endif

/*
 * Handler for gcj. These provide a _Jv_RegisterClasses function and fill
 * out the .jcr section. We just need to call this function with a pointer
 * to the appropriate section.
 */
extern void _Jv_RegisterClasses(void *);
static void register_classes(void);

static void *__JCR_LIST__[]
    __attribute__((section(".jcr"))) = { };

#ifndef CTORS_CONSTRUCTORS
__attribute__((constructor))
#endif
static void
register_classes(void)
{

	if (_Jv_RegisterClasses != NULL && __JCR_LIST__[0] != 0)
		_Jv_RegisterClasses(__JCR_LIST__);
}

/*
 * We can't use constructors when they use the .ctors section as they may be
 * placed before __CTOR_LIST__.
 */
#ifdef CTORS_CONSTRUCTORS
asm (
    ".pushsection .init		\n"
    "\t" INIT_CALL_SEQ(register_classes) "\n"
    ".popsection		\n"
);
#endif
