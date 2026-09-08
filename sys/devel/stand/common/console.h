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
 * $FreeBSD$
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

/*
 * Modular console support.
 */
struct console {
    const char *c_name;
    const char *c_desc;
    int	c_flags;
#define C_PRESENTIN		(1<<0)	    	/* console can provide input */
#define C_PRESENTOUT	(1<<1)	    	/* console can provide output */
#define C_ACTIVEIN		(1<<2)	    	/* user wants input from console */
#define C_ACTIVEOUT		(1<<3)	    	/* user wants output to console */
#define	C_WIDEOUT		(1<<4)	   		/* c_out routine groks wide chars */
    void (*c_probe)(struct console *);	/* set c_flags to match hardware */
    int (*c_init)(int);					/* reinit XXX may need more args */
    void (*c_out)(int);					/* emit c */
    int	(*c_in)(void);					/* wait for and return input */
    int (*c_ready)(void);				/* return nonzer if input waiting */
};

extern struct console	*consoles[];
void cons_probe(void);

#endif /* _CONSOLE_H_ */
