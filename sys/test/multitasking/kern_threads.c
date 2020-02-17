/*
 * kern_threads.c
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 */


#include <test/multitasking/kthreads.h>

struct 	tgrp tgrp0;
struct 	kthread kthread0;

void
kthread_init(p)
	register struct proc *p;
{
	tgrp0 = p->p_pgrp;

}

struct kthread *
ktfind(tid)
register int tid;
{
	register struct kthread *t = tidhash(TIDHASH(tid));
	for(; t; t = t->kt_hash) {
		if(t->kt_tid == tid) {
			return (t);
		}
	}
	return ((struct kthread *) 0);
}

struct kthread *
kthread_create(struct proc *p, int qty)
{
	return NULL;
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

/* Other functions */
