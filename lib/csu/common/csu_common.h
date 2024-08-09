/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#ifndef _CSU_COMMON_H_
#define _CSU_COMMON_H_

#include <sys/types.h>
#include <sys/syscall.h>

#include <stdlib.h>
#ifdef DYNAMIC
#ifdef __weak_alias
__weak_alias(dlopen,_dlopen)
__weak_alias(dlclose,_dlclose)
__weak_alias(dlsym,_dlsym)
__weak_alias(dlerror,_dlerror)
__weak_alias(dladdr,_dladdr)
#endif
#include <dlfcn.h>
#include "rtld.h"
#else
typedef void Obj_Entry;
#endif

typedef void (*fptr)(void);

extern int				__syscall(quad_t, ...);
#define	_exit(v)		__syscall(SYS_exit, (v))
#define	write(fd, s, n)	__syscall(SYS_write, (fd), (s), (n))

#define	_FATAL(str)				\
do {							\
	write(2, str, sizeof(str));	\
	_exit(1);					\
} while (0)

extern void (*__preinit_array_start[])(int, char **, char **);
extern void (*__preinit_array_end[])(int, char **, char **);
extern void (*__init_array_start[])(int, char **, char **);
extern void (*__init_array_end[])(int, char **, char **);
extern void (*__fini_array_start[])(void);
extern void (*__fini_array_end[])(void);
extern void	_init(void);
extern void	_fini(void);

static char	 empty_string[] = "";

char 				**environ;
const char 			*__progname = empty_string;
struct ps_strings 	*__ps_strings = 0;
const Obj_Entry 	*__mainprog_obj;

#ifdef DYNAMIC
void	_rtld_setup(void (*)(void), const Obj_Entry *obj);

/*
 * Arrange for _DYNAMIC to be weak and undefined (and therefore to show up
 * as being at address zero, unless something else defines it).  That way,
 * if we happen to be compiling without -static but with without any
 * shared libs present, things will still work.
 */
extern int _DYNAMIC __weak_reference(_DYNAMIC);
#endif /* DYNAMIC */

#ifdef MCRT0
extern void _mcleanup(void);
extern void monstartup(void *, void *);
extern int 	eprol;
extern int 	etext;
#endif

void handle_static_init(int argc, char **argv, char **env);
void handle_argv(int argc, char *argv[], char **env);
void process_irelocs(void);
int  main(int argc, char **argv, char **env);

#endif
