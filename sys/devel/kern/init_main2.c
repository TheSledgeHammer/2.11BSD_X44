/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)init_main.c	2.5 (2.11BSD GTE) 1997/9/26
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1992, 1993
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
 *	@(#)init_main.c	8.16 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/protosw.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/ksyms.h>
#include <vm/include/vm.h>

#include <machine/cpu.h>

char 	copyright[] = "Copyright (c) 1982, 1986, 1989, 1991, 1993\n\tThe Regents of the University of California.  All rights reserved.\n\n";

struct 	session session0;
struct 	pgrp pgrp0;
struct 	proc proc0;
struct 	pcred cred0;
struct 	filedesc0 filedesc0;
struct 	plimit limit0;
struct 	vmspace vmspace0;
struct 	proc *curproc = &proc0;
struct	proc *initproc, *pageproc;

int		securelevel;
int 	cmask = CMASK;
extern	struct user *proc0paddr;

struct 	vnode *rootvp, *swapdev_vp;
int		boothowto;
struct	timeval boottime;
struct	timeval runtime;

#ifndef BOOTVERBOSE
#define	BOOTVERBOSE	0
#endif
int	bootverbose = 	BOOTVERBOSE;

extern const struct emul emul_211bsd; 					/* defined in kern_exec.c */

static void start_init (struct proc *p, void *framep);

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 */
void
main(framep)
	void *framep;
{
	register struct proc *p;
	register struct kthread *kt;
	register struct uthread *ut;
	register struct filedesc0 *fdp;
	register struct pdevinit *pdev;
	register int i;
	int s;
	register_t rval[2];
	extern struct pdevinit pdevinit[];
	extern void roundrobin (void *);
	extern void schedcpu (void *);

	/*
	 * Initialize the current process pointer (curproc) before
	 * any possible traps/probes to simplify trap processing.
	 */
	p = &proc0;
	curproc = p;

	/*
	 * Attempt to find console and initialize
	 * in case of early panic or other messages.
	 */
	consinit();
	printf(copyright);

	vm_mem_init();
	kmeminit();

	startup();

	kmem_rmapinit();

	ksyms_init();

	/* Initialize callouts, part 1. */
	callout_startup();

	/*
	 * Initialize process and pgrp structures.
	 */
	procinit();

	/*
	 * Initialize process lockholder structure
	 */
	lockholder_init(p);

	/*
	 * Initialize device switch tables
	 */
	devswtable_init();

	/*
	 * set up system process 0 (swapper)
	 */
	allproc = (struct proc*) p;
	p->p_prev = (struct proc**) &allproc;
	p->p_pgrp = &pgrp0;
	pgrphash[0] = &pgrp0;

	p->p_stat = SRUN;
	p->p_flag |= P_SLOAD|P_SSYS;
	p->p_nice = NZERO;
	p->p_emul = &emul_211bsd;

	u->u_procp = p;
	bcopy("swapper", p->p_comm, sizeof ("swapper"));

	/* Create credentials. */
	cred0.p_refcnt = 1;
	p->p_cred = &cred0;
	u->u_cred = crget();
	u->u_cred->cr_ngroups = 1; /* group 0 */
	p->p_ucred = u->u_cred;

	callout_init(&p->p_tsleep_ch);

	/* Create the file descriptor table. */
	fdp = &filedesc0;
	p->p_fd = &fdp->fd_fd;
	fdp->fd_fd.fd_refcnt = 1;
	fdp->fd_fd.fd_cmask = cmask;
	fdp->fd_fd.fd_ofiles = fdp->fd_dfiles;
	fdp->fd_fd.fd_ofileflags = fdp->fd_dfileflags;
	fdp->fd_fd.fd_nfiles = NDFILE;

	/* Create the limits structures. */
	p->p_limit = &limit0;
	for (i = 0; i < sizeof(u->u_rlimit)/sizeof(u->u_rlimit[0]); i++) {
		u->u_rlimit[i]->rlim_cur = u->u_rlimit[i].rlim_max = RLIM_INFINITY;
	}
	u->u_rlimit[RLIMIT_NOFILE].rlim_cur = NOFILE;
	u->u_rlimit[RLIMIT_NPROC].rlim_cur = MAXUPRC;
	i = ptoa(cnt.v_free_count);
	u->u_rlimit[RLIMIT_RSS].rlim_max = i;
	u->u_rlimit[RLIMIT_MEMLOCK].rlim_max = i;
	u->u_rlimit[RLIMIT_MEMLOCK].rlim_cur = i / 3;

	limit0->pl_rlimit = u->u_rlimit;
	limit0.p_refcnt = 1;

	bcopy("root", u->u_login, sizeof ("root"));

	/* initialize kthread0 & kthreadpool system */
	kthread_init(p, kt);

	/* Allocate a prototype map so we have something to fork. */
	p->p_vmspace = kt->kt_vmspace = &vmspace0;
	vmspace0.vm_refcnt = 1;
	pmap_pinit(&vmspace0.vm_pmap);
	vm_map_init(&vmspace0.vm_map, round_page(VM_MIN_ADDRESS), trunc_page(VM_MAX_ADDRESS), TRUE);
	vmspace0.vm_map.pmap = &vmspace0.vm_pmap;
	p->p_addr = proc0paddr;				/* XXX */

	/*
	 * We continue to place resource usage info and signal
	 * actions in the user struct so they're pageable.
	 */
	p->p_stats = &u->u_stats;
	p->p_sigacts = &u->u_sigacts;

	/*
	 * Initialize per uid information structure and charge
	 * root for one process.
	 */
	(void) chgproccnt(0, 1);

	rqinit();

	/* Configure virtual memory system, set vm rlimits. */
	vm_init_limits(p);

	/* Initialize the file systems. */
	vfsinit();

	/* Initialize kqueue. */
	kqueue_init();

	/* Start real time and statistics clocks. */
	initclocks();

	/* Initialize mbuf's. */
	mbinit();

	/* Initialize clists, tables, protocols. */
	cinit();
	//bhinit();
	//binit();

	/* Initialize the log device. */
	loginit();

	/* Attach pseudo-devices. */
	for (pdev = pdevinit; pdev->pdev_attach != NULL; pdev++) {
		(*pdev->pdev_attach)(pdev->pdev_count);
	}

	/*
	 * Initialize protocols.  Block reception of incoming packets
	 * until everything is ready.
	 */
	s = splimp();
	ifinit();
	domaininit();
	splx(s);

#ifdef GPROF
	/* Initialize kernel profiling. */
	kmstartup();
#endif

	/* Initialize Global Scheduler */
//	gsched_init(p);

	/* Kick off timeout driven events by calling first time. */
	roundrobin(NULL);
	schedcpu(NULL);

	/* Mount the root file system. */
	if (vfs_mountroot())
		panic("cannot mount root");
	CIRCLEQ_FIRST(&mountlist)->mnt_flag |= MNT_ROOTFS;

	/* Get the vnode for '/'.  Set fdp->fd_fd.fd_cdir to reference it. */
	if (VFS_ROOT(CIRCLEQ_FIRST(&mountlist), &rootvnode))
		panic("cannot find root vnode");
	fdp->fd_fd.fd_cdir = rootvnode;
	VREF(fdp->fd_fd.fd_cdir);
	VOP_UNLOCK(rootvnode, 0, p);
	fdp->fd_fd.fd_rdir = NULL;
	swapinit();

	p->p_stats->p_start = runtime = mono_time = boottime = &time;
	p->p_rtime.tv_sec = p->p_rtime.tv_usec = 0;

	/* Initialize signal state for process 0 */
	siginit(p);

	/* Create process 1 (init(8)). */
	if (newproc(0))
		panic("fork init");
	if (rval[1]) {
		start_init(curproc, framep);
		return;
	}

	/*
	 * Create any kernel threads whose creation was deferred because
	 * initprocess had not yet been created.
	 */
	kthread_run_deferred_queue();

	/* Create kthread 1 (the pageout daemon). */
	if (kthread_create(vm_pageout, NULL, NULL, "pagedaemon")) {
		panic("fork pager");
	}

	/* initialize uthread0 & uthreadpool system */
	uthread_init(kt, ut);
	uthread_run_deferred_queue();

	/* Initialize exec structures */
	exec_init();

	scheduler();
	/* NOTREACHED */
}

/*
 * List of paths to try when searching for "init".
 */
static const  char * const initpaths[] = {
	"/sbin/init",
	"/sbin/oinit",
	"/sbin/init.bak",
	NULL,
};

/*
 * Start the initial user process; try exec'ing each pathname in "initpaths".
 * The program is invoked with one argument containing the boot flags.
 */
static void
start_init(p, framep)
	struct proc *p;
	void *framep;
{
	vm_offset_t addr;
	struct execa args;
	int options, i, error;
	register_t retval[2];
	char flags[4] = "-", *flagsp;
	char **pathp, *path, *ucp, **uap, *arg0, *arg1, *var;
	char *free_initpaths, *tmp_initpaths;

	initproc = p;

	/*
	 * We need to set the system call frame as if we were entered through
	 * a syscall() so that when we call execve() below, it will be able
	 * to set the entry point (see setregs) when it tries to exec.  The
	 * startup code in "locore.s" has allocated space for the frame and
	 * passed a pointer to that space as main's argument.
	 */
	cpu_set_init_frame(p, framep);

	/*
	 * Need just enough stack to hold the faked-up "execve()" arguments.
	 */
	addr = trunc_page(VM_MAX_ADDRESS - PAGE_SIZE);
	if (vm_allocate(&p->p_vmspace->vm_map, &addr, PAGE_SIZE, FALSE) != 0)
		panic("init: couldn't allocate argument space");
	p->p_vmspace->vm_maxsaddr = (caddr_t) addr;

	if ((var = kern_getenv("init_path")) != NULL) {
		strlcpy(initpaths, var, sizeof(initpaths));
		freeenv(var);
	}
	free_initpaths = tmp_initpaths = strdup(initpaths, M_TEMP);

	for (pathp = &initpaths[0]; (path = *pathp) != NULL; pathp++) {
		/*
		 * Construct the boot flag argument.
		 */
		options = 0;
		flagsp = flags + 1;
		ucp = (char*) USRSTACK;
		if (boothowto & RB_SINGLE) {
			*flagsp++ = 's';
			options = 1;
		}
#ifdef notyet
		if (boothowto & RB_FASTBOOT) {
			*flagsp++ = 'f';
			options = 1;
		}
#endif
		/*
		 * Move out the flags (arg 1), if necessary.
		 */
		if (options != 0) {
			*flagsp++ = '\0';
			i = flagsp - flags;
			(void) copyout((caddr_t) flags, (caddr_t) (ucp -= i), i);
			arg1 = ucp;
		}

		/*
		 * Move out the file name (also arg 0).
		 */
		i = strlen(path) + 1;
		(void) copyout((caddr_t) path, (caddr_t) (ucp -= i), i);
		arg0 = ucp;

		/*
		 * Move out the arg pointers.
		 */
		uap = (char**) ((long) ucp & ~ALIGNBYTES);
		(void) suword((caddr_t) --uap, 0); 				/* terminator */
		if (options != 0)
			(void) suword((caddr_t) --uap, (long) arg1);
		(void) suword((caddr_t) --uap, (long) arg0);

		/*
		 * Point at the arguments.
		 */
		&args->fname = arg0;
		&args->argp = uap;
		&args->envp = NULL;

		/*
		 * Now try to exec the program.  If can't for any reason
		 * other than it doesn't exist, complain.
		 */
		error = execve(p, &args, retval);
		if (error == 0 || error == EJUSTRETURN)
			free(free_initpaths, M_TEMP);
			return;
		if (error != ENOENT)
			printf("exec %s: error %d\n", path, error);
	}
	free(free_initpaths, M_TEMP);
	printf("init: not found\n");
	panic("no init");
}

/*
 * Initialize hash links for buffers.
 */
static void
bhinit()
{
	register int i;
	register struct bufhd *bp;

	for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

memaddr_t bpaddr;		/* physical click-address of buffers */

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
static void
binit()
{
	register struct buf *bp;
	register int i;
	long paddr;

	for (bp = bufqueues; bp < &bufqueues[BQUEUES]; bp++)
		bp->b_forw = bp->b_back = bp->av_forw = bp->av_back = bp;
	paddr = ((long)bpaddr) << 6;
	for (i = 0; i < nbuf; i++, paddr += MAXBSIZE) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_bcount = 0;
		_binsheadfree(bp, TAILQ_FIRST(&bufqueues));
		_binshash(bp, TAILQ_FIRST(&bufqueues));
		bp->b_flags = B_BUSY|B_INVAL;
		brelse(bp);
	}
}
