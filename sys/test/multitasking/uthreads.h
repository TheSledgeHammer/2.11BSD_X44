/*
 * uthreads.h
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 *  User Threads: Runs from user struct in Userspace (usr_pcb)
 */

#ifndef SYS_UTHREADS_H_
#define SYS_UTHREADS_H_

#include <sys/test/multitasking/kthreads.h>
#include <sys/user.h>

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
};
#define	ut_session		ut_tgrp->tg_session
#define	ut_tgid			ut_tgrp->tg_id

#define UTHREAD_RATIO 1  /* M:N Ratio. number of user threads to kernel threads */

#define	TIDHASH(tid)	(&tidhashtbl[(tid) & tid_hash & (TIDHSZ * ((tid) + tid_hash) - 1)])
extern LIST_HEAD(tidhashhead, uthread) *tidhashtbl;
extern struct uthread *utidhash[];		/* In param.c. */

struct uthread 	*utfind (tid_t);		/* Find user thread by id. */

void			threadinit(void);
int				leavetgrp(struct uthread *);
int				entertgrp(struct uthread *, tid_t, int);
void			fixjobc(struct uthread *, struct tgrp *, int);
int				inferior(struct uthread *);

#endif /* SYS_UTHREADS_H_ */
