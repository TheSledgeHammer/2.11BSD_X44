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
