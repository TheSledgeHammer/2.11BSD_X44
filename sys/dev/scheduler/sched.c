//
// Created by marti on 12/02/2020.
//
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "sched.h"
//#include "sched_cfs.h"
/*
void
sched_init()
{
    struct sched_rq *rq = (struct sched_rq *)malloc(sizeof(struct sched_rq *));
    TAILQ_INIT(&rq->rq_head);
}

void
add_next(rq)
    struct sched_rq *rq;
{

    TAILQ_INSERT_HEAD(&rq->rq_head, rq->cfs, rq_list);
}

add_prev()
{

}

get_next()
{

}

get_prev()
{

}

enqueue(cfs, p)
    struct sched_cfs *cfs;
{

}

dequeue()
{

}


//Negative means a higher weighting
int
setpriweight(pwp, pwd, pwr, pws)
	float pwp, pwd, pwr, pws;
{
	int pw_pri = PW_FACTOR(pwp, PW_PRIORITY);
	int pw_dead = PW_FACTOR(pwd, PW_DEADLINE);
	int pw_rel = PW_FACTOR(pwr, PW_RELEASE);
	int pw_slp = PW_FACTOR(pws, PW_SLEEP);

	int priweight = ((pw_pri + pw_dead + pw_rel + pw_slp) / 4);

	if(priweight > 0) {
		priweight = priweight * - 1;
	}

	return (priweight);
}

*/
