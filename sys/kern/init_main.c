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

#include <machine/cpu.h>

#include <vm/vm.h>

struct 	session session0;
struct 	pgrp pgrp0;
struct 	proc proc0;
struct 	pcred cred0;
struct 	fs0 filedesc0;
struct 	plimit limit0;
struct 	vmspace vmspace0;
struct 	proc *curproc = &proc0;

int		securelevel;
int 	cmask = CMASK;
extern	struct user *proc0paddr;

struct 	inode *rootvp, *swapdev_vp;
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
	register struct user *u;
	register int i;
	//extern struct sysentvec aout_sysvec; /* use exec_linker */

	/*
	 * Initialize the current process pointer (curproc) before
	 * any possible traps/probes to simplify trap processing.
	 */
	p = &proc0;
	curproc = p;

	vm_mem_init();

	startup();

	/*
	 * set up system process 0 (swapper)
	 */

	allproc = (volatile struct proc *)p;
	p->p_prev = (struct proc **)&allproc;
	p->p_pgrp = &pgrp0;
	pgrphash[0] = &pgrp0;

	//p->p_sysent = &sysvec;

	p->p_stat = SRUN;
	p->p_flag |= SLOAD|SSYS;
	p->p_nice = NZERO;
	p->p_rtprio.type = RTP_PRIO_NORMAL;
	p->p_rtprio.prio = 0;

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
	//p->p_stats = &p->p_addr->u_stats;
	//p->p_sig = &p->p_addr->u_sigacts;

	p->p_stats = &u->u_stats;
	p->p_sig = &u->u_sigacts;

	/*
	 * Initialize per uid information structure and charge
	 * root for one process.
	 */
	(void)chgproccnt(0, 1);

	rqinit();

	/* Configure virtual memory system, set vm rlimits. */
	vm_init_limits(p);


	/* Initialize signal state for process 0 */
	siginit(p);

	/*
	 * Initialize tables, protocols.
	 */
	cinit();
	pqinit();

	/* Kick off timeout driven events by calling first time. */
	schedcpu(NULL);


	/*
	 * make init process
	 */
	if (newproc(0)) {
		expand((int)btoc(szicode), S_DATA);
		expand((int)1, S_STACK);	/* one click of stack */
		estabur((u_int)0, (u_int)btoc(szicode), (u_int)1, 0, RO);
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
static void
cinit()
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int)cfree;
	ccp = (ccp + CROUND) & ~CROUND;

	for (cp = (struct cblock *)ccp; cp <= &cfree[nclist - 1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}
}
