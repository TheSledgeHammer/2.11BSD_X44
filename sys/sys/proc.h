/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)proc.h	1.5 (2.11BSD) 1999/9/5
 *
 *	Parts of proc.h are borrowed from 4.4BSD-lite2 / FreeBSD 2.0 proc.h.
 */

#ifndef	_SYS_PROC_H_
#define	_SYS_PROC_H_

#include <machine/proc.h>		/* Machine-dependent proc substruct. */
#include <sys/rtprio.h>			/* For struct rtprio. */
#include <sys/select.h>			/* For struct selinfo. */
#include <sys/time.h>			/* For structs itimerval, timeval. */

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
struct	proc {
	struct	proc *p_nxt;				/* linked list of allocated proc slots */
	struct	proc **p_prev;				/* also zombies, and free proc's */
	struct	proc *p_pptr;				/* pointer to process structure of parent */

	short	p_flag;
	short	p_uid;						/* user id, used to direct tty signals */
	short	p_pid;						/* unique process id */
	short	p_ppid;						/* process id of parent */

	char	p_stat;
	char	p_dummy;					/* room for one more, here */

	/* Substructures: */
	struct	pcred 	 	*p_cred;		/* Process owner's identity. */
	struct	filedesc 	*p_fd;			/* Ptr to open files structure. */
	struct	pstats 	 	*p_stats;		/* Accounting/statistics (PROC ONLY). */
	struct	plimit 	 	*p_limit;		/* Process limits. */
	struct	vmspace  	*p_vmspace;		/* Address space. */
	struct 	sigacts 	*p_sig;			/* signals pending to this process */

#define	p_ucred		p_cred->pc_ucred
#define	p_rlimit	p_limit->pl_rlimit

#define	p_startzero	p_ysptr
	/* scheduling */
	struct	vnode 		*p_tracep;		/* Trace to vnode. */
	struct	vnode 		*p_textvp;		/* Vnode of executable. */

/* End area that is zeroed on creation. */
#define	p_endzero	p_startcopy

/* The following fields are all copied upon creation in fork. */
#define	p_startcopy	p_sigmask

	struct 	pgrp 	 	*p_pgrp;
	struct 	sysentvec 	*p_sysent; 		/* System call dispatch information. */

	struct	rtprio 		p_rtprio;		/* Realtime priority. */

	/* End area that is copied on creation. */
	struct	user 		*p_addr;		/* Kernel virtual addr of u-area (PROC ONLY). */

	/*
	 * Union to overwrite information no longer needed by ZOMBIED
	 * process with exit information for the parent process.  The
	 * two structures have been carefully set up to use the same
	 * amount of memory.  Must be very careful that any values in
	 * p_alive are not used for zombies (zombproc).
	 *
	 * Following union may be replaced..? or merge with new procs but defined below!
	 */
	union {
	    struct {
		char	P_pri;						/* priority, negative is high */
		char	P_cpu;						/* cpu usage for scheduling */
		char	P_time;						/* resident time for scheduling */
		char	P_nice;						/* nice for cpu usage */
		char	P_slptime;					/* secs sleeping */
		char	P_ptracesig;				/* used between parent & traced child */
		struct proc *P_hash;				/* hashed based on p_pid */
		long	P_sigmask;					/* current signal mask */
		long	P_sigignore;				/* signals being ignored */
		long	P_sigcatch;					/* signals being caught by user */
		struct	proc 		*P_link;		/* linked list of running processes */
		memaddr	P_addr;						/* address of u. area */
		memaddr	P_daddr;					/* address of data area */
		memaddr	P_saddr;					/* address of stack area */
		size_t	P_dsize;					/* size of data area (clicks) */
		size_t	P_ssize;					/* size of stack segment (clicks) */
		caddr_t	P_wchan;					/* event process is awaiting */
		caddr_t	P_wmesg;	 				/* Reason for sleep. */

		struct	k_itimerval P_realtimer;
	    } p_alive;

	    struct {
	    	short	P_xstat;				/* exit status for wait */
	    	struct k_rusage P_ru;			/* exit information */
	    } p_dead;

	} p_un;
};

struct	session {
	int		s_count;				/* Ref cnt; pgrps in session. */
	struct	proc *s_leader;			/* Session leader. */
	struct	vnode *s_ttyvp;			/* inode of controlling terminal. */
	struct	tty *s_ttyp;			/* Controlling terminal. */
	char	s_login[MAXLOGNAME];	/* Setlogin() name. */
};

struct	pgrp {
	struct	pgrp *pg_hforw;			/* Forward link in hash bucket. */
	struct	proc *pg_mem;			/* Pointer to pgrp members. */
	struct	session *pg_session;	/* Pointer to session. */
	pid_t	pg_id;					/* Pgrp id. */
	int		pg_jobc;				/* # procs qualifying pgrp for job control */
};

