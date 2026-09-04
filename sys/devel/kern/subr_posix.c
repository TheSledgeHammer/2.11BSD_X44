/*
 * Copyright (c) 1996, 1997
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
 * $FreeBSD$
 */

/*
 * ksched: Soft real time scheduling based on "rtprio".
 */

#include <devel/sys/posix4.h>
#include <devel/sys/sched_posix.h>

#define M_P31B 106

#define p4pri_to_pri(p)	(SCHED_PRIO_MAX - (p))
#define pri_to_p4pri(p)	(SCHED_PRIO_MAX - (p))

#define P1B_PRIO_MIN 	pri_to_p4pri(SCHED_PRIO_MAX)
#define P1B_PRIO_MAX 	pri_to_p4pri(SCHED_PRIO_MIN)

static int
convert_pri(pri, policy, params)
	pri_t pri;
	int policy;
	const struct sched_param *params;
{
	int error = 0;

	switch (policy) {
	case SCHED_RR:
	case SCHED_FIFO:
		if (params->sched_priority >= P1B_PRIO_MIN
				&& params->sched_priority <= P1B_PRIO_MAX) {
			pri = p4pri_to_pri(params->sched_priority);
		} else {
			error = EINVAL;
		}
		break;
	case SCHED_OTHER:
		pri = p4pri_to_pri(params->sched_priority);
		break;
	}
	return (error);
}

static int
do_ksched_setparam(pid, policy, ksched, params)
	pid_t pid;
	int policy;
	struct sched_posix *ksched;
	const struct sched_param *params;
{
	struct proc *p;
	pri_t pri;

	pri = params->sched_priority;
	if (pri == -1 && policy == SCHED_NONE) {
		return (0);
	}

	if (policy != SCHED_NONE && (policy < SCHED_OTHER || policy > SCHED_RR)) {
		return (EINVAL);
	}

	if (pri != -1 && (pri < SCHED_PRIO_MIN || pri > SCHED_PRIO_MAX)) {
		return (EINVAL);
	}

	if (pid != 0) {
		p = pfind(pid);
		if (p == NULL) {
			return (ESRCH);
		}
	} else {
		p = curproc;
	}

	if (convert_pri(pri, policy, params)) {
		p->p_pri = pri;
		if (policy == (SCHED_FIFO | SCHED_OTHER)) {
			need_resched(p);
		}
	}
	return (0);
}

static int
do_ksched_getparam(pid, policy, ksched, params)
	pid_t pid;
	int *policy;
	struct sched_posix *ksched;
	struct sched_param *params;
{
	struct proc *p;
	int error = 0;

	p = pfind(pid);
	if (p == NULL) {
		return (ESRCH);
	}

	error = suser1(p->p_ucred, &p->p_acflag);
	if (error != 0) {
		return (error);
	}

	switch (policy) {
	case SCHED_OTHER:
		error = EINVAL;
		break;
	case SCHED_FIFO:
	case SCHED_RR:
		params->sched_priority = pri_to_p4pri(p->p_pri);
		break;
	}
	return (error);
}

int
ksched_attach(p)
	struct sched_posix **p;
{
	struct sched_posix *ksched;

	ksched = malloc(sizeof(struct sched_posix *), M_P31B, M_WAITOK);
	ksched->rr_interval.tv_sec = 0;
	ksched->rr_interval.tv_nsec = 1000000000L / 10;

	*p = ksched;
	return (0);
}

void
ksched_detach(p)
	struct sched_posix *p;
{
	free(p, M_P31B);
}

int
ksched_setparam(pid, policy, ksched, params)
	pid_t pid;
	int policy;
	struct sched_posix *ksched;
	const struct sched_param *params;
{
	return (do_ksched_setparam(pid, policy, ksched, params));
}

int
ksched_getparam(pid, policy, ksched, params)
	pid_t pid;
	int policy;
	struct sched_posix *ksched;
	const struct sched_param *params;
{
	return (do_ksched_getparam(pid, policy, ksched, params));
}

int
ksched_setscheduler(pid, policy, ksched, param)
	pid_t pid;
	int policy;
	struct sched_posix *ksched;
	const struct sched_param *param;
{
	return (do_ksched_setparam(pid, policy, ksched, param));
}

int
ksched_getscheduler(pid, policy, ksched, param)
	pid_t pid;
	int policy;
	struct sched_posix *ksched;
	struct sched_param *param;
{
	return (do_ksched_getparam(pid, policy, ksched, param));
}

int
ksched_yield(p, ksched)
	struct proc *p;
	struct sched_posix *ksched;
{
	need_resched(p);
	return (0);
}

int
ksched_get_priority_max(p, policy, ksched)
	struct proc *p;
	int policy;
	struct sched_posix *ksched;
{
	int error = -1;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_OTHER:
		error = PRIO_MAX;
		break;
	case SCHED_RR:
		error = SCHED_PRIO_MAX;
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

int
ksched_get_priority_min(p, policy, ksched)
	struct proc *p;
	int policy;
	struct sched_posix *ksched;
{
	int error = -1;

	switch (policy) {
	case SCHED_FIFO:
	case SCHED_OTHER:
		error = PRIO_MIN;
		break;
	case SCHED_RR:
		error = P1B_PRIO_MIN;
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

int
ksched_rr_get_interval(p, ksched, timespec)
	struct proc *p;
	struct sched_posix *ksched;
	struct timespec *timespec;
{
	*timespec = ksched->rr_interval;
	return (0);
}
