/*-
 * Copyright (c) 1986, 1989, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)proc.h	8.15 (Berkeley) 5/19/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)proc.h	1.5 (2.11BSD) 5/9/1999
 */

#ifndef	_SYS_PROC_H_
#define	_SYS_PROC_H_
#ifdef _KERNEL
#include <machine/cpuinfo.h>
#endif
#include <machine/proc.h>		/* Machine-dependent proc substruct. */
#include <machine/frame.h>

#include <sys/select.h>			/* For struct selinfo. */
#include <sys/queue.h>
#include <sys/callout.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/lock.h>
#include <sys/thread.h>

/*
 * One structure allocated per active
 * process. It contains all data needed
 * about the process while the
 * process may be swapped out.
 * Other per process data (user.h)
 * is swapped with the process.
 */
struct	proc {
    TAILQ_ENTRY(proc)	p_link;			/* Doubly-linked run/sleep queue. */
	LIST_ENTRY(proc) 	p_list;			/* List of all processes */
	 	 	 	 	 	 	 	 	 	/* linked list of allocated proc slots */
										/* also zombies, and free proc's */
	struct mtx			*p_mtx;			/* proc structure mutex */
    int	    			p_flag;			/* P_* flags. */
	char				p_stat;			/* S* process status. */
	char				p_lock;			/* Process lock count. */
	char				p_pad1[2];
	char				p_dummy;		/* room for one more, here */
	void				*p_ctxlink;		/* p: uc_link {get,set}context */

    short				p_uid;			/* user id, used to direct tty signals */
    pid_t				p_pid;			/* unique process id */
    pid_t				p_ppid;			/* process id of parent */

    /* Thread Structure: */
    union {
    	struct threadlist 	*allthread;	/* list of all threads */
    	struct threadhd	 	*threadrq; 	/* Linked list of running threads. */
    	struct threadhd		*threadsq;	/* Linked list of sleeping threads. */
    } p_threads;
#define p_allthread		p_threads.allthread
#define p_threadrq		p_threads.threadrq
#define p_threadsq		p_threads.threadsq
    struct thread		*p_curthread;	/* current running thread */
    struct thread		*p_threado;		/* thread overseer (original thread of process)  */
    int 				p_nthreads;		/* number of attached threads to this proc */
    int					p_tqsready;		/* number of threads ready */
    int					p_tqssleep;		/* number of threads sleeping */

    /* Substructures: */
	struct	pcred 	 	*p_cred;		/* Process owner's identity. */
	struct	filedesc 	*p_fd;			/* pointer to open files structure. */
	struct	pstats 	 	*p_stats;		/* Accounting/statistics (PROC ONLY). */
	struct	plimit 	 	*p_limit;		/* Process limits. */
	struct	vmspace  	*p_vmspace;		/* VM Address space. */
	struct 	sigacts 	*p_sigacts;		/* Signal actions, state (PROC ONLY). */
#ifdef OVERLAY
	struct 	ovlspace	*p_ovlspace;	/* OVL Address space. */
#endif
#define	p_ucred			p_cred->pc_ucred
#define	p_rlimit		p_limit->pl_rlimit

	LIST_ENTRY(proc) 	p_pglist;		/* List of processes in pgrp. */
    struct	proc        *p_pptr;		/* pointer to process structure of parent */
	LIST_ENTRY(proc) 	p_sibling;		/* List of sibling processes. */
	LIST_HEAD(, proc) 	p_children;		/* Pointer to list of children. */
	LIST_ENTRY(proc)    p_hash;	        /* Hash chain. */

/* The following fields are all zeroed upon creation in fork. */
#define	p_startzero		p_oppid

    pid_t				p_oppid;	    /* Save parent pid during ptrace. XXX */

    /* scheduling */
    u_int				p_estcpu;	 	/* Time averaged value of p_cpticks. */
    int					p_cpticks;	 	/* Ticks of cpu time. */
    fixpt_t				p_pctcpu;	 	/* %cpu for this process during p_swtime */

