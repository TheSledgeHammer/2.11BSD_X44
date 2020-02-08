/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)init_main.c	2.5 (2.11BSD GTE) 1997/9/26
 */

#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/errno.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/signalvar.h>
#include <sys/systm.h>
#include <sys/vnode.h>
#include <sys/sysent.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/protosw.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <vm/include/vm.h>

#include <machine/cpu.h>

struct 	session session0;
struct 	pgrp pgrp0;
struct 	proc proc0;
struct 	pcred cred0;
struct 	fs0 filedesc0;
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
int
main()
{
	register struct proc *p;
	register struct filedesc0 *fdp;
	register struct pdevinit *pdev;
	register int i;
	int s;
	register_t rval[2];
	extern struct pdevinit pdevinit[];
	extern struct sysentvec sysvec;
	extern void roundrobin __P((void *));
	extern void schedcpu __P((void *));

	/*
	 * Initialize the current process pointer (curproc) before
	 * any possible traps/probes to simplify trap processing.
	 */
	p = &proc0;
	curproc = p;

	vm_mem_init();
	kmeminit();

	startup();

	/*
	 * Initialize process and pgrp structures.
	 */
	procinit();

	/*
	 * set up system process 0 (swapper)
	 */
	allproc = (struct proc *)p;
	p->p_prev = (struct proc **)&allproc;
	p->p_pgrp = &pgrp0;
	pgrphash[0] = &pgrp0;

	p->p_sysent = &sysvec;

	p->p_stat = SRUN;
	p->p_flag |= P_SLOAD|P_SSYS;
	p->p_nice = NZERO;

	u->u_procp = p;
	bcopy("swapper", p->p_comm, sizeof ("swapper"));

	/* Create credentials. */
	cred0.p_refcnt = 1;
	p->p_cred = &cred0;
	u->u_cred = crget();
	u->u_cred->cr_ngroups = 1; /* group 0 */
	p->p_ucred = u->u_cred;

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

	/* Allocate a prototype map so we have something to fork. */
	p->p_vmspace = &vmspace0;
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
	(void)chgproccnt(0, 1);

	rqinit();

	/* Configure virtual memory system, set vm rlimits. */
	vm_init_limits(p);

	/* Initialize the file systems. */
	vfsinit();

	/* Start real time and statistics clocks. */
	initclocks();

	/* Initialize mbuf's. */
	mbinit();

	/* Initialize clists, tables, protocols. */
	cinit();

	/* Attach pseudo-devices. */
	for (pdev = pdevinit; pdev->pdev_attach != NULL; pdev++)
		(*pdev->pdev_attach)(pdev->pdev_count);

	/*
	 * Initialize protocols.  Block reception of incoming packets
	 * until everything is ready.
	 */
	s = splimp();
	ifinit();
	domaininit();
	splx(s);

	/* Kick off timeout driven events by calling first time. */
	roundrobin(NULL);
	schedcpu(NULL);

	/* Mount the root file system. */
	if (vfs_mountroot())
		panic("cannot mount root");
	mountlist.cqh_first->mnt_flag |= MNT_ROOTFS;
	/* Get the vnode for '/'.  Set fdp->fd_fd.fd_cdir to reference it. */
	if (VFS_ROOT(mountlist.cqh_first, &rootvnode))
		panic("cannot find root vnode");
	fdp->fd_fd.fd_cdir = rootvnode;
	VREF(fdp->fd_fd.fd_cdir);
	VOP_UNLOCK(rootvnode, 0, p);
	fdp->fd_fd.fd_rdir = NULL;
	swapinit();

	p->p_stats->p_start = runtime = mono_time = boottime = time;
	p->p_rtime.tv_sec = p->p_rtime.tv_usec = 0;

	/* Initialize signal state for process 0 */
	siginit(p);

	/* Create process 1 (init(8)). */
	if (fork(NULL))
		panic("fork init");
	if (rval[1]) {
		//start_init(curproc, framep);
		//return;
	}

	/* Create process 2 (the pageout daemon). */
	if (fork(NULL))
		panic("fork pager");
	if (rval[1]) {
		/*
		 * Now in process 2.
		 */
		p = curproc;
		pageproc = p;
		p->p_flag |= P_INMEM | P_SYSTEM;	/* XXX */
		bcopy("pagedaemon", curproc->p_comm, sizeof ("pagedaemon"));
		vm_pageout();
		/* NOTREACHED */
	}

	/* Initialize exec structures */
	exec_init(1);

	scheduler();
	/* NOTREACHED */

	/* Below needs attention: Not in correct place: Old VM references */
	/*
	 * make init process
	 */
	if (newproc(0)) {
		expand((int)btoc(szicode), S_DATA);
		expand((int)1, S_STACK);						/* one click of stack */
		copyout((caddr_t)icode, (caddr_t)0, szicode);
		/*
		 * return goes to location 0 of user init code
		 * just copied out.
		 */
		return 0;
	}
	else
		sched();
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
void
cinit()
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int) cfree;
	ccp = (ccp + CROUND) & ~CROUND;

	for (cp = (struct cblock *)ccp; cp <= &cfree[nclist - 1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}
}
