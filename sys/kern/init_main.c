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
#include <sys/systm.h>
#include <sys/sysdecl.h>
#include <sys/filedesc.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/kthread.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/hint.h>
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/protosw.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/sched.h>
#include <sys/ksyms.h>
#include <sys/kenv.h>
#include <sys/msgbuf.h>
#include <sys/tprintf.h>
#include <sys/stdint.h>
#include <sys/null.h>
#include <sys/domain.h>
#include <sys/uuid.h>

#include <vm/include/vm.h>
#include <vm/include/vm_psegment.h>

#ifdef OVERLAY
#include <ovl/include/ovl.h>
#endif

#include <machine/cpu.h>

#if NRND > 0
#include <dev/disk/rnd/rnd.h>
#endif

extern char copyright[];

struct 	session session0;
struct 	pgrp pgrp0;
struct 	proc proc0;
struct thread thread0;
struct 	pcred cred0;
struct 	filedesc0 filedesc0;
struct 	plimit limit0;
struct 	vmspace vmspace0;
#ifdef OVERLAY
struct ovlspace ovlspace0;
#endif
#ifndef curproc
struct 	proc *curproc = &proc0;
#endif
struct proc *initproc, *pageproc;

int	netoff = 1;
int	securelevel;
int 	cmask = CMASK;
extern	struct user *proc0paddr;

struct 	vnode *rootvp, *swapdev_vp;
int		boothowto;
struct	timeval boottime;
struct	timeval runtime;

#ifndef BOOTVERBOSE
#define	BOOTVERBOSE	0
#endif
int	bootverbose = BOOTVERBOSE;

#ifdef OVERLAY
static void ovlspace_init(struct proc *p);
#endif

static void	vmspace_init(struct proc *p);
static void start_init(struct proc *p, void *framep);
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
	register struct thread *td;
	register struct filedesc0 *fdp;
	register struct pdevinit *pdev;
	register int i;
	register_t rval[2];
	extern struct pdevinit pdevinit[];
	extern void roundrobin(void *);
	extern void schedcpu(void *);

	/*
	 * Initialize the current process pointer (curproc) before
	 * any possible traps/probes to simplify trap processing.
	 */
	p = &proc0;
	curproc = p;

	/* initialize current thread & proc overseer from thread0 */
	p->p_threado = td = &thread0;
	p->p_curthread = td;

	/*
	 * Attempt to find console and initialize
	 * in case of early panic or other messages.
	 */
	consinit();
	printf(copyright);

	vm_mem_init();
#ifdef OVERLAY
	ovl_mem_init();
#endif
	kmeminit();
	startup();
	rmapinit();

	ksyms_init();
	prf_init();

	/* Initialize callouts, part 1. */
	callout_startup();

	/*
	 * Initialize process and pgrp structures.
	 */
	procinit(p);

	/* Initialize thread structures. */
	threadinit(p, td);

	/*
	 * Initialize device switch tables
	 */
	devswtable_init();

	/*
	 * Initialize kernel environment & resource structures
	 */
	kenv_init();
	resource_init();

	/*
	 * set up system process 0 (swapper)
	 */
    LIST_INSERT_HEAD(&allproc, p, p_list);
	p->p_pgrp = &pgrp0;

	/* set up kernel thread 0 */
	LIST_INSERT_HEAD(p->p_allthread, td, td_list);
	td->td_pgrp = &pgrp0;

	LIST_INSERT_HEAD(PGRPHASH(0), &pgrp0, pg_hash);
	LIST_INIT(&pgrp0.pg_mem);
	LIST_INSERT_HEAD(&pgrp0.pg_mem, p, p_pglist);
	
	pgrp0.pg_session = &session0;
	session0.s_count = 1;
	session0.s_leader = p;

	p->p_stat = SRUN;
	p->p_flag |= P_SLOAD|P_SSYS;
	p->p_nice = NZERO;
	p->p_emul = &emul_211bsd;

	u.u_procp = p;
	u.u_ap = u.u_arg;
	bcopy("swapper", p->p_comm, sizeof ("swapper"));

	/* Create credentials. */
	cred0.p_refcnt = 1;
	p->p_cred = &cred0;
	u.u_ucred = crget();
	u.u_ucred->cr_ngroups = 1; /* group 0 */
	p->p_ucred = u.u_ucred;

	callout_init(&p->p_tsleep_ch);

	/* give the thread the same creds as the initial thread */
	td->td_ucred = p->p_ucred;
	crhold(td->td_ucred);

	/* Create the file descriptor table. */
	fdp = &filedesc0;
	u.u_fd = &fdp->fd_fd;
	p->p_fd = u.u_fd;
	fdp->fd_fd.fd_refcnt = 1;
	fdp->fd_fd.fd_cmask = cmask;
	fdp->fd_fd.fd_ofiles = fdp->fd_dfiles;
	fdp->fd_fd.fd_ofileflags = fdp->fd_dfileflags;
	fdp->fd_fd.fd_nfiles = NDFILE;
	finit(&fdp->fd_fd);

	/* Create the limits structures. */
	p->p_limit = &limit0;
	for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++) {
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max = RLIM_INFINITY;
	}
	u.u_rlimit[RLIMIT_NOFILE].rlim_cur = NOFILE;
	u.u_rlimit[RLIMIT_NPROC].rlim_cur = MAXUPRC;
	i = ptoa(cnt.v_page_free_count);
	u.u_rlimit[RLIMIT_RSS].rlim_max = i;
	u.u_rlimit[RLIMIT_MEMLOCK].rlim_max = i;
	u.u_rlimit[RLIMIT_MEMLOCK].rlim_cur = i / 3;
	limit0.p_refcnt = 1;

	bcopy("root", u.u_login, sizeof ("root"));

	/* Allocate a prototype map so we have something to fork. */
	vmspace_init(p);

