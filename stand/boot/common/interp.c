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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

/*
 * Simple commandline interpreter, toplevel and misc.
 *
 * XXX may be obsoleted by BootFORTH or some other, better, interpreter.
 */
#include <lib/libsa/loadfile.h>
#include <lib/libkern/libkern.h>
#include <lib/libsa/stand.h>
#include <string.h>
#include "bootstrap.h"
#include "commands.h"

COMMAND_SET(include, "include", "read commands from a file", command_include);
COMMAND_SET(optinclude, "optinclude", "run commands from file; ignore exit status", command_optinclude);

#define	MAXARGS	20			/* maximum number of arguments allowed */

/*
 * Interactive mode
 */
void
interact(void)
{
	static char				input[256];		/* big enough? */
	const char * volatile	interp_identifier;

	/*
	 * Because interp_identifier is volatile, it cannot be optimized out by
	 * the compiler as it's considered an externally observable event.  This
	 * prevents the compiler from optimizing out our carefully placed
	 * $Interpreter:4th string that userboot may use to determine that
	 * we need to switch interpreters.
	 */
	interp_identifier = bootprog_interp;
	interp_init();

	printf("\n");

	/*
	 * Before interacting, we might want to autoboot.
	 */
	autoboot_maybe();

	/*
	 * Not autobooting, go manual
	 */
	printf(
			"\nType '?' for a list of commands, 'help' for more detailed help.\n");
	if (getenv("prompt") == NULL)
		setenv("prompt", "${interpret}", 1);
	if (getenv("interpret") == NULL)
		setenv("interpret", "OK", 1);

	for (;;) {
		input[0] = '\0';
		interp_emit_prompt();
		ngets(input, sizeof(input));
		interp_run(input);
	}
}

/*
 * Read commands from a file, then execute them.
 *
 * We store the commands in memory and close the source file so that the media
 * holding it can safely go away while we are executing.
 *
 * Commands may be prefixed with '@' (so they aren't displayed) or '-' (so
 * that the script won't stop if they fail).
 */
int
command_include(int argc, char *argv[])
{
	int i;
	int res;
	char **argvbuf;

	/*
	 * Since argv is static, we need to save it here.
	 */
	argvbuf = (char**) calloc((u_int) argc, sizeof(char*));
	for (i = 0; i < argc; i++)
		argvbuf[i] = strdup(argv[i]);

	res = CMD_OK;
	for (i = 1; (i < argc) && (res == CMD_OK); i++)
		res = interp_include(argvbuf[i]);

	for (i = 0; i < argc; i++)
		free(argvbuf[i]);
	free(argvbuf);

	return (res);
}

int
command_optinclude(int argc, char *argv[])
{
    int		i;
    char	**argvbuf;

	/*
	 * Since argv is static, we need to save it here.
	 */
	argvbuf = (char**) calloc((u_int) argc, sizeof(char*));
	for (i = 0; i < argc; i++)
		argvbuf[i] = strdup(argv[i]);

	for (i = 1; (i < argc); i++)
		interp_include(argvbuf[i]);

	for (i = 0; i < argc; i++)
		free(argvbuf[i]);
	free(argvbuf);

	return (CMD_OK);
}

/*
 * Emit the current prompt; use the same syntax as the parser
 * for embedding environment variables. Does not accept input.
 */
void
interp_emit_prompt(void)
{
	char		*pr, *p, *cp, *ev;

	if ((cp = getenv("prompt")) == NULL)
		cp = ">";
	pr = p = strdup(cp);

	while (*p != 0) {
		if ((*p == '$') && (*(p + 1) == '{')) {
			for (cp = p + 2; (*cp != 0) && (*cp != '}'); cp++)
				;
			*cp = 0;
			ev = getenv(p + 2);

			if (ev != NULL)
				printf("%s", ev);
			p = cp + 1;
			continue;
		}
		putchar(*p++);
	}
	putchar(' ');
	free(pr);
}

void
interp_search_set(struct bootblk_command **cmdp, bootblk_cmd_t	*cmd)
{
	SET_FOREACH(cmdp, Xcommand_set) {
		if (((*cmdp)->c_name != NULL) && !strcmp(argv[0], (*cmdp)->c_name)) {
			cmd = (*cmdp)->c_fn;
		}
	}
}