    caddr_t	            p_wchan;		/* event process is awaiting */
	caddr_t       		p_wmesg;	 	/* Reason for sleep. */
	u_int				p_swtime;	 	/* Time swapped in or out. */
	struct callout 		p_tsleep_ch;	/* callout for tsleep */

    struct itimerval 	p_krealtimer;	/* Alarm timer. 4.4BSD Compat */
	struct k_itimerval 	p_realtimer;   	/* Alarm Timer. 2.11BSD */
    struct timeval     	p_rtime;	    /* Real time. */
    u_quad_t 			p_uticks;		/* Statclock hits in user mode. */
    u_quad_t 			p_sticks;		/* Statclock hits in system mode. */
    u_quad_t 			p_iticks;		/* Statclock hits processing intr. */

    char				p_ptracesig;	/* used between parent & traced child */
    int	    			p_traceflag;	/* Kernel trace points. */
    struct file 	    *p_tracep;		/* Trace to file. */
    
    struct vnode 	    *p_textvp;		/* Vnode of executable. */
    struct vm_text		*p_textp;		/* vm text */
    
    long				p_sig;		    /* Signals pending to this process */
    
    struct emul			*p_emul;		/* Emulation information */
    const struct execsw *p_execsw;		/* Exec package information */
    struct klist		p_klist;		/* Knotes attached to this process */

/* End area that is zeroed on creation. */
#define	p_endzero	    p_startcopy

/* The following fields are all copied upon creation in fork. */
#define	p_startcopy	    p_sigmask

    sigset_t 			p_sigmask;		/* Current signal mask. */
    sigset_t 			p_sigignore;	/* Signals being ignored */
    sigset_t 			p_sigcatch;		/* Signals being caught by user */

    u_char				p_pri;			/* Process  priority, negative is high */
    u_char				p_cpu;			/* cpu usage for scheduling */
    u_char				p_time;			/* resident time for scheduling */
    u_char				p_usrpri;		/* User-priority based on p_cpu and p_nice. */
    char				p_nice;			/* nice for cpu usage */
    char				p_slptime;		/* Time since last blocked. secs sleeping */
    char				p_comm[MAXCOMLEN+1];/* p: basename of last exec file */

    struct  pgrp 	    *p_pgrp;        /* Pointer to process group. */
    struct  sysentvec   *p_sysent; 	    /* System call dispatch information. */

/* End area that is copied on creation. */
#define	p_endcopy		p_thread

    void				*p_thread;		/* Id for this "thread"; Mach glue. XXX */

    struct user			*p_addr;		/* virtual address of u. area */

    caddr_t				p_daddr;		/* virtual address of data area */
    caddr_t				p_saddr;		/* virtual address of stack area */
    caddr_t				p_taddr;		/* virtual address of text area */
	size_t				p_dsize;		/* size of data segment (clicks) */
	size_t				p_ssize;		/* size of stack segment (clicks) */
	size_t				p_tsize;		/* size of text segment (clicks) */

    u_short 			p_acflag;	    /* Accounting flags. */

	long				p_spare[2];		/* pad to 256, avoid shifting eproc. */

	caddr_t  			p_psstrp;		/* :: address of process's ps_strings */
	struct	mdproc 		p_md;			/* Any machine-dependent fields. */
	int					p_pflag;		/* private flags (machine-dependent) */

    short				p_xstat;		/* exit status for wait */
	struct  rusage    	p_ru;			/* exit information */
	struct  k_rusage    p_kru;			/* exit information kernel */

	int					p_tcgen;		/* for timecounter removal */

	struct sched		*p_sched;		/* global scheduler */
};
#define	p_session		p_pgrp->pg_session
#define	p_pgid			p_pgrp->pg_id

struct session {
	int					s_count;		/* Ref cnt; pgrps in session. */
	struct	proc 		*s_leader;		/* Session leader. */
	struct	vnode 		*s_ttyvp;		/* inode of controlling terminal. */
	struct	tty 		*s_ttyp;		/* Controlling terminal. */
	char				s_login[MAXLOGNAME];/* Setlogin() name. */
};

