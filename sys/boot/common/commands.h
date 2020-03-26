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

/* Commands and return values; nonzero return sets command_errmsg != NULL */
#define	COMMAND_ERRBUFSZ	(256)
extern const char *command_errmsg;
extern char	command_errbuf[COMMAND_ERRBUFSZ];
#define CMD_OK		0
#define CMD_WARN	1
#define CMD_ERROR	2
#define CMD_CRIT	3
#define CMD_FATAL	4

typedef int	(bootblk_cmd_t)(int argc, char *argv[]);

/*
 * Support for commands
 */
struct bootblk_command
{
    const char			*c_name;
    const char			*c_desc;
    bootblk_cmd_t		*c_fn;
};

/* Prototypes for the command handlers within stand/common/ */

/*	commands.c		*/
int command_help(int argc, char *argv[]) ;
int command_commandlist(int argc, char *argv[]);
int command_show(int argc, char *argv[]);
int command_set(int argc, char *argv[]);
int command_unset(int argc, char *argv[]);
int command_echo(int argc, char *argv[]);
int command_read(int argc, char *argv[]);
int command_more(int argc, char *argv[]);
int command_lsdev(int argc, char *argv[]);
int command_quit(int argc, char *argv[]);

/*	boot.c		*/
int command_boot(int argc, char *argv[]);
int command_autoboot(int argc, char *argv[]);

/*	fileload.c	*/
int command_load(int argc, char *argv[]);
int command_unload(int argc, char *argv[]);
int command_lskern(int argc, char *argv[]);

/*	interp.c	*/
int command_include(int argc, char *argv[]);

/*	ls.c		*/
int command_ls(int argc, char *argv[]);

/*  bcache.c	*/
int command_bcache(int argc, char *argv[]);

/*  biosmem.c	*/
int command_biosmem(int argc, char *argv[]);

/*  biossmap.c	*/
int command_smap(int argc, char *argv[]);

#define COMMAND_SET(a, b, c, d) /* Nothing */

#define COMMON_COMMANDS																			\
		{ "help", "detailed help", command_help },												\
		{ "commandlist", "list commands", command_commandlist },								\
		{ "show", "show variable(s)", command_show },											\
		{ "set", "set a variable", command_set },												\
		{ "unset", "unset a variable", command_unset },											\
		{ "echo", "echo arguments", command_echo },												\
		{ "read", "read input from the terminal", command_read },								\
		{ "more", "show contents of a file", command_more },									\
		{ "lsdev", "list all devices", command_lsdev },											\
		{ "quit", "exit the loader", command_quit },											\
		{ "bcachestat", "get disk block cache stats", command_bcache },							\
		{ "boot", "boot a file or loaded kernel", command_boot },								\
		{ "autoboot", "boot automatically after a delay", command_autoboot},					\
		{ "load", "load a kernel", command_load },												\
		{ "unload", "unload all modules", command_unload },										\
		{ "lskern", "list loaded kernel", command_lskern },										\
		{ "include", "read commands from a file", command_include },							\
		{ "ls", "list files", command_ls },														\
		{ "biosmem", "show BIOS memory setup", command_biosmem },								\
		{ "smap", "show BIOS SMAP", command_smap }


extern struct bootblk_command commands[];
/*
struct command_set {
    int     (*command_help)(int argc, char *argv[]);
    int 	(*command_commandlist)(int argc, char *argv[]);
    int 	(*command_show)(int argc, char *argv[]);
    int 	(*command_set)(int argc, char *argv[]);
    int 	(*command_unset)(int argc, char *argv[]);
    int 	(*command_echo)(int argc, char *argv[]);
    int 	(*command_read)(int argc, char *argv[]);
    int 	(*command_more)(int argc, char *argv[]);
    int 	(*command_lsdev)(int argc, char *argv[]);
    int 	(*command_boot)(int argc, char *argv[]);
    int 	(*command_autoboot)(int argc, char *argv[]);
    int 	(*command_load)(int argc, char *argv[]);
    int 	(*command_unload)(int argc, char *argv[]);
    int 	(*command_lskern)(int argc, char *argv[]);
    int 	(*command_include)(int argc, char *argv[]);
    int 	(*command_ls)(int argc, char *argv[]);
    int		(*command_bcache)(int argc, char *argv[]);
};
*/
/* Command Set Commands */
/*
struct command_set cmds[] = {
        &command_help,
		&command_commandlist,
		&command_show,
		&command_set,
		&command_unset,
		&command_echo,
		&command_read,
		&command_more,
		&command_lsdev,
		&command_boot,
		&command_autoboot,
		&command_load,
		&command_unload,
		&command_lskern,
		&command_include,
		&command_ls,
		&command_bcache,
};
/*

/* Number of Commands in Command set:
struct command_set *cmd = &cmds[0];
int cmd_setsize = (sizeof(cmds[0])/ (sizeof(&cmd)));
*/

#endif /* _COMMANDS_H_ */
