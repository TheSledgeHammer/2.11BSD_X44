/* LINTLIBRARY */
/*-
 * Copyright 1996-1998 John D. Polstra.
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
 *
 * $FreeBSD$
 */

#include "crt0_common.c"

typedef void (*fptr_t)(void);

void ___start(fptr_t, struct ps_strings *);

__asm(
		"	.text							\n"
		"	.align	4						\n"
		"	.globl	__start					\n"
		"	.globl	_start					\n"
		"_start:							\n"
		"__start:							\n"
		"	pushl	%ebx					# ps_strings	\n"
		"	pushl	%edx					# cleanup		\n"
		"	call	___start"
);

#ifdef MCRT0
extern void _mcleanup(void);
extern void monstartup(void *, void *);
extern int 	eprol;
extern int 	etext;
#endif

static inline void
crt0_start(fptr_t cleanup, int argc, char **argv, char **env)
{
    env = (argc + argv + 1);
    handle_argv(argc, argv, env);
	if (&_DYNAMIC != NULL) {
		atexit(cleanup);
	} else {
		process_irelocs();
	}

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup(&eprol, &etext);
    __asm__("eprol:");
#endif

    handle_static_init(argc, argv, env);
    exit(main(argc, argv, env));
}

/* The entry function, C part. */
void
___start(fptr_t cleanup, struct ps_strings *ps_strings)
{
	if (ps_strings != (struct ps_strings *)0) {
		__ps_strings = ps_strings;
    }
    environ = &ps_strings->ps_envstr;
    crt0_start(cleanup, ps_strings->ps_nargvstr, &ps_strings->ps_argvstr, environ);
}

