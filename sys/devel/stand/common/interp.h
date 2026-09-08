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

#ifndef _INTERP_H_
#define _INTERP_H_

/* interp.c */
void interact(void);
void interp_emit_prompt(void);
int interp_builtin_cmd(int, char **);

/* Called by interp.c for interp_*.c embedded interpreters */
int	interp_include(const char *);	/* Execute commands from filename */
void interp_init(void);				/* Initialize interpreater */
int interp_run(const char *);		/* Run a single command */

/* interp_backslash.c */
char *backslash(const char *);

/* interp_parse.c */
int parse(int *, char ***, const char *);

/*
 * Interpreter information
 */
extern const char bootprog_interp[];
#define	INTERP_DEFINE(interpstr) \
	const char bootprog_interp[] = "$Interpreter:" interpstr

#endif /* _INTERP_H_ */
