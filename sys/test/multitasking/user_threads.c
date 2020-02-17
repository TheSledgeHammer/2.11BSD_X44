/*
 * user_threads.c
 *
 *  Created on: 17 Feb 2020
 *      Author: marti
 */

struct 	tgrp tgrp0;
struct 	uthread uthread0;

void
uthread_init(p, t)
	register struct proc *p;
	register struct	kthread *t;
{

}

struct uthread *
uthread_create(struct proc *p, int qty)
{
	return NULL;
}

struct uthread *
utfind(tid)
	register int tid;
{
	register struct uthread *ut = tidhash(TIDHASH(tid));
	for(; ut; ut = ut->ut_hash) {
		if(ut->ut_tid == tid) {
			return (ut);
		}
	}
	return ((struct uthread *) 0);
}

/* Other functions */