struct pcred {
	struct	ucred *pc_ucred;	/* Current credentials. */
	uid_t	p_ruid;				/* Real user id. */
	uid_t	p_svuid;			/* Saved effective user id. */
	gid_t	p_rgid;				/* Real group id. */
	gid_t	p_svgid;			/* Saved effective group id. */
	int		p_refcnt;			/* Number of references. */
};

#define	p_session	p_pgrp->pg_session
#define	p_pgid		p_pgrp->pg_id

#define	p_pri		p_un.p_alive.P_pri
#define	p_cpu		p_un.p_alive.P_cpu
#define	p_time		p_un.p_alive.P_time
#define	p_nice		p_un.p_alive.P_nice
#define	p_slptime	p_un.p_alive.P_slptime
#define	p_hash		p_un.p_alive.P_hash
#define	p_ptracesig	p_un.p_alive.P_ptracesig
#define	p_sigmask	p_un.p_alive.P_sigmask
#define	p_sigignore	p_un.p_alive.P_sigignore
#define	p_sigcatch	p_un.p_alive.P_sigcatch
//#define	p_pgrp		p_un.p_alive.P_pgrp
//#define p_sysent	p_un.p_alive.P_sysent;
//#define p_rtprio	p_un.p_alive.P_rtprio;
#define	p_link		p_un.p_alive.P_link
//#define	p_addr		p_un.p_alive.P_addr
#define	p_daddr		p_un.p_alive.P_daddr
#define	p_saddr		p_un.p_alive.P_saddr
#define	p_dsize		p_un.p_alive.P_dsize
#define	p_ssize		p_un.p_alive.P_ssize
#define	p_wchan		p_un.p_alive.P_wchan
#define	p_wmesg		p_un.p_alive.P_wmesg
#define	p_realtimer	p_un.p_alive.P_realtimer
#define	p_clktim	p_realtimer.it_value
#define	p_xstat		p_un.p_dead.P_xstat
#define	p_ru		p_un.p_dead.P_ru

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* running */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */

/* flag codes */
#define	P_CONTROLT	0x00002	/* Has a controlling terminal. */

#define	SLOAD		0x0001	/* in core */
#define	SSYS		0x0002	/* swapper or pager process */
#define	SLOCK		0x0004	/* process being swapped out */
#define	SSWAP		0x0008	/* save area flag */
#define	P_TRACED	0x0010	/* process is being traced */
#define	P_WAITED	0x0020	/* another tracing flag */
#define	SULOCK		0x0040	/* user settable lock in core */
#define	P_SINTR		0x0080	/* sleeping interruptibly */
#define	SVFORK		0x0100	/* process resulted from vfork() */
#define	SVFPRNT		0x0200	/* parent in vfork, waiting for child */
#define	SVFDONE		0x0400	/* parent has released child in vfork */
			/*		0x0800	/* unused */
#define	P_TIMEOUT	0x1000	/* tsleep timeout expired */
#define	P_NOCLDSTOP	0x2000	/* no SIGCHLD signal to parent */
#define	P_SELECT	0x4000	/* selecting; wakeup/waiting danger */
			/*		0x8000	/* unused */

#define	S_DATA	0			/* specified segment */
#define	S_STACK	1

//#ifdef KERNEL
#define	PID_MAX		30000
#define	NO_PID		30001

#define	PIDHSZ		16
#define	PIDHASH(pid)	((pid) & (PIDHSZ - 1))

extern struct proc *pidhash[];			/* In param.c. */
extern struct pgrp *pgrphash[];			/* In param.c. */
extern int pidhashmask;					/* In param.c. */

extern struct proc *pfind();
extern struct proc proc[], *procNPROC;	/* the proc table itself */
extern struct proc *freeproc;
extern struct proc *zombproc;			/* List of zombie procs. */
extern volatile struct proc *allproc;	/* List of active procs. */
extern struct proc proc0;				/* Process slot for swapper. */
int	nproc;

#define	NQS	32							/* 32 run queues. */
extern struct prochd qs[];				/* queue schedule */
extern struct prochd rtqs[];
extern struct prochd idqs[];

struct	prochd {
	struct	proc *ph_link;				/* Linked list of running processes. */
	struct	proc *ph_rlink;
};


int		chgproccnt __P((uid_t, int));
int		setpri __P((struct proc *));
void	setrun __P((struct proc *));
void	setrq __P((struct proc *));
void	remrq __P((struct proc *));
void	swtch __P();
void	sleep __P((void *chan, int pri));
int		tsleep __P((void *chan, int pri, char *wmesg, int timo));
void	unsleep __P((struct proc *));
void	wakeup __P((void *chan));

int	inferior __P((struct proc *));
#endif

#endif	/* !_SYS_PROC_H_ */
