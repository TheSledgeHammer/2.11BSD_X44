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

#ifndef _SYS_SCHED_POSIX_H_
#define _SYS_SCHED_POSIX_H_

#include <sys/cdefs.h>

#define SCHED_NONE  	-1
#define SCHED_FIFO  	1
#define SCHED_OTHER 	2
#define SCHED_RR    	3

struct sched_param {
	int sched_priority;
};

#define M_P31B 106

/*
 * cmd options for posix syscalls
 * - setparam, getparam
 * - setscheduler, getscheduler, yield
 * - get_priority_min, get_priority_max
 * - get_rr_interval
 */
enum posix_cmdops {
	SETPARAM,
	GETPARAM,
	SETSCHED,
	GETSCHED,
	YIELD,
	GETPRIOMAX,
	GETPRIOMIN,
	GETRRINTRVAL
};

#ifdef _KERNEL
void sched_posix_init(void);

#else /* !_KERNEL */
__BEGIN_DECLS
int posix_schedule(int, pid_t, int, struct sched_param *, struct timespec *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* _SYS_SCHED_POSIX_H_ */
