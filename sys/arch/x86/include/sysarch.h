/*	$NetBSD: sysarch.h,v 1.7.10.1 2009/04/04 17:39:09 snj Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _X86_SYSARCH_H_
#define _X86_SYSARCH_H_

#include <i386/include/sysarch.h>

#define	X86_PMC_INFO		8
#define	X86_PMC_STARTSTOP	9
#define	X86_PMC_READ		10
#define X86_GET_MTRR		11
#define X86_SET_MTRR		12
#define	X86_VM86			13
#define	X86_GET_GSBASE		14
#define	X86_GET_FSBASE		15
#define	X86_SET_GSBASE		16
#define	X86_SET_FSBASE		17

#define	PMC_TYPE_NONE		0
#define	PMC_TYPE_I586		1
#define	PMC_TYPE_I686		2
#define	PMC_TYPE_K7			3

#define	PMC_INFO_HASTSC		0x01
#define	PMC_NCOUNTERS		2

/*
 * Architecture specific syscalls (x86)
 */
struct x86_get_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

struct x86_set_ldt_args {
	int start;
	union descriptor *desc;
	int num;
};

struct x86_iopl_args {
	int iopl;
};

struct x86_get_ioperm_args {
	u_long *iomap;
};

struct x86_set_ioperm_args {
	u_long *iomap;
};


#ifndef _KERNEL
int x86_get_ldt (int, union descriptor *, int);
int x86_set_ldt (int, union descriptor *, int);
int x86_iopl (int);
int x86_get_ioperm (u_long *);
int x86_set_ioperm (u_long *);
int sysarch (int, void *);
#endif

#endif /* !_X86_SYSARCH_H_ */
