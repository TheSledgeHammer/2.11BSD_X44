/*	$NetBSD: bootstrap.h,v 1.10 2017/12/10 02:32:03 christos Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/boot/common/bootstrap.h,v 1.38.6.1 2004/09/03 19:25:40 iedowse Exp $
 */

#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include <sys/types.h>

/*
 * Support for commands
 */
typedef int	(bootblk_cmd_t)(int, char **);
struct bootblk_command {
    const char			*c_name;
    const char			*c_desc;
    bootblk_cmd_t		*c_fn;
};

/* Commands and return values; nonzero return sets command_errmsg != NULL */
#define	COMMAND_ERRBUFSZ	(256)
extern const char *command_errmsg;
extern char	command_errbuf[COMMAND_ERRBUFSZ];
#define CMD_OK		0
#define CMD_WARN	1
#define CMD_ERROR	2
#define CMD_CRIT	3
#define CMD_FATAL	4

extern struct bootblk_command commands[];

struct bootblk_command *command_search(struct bootblk_command *, const char *);
bootblk_cmd_t *command_do(struct bootblk_command *, int, char **);
int command_do_interp(struct bootblk_command *, bootblk_cmd_t *, int, char **);
int command_seterr(const char *fmt, ...);
const char *command_geterr(void);

/* Prototypes for the command handlers within stand/common/ */
/*	commands.c		*/
int command_help(int, char **);
int command_commandlist(int, char **);
int command_show(int, char **);
int command_set(int, char **);
int command_unset(int, char **);
int command_echo(int, char **);
int command_read(int, char **);
int command_more(int, char **);
int command_lsdev(int, char **);

#ifdef USE_BCACHE
/*	bcache.c		*/
int command_bcache(int, char **);
#endif

/*	boot.c		*/
int command_boot(int, char **);
int command_autoboot(int, char **);

/*	fileload.c	*/
int command_load(int, char **);
int command_unload(int, char **);
int command_lskern(int, char **);

/*	install.c	*/
int command_install(int, char **);

/*	interp.c	*/
int command_include(int, char **);
int command_optinclude(int, char **);

/*	ls.c */
int command_ls(int, char **);

/* i386/main.c */
int command_reboot(int, char **);
int command_heap(int, char **);

/*  i386/libi386/biosmem.c	*/
int command_biosmem(int, char **);

#ifdef USE_BCACHE
#define COMMON_COMMANDS \
	{ "help", "detailed help", command_help }, \
	{ "commandlist", "list commands", command_commandlist }, \
	{ "show", "show variable(s)", command_show }, \
	{ "set", "set a variable", command_set }, \
	{ "unset", "unset a variable", command_unset }, \
	{ "echo", "echo arguments", command_echo }, \
	{ "read", "read input from the terminal", command_read }, \
	{ "more", "show contents of a file", command_more }, \
	{ "lsdev", "list all devices", command_lsdev },	\
	{ "bcachestat", "get disk block cache stats", command_bcache },
	{ "boot", "boot a file or loaded kernel", command_boot }, \
	{ "autoboot", "boot automatically after a delay", command_autoboot}, \
	{ "load", "load a kernel", command_load }, \
	{ "unload", "unload all modules", command_unload },	\
	{ "lskern", "list loaded kernel", command_lskern }, \
	{ "include", "read commands from a file", command_include }, \
	{ "optinclude", "run commands from file; ignore exit status", command_optinclude }, \
	{ "ls", "list files", command_ls }, \
	{ "install",  "install software package", command_install },
#else
#define COMMON_COMMANDS \
	{ "help", "detailed help", command_help }, \
	{ "commandlist", "list commands", command_commandlist }, \
	{ "show", "show variable(s)", command_show }, \
	{ "set", "set a variable", command_set }, \
	{ "unset", "unset a variable", command_unset }, \
	{ "echo", "echo arguments", command_echo }, \
	{ "read", "read input from the terminal", command_read }, \
	{ "more", "show contents of a file", command_more }, \
	{ "lsdev", "list all devices", command_lsdev },	\
	{ "boot", "boot a file or loaded kernel", command_boot }, \
	{ "autoboot", "boot automatically after a delay", command_autoboot}, \
	{ "load", "load a kernel", command_load }, \
	{ "unload", "unload all modules", command_unload },	\
	{ "lskern", "list loaded kernel", command_lskern }, \
	{ "include", "read commands from a file", command_include }, \
	{ "optinclude", "run commands from file; ignore exit status", command_optinclude }, \
	{ "ls", "list files", command_ls }, \
	{ "install",  "install software package", command_install },
#endif
#endif /* _COMMANDS_H_ */