#ifdef OVERLAY
	ovlspace_init(p);
#endif

	/* Initialize pseudo-segmentation */
	vm_psegment_init(&vmspace0.vm_psegment);
	vmspace0.vm_psegment.ps_vmspace = &vmspace0;

	p->p_addr = proc0paddr;				/* XXX */

	/*
	 * We continue to place resource usage info and signal
	 * actions in the user struct so they're pageable.
	*/
	p->p_stats = &u.u_stats;
	p->p_sigacts = &u.u_sigacts;

	/*
	 * Initialize per uid information structure and charge
	 * root for one process.
	 */
	(void)chgproccnt(0, 1);

	rqinit();
	sqinit();

	thread_rqinit(p);
	thread_sqinit(p);

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
	netstart();

	/* initialize entropy pool */
#if NRND > 0
	rnd_init();
#endif

	/* Initialize the UUID system calls. */
	uuid_init();

#ifdef GPROF
	/* Initialize kernel profiling. */
	kmstartup();
#endif

	/* Initialize Global Scheduler */
	sched_init(p);

	/* Kick off timeout driven events by calling first time. */
	roundrobin(NULL);
	schedcpu(NULL);

	/* Mount the root file system. */
	if (vfs_mountroot()) {
		panic("cannot mount root");
	}
	CIRCLEQ_FIRST(&mountlist)->mnt_flag |= MNT_ROOTFS;

	/* Get the vnode for '/'.  Set fdp->fd_fd.fd_cdir to reference it. */
	if (VFS_ROOT(CIRCLEQ_FIRST(&mountlist), &rootvnode)) {
		panic("cannot find root vnode");
	}
	fdp->fd_fd.fd_cdir = rootvnode;
	VREF(fdp->fd_fd.fd_cdir);
	fdp->fd_fd.fd_rdir = rootvnode;
	VREF(fdp->fd_fd.fd_rdir);
	VOP_UNLOCK(rootvnode, 0, p);
	swapinit();

	p->p_stats->p_start = runtime = mono_time = boottime = time;
	p->p_rtime.tv_sec = p->p_rtime.tv_usec = 0;

	/* Initialize signal state for process 0 */
	siginit(p);

	/* Create process 1 (init(8)). */
	if (newproc(0)) {
		panic("fork init");
	}
	if (rval[1]) {
		start_init(curproc, framep);
		return;
	}

	/* Create process 2 (the pageout daemon). */
	if (newproc(0)) {
		panic("fork pager");
	}
	if (rval[1]) {
		/*
		 * Now in process 2.
		 */
		p = curproc;
		pageproc = p;
		p->p_flag |= P_INMEM | P_SYSTEM;
		bcopy("pagedaemon", curproc->p_comm, sizeof("pagedaemon"));
		vm_pageout();
		/* NOTREACHED */
	}

	/*
	 * Create any kernel threads who's creation was deferred because
	 * initproc had not yet been created.
	 */
	kthread_run_deferred_queue();

	/* Initialize exec structures */
	exec_init();

	scheduler();
	/* NOTREACHED */
}

