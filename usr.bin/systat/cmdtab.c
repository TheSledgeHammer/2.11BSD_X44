/*-
 * Copyright (c) 1980, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)cmdtab.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include "systat.h"
#include "extern.h"

struct cmdtab cmdtab[] = {
		{
				.c_name = "pigs",
				.c_refresh = showpigs,
				.c_fetch = fetchpigs,
				.c_label = labelpigs,
				.c_init = initpigs,
				.c_open = openpigs,
				.c_close = closepigs,
				.c_cmd = 0,
				.c_flags = CF_LOADAV,
		},
		{
				.c_name = "swap",
				.c_refresh = showswap,
				.c_fetch = fetchswap,
				.c_label = labelswap,
				.c_init = initswap,
				.c_open = openswap,
				.c_close = closeswap,
				.c_cmd = 0,
				.c_flags = CF_LOADAV,
		},
		{
				.c_name = "mbufs",
				.c_refresh = showmbufs,
				.c_fetch = fetchmbufs,
				.c_label = labelmbufs,
				.c_init = initmbufs,
				.c_open = openmbufs,
				.c_close = closembufs,
				.c_cmd = 0,
				.c_flags = CF_LOADAV,
		},
		{
				.c_name = "iostat",
				.c_refresh = showiostat,
				.c_fetch = fetchiostat,
				.c_label = labeliostat,
				.c_init = initiostat,
				.c_open = openiostat,
				.c_close = closeiostat,
				.c_cmd = cmdiostat,
				.c_flags = CF_LOADAV,
		},
		{
				.c_name = "vmstat",
				.c_refresh = showkre,
				.c_fetch = fetchkre,
				.c_label = labelkre,
				.c_init = initkre,
				.c_open = openkre,
				.c_close = closekre,
				.c_cmd = cmdkre,
				.c_flags = 0,
		},
		{
				.c_name = "netstat",
				.c_refresh = shownetstat,
				.c_fetch = fetchnetstat,
				.c_label = labelnetstat,
				.c_init = initnetstat,
				.c_open = opennetstat,
				.c_close = closenetstat,
				.c_cmd = cmdnetstat,
				.c_flags = CF_LOADAV,
		},
};

struct  cmdtab *curcmd = &cmdtab[0];