struct pgrp {
	LIST_ENTRY(pgrp)    pg_hash;	    /* Hash chain. */
	LIST_HEAD(, proc) 	pg_mem;			/* Pointer to pgrp members. */
	struct	session 	*pg_session;	/* Pointer to session. */
	pid_t				pg_id;			/* Pgrp id. */
	int					pg_jobc;		/* # procs qualifying pgrp for job control */
};

struct pcred {
	struct ucred 		*pc_ucred;		/* Current credentials. */
	uid_t				p_ruid;			/* Real user id. */
	uid_t				p_svuid;		/* Saved effective user id. */
	gid_t				p_rgid;			/* Real group id. */
	gid_t				p_svgid;		/* Saved effective group id. */
	int					p_refcnt;		/* Number of references. */
};

struct exec_linker;
struct proc;
struct ps_strings;

struct emul {
	char				e_name[8];			/* Symbolic name */
	const char			*e_path;			/* Extra emulation path (NULL if none)*/
	int					e_nosys;			/* Offset of the nosys() syscall */
	int					e_nsysent;			/* Number of system call entries */

	const struct sysent *e_sysent;			/* System call array */
	const char * const 	*e_syscallnames; 	/* System call name array */
	int					e_arglen;			/* Extra argument size in words */
	
	void				(*e_sendsig)(sig_t, int, int, u_long); /* Signal sending function */
	void				(*e_setregs)(struct proc *, struct exec_linker *, u_long);
	char				*e_sigcode;			/* Start of sigcode */
	char				*e_esigcode;		/* End of sigcode */
	void				(*e_syscall)(void *);
};

/* stat codes */
#define	SSLEEP		1			/* awaiting an event */
#define	SWAIT		2			/* (abandoned state) */
#define	SRUN		3			/* running */
#define	SIDL		4			/* intermediate state in process creation */
#define	SZOMB		5			/* intermediate state in process termination */
#define	SSTOP		6			/* process being traced */

/* flag codes */
#define	P_SLOAD		0x0000001	/* in core */
#define	P_SSYS		0x0000002	/* swapper or pager process */
#define	P_SLOCK		0x0000004	/* process being swapped out */
#define	P_SSWAP		0x0000008	/* save area flag */
#define	P_TRACED	0x0000010	/* process is being traced */
#define	P_WAITED	0x0000020	/* another tracing flag */
#define	P_SULOCK	0x0000040	/* user settable lock in core */
#define	P_SINTR		0x0000080	/* sleeping interruptibly */
#define	P_SVFORK	0x0000100	/* process resulted from vfork() */
#define	P_SVFPRNT	0x0000200	/* parent in vfork, waiting for child */
#define	P_SVFDONE	0x0000400	/* parent has released child in vfork */
#define	P_CONTROLT	0x0000800	/* Has a controlling terminal. */
#define	P_TIMEOUT	0x0001000	/* tsleep timeout expired */
#define	P_NOCLDWAIT	0x0002000	/* No zombies if child dies */
#define	P_SELECT	0x0004000	/* selecting; wakeup/waiting danger */
#define	P_PPWAIT	0x0008000	/* Parent is waiting for child to exec/exit. */
#define	P_PROFIL	0x0010000	/* Has started profiling. */
#define	P_SUGID		0x0020000	/* Had set id privileges since last exec. */
#define	P_NOCLDSTOP	0x0040000	/* no SIGCHLD signal to parent */
#define P_EXEC		0x0080000	/* Process called exec. */
#define	P_SYSTEM	0x0100000	/* System proc: no sigs, stats or swapping. */
#define	P_INMEM		0x0200000	/* Loaded into memory. */
#define P_INEXEC	0x0400000	/* Process is exec'ing and cannot be traced */

