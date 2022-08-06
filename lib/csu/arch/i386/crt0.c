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

#define HAS_IPLT

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD$"); */

#include <stdlib.h>
#include "init_note.c"

typedef void (*fptr)(void);

#ifdef MCRT0
extern void _mcleanup(void);
extern void monstartup(void *, void *);
extern int 	eprol;
extern int 	etext;
#endif

void ___start(fptr, const Obj_Entry *, struct ps_strings *, int, char *[]);

__asm(
		"	.text							\n"
		"	.align	4						\n"
		"	.globl	__start					\n"
		"	.globl	_start					\n"
		"_start:							\n"
		"__start:							\n"
		"	pushl	%ebx					# ps_strings	\n"
		"	pushl	%ecx					# obj			\n"
		"	pushl	%edx					# cleanup		\n"
		"	movl	12(%esp),%eax			\n"
		"	leal	20(%esp,%eax,4),%ecx	\n"
		"	leal	16(%esp),%edx			\n"
		"	pushl	%ecx					\n"
		"	pushl	%edx					\n"
		"	pushl	%eax					\n"
		"	call	___start"
);

/* The entry function, C part. */
void
___start(cleanup, obj, ps_strings, argc, argv)
	fptr cleanup;
	const Obj_Entry *obj;
	struct ps_strings *ps_strings;
	int argc;
	char *argv[];
{
	char **env;

	env = argv + argc + 1;
	handle_argv(argc, argv, env);

	if (ps_strings != (struct ps_strings *)0)
		__ps_strings = ps_strings;

	if (&_DYNAMIC != NULL) {
		_rtld_setup(cleanup, obj);
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

__asm(".hidden	_start");
