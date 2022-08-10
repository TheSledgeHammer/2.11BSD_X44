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
/* __FBSDID("$FreeBSD$"); */

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_elf.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef DYNAMIC
#include <dlfcn.h>
#include "rtld.h"
#else
typedef void Obj_Entry;
#endif

typedef void (*fptr)(void);

extern int main(int, char **, char **);

extern void (*__preinit_array_start[])(int, char **, char **);
extern void (*__preinit_array_end[])(int, char **, char **);
extern void (*__init_array_start[])(int, char **, char **);
extern void (*__init_array_end[])(int, char **, char **);
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
extern void _fini(void);
extern void _init(void);

#define	write(fd, s, n)	__syscall(SYS_write, (fd), (s), (n))

#define	_FATAL(str)				\
do {							\
	write(2, str, sizeof(str));	\
	_exit(1);					\
} while (0)

extern int _DYNAMIC;
#pragma weak _DYNAMIC

char 				**environ;
const char 			*__progname = "";
struct ps_strings 	*__ps_strings = 0;

void				_rtld_setup(fptr cleanup, const Obj_Entry *obj);
const Obj_Entry 	*__mainprog_obj;

#ifdef MCRT0
extern void _mcleanup(void);
extern void monstartup(void *, void *);
extern int 	eprol;
extern int 	etext;
#endif

static void
finalizer(void)
{
	void (*fn)(void);
	size_t array_size, n;

	array_size = __fini_array_end - __fini_array_start;
	for (n = array_size; n > 0; n--) {
		fn = __fini_array_start[n - 1];
		if ((uintptr_t)fn != 0 && (uintptr_t)fn != 1)
			(fn)();
	}
	_fini();
}

static inline void
handle_static_init(int argc, char **argv, char **env)
{
	void (*fn)(int, char **, char **);
	size_t array_size, n;

	if (&_DYNAMIC != NULL)
		return;

	atexit(finalizer);

	array_size = __preinit_array_end - __preinit_array_start;
	for (n = 0; n < array_size; n++) {
		fn = __preinit_array_start[n];
		if ((uintptr_t)fn != 0 && (uintptr_t)fn != 1)
			fn(argc, argv, env);
	}
	_init();
	array_size = __init_array_end - __init_array_start;
	for (n = 0; n < array_size; n++) {
		fn = __init_array_start[n];
		if ((uintptr_t)fn != 0 && (uintptr_t)fn != 1)
			fn(argc, argv, env);
	}
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
	}
}

#if defined(HAS_IPLTA)
#include <stdio.h>
extern const Elf_Rela __rela_iplt_start[] __dso_hidden __weak;
extern const Elf_Rela __rela_iplt_end[] __dso_hidden __weak;
#define write_plt(where, value) *where = value
#define IFUNC_RELOCATION	RTYPE(IRELATIVE)

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
#define IFUNC_RELOCATION	RTYPE(IRELATIVE)

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

void
process_irelocs(void)
{
#ifdef HAS_IPLTA
		fix_iplta();
#endif
#ifdef HAS_IPLT
		fix_iplt();
#endif
}

#ifdef DYNAMIC
void
_rtld_setup(cleanup, obj)
	void (*cleanup)(void);
	const Obj_Entry *obj;
{
	if ((obj == NULL) || (obj->magic != RTLD_MAGIC))
		_FATAL("Corrupt Obj_Entry pointer in GOT");
	if (obj->version != RTLD_VERSION)
		_FATAL("Dynamic linker version mismatch");

	atexit(cleanup);
}
#endif /* DYNAMIC */
