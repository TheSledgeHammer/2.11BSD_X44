/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

/*
 * TODO:
 * - Change a thread's priority
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <devel/sys/posix4.h>

#define M_P31B 106

#define CAN_AFFECT(p, pc, q) ((pc)->pc_ucred->cr_uid == 0)

static struct sched_posix *ksched;

int
p31b_proc(p, pid, pp)
	struct proc *p, **pp;
	pid_t pid;
{
	struct proc *op;
	int error;

	if (pid == 0) {
		op = p;
	} else {
		op = pfind(pid);
	}
	if (op != NULL) {
		if (CAN_AFFECT(p, p->p_cred, op)) {
			*pp = op;
		} else {
			error = EPERM;
		}
	} else {
		error = ESRCH;
	}
	return (error);
}

int
sched_attach(void)
{
	return (ksched_attach(&ksched));
}

int
sched_detach(void)
{
	return (ksched_detach(ksched));
}

int
sched_setparam(pid, policy, params)
	pid_t pid;
	int policy;
	struct sched_param *params;
{
	return (ksched_setparam(pid, policy, ksched, params));
}

int
sched_getparam(pid, policy, params)
	pid_t pid;
	int policy;
	struct sched_param *params;
{
	return (ksched_getparam(pid, policy, ksched, params));
}

int
sched_setscheduler(pid, policy, param)
	pid_t pid;
	int policy;
	struct sched_param *param;
{
	return (ksched_setscheduler(pid, policy, ksched, param));
}

int
sched_getscheduler(pid, policy, param)
	pid_t pid;
	int policy;
	struct sched_param *param;
{
	return (ksched_getscheduler(pid, policy, ksched, param));
}

int
sched_yield(p)
	struct proc *p;
{
	return (ksched_yield(p, ksched));
}

int
sched_get_priority_max(p, policy)
	struct proc *p;
	int policy;
{
	return (ksched_get_priority_max(p, policy, ksched));
}

int
sched_get_priority_min(p, policy)
	struct proc *p;
	int policy;
{
	return (ksched_get_priority_max(p, policy, ksched));
}

int
sched_rr_get_interval(p, timespec)
	struct proc *p;
	struct timespec *timespec;
{
	return (ksched_rr_get_interval(p, &ksched, timespec));
}

enum posixcmds {
	GETSCHED,
	GETPARAM,
	SETSCHED,
	SETPARAM
};

int
posix_setschedule()
{
	register struct posix_setschedule_args {
		syscallarg(int) cmd;
		syscallarg(pid_t) pid;
		syscallarg(int) policy;
		syscallarg(const struct sched_param *) param;
	} *uap = u.u_ap;

	struct proc *p;
	struct sched_param sched_param;
	int error;

	p = u.u_procp;


	error = copyin(SCARG(uap, param), &sched_param, sizeof(sched_param));

	error = p31b_proc(p, SCARG(uap, pid), &p);
	if (error) {
		return (error);
	}

	switch (SCARG(uap, cmd)) {
	case SETSCHED:
		error = ksched_setscheduler(SCARG(uap, pid), SCARG(uap, policy), ksched, SCARG(uap, param));
		break;
	case SETPARAM:
		error = ksched_setparam(SCARG(uap, pid), SCARG(uap, policy), ksched, SCARG(uap, param));
		break;
	default:
		error = ENIVAL;
	}

	return (error);
}

int
posix_getschedule()
{
	register struct posix_getschedule_args {
		syscallarg(int) cmd;
		syscallarg(pid_t) pid;
		syscallarg(int) policy;
		syscallarg(struct sched_param *) param;
	} *uap = u.u_ap;

	struct proc *p, *targetp;
	struct sched_param sched_param;
	int error;

	p = u.u_procp;

	error = p31b_proc(p, SCARG(uap, pid), &targetp);
	if (error) {
		return (error);
	} else {
		targetp = p;
	}

	switch (SCARG(uap, cmd)) {
	case GETSCHED:
		error = ksched_getscheduler(SCARG(uap, pid), SCARG(uap, policy), ksched, SCARG(uap, param));
		break;
	case GETPARAM:
		error = ksched_getparam(SCARG(uap, pid), SCARG(uap, policy), ksched, SCARG(uap, param));
		break;
	default:
		error = ENIVAL;
	}
	if (!error) {
		error = copyout(&sched_param, SCARG(uap, param), sizeof(sched_param));
	}
	return (error);
}