/*
 * Initialize Vmspace
 */
static void
vmspace_init(p)
	struct proc *p;
{
	p->p_vmspace = &vmspace0;
	vmspace0.vm_refcnt = 1;
	pmap_pinit(&vmspace0.vm_pmap);
	vm_map_init(&vmspace0.vm_map, round_page(VM_MIN_ADDRESS), trunc_page(VM_MAX_ADDRESS), TRUE);
	vmspace0.vm_map.pmap = &vmspace0.vm_pmap;
}

/*
 * Initialize Ovlspace
 */
#ifdef OVERLAY
static void
ovlspace_init(p)
	struct proc *p;
{
	p->p_ovlspace = &ovlspace0;
	ovlspace0.ovl_refcnt = 1;
	pmap_pinit(&ovlspace0.ovl_pmap);
	ovl_map_init(&ovlspace0.ovl_map, round_page(OVL_MIN_ADDRESS), trunc_page(OVL_MAX_ADDRESS));
	ovlspace0.ovl_map.pmap = &ovlspace0.ovl_pmap;
}
#endif

/*
 * List of paths to try when searching for "init".
 */
static char initpaths[MAXPATHLEN] = "/sbin/init:/sbin/oinit:/sbin/init.bak";

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
	struct execa_args args;
	register_t retval[2];
	int options, error;
	char *var, *path, *next, *s;
	char *ucp, **uap, *arg0, *arg1;

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
	if (vm_map_find(&p->p_vmspace->vm_map, NULL, 0, &addr, PAGE_SIZE, FALSE) != 0) {
		panic("init: couldn't allocate argument space");
	}
	p->p_vmspace->vm_maxsaddr = (caddr_t)addr;
	p->p_vmspace->vm_ssize = 1;

	if ((var = kern_getenv("init_path")) != NULL) {
		strlcpy(initpaths, var, sizeof(initpaths));
		freeenv(var);
	}

	for (path = initpaths; *path != '\0'; path = next) {
		while (*path == ':') {
			path++;
		}
		if (*path == '\0') {
			break;
		}
		for (next = path; *next != '\0' && *next != ':'; next++)
			/* nothing */;
		if (bootverbose) {
			printf("start_init: trying %.*s\n", (int) (next - path), path);
		}

		/*
		 * Move out the boot flag argument.
		 */
		options = 0;
		ucp = (char*) USRSTACK;
		(void) subyte(--ucp, 0); /* trailing zero */
		if (boothowto & RB_SINGLE) {
			(void) subyte(--ucp, 's');
			options = 1;
		}
#ifdef notyet
		if (boothowto & RB_FASTBOOT) {
			(void) subyte(--ucp, 'f');
			options = 1;
		}
#endif

#ifdef BOOTCDROM
		(void) subyte(--ucp, 'C');
		options = 1;
#endif
		if (options == 0) {
			(void) subyte(--ucp, '-');
		}
		(void) subyte(--ucp, '-'); /* leading hyphen */
		arg1 = ucp;

		/*
		 * Move out the file name (also arg 0).
		 */
		(void) subyte(--ucp, 0);
		for (s = next - 1; s >= path; s--)
			(void) subyte(--ucp, *s);
		arg0 = ucp;

		/*
		 * Move out the arg pointers.
		 */
		uap = (char **)((intptr_t)ucp & ~(sizeof(intptr_t)-1));
		(void) suword((caddr_t) --uap, (long) 0); /* terminator */
		(void) suword((caddr_t) --uap, (long) (intptr_t) arg1);
		(void) suword((caddr_t) --uap, (long) (intptr_t) arg0);

		/*
		 * Point at the arguments.
		 */
		doexeca(&args, arg0, uap, NULL);

		/*
		 * Now try to exec the program.  If can't for any reason
		 * other than it doesn't exist, complain.
		 */
		error = execa(p, &args, retval);
		if (error == 0) {
			return;
		}
		if (error != ENOENT) {
			printf("exec %.*s: error %d\n", (int) (next - path), path, error);
		}
	}
	printf("init: not found\n");
	panic("no init");
}
