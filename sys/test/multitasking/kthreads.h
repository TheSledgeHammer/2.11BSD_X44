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

/* put into types.h */
typedef long				tid_t;		/* thread id */

/* Number of Threads per Process? */

/* kernel threads */
struct kthread {
	struct kthread		*t_nxt;			/* linked list of allocated thread slots */
	struct kthread		**t_prev;

	struct kthread 		*t_forw;		/* Doubly-linked run/sleep queue. */
	struct kthread 		*t_back;

	int	 t_flag;						/* T_* flags. */
	char t_stat;						/* TS* thread status. */
	char t_lock;						/* Thread lock count. */

	short t_tid;						/* unique thread id */
	short t_ptid;						/* thread id of parent */

	/* Substructures: */
	struct pcred 	 	*t_cred;		/* Thread owner's identity. */
	struct filedesc 	*t_fd;			/* Ptr to open files structure. */
	struct pstats 	 	*t_stats;		/* Accounting/statistics (PROC ONLY). */
	struct sigacts 		*t_sig;			/* Signal actions, state (PROC ONLY). */

#define	t_ucred		t_cred->pc_ucred

	struct kthread    	*t_hash;        /* hashed based on t_tid & p_pid for kill+exit+... */
	struct kthread    	*t_tgrpnxt;	    /* Pointer to next thread in thread group. */
    struct kthread      *t_tptr;		/* pointer to process structure of parent */
    struct kthread 		*t_ostptr;	 	/* Pointer to older sibling processes. */

	struct tgrp 	    *t_tgrp;        /* Pointer to thread group. */
	struct sysentvec	*t_sysent;		/* System call dispatch information. */
	struct rtprio 	    t_rtprio;		/* Thread Realtime priority. */

	struct kthread 		*t_link;		/* linked list of running threads */
	struct user 		*t_addr;        /* address of u. area */
    struct user  		*t_daddr;		/* address of data area */
    struct user  		*t_saddr;		/* address of stack area */
	size_t	t_dsize;				    /* size of data area (clicks) */
	size_t	t_ssize;				    /* size of stack segment (clicks) */

	struct proc 		*p;				/* Pointer to Proc */
	struct uthread		*uth;			/* Pointer User Threads */
};

struct tgrp {
	struct	tgrp *tg_hforw;				/* Forward link in hash bucket. */
	struct	tgrp *tg_mem;				/* Pointer to tgrp members. */
	struct	session *tg_session;		/* Pointer to session. */
	tid_t	tg_id;						/* Tgrp id. */
	int		tg_jobc;					/* # threads qualifying tgrp for job control */
};

#define	t_session	t_tgrp->tg_session
#define	t_tgid		t_tgrp->tg_id

struct threadtable {

};

#define THREAD_RATIO 1  /* M:N Ratio. number of threads per process */

/* stat codes */
#define TSSLEEP	1
#define TSWAIT	2
#define TSRUN	3
#define TSIDL	4
#define TSREADY	5
#define TSSTART	6
#define TSSTOP	7

#define	TIDHSZ		16
#define TIDHASH(tid)  ((tid) & (TIDHSZ - 1))


extern struct kthread *tidhash[];		/* In param.c. */
extern struct tgrp *tgrphash[];			/* In param.c. */
extern int tidhashmask;					/* In param.c. */

/*
void thread_init(struct proc *p);
struct kthread * thread_create(struct proc *p, int qty);
thread_destroy(struct kthread *t);
thread_stop(struct kthread *t);
thread_start(struct kthread *t);
thread_wait();
*/
#endif /* SYS_KTHREADS_H_ */