/*
 * Perform a builtin command
 */
int
interp_builtin_cmd(int argc, char *argv[])
{
	int						i, result;
	struct bootblk_command	*cmdp;
	bootblk_cmd_t			*cmd;

	if (argc < 1) {
		return (CMD_OK);
	}

	/* set return defaults; a successful command will override these */
	command_seterr("no error message");
	cmd = NULL;
	result = CMD_ERROR;

	/* search the command set for the command */
	for (i = 0, cmdp = commands;
			(cmdp->c_name != NULL) && (cmdp->c_desc != NULL);
			i++, cmdp = commands + i) {
		if ((cmdp->c_name != NULL) && !strcmp(argv[0], cmdp->c_name)) {
			cmd = cmdp->c_fn;
		}
	}
	if (cmd != NULL) {
		result = (cmd)(argc, argv);
	} else {
		command_seterr("unknown command");
	}
	return (result);
}

int
interp_run(const char *input)
{
	int			argc;
	char		**argv;

	if (parse(&argc, &argv, input)) {
		printf("parse error\n");
		return CMD_ERROR;
	}

	if (interp_builtin_cmd(argc, argv)) {
		printf("%s: %s\n", argv[0], command_errmsg);
		free(argv);
		return CMD_ERROR;
	}
	free(argv);
	return CMD_OK;
}

struct includeline {
    char				*text;
    int					flags;
    int					line;
#define SL_QUIET		(1<<0)
#define SL_IGNOREERR	(1<<1)
    struct includeline	*next;
};

int
interp_include(const char *filename)
{
	struct includeline	*script, *se, *sp;
	char				input[256];			/* big enough? */
	int					argc,res;
	char				**argv, *cp;
	int					fd, flags, line;

	if (((fd = open(filename, O_RDONLY)) == -1)) {
		command_seterr("can't open '%s': %s\n", filename, strerror(errno));
		return (CMD_ERROR);
	}
	/*
	 * Read the script into memory.
	 */
	script = se = NULL;
	line = 0;

	while (fgetstr(input, sizeof(input), fd) >= 0) {
		line++;
		flags = 0;
		/* Discard comments */
		if (strncmp(input + strspn(input, " "), "\\", 1) == 0)
			continue;
		cp = input;
		/* Echo? */
		if (input[0] == '@') {
			cp++;
			flags |= SL_QUIET;
		}
		/* Error OK? */
		if (input[0] == '-') {
			cp++;
			flags |= SL_IGNOREERR;
		}

		/* Allocate script line structure and copy line, flags */
		if (*cp == '\0')
			continue; /* ignore empty line, save memory */
		sp = malloc(sizeof(struct includeline) + strlen(cp) + 1);
		/* On malloc failure (it happens!), free as much as possible and exit */
		if (sp == NULL) {
			while (script != NULL) {
				se = script;
				script = script->next;
				free(se);
			}
			command_seterr(
					"file '%s' line %d: memory allocation failure - aborting",
					filename, line);
			close(fd);
			return (CMD_ERROR);
		}
		strcpy(sp->text, cp);
		sp->flags = flags;
		sp->line = line;
		sp->next = NULL;

		if (script == NULL) {
			script = sp;
		} else {
			se->next = sp;
		}
		se = sp;
	}
	close(fd);

	/*
	 * Execute the script
	 */
	argv = NULL;
	res = CMD_OK;
	for (sp = script; sp != NULL; sp = sp->next) {

		/* print if not being quiet */
		if (!(sp->flags & SL_QUIET)) {
			interp_emit_prompt();
			printf("%s\n", sp->text);
		}

		/* Parse the command */
		if (!parse(&argc, &argv, sp->text)) {
			if ((argc > 0) && (interp_builtin_cmd(argc, argv) != 0)) {
				/* normal command */
				printf("%s: %s\n", argv[0], command_geterr());
				if (!(sp->flags & SL_IGNOREERR)) {
					res = CMD_ERROR;
					break;
				}
			}
			free(argv);
			argv = NULL;
		} else {
			printf("%s line %d: parse error\n", filename, sp->line);
			res = CMD_ERROR;
			break;
		}
	}
	if (argv != NULL)
		free(argv);

	while (script != NULL) {
		se = script;
		script = script->next;
		free(se);
	}
	return (res);
}
