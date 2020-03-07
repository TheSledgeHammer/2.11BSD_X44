//
// Created by marti on 12/02/2020.
//

#ifndef SIMPLE_LIST_OF_POOLS_SCHED_H
#define SIMPLE_LIST_OF_POOLS_SCHED_H

#include "proc.h"

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

#endif //SIMPLE_LIST_OF_POOLS_SCHED_H
