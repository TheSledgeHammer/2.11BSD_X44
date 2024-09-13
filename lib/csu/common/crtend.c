/*-
 * SPDX-License-Identifier: BSD-1-Clause
 *
 * Copyright 2018 Andrew Turner
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <stdlib.h>

#include <dot_init.h>

typedef void (*fptr_t)(void);

static fptr_t __CTOR_LIST__[] __section(".ctors") = { (fptr_t)-1 };

static fptr_t __CTOR_END__[] __section(".ctors") = { (fptr_t)0 };

/*
 * On some architectures and toolchains we may need to call the .dtors.
 * These are called in the order they are in the ELF file.
 */
#ifdef HAVE_CTORS
static void
__ctors(void)
{
	void (**p)(void);
	for (p = __CTOR_END__; p > __CTOR_LIST__ + 1; ) {
		(*(*--p))();
	}
}

static void __do_global_ctors_aux(void) __used;

static void
__do_global_ctors_aux(void)
{
	static int initialized;

	if (!initialized) {
		initialized = 1;
	}

	/*
	 * Call global constructors.
	 */
	__ctors();
}

asm (
    ".pushsection .init		\n"
    "\t" INIT_CALL_SEQ(__do_global_ctors_aux) "\n"
    ".popsection		\n"
);
#endif
