/*
 * Thread Control Block:
 * Threads work nearly identically the same as proc in structure and operation.
 * Threads however cannot create child threads, this is done through proc. Threads have the same creds as the process
 * which created it.
 * Thread groups are linked to there proc group and cannot exceed the limit of a proc group and M:N ratio.
 * Example: If the M:N ratio (1:2) and prgp limit is 4. Then a Thread group limit would be 8 (2 Threads per Proc).
 * 
 */

#ifndef SYS_KTHREADS_H_
#define SYS_KTHREADS_H_

#include <sys/proc.h>
#include <devel/multitasking/tcb.h>

/* Number of Threads per Process? */

/* kernel threads */
struct kthread {
	struct kthread		*kt_nxt;		/* linked list of allocated thread slots */
	struct kthread		**kt_prev;

	struct kthread 		*kt_forw;		/* Doubly-linked run/sleep queue. */
	struct kthread 		*kt_back;

	int	 				kt_flag;		/* T_* flags. */
	char 				kt_stat;		/* TS* thread status. */
	char 				kt_lock;		/* Thread lock count. */

	short 				kt_tid;			/* unique thread id */
	short 				kt_ptid;		/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*kt_cred;		/* Thread owner's identity. */
	struct filedesc 	*kt_fd;			/* Ptr to open files structure. */
	struct pstats 	 	*kt_stats;		/* Accounting/statistics (PROC ONLY). */
	struct sigacts 		*kt_sig;		/* Signal actions, state (PROC ONLY). */

#define	t_ucred			t_cred->pc_ucred

	struct proc 		*kt_procp;		/* Pointer to Proc */
	struct uthread		*kt_uthreadp;	/* Pointer User Threads */

	struct kthread    	*kt_hash;       /* hashed based on t_tid & p_pid for kill+exit+... */
	struct kthread    	*kt_tgrpnxt;	/* Pointer to next thread in thread group. */
    struct kthread      *kt_tptr;		/* pointer to process structure of parent */
    struct kthread 		*kt_ostptr;	 	/* Pointer to older sibling processes. */

	struct kthread 		*kt_ysptr;	 	/* Pointer to younger siblings. */
	struct kthread 		*kt_cptr;	 	/* Pointer to youngest living child. */

	struct tgrp 	    *kt_tgrp;       /* Pointer to thread group. */
	struct sysentvec	*kt_sysent;		/* System call dispatch information. */

	struct kthread 		*kt_link;		/* linked list of running threads */

	struct mutex        *kt_mutex;
    short               kt_locks;
    short               kt_simple_locks;
};

#define	kt_session		kt_tgrp->tg_session
#define	kt_tgid			kt_tgrp->tg_id

#define KTHREAD_RATIO 1  /* M:N Ratio. number of kernel threads per process */


extern struct kthread *ktidhash[];		/* In param.c. */

struct kthread 	*ktfind (tid_t);		/* Find kernel thread by id. */
int	 			leavetgrp(struct kthread *);
int	 			entertgrp(struct kthread *, tid_t, int);
void 			fixjobc(struct kthread *, struct tgrp *, int);
int	 			inferior(struct kthread *);

/* Kernel Thread */
int kthread_create(kthread_t *kt);
int kthread_join(kthread_t *kt);
int kthread_cancel(kthread_t *kt);
int kthread_exit(kthread_t *kt);
int kthread_detach(kthread_t *kt);
int kthread_equal(kthread_t *kt1, kthread_t *kt2);
int kthread_kill(kthread_t *kt);

/* Kernel Thread Mutex */
int kthread_mutexmgr(mutex_t m, unsigned int flags, struct simplelock *interlkp, kthread_t kt);
int kthread_mutex_init(mutex_t m, kthread_t kt);
int kthread_mutex_lock(kthread_t *kt, mutex_t *m);
int kthread_mutex_lock_try(kthread_t *kt, mutex_t *m);
int kthread_mutex_timedlock(kthread_t *kt, mutex_t *m);
int kthread_mutex_unlock(kthread_t *kt, mutex_t *m);
int kthread_mutex_destroy(kthread_t *kt, mutex_t *m);

#endif /* SYS_KTHREADS_H_ */
