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

/* posix scheduling for libc/sys */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/syscall.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>

int
posix_schedule(int cmd, pid_t pid, int policy, struct sched_param *param, struct timespec *interval)
{
	return (__syscall((quad_t)SYS_posix_schedule, cmd, pid, policy, param, interval));
}

int
sched_setparam(pid_t pid, const struct sched_param *param)
{
	struct sched_param sp;

	memset(&sp, 0, sizeof(struct sched_param));
	sp.sched_priority = param->sched_priority;
	return (posix_schedule(SETPARAM, pid, SCHED_NONE, &sp, NULL));
}

int
sched_getparam(pid_t pid, struct sched_param *param)
{
	return (posix_schedule(GETPARAM, pid, NULL, param, NULL));
}

int
sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
	struct sched_param sp;

	memset(&sp, 0, sizeof(struct sched_param));
	sp.sched_priority = param->sched_priority;
	return (posix_schedule(SETSCHED, pid, policy, &sp, NULL));
}

int
sched_getscheduler(pid_t pid)
{
	struct sched_param sp;
	int policy;

	return (posix_schedule(GETSCHED, pid, NULL, &sp, NULL));
}

int
sched_get_priority_max(int policy)
{
	return (posix_schedule(GETPRIOMAX, NULL, policy, NULL, NULL));
}

int
sched_get_priority_min(int policy)
{
	return (posix_schedule(GETPRIOMIN, NULL, policy, NULL, NULL));
}

int
sched_rr_get_interval2(pid_t pid, struct timespec *interval)
{
	return (posix_schedule(GETPRIOMIN, pid, NULL, NULL, interval));
}