/* Should probably be changed into a hold count (They have. -DG). */
#define	P_NOSWAP	0x0800000	/* Another flag to prevent swap out. */
#define	P_PHYSIO	0x1000000	/* Doing physical I/O. */
#define	P_WEXIT		0x2000000
#define P_SWAPPING	0x4000000
#define	P_ADVLOCK	0x8000000	/* Process may hold a POSIX advisory lock. */

#define	S_DATA		0			/* specified segment */
#define	S_STACK		1

#ifdef _KERNEL
extern struct lock_holder 	proc_loholder;
#define PROC_LOCK(p)		(mtx_lock(&(p)->p_mtx, &proc_loholder))
#define PROC_UNLOCK(p)		(mtx_unlock(&(p)->p_mtx, &proc_loholder))

#define	PID_MAX			30000
#define	NO_PID			100000

#define SESS_LEADER(p)	((p)->p_session->s_leader == (p))
#define	SESSHOLD(s)		((s)->s_count++)
#define	SESSRELE(s) {							\
	if (--(s)->s_count == 0)					\
		FREE(s, M_SESSION);						\
}

#define	PIDHSZ						16
#define	PIDHASH(pid)				(&pidhashtbl[(pid) & pidhash & (PIDHSZ * ((pid) + pidhash) - 1)])
extern LIST_HEAD(pidhashhead, proc) *pidhashtbl;
extern u_long pidhash;

#define	PGRPHASH(pgid)				(&pgrphashtbl[(pgid) & pgrphash])
extern LIST_HEAD(pgrphashhead, pgrp) *pgrphashtbl;
extern u_long pgrphash;

#if !defined(curproc)
#if defined(SMP)
#define curproc	curcpu()->cpu_curproc	/* current running proc */
#else
extern struct proc *curproc;			/* current running proc */
#endif	 /* SMP */
#endif  /* !curproc */

extern int pidhashmask;					/* In param.c. */
extern struct proc proc0;				/* Process slot for swapper. */
extern int	nproc, maxproc;				/* Current and max number of procs. */

LIST_HEAD(proclist, proc);
extern struct proclist allproc;			/* List of active procs. */
extern struct proclist zombproc;		/* List of zombie procs. */
extern struct proclist freeproc;		/* List of free procs. */
extern struct proc *initproc, *pageproc;/* Process slots for init, pager. */

#define	NQS	32							/* 32 run queues. */
extern int	whichqs;					/* Bit mask summary of non-empty Q's. */
TAILQ_HEAD(prochd, proc);				/* Linked list of running processes. */
TAILQ_HEAD(sleepque, proc);				/* Linked list of sleeping processes. */
extern struct prochd qs[];
extern struct sleepque slpque[];
extern struct emul emul_211bsd;			/* emulation struct */

struct proc 	*pfind(pid_t);			    /* Find process by id. */
struct pgrp 	*pgfind(pid_t);				/* Find process group by id. */
int		setpri(struct proc *);
void		setrun(struct proc *);
void		setrq(struct proc *);
void		remrq(struct proc *);
struct proc 	*getrq(struct proc *);
void		swtch();
void		sleep(void *, int);
int		tsleep(void *, int, char *, u_short);
int 		ltsleep(void *, int, char *, u_short, __volatile struct lock_object *);
void        	endtsleep(struct proc *);
void		unsleep(struct proc *);
void		wakeup(const void *);
void		wakeup_one(const void *);
void		reschedule(struct proc *);
void		procinit(struct proc *);
void		rqinit(void);
void		sqinit(void);
void		idle_check(void);
int 		chgproccnt(uid_t, int);
void		pgdelete(struct pgrp *);
int		leavepgrp(struct proc *);
int		enterpgrp(struct proc *, pid_t, int);
void		fixjobc(struct proc *, struct pgrp *, int);
int		inferior(struct proc *);
int		newproc(int);
int 		proc_create(struct proc **);
/* kern_exit.c */
void    	exit(int);
void		endvfork();
#endif 	/* KERNEL */
#endif	/* !_SYS_PROC_H_ */
