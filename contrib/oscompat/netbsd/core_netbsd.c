/*	$NetBSD: core_netbsd.c,v 1.24 2019/11/20 19:37:53 pgoyette Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1991, 1993 The Regents of the University of California.
 * Copyright (c) 1988 University of Utah.
 *
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *
 * from: Utah $Hdr: vm_unix.c 1.1 89/11/07$
 *      @(#)vm_unix.c   8.1 (Berkeley) 6/11/93
 * from: NetBSD: uvm_unix.c,v 1.25 2001/11/10 07:37:01 lukem Exp
 */

/* Dummy file */

/*
 * core_netbsd.c: Support for the historic NetBSD core file format.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: core_netbsd.c,v 1.24 2019/11/20 19:37:53 pgoyette Exp $");

#include <sys/param.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"

#ifndef CORENAME
#define	CORENAME(x)	x
#endif

struct coredump_state {
	void *iocookie;
	struct CORENAME(core) core;
	off_t offset;
};

static int coredump_internal(char *, struct siginfo *, struct coredump_state *, void *);
int coredump_netbsd(void *, void *);

static int
coredump_internal(char *name, struct siginfo *sig, struct coredump_state *cs, void *iocookie)
{
	cs->iocookie = iocookie;
	cs->core.c_midmag = 0;
	if (name != NULL) {
		strncpy(cs->core.c_name, name, MAXCOMLEN);
	}
	cs->core.c_nseg = 0;
	cs->core.c_signo = sig->si_signo;
	cs->core.c_ucode = sig->si_code;
	cs->core.c_cpusize = 0;

	return (0);
}

int
coredump_netbsd(void *arg, void *iocookie)
{
	struct coredump_state *cs, css;
	
	cs = (struct coredump_state *)iocookie;
	if (cs == NULL) {
		cs = (struct coredump_state *)malloc(sizeof(*cs));
		if (cs == NULL) {
			return (-1);
		}
	}
	css = *cs;
	return (coredump_internal(NULL, (struct siginfo *)arg, &css, iocookie));
}
