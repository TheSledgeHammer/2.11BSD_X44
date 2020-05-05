/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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

#ifndef _SYS_SCHED_H
#define _SYS_SCHED_H

#include <sys/proc.h>

/* Schedulers + Info used by both EDF & CFS */
struct sched {
    struct proc 		*sc_proc;
    struct sched_edf	*sc_edf;
    struct sched_cfs 	*sc_cfs;

    u_char  			sc_priweight;	/* priority weighting */

    char 				sc_release;		/* Time till release from current block */
    char 				sc_deadline;	/* Deadline */
    char 				sc_remtime; 	/* time remaining */
};

/* Scheduler Domains: Hyperthreading, multi-cpu, etc... */
struct sched_domains {

};

/* Priority Weighting Factors */
#define PW_PRIORITY 25      /* Current Processes Priority */
#define PW_DEADLINE 25      /* Current Processes Deadline Time */
#define PW_RELEASE  25      /* Current Processes Release Time */
#define PW_SLEEP    25      /* Current Processes Sleep Time */

#define PW_FACTOR(w, f)  ((float)(w) / 100 * (f)) /* w's weighting for a given factor(f)(above) */

int			setpriweight(float pwp, float pwd, float pwr, float pws);
void 		updatepri(struct proc *);
int 		tsleep(caddr_t , int, u_short);
void 		endtsleep(struct proc *);
void 		sleep(caddr_t , int);
void 		unsleep(struct proc *);
void 		wakeup(register caddr_t);
void 		setrun(struct proc *);
int 		setpri(struct proc *);
void 		setrq(struct proc *);
void 		remrq(struct proc *);
struct proc *getrq(struct proc *);
void 		resetpri(struct proc *);

void 		getpriority();
void 		setpriority();

#endif /* _SYS_SCHED_H */
