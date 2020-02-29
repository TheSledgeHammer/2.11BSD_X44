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
/* __FBSDID("$FreeBSD$"); */

#include <sys/param.h>		/* to pick up __FreeBSD_version */
#include <sys/libkern.h>
#include <lib/libsa/loadfile.h>
#include <boot/libsa/stand.h>
#include "bootstrap.h"
#include "ficl.h"

extern unsigned bootprog_rev;
INTERP_DEFINE("4th");

/* #define BFORTH_DEBUG */

#ifdef BFORTH_DEBUG
#define	DPRINTF(fmt, args...)	printf("%s: " fmt "\n" , __func__ , ## args)
#else
#define	DPRINTF(fmt, args...)	((void)0)
#endif

/*
 * Eventually, all builtin commands throw codes must be defined
 * elsewhere, possibly bootstrap.h. For now, just this code, used
 * just in this file, it is getting defined.
 */
#define BF_PARSE 100

/*
 * FreeBSD loader default dictionary cells
 */
#ifndef	BF_DICTSIZE
#define	BF_DICTSIZE	10000
#endif

/*
 * BootForth   Interface to Ficl Forth interpreter.
 */

FICL_SYSTEM *bf_sys;
FICL_VM		*bf_vm;

/*
 * Shim for taking commands from BF and passing them out to 'standard'
 * argv/argc command functions.
 */
static void
bf_command(FICL_VM *vm)
{

}

/*
 * Initialise the Forth interpreter, create all our commands as words.
 */
void
bf_init(void)
{

}

/*
 * Feed a line of user input to the Forth interpreter
 */
static int
bf_run(const char *line)
{

}

void
interp_init(void)
{

}

int
interp_run(const char *input)
{

}

int
interp_include(const char *filename)
{

}
