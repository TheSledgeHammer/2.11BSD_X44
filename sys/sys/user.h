/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)user.h	1.6 (2.11BSD) 1999/9/13
 */

#ifndef _SYS_USER_H_
#define _SYS_USER_H_
#include <sys/cdefs.h>

#include <machine/pcb.h>
#include <machine/param.h>

#ifndef KERNEL
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/fperr.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/sysctl.h>
#include <sys/syslimits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ucred.h>
#include <vm/include/vm.h>			/* XXX */
#endif
/*
 * data that doesn't need to be referenced while the process is swapped.
 * The user block is USIZE*64 bytes long; resides at virtual kernel loc
 * 0140000; contains the system stack (and possibly network stack) per
 * user; is cross referenced with the proc structure for the same process.
 */
#define	MAXCOMLEN		MAXNAMLEN				/* <= MAXNAMLEN, >= sizeof(ac_comm) */
#define USIZE 			UPAGES					/* pdp11 equivalent of UPAGES */

struct	pcb {									/* fake pcb structure */
	int					(*pcb_sigc)();			/* pointer to trampoline code in user space */
};

struct	fps {
	short				u_fpsr;					/* FP status register */
	double				u_fpregs[6];			/* FP registers */
};

struct user {
	struct trapframe	*u_frame;
	struct pcb 			u_pcb;
	struct fps 			u_fps;
	short				u_fpsaved;				/* FP regs saved for this proc */
	struct fperr 		u_fperr;				/* floating point error save */
	struct proc 		*u_procp;				/* pointer to proc structure */
	int					*u_ar0;					/* address of users saved R0 */
	char				u_comm[MAXCOMLEN + 1];

/* syscall parameters, results and catches */
	int					u_arg[6];				/* arguments to current system call */
	int					*u_ap;					/* pointer to arglist */
	label_t				u_qsave;				/* for non-local gotos on interrupts */
	union {										/* syscall return values */
		struct	{
			int			R_val1;
			int			R_val2;
		} u_rv;
#define	r_val1			u_rv.R_val1
#define	r_val2			u_rv.R_val2
		long			r_long;
		off_t			r_off;
		time_t			r_time;
	} u_r;
	char				u_error;				/* return error code */
	char				u_dummy0;

/* 1.1 - processes and protection */
	struct pcred		*u_pcred;				/* Process owner's identity. */
	struct ucred		*u_ucred;				/* Credentials */

#define u_uid			u_ucred->cr_uid			/* effective user id */

/* 1.2 - memory management */
	size_t				u_tsize;				/* text size (clicks) */
	size_t				u_dsize;				/* data size (clicks) */
	size_t				u_ssize;				/* stack size (clicks) */

	label_t				u_ssave;				/* label variable for swapping */
	label_t				u_rsave;				/* save info when exchanging stacks */
	short				u_uisa[16];				/* segmentation address prototypes */
	short				u_uisd[16];				/* segmentation descriptor prototypes */
	char				u_sep;					/* flag for I and D separation */

/* 1.2.5 - overlay information */
	struct u_ovd { 								/* automatic overlay data */
		short 			uo_curov; 				/* current overlay */
		short 			uo_ovbase; 				/* base of overlay area, seg. */
		u_short 		uo_dbase; 				/* start of data, clicks */
		u_short 		uo_ov_offst[NOVL + 1]; 	/* overlay offsets in text */
		short 			uo_nseg; 				/* number of overlay seg. regs. */
	} u_ovdata;

/* 1.3 - signal management */
	int					u_signal[NSIG];			/* disposition of signals */
	long				u_sigmask[NSIG];		/* signals to be blocked */
	long				u_sigonstack;			/* signals to take on sigstack */
	long				u_sigintr;				/* signals that interrupt syscalls */
	long				u_oldmask;				/* saved mask from before sigpause */
	int					u_code;					/* ``code'' to trap */
	char				dummy2;					/* Room for another flags byte */
	char				u_psflags;				/* Process Signal flags */
	struct sigaltstack 	u_sigstk;				/* signal stack info */
	struct sigstack 	u_sigstk2;				/* signal stack info */
	struct sigacts 		u_sigacts;				/* p_sigacts points here (use it!) */
	struct pstats 		u_stats;				/* p_stats points here (use it!) */

/* 1.4 - descriptor management */
	struct file 		*u_ofile[NOFILE];		/* file structures for open files */
	char				u_pofile[NOFILE];		/* per-process flags of open files */
	int					u_lastfile;				/* high-water mark of u_ofile */
	struct filedesc		*u_fd;					/* Ptr to open files structure. */
#define	UF_EXCLOSE 		0x1						/* auto-close on exec */
#define	UF_MAPPED 		0x2						/* mapped from device */
#define u_cdir 			u_nd->ni_cdir			/* current directory */
#define u_rdir 			u_nd->ni_rdir			/* root directory of current process */
	struct tty 			*u_ttyp;				/* controlling tty pointer */
	dev_t				u_ttyd;					/* controlling tty dev */
	short				u_cmask;				/* mask for file creation */

/* 1.5 - timing and statistics */
	struct k_rusage 	u_ru;					/* stats for this proc */
	struct k_rusage 	u_cru;					/* sum of stats for reaped children */
	struct k_itimerval 	u_timer[2];				/* profile/virtual timers */
	struct timeval  	u_start;
	char				u_acflag;
	char				u_dupfd;				/* XXX - see kern_descrip.c/fdopen */

	struct uprof {								/* profile arguments */
		caddr_t			pr_base;				/* buffer base */
		u_long			pr_size;				/* buffer size */
		u_long			pr_off;					/* pc offset */
		u_long			pr_scale;				/* pc scaling */
		u_long			pr_addr;				/* temp storage for addr until AST */
		u_long			pr_ticks;				/* temp storage for ticks until AST */
	} u_prof;

/* 1.6 - resource controls */
	struct	rlimit 		u_rlimit[RLIM_NLIMITS];
	struct	quota 		*u_quota;				/* user's quota structure */

#define u_cmpn 			u_nd->ni_cnd			/* namei component name */
#define u_cred 			u_cmpn->cn_cred			/* namei component name credentials */

	/* namei & co. */
	struct 	nameidata 	*u_nd;

	short				u_xxxx[2];				/* spare */
	char				u_login[MAXLOGNAME];	/* setlogin/getlogin */
	short				u_stack[1];				/* kernel stack per user
					 	 	 	 	 	 	 	 * extends from u + USIZE*64
					 	 	 	 	 	 	 	 * backward not to reach here
					 	 	 	 	 	 	 	 */
	vm_offset_t			u_kstack;				/* (a) Kernel VA of kstack. */
	int					u_kstack_pages;			/* (a) Size of the kstack. */
	int					u_uspace;				/* (a) Size of the u-area (UPAGES * PGSIZE) */

/* 1.7 Remaining fields only for core dump and/or ptrace-- not valid at other times! */
	struct kinfo_proc 	u_kproc;				/* proc + eproc */
	struct md_coredump 	u_md;					/* machine dependent glop */

/* 1.8 User Threads */
	//struct uthread		*u_uthread;			/* ptr to uthread */
};

#ifdef KERNEL
extern struct user 		u;
//#else
//extern struct user 		u;
#endif
