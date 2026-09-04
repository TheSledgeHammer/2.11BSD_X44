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
 * Copyright (c) 1996, 1997, 1998
 *	HD Associates, Inc.  All rights reserved.
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
 *	This product includes software developed by HD Associates, Inc
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY HD ASSOCIATES AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL HD ASSOCIATES OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/posix4/p1003_1b.c,v 1.5.2.2 2003/03/25 06:13:35 rwatson Exp $
 */

/*
 * p1003_1b: Real Time common code.
 */

/*
 * TODO:
 * - Tweak setscheduler and getscheduler
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <devel/sys/posix4.h>
#include <devel/sys/sched_posix.h>

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
sched_posix_attach(void)
{
	return (ksched_attach(&ksched));
}

void
sched_posix_detach(void)
{
	ksched_detach(ksched);
}

void
sched_posix_init(void)
{
	int error;

	error = sched_posix_attach();
	if (error != 0) {
		sched_posix_detach();
		panic("sched_posix_init: failed to start");
	}
}

int
sched_setparam(p, cmd, pid, policy, param)
	struct proc *p;
	int cmd;
	pid_t pid;
	int policy;
	const struct sched_param *param;
{
	struct sched_param sched_param;
	int error;

	if (cmd != SETPARAM) {
		return (EINVAL);
	}
	error = copyin(param, &sched_param, sizeof(sched_param));
	if (error) {
		return (error);
	}
	error = p31b_proc(p, pid, &p);
	if (error) {
		return (error);
	}
	return (ksched_setparam(pid, policy, ksched, (const struct sched_param *)&sched_param));
}

int
sched_getparam(p, cmd, pid, policy, param)
	struct proc *p;
	int cmd;
	pid_t pid;
	int policy;
	struct sched_param *param;
{
	struct proc *targetp;
	struct sched_param sched_param;
	int error;

	if (cmd != GETPARAM) {
		return (EINVAL);
	}

	error = p31b_proc(p, pid, &targetp);
	if (error) {
		return (error);
	}
	error = ksched_getparam(pid, policy, ksched, &sched_param);
	if (error) {
		error = copyout(&sched_param, param, sizeof(sched_param));
	}
	return (error);
}

int
sched_setscheduler(p, cmd, pid, policy, param)
	struct proc *p;
	int cmd;
	pid_t pid;
	int policy;
	const struct sched_param *param;
{
	struct sched_param sched_param;
	int error;

	if (cmd != SETSCHED) {
		return (EINVAL);
	}
	error = copyin(param, &sched_param, sizeof(sched_param));
	if (error) {
		return (error);
	}
	error = p31b_proc(p, pid, &p);
	if (error) {
		return (error);
	}
	return (ksched_setscheduler(pid, policy, ksched, (const struct sched_param *)&sched_param));
}

int
sched_getscheduler(p, cmd, pid, policy, param)
	struct proc *p;
	int cmd;
	pid_t pid;
	int policy;
	struct sched_param *param;
{
	struct proc *targetp;
	struct sched_param sched_param;
	int error;

	if (cmd != GETSCHED) {
		return (EINVAL);
	}
	error = p31b_proc(p, pid, &targetp);
	if (error) {
		return (error);
	}
	error = ksched_getscheduler(pid, policy, ksched, &sched_param);
	if (error) {
		error = copyout(&sched_param, param, sizeof(sched_param));
	}
	return (error);
}

int
sched_yield(p, cmd)
	struct proc *p;
	int cmd;
{
	if (cmd != YIELD) {
		return (EINVAL);
	}
	return (ksched_yield(p, ksched));
}

int
sched_get_priority_max(p, cmd, policy)
	struct proc *p;
	int cmd, policy;
{
	if (cmd != GETPRIOMAX) {
		return (EINVAL);
	}
	return (ksched_get_priority_max(p, policy, ksched));
}

int
sched_get_priority_min(p, cmd, policy)
	struct proc *p;
	int cmd, policy;
{
	if (cmd != GETPRIOMIN) {
		return (EINVAL);
	}
	return (ksched_get_priority_min(p, policy, ksched));
}

int
sched_rr_get_interval(p, cmd, pid, timespec)
	struct proc *p;
	int cmd;
	pid_t pid;
	struct timespec *timespec;
{
	int error;

	if (cmd != GETRRINTRVAL) {
		return (EINVAL);
	}
	error = p31b_proc(p, pid, &p);
	if (error) {
		return (error);
	}
	return (ksched_rr_get_interval(p, ksched, timespec));
}

int
posix_schedule()
{
	register struct posix_schedule_args {
		syscallarg(int) cmd;
		syscallarg(pid_t) pid;
		syscallarg(int) policy;
		syscallarg(struct sched_param *) param;
		syscallarg(struct timespec *) interval;
	} *uap = u.u_ap;
	struct proc *p;

	p = u.u_procp;

	switch (SCARG(uap, cmd)) {
	case SETPARAM:
		u.u_error = sched_setparam(p, SCARG(uap, cmd), SCARG(uap, pid),
				SCARG(uap, policy), (const struct sched_param *)SCARG(uap, param));
		break;
	case GETPARAM:
		u.u_error = sched_getparam(p, SCARG(uap, cmd), SCARG(uap, pid),
				SCARG(uap, policy), SCARG(uap, param));
		break;
	case SETSCHED:
		u.u_error = sched_setscheduler(p, SCARG(uap, cmd), SCARG(uap, pid),
				SCARG(uap, policy), (const struct sched_param *)SCARG(uap, param));
		break;
	case GETSCHED:
		u.u_error = sched_getscheduler(p, SCARG(uap, cmd), SCARG(uap, pid),
				SCARG(uap, policy), SCARG(uap, param));
		break;
	case YIELD:
		u.u_error = sched_yield(p, SCARG(uap, cmd));
		break;
	case GETPRIOMAX:
		u.u_error = sched_get_priority_max(p, SCARG(uap, cmd),
				SCARG(uap, policy));
		break;
	case GETPRIOMIN:
		u.u_error = sched_get_priority_min(p, SCARG(uap, cmd),
				SCARG(uap, policy));
		break;
	case GETRRINTRVAL:
		u.u_error = sched_rr_get_interval(p, SCARG(uap, cmd), SCARG(uap, pid),
				SCARG(uap, interval));
		break;
	default:
		u.u_error = ENIVAL;
		break;
	}
	return (u.u_error);
}
