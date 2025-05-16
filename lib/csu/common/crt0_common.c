/*-
 * SPDX-License-Identifier: BSD-1-Clause
 *
 * Copyright 2012 Konstantin Belousov <kib@FreeBSD.org>
 * Copyright (c) 2018 The FreeBSD Foundation
 *
 * Parts of this software was developed by Konstantin Belousov
 * <kib@FreeBSD.org> under sponsorship from the FreeBSD Foundation.
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

#include <sys/types.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <machine/profile.h>
#include <stdlib.h>

typedef void (*fptr_t)(void);

static char	        empty_string[] = "";
char 			**environ;
const char 		*__progname = empty_string;
struct ps_strings 	*__ps_strings = 0;

extern int   main(int argc, char **argv, char **env);

extern void (*__preinit_array_start[])(int, char **, char **) __dso_hidden;
extern void (*__preinit_array_end[])(int, char **, char **) __dso_hidden;
extern void (*__init_array_start[])(int, char **, char **) __dso_hidden;
extern void (*__init_array_end[])(int, char **, char **) __dso_hidden;
extern void (*__fini_array_start[])(void) __dso_hidden;
extern void (*__fini_array_end[])(void) __dso_hidden;
extern void _init(void);
extern void _fini(void);

extern int _DYNAMIC;
#pragma weak _DYNAMIC

static void
preinitializer(int argc, char **argv, char **env)
{
	void (*func)(int, char **, char **);
	size_t array_size, n;

	array_size = __preinit_array_end - __preinit_array_start;
	for (n = 0; n < array_size; n++) {
		func = __preinit_array_start[n];
		if ((uintptr_t)func != 0 && (uintptr_t)func != 1)
			func(argc, argv, env);
	}
}

static void
initializer(int argc, char **argv, char **env)
{
	void (*func)(int, char **, char **);
	size_t array_size, n;

	_init();
	array_size = __init_array_end - __init_array_start;
	for (n = 0; n < array_size; n++) {
		func = __init_array_start[n];
		if ((uintptr_t)func != 0 && (uintptr_t)func != 1)
			func(argc, argv, env);
	}
}

static void
finalizer(void)
{
	void (*func)(void);
	size_t array_size, n;

	array_size = __fini_array_end - __fini_array_start;
	for (n = array_size; n > 0; n--) {
		func = __fini_array_start[n - 1];
		if ((uintptr_t)func != 0 && (uintptr_t)func != 1)
			(func)();
	}
	_fini();
}

static inline void
handle_static_init(int argc, char **argv, char **env)
{
	if (&_DYNAMIC != NULL)
		return;

	atexit(finalizer);
	preinitializer(argc, argv, env);
	initializer(argc, argv, env);
}

static inline void
handle_argv(int argc, char *argv[], char **env)
{
	const char *s;

	if (environ == NULL)
		environ = env;
	if (argc > 0 && argv[0] != NULL) {
		__progname = argv[0];
		for (s = __progname; *s != '\0'; s++) {
			if (*s == '/')
				__progname = s + 1;
		}
	} else {
        __progname = empty_string;
    }
}

#if defined(HAS_IPLTA)
#include <stdio.h>
extern const Elf_Rela __rela_iplt_start[] __dso_hidden __weak;
extern const Elf_Rela __rela_iplt_end[] __dso_hidden __weak;
#define write_plt(where, value) *where = value
#define IFUNC_RELOCATION	R_TYPE(IRELATIVE)

static void
fix_iplta(void)
{
	const Elf_Rela *rela, *relalim;
	uintptr_t relocbase = 0;
	Elf_Addr *where, target;

	rela = __rela_iplt_start;
	relalim = __rela_iplt_end;
	for (; rela < relalim; ++rela) {
		if (ELF_R_TYPE(rela->r_info) != IFUNC_RELOCATION) {
			abort();
		}
		where = (Elf_Addr *)(relocbase + rela->r_offset);
		target = (Elf_Addr)(relocbase + rela->r_addend);
		target = ((Elf_Addr(*)(void))target)();
		write_plt(where, target);
	}
}
#endif

#if defined(HAS_IPLT)
#include <stdio.h>
extern const Elf_Rel __rel_iplt_start[] __dso_hidden __weak;
extern const Elf_Rel __rel_iplt_end[] __dso_hidden __weak;
#define IFUNC_RELOCATION	R_TYPE(IRELATIVE)

static void
fix_iplt(void)
{
	const Elf_Rel *rel, *rellim;
	uintptr_t relocbase = 0;
	Elf_Addr *where, target;

	rel = __rel_iplt_start;
	rellim = __rel_iplt_end;
	for (; rel < rellim; ++rel) {
		if (ELF_R_TYPE(rel->r_info) != IFUNC_RELOCATION) {
			abort();
		}
		where = (Elf_Addr *)(relocbase + rel->r_offset);
		target = ((Elf_Addr(*)(void))*where)();
		*where = target;
	}
}
#endif

static inline void
process_irelocs(void)
{
#ifdef HAS_IPLTA
		fix_iplta();
#endif
#ifdef HAS_IPLT
		fix_iplt();
#endif
}

#ifdef MCRT0
extern void _mcleanup(void);
extern void monstartup(void *, void *);
extern int 	eprol;
extern int 	etext;
#endif

static inline void
crt0_start(fptr_t cleanup, int argc, char **argv, char **env)
{
    handle_argv(argc, argv, env);
	if (&_DYNAMIC != NULL) {
		atexit(cleanup);
	} else {
		process_irelocs();
	}

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup(&eprol, &etext);
	__asm__("  .text");
	__asm__("eprol:");
#endif

    handle_static_init(argc, argv, env);
    exit(main(argc, argv, env));
}
