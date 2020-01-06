/*
 * uthreads.h
 *
 *  Created on: 3 Jan 2020
 *      Author: marti
 */

#ifndef SYS_UTHREADS_H_
#define SYS_UTHREADS_H_

#include <sys/test/multitasking/kthreads.h>

/* user threads */
struct uthread {
	struct uthread		*ut_nxt;			/* linked list of allocated thread slots */
	struct uthread		**ut_prev;

	struct uthread 		*ut_forw;		/* Doubly-linked run/sleep queue. */
	struct uthread 		*ut_back;

	int	 ut_flag;						/* T_* flags. */
	char ut_stat;						/* TS* thread status. */
	char ut_lock;						/* Thread lock count. */

	short ut_tid;						/* unique thread id */
	short ut_ptid;						/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*ut_cred;		/* Thread owner's identity. */
	struct filedesc 	*ut_fd;			/* Ptr to open files structure. */
	struct pstats 	 	*ut_stats;		/* Accounting/statistics (PROC ONLY). */
	struct sigacts 		*ut_sig;		/* Signal actions, state (PROC ONLY). */


	struct uthread    	*ut_hash;        /* hashed based on t_tid & p_pid for kill+exit+... */

	struct proc 		*p;				/* Pointer to Proc */
	struct uthread		*kth;			/* Pointer Kernel Threads */
};


#endif /* SYS_UTHREADS_H_ */
