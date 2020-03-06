/*
 * uthreads.h
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 *  User Threads: Runs from user struct in Userspace (usr_pcb)
 */

#ifndef SYS_UTHREADS_H_
#define SYS_UTHREADS_H_

#include <sys/user.h>
#include <kernthreads/kthread.h>

/* user threads */
struct uthread {
	struct uthread		*ut_nxt;		/* linked list of allocated thread slots */
	struct uthread		**ut_prev;

	struct uthread 		*ut_forw;		/* Doubly-linked run/sleep queue. */
	struct uthread 		*ut_back;

	int	 				ut_flag;		/* T_* flags. */
	char 				ut_stat;		/* TS* thread status. */
	char 				ut_lock;		/* Thread lock count. */

	short 				ut_tid;			/* unique thread id */
	short 				ut_ptid;		/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*ut_cred;		/* Thread owner's identity. */
	struct filedesc 	*ut_fd;			/* Ptr to open files structure. */
	struct pstats 	 	*ut_stats;		/* Accounting/statistics (PROC ONLY). */
	struct sigacts 		*ut_sigacts;	/* Signal actions, state (PROC ONLY). */

	struct user 		*ut_userp;		/* Pointer to User Structure */
	struct kthread		*ut_kthreadp;	/* Pointer to Kernel Thread Structure */

	struct uthread    	*ut_hash;       /* hashed based on t_tid & p_pid for kill+exit+... */
	struct uthread    	*ut_tgrpnxt;	/* Pointer to next thread in thread group. */
    struct uthread      *ut_tptr;		/* pointer to process structure of parent */
    struct uthread 		*ut_ostptr;	 	/* Pointer to older sibling processes. */

	struct uthread 		*ut_ysptr;	 	/* Pointer to younger siblings. */
	struct uthread 		*ut_cptr;	 	/* Pointer to youngest living child. */

	struct tgrp 	    *ut_tgrp;       /* Pointer to thread group. */

	struct mutex        *ut_mutex;
    short               ut_locks;
    short               ut_simple_locks;
};
#define	ut_session		ut_tgrp->tg_session
#define	ut_tgid			ut_tgrp->tg_id

#define UTHREAD_RATIO 1  /* M:N Ratio. number of user threads to kernel threads */

extern struct uthread *utidhash[];		/* In param.c. */

struct uthread 	*utfind (tid_t);		/* Find user thread by id. */
int				leavetgrp(struct uthread *);
int				entertgrp(struct uthread *, tid_t, int);
void			fixjobc(struct uthread *, struct tgrp *, int);
int				inferior(struct uthread *);

/* User Thread */
int uthread_create(uthread_t *ut);
int uthread_join(uthread_t *ut);
int uthread_cancel(uthread_t *ut);
int uthread_exit(uthread_t *ut);
int uthread_detach(uthread_t *ut);
int uthread_equal(uthread_t *ut1, uthread_t *ut2);
int uthread_kill(uthread_t *ut);

/* User Thread Mutex */
int uthread_mutexmgr(mutex_t m, unsigned int flags, struct simplelock *interlkp, uthread_t ut);
int uthread_mutex_init(mutex_t m, uthread_t ut);
int uthread_mutex_lock(uthread_t ut, mutex_t m);
int uthread_mutex_lock_try(uthread_t ut, mutex_t m);
int uthread_mutex_timedlock(uthread_t ut, mutex_t m);
int uthread_mutex_unlock(uthread_t ut, mutex_t m);
int uthread_mutex_destroy(uthread_t ut, mutex_t m);

#endif /* SYS_UTHREADS_H_ */
