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
	struct user 		*kt_addr;       /* address of u. area */
    struct user  		*kt_daddr;		/* address of data area */
    struct user  		*kt_saddr;		/* address of stack area */
	size_t				kt_dsize;		/* size of data area (clicks) */
	size_t				kt_ssize;		/* size of stack segment (clicks) */
};

struct tgrp {
	struct	tgrp 		*tg_hforw;		/* Forward link in hash bucket. */
	struct	tgrp 		*tg_mem;		/* Pointer to tgrp members. */
	struct	session 	*tg_session;	/* Pointer to session. */
	tid_t				tg_id;			/* Tgrp id. */
	int					tg_jobc;		/* # threads qualifying tgrp for job control */
};

#define	kt_session		kt_tgrp->tg_session
#define	kt_tgid			kt_tgrp->tg_id

#define KTHREAD_RATIO 1  /* M:N Ratio. number of kernel threads per process */

/* stat codes */
#define TSSLEEP	1
#define TSWAIT	2
#define TSRUN	3
#define TSIDL	4
#define TSREADY	5
#define TSSTART	6
#define TSSTOP	7

#define	TIDHSZ			16
#define	TIDHASH(tid)	(&tidhashtbl[(tid) & tid_hash & (TIDHSZ * ((tid) + tid_hash) - 1)])
extern LIST_HEAD(tidhashhead, kthread) *tidhashtbl;
u_long tid_hash;

#define	TGRPHASH(tgid)	(&tgrphashtbl[(tgid) & tgrphash])
extern LIST_HEAD(tgrphashhead, tgrp) *tgrphashtbl;
extern u_long tgrphash;

extern struct kthread *ktidhash[];		/* In param.c. */
extern int tidhashmask;					/* In param.c. */

struct 	kthread *ktfind (tid_t);		/* Find kernel thread by id. */
struct 	tgrp 	*tgfind(tid_t);			/* Find Thread group by id. */

void			threadinit(void);
int				leavetgrp(struct kthread *);
int				entertgrp(struct kthread *, tid_t, int);
void			fixjobc(struct kthread *, struct tgrp *, int);
int				inferior(struct kthread *);

#endif /* SYS_KTHREADS_H_ */
