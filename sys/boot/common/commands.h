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

/* Number of Commands in Command set:
struct command_set *cmd = &cmds[0];
int cmd_setsize = (sizeof(cmds[0])/ (sizeof(&cmd)));
*/

/*
 * Support for commands
 */
struct bootblk_command
{
    const char			*c_name;
    const char			*c_desc;
    struct command_set	*c_fn;
};
extern struct bootblk_command commands[];

/* Prototypes for the command handlers within stand/common/ */

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
};

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

/* Command Set Commands */
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
};

#define COMMAND_HELP(argc, argv) 	\
	command_help(argc, argv)
#define COMMAND_COMMANDLIST(argc, argv) \
	command_commandlist(argc, argv)
#define COMMAND_SHOW(argc, argv) 	\
	command_show(argc, argv)
#define COMMAND_SET(argc, argv) 	\
	command_set(argc, argv)
#define COMMAND_UNSET(argc, argv) 	\
	command_unset(argc, argv)
#define COMMAND_ECHO(argc, argv) 	\
	command_echo(argc, argv)
#define COMMAND_READ(argc, argv) 	\
	command_read(argc, argv)
#define COMMAND_MORE(argc, argv) 	\
	command_more(argc, argv)
#define COMMAND_LSDEV(argc, argv) 	\
	command_lsdev(argc, argv)
#define COMMAND_BOOT(argc, argv) 	\
	command_boot(argc, argv)
#define COMMAND_AUTOBOOT(argc, argv) \
	command_autoboot(argc, argv)
#define COMMAND_LOAD(argc, argv) 	\
	command_load(argc, argv)
#define COMMAND_UNLOAD(argc, argv) 	\
	command_unload(argc, argv)
#define COMMAND_LSKERN(argc, argv) 	\
	command_lskern(argc, argv)
#define COMMAND_INCLUDE(argc, argv) \
	command_include(argc, argv)
#define COMMAND_LS(argc, argv) 		\
	command_ls(argc, argv)

#endif /* _COMMANDS_H_ */
