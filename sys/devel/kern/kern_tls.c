/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/sysdecl.h>
#include <sys/tls.h>

/* MD tls (set and get) */
int
cpu_get_tls_tcb(p, arg, which)
	struct proc *p;
	void *arg;
	char which;
{
	return (i386_get_sdbase(p, arg, which));
}

int
cpu_set_tls_tcb(p, arg, which)
	struct proc *p;
	void *arg;
	char which;
{
	return (i386_set_sdbase(p, arg, which));
}

/* MI tls (set and get) */
static int kern_get_tls(struct proc *, int, void *, char);
static int kern_set_tls(struct proc *, int, void *, char);

static int
kern_get_tls(p, cmd, param, which)
	struct proc *p;
	int cmd;
	void *param;
	char which;
{
	int error;

	if (cmd != GETTLS) {
		return (EINVAL);
	}
	switch (which) {
	case FSBASE:
		error = cpu_get_tls_tcb(p, param, which);
		break;
	case GSBASE:
		error = cpu_get_tls_tcb(p, param, which);
		break;
	}
	return (error);
}

static int
kern_set_tls(p, cmd, param, which)
	struct proc *p;
	int cmd;
	void *param;
	char which;
{
	int error;

	if (cmd != SETTLS) {
		return (EINVAL);
	}
	switch (which) {
	case FSBASE:
		error = cpu_set_tls_tcb(p, param, which);
		break;
	case GSBASE:
		error = cpu_set_tls_tcb(p, param, which);
		break;
	}
	return (error);
}

int
tls()
{
	register struct tls_args {
		syscallarg(int) cmd;
		syscallarg(void *) param;
		syscallarg(char) which;
	} *uap = (struct tls_args *)u.u_ap;
	register struct proc *p;
	int error;

	p = u.u_procp;
	switch (SCARG(uap, cmd)) {
	case GETTLS:
		error = kern_get_tls(p, SCARG(uap, cmd), SCARG(uap, param), SCARG(uap, which));
		break;
	case SETTLS:
		error = kern_set_tls(p, SCARG(uap, cmd), SCARG(uap, param), SCARG(uap, which));
		break;
	default:
		error = EINVAL;
		break;
	}
	u.u_error = error;
	return (error);
}
