/*
 * kern_threads.c
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 */


#include<test/multitasking/kthreads.h>

struct 	tgrp tgrp0;
struct 	kthread kthread0;

void
kthread_init(p)
	register struct proc *p;
{
	tgrp0 = p->p_pgrp;

}

struct kthread *
tfind(tid)
register int tid;
{
	register struct kthread *t = tidhash(TIDHASH(tid));
	for(; t; t = t->t_hash) {
		if(t->t_tid == tid) {
			return (t);
		}
	}
	return ((struct kthread *) 0);
}

struct tgrp *
tgfind(tgid)
	register tid_t tgid;
{
	register struct tgrp *tgrp;
	for(tgrp = tgrphash[TIDHASH(tgid)]; tgrp != NULL; tgrp = tgrp->tg_hforw) {
		if(tgrp->tg_id == tgid) {
			return (tgrp);
		}
	}
	return (NULL);
}

struct kthread *
thread_create(struct proc *p, int qty)
{
	return NULL;
}


void
uthread_init(p, t)
	register struct proc *p;
	register struct	kthread *t;
{

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
