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

#ifndef _SYS_POSIX4_H_
#define _SYS_POSIX4_H_

#define SCHED_NONE  	-1
#define SCHED_FIFO  	1
#define SCHED_OTHER 	2
#define SCHED_RR    	3

struct sched_param {
	int sched_priority;
};

struct sched_posix {
	struct timespec rr_interval;
};

#define SCHED_PRIO_MIN 	0
#define SCHED_PRIO_MAX 	31

int ksched_attach(struct sched_posix **);
int ksched_detach(struct sched_posix *);
int ksched_setparam(pid_t, int, struct sched_posix *, const struct sched_param *);
int ksched_getparam(pid_t, int, struct sched_posix *, const struct sched_param *);
int ksched_setscheduler(pid_t, int, struct sched_posix *, struct sched_param *);
int ksched_getscheduler(pid_t, int, struct sched_posix *, struct sched_param *);
int ksched_yield(struct proc *, struct sched_posix *);
int ksched_get_priority_max(struct proc *, int, struct sched_posix *);
int ksched_get_priority_min(struct proc *, int, struct sched_posix *);
int ksched_rr_get_interval(struct proc *, struct sched_posix *, struct timespec *);

#endif /* _SYS_POSIX4_H_ */
