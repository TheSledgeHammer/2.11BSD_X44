/* 
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	@(#)vm_glue.c	8.9 (Berkeley) 3/4/95
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/buf.h>
#include <sys/user.h>

#include <devel/vm/include/vm_kern.h>
#include <devel/vm/include/vm_page.h>

#include <machine/cpu.h>
#include <machine/stdarg.h>
#include <devel/vm/include/vm.h>

int	avefree = 0;			/* XXX */
unsigned maxdmap = MAXDSIZ;	/* XXX */
int	readbuffers = 0;		/* XXX allow kgdb to read kernel buffer pool */

int
kernacc(addr, len, rw)
	caddr_t addr;
	int len, rw;
{
	boolean_t rv;
	vm_offset_t saddr, eaddr;
	vm_prot_t prot = rw == B_READ ? VM_PROT_READ : VM_PROT_WRITE;

	saddr = trunc_page(addr);
	eaddr = round_page(addr+len);
	rv = vm_map_check_protection(kernel_map, saddr, eaddr, prot);
	/*
	 * XXX there are still some things (e.g. the buffer cache) that
	 * are managed behind the VM system's back so even though an
	 * address is accessible in the mind of the VM system, there may
	 * not be physical pages where the VM thinks there is.  This can
	 * lead to bogus allocation of pages in the kernel address space
	 * or worse, inconsistencies at the pmap level.  We only worry
	 * about the buffer cache for now.
	 */
	if (!readbuffers && rv && (eaddr > (vm_offset_t)buffers &&
		   saddr < (vm_offset_t)buffers + MAXBSIZE * nbuf))
		rv = FALSE;
	return(rv == TRUE);
}

int
useracc(addr, len, rw)
	caddr_t addr;
	int len, rw;
{
	boolean_t rv;
	vm_prot_t prot = rw == B_READ ? VM_PROT_READ : VM_PROT_WRITE;

	rv = vm_map_check_protection(&curproc->p_vmspace->vm_map,
	    trunc_page(addr), round_page(addr+len), prot);
	return(rv == TRUE);
}

#ifdef KGDB
/*
 * Change protections on kernel pages from addr to addr+len
 * (presumably so debugger can plant a breakpoint).
 *
 * We force the protection change at the pmap level.  If we were
 * to use vm_map_protect a change to allow writing would be lazily-
 * applied meaning we would still take a protection fault, something
 * we really don't want to do.  It would also fragment the kernel
 * map unnecessarily.  We cannot use pmap_protect since it also won't
 * enforce a write-enable request.  Using pmap_enter is the only way
 * we can ensure the change takes place properly.
 */
void
chgkprot(addr, len, rw)
	register caddr_t addr;
	int len, rw;
{
	vm_prot_t prot;
	vm_offset_t pa, sva, eva;

	prot = rw == B_READ ? VM_PROT_READ : VM_PROT_READ|VM_PROT_WRITE;
	eva = round_page(addr + len);
	for (sva = trunc_page(addr); sva < eva; sva += PAGE_SIZE) {
		/*
		 * Extract physical address for the page.
		 * We use a cheezy hack to differentiate physical
		 * page 0 from an invalid mapping, not that it
		 * really matters...
		 */
		pa = pmap_extract(kernel_pmap, sva|1);
		if (pa == 0)
			panic("chgkprot: invalid page");
		pmap_enter(kernel_pmap, sva, pa&~1, prot, TRUE);
	}
}
#endif

void
vslock(addr, len)
	caddr_t	addr;
	u_int	len;
{
	vm_map_pageable(&curproc->p_vmspace->vm_map, trunc_page(addr),
			round_page(addr+len), FALSE);
}

void
vsunlock(addr, len, dirtied)
	caddr_t	addr;
	u_int	len;
	int dirtied;
{
#ifdef	lint
	dirtied++;
#endif
	vm_map_pageable(&curproc->p_vmspace->vm_map, trunc_page(addr),
			round_page(addr+len), TRUE);
}

/*
 * Implement fork's actions on an address space.
 * Here we arrange for the address space to be copied or referenced,
 * allocate a user struct (pcb and kernel stack), then call the
 * machine-dependent layer to fill those in and make the new process
 * ready to run.
 * NOTE: the kernel stack may be at a different location in the child
 * process, and thus addresses of automatic variables may be invalid
 * after cpu_fork returns in the child process.  We do nothing here
 * after cpu_fork returns.
 */
int
vm_fork(p1, p2, isvfork)
	register struct proc *p1, *p2;
	int isvfork;
{
	register struct user *up;
	vm_offset_t addr;


	/*
	 * avoid copying any of the parent's pagetables or other per-process
	 * objects that reside in the map by marking all of them non-inheritable
	 */
	(void)vm_map_inherit(&p1->p_vmspace->vm_map,
		UPT_MIN_ADDRESS-UPAGES*NBPG, VM_MAX_ADDRESS, VM_INHERIT_NONE);

	p2->p_vmspace = vmspace_fork(p1->p_vmspace);

#ifdef SYSVSHM
	if (p1->p_vmspace->vm_shm)
		shmfork(p1, p2, isvfork);
#endif


	/*
	 * Allocate a wired-down (for now) pcb and kernel stack for the process
	 */
	addr = kmem_alloc_pageable(kernel_map, ctob(UPAGES));
	if (addr == 0)
		panic("vm_fork: no more kernel virtual memory");
	vm_map_pageable(kernel_map, addr, addr + ctob(UPAGES), FALSE);

/* XXX somehow, on 386, ocassionally pageout removes active, wired down kstack,
and pagetables, WITHOUT going thru vm_page_unwire! Why this appears to work is
not yet clear, yet it does... */
	addr = kmem_alloc(kernel_map, ctob(UPAGES));
	if (addr == 0)
		panic("vm_fork: no more kernel virtual memory");

	up = (struct user *)addr;
	p2->p_addr = up;

	/*
	 * p_stats and p_sigacts currently point at fields
	 * in the user struct but not at &u, instead at p_addr.
	 * Copy p_sigacts and parts of p_stats; zero the rest
	 * of p_stats (statistics).
	 */
	p2->p_stats = &up->u_stats;
	p2->p_sigacts = &up->u_sigacts;
	up->u_sigacts = *p1->p_sigacts;
	bzero(&up->u_stats.pstat_startzero,
	    (unsigned) ((caddr_t)&up->u_stats.pstat_endzero -
	    (caddr_t)&up->u_stats.pstat_startzero));
	bcopy(&p1->p_stats->pstat_startcopy, &up->u_stats.pstat_startcopy,
	    ((caddr_t)&up->u_stats.pstat_endcopy -
	     (caddr_t)&up->u_stats.pstat_startcopy));

#ifdef i386
	{ u_int addr = UPT_MIN_ADDRESS - UPAGES*NBPG; struct vm_map *vp;

	vp = &p2->p_vmspace->vm_map;
	(void)vm_deallocate(vp, addr, UPT_MAX_ADDRESS - addr);
	(void)vm_allocate(vp, &addr, UPT_MAX_ADDRESS - addr, FALSE);
	(void)vm_map_inherit(vp, addr, UPT_MAX_ADDRESS, VM_INHERIT_NONE);
	}
#endif
	/*
	 * cpu_fork will copy and update the kernel stack and pcb,
	 * and make the child ready to run.  It marks the child
	 * so that it can return differently than the parent.
	 * It returns twice, once in the parent process and
	 * once in the child.
	 */
	return (cpu_fork(p1, p2));
}

/*
 * Set default limits for VM system.
 * Called for proc 0, and then inherited by all others.
 */
void
vm_init_limits(p)
	register struct proc *p;
{
	int rss_limit;
	/*
	 * Set up the initial limits on process VM.
	 * Set the maximum resident set size to be all
	 * of (reasonably) available memory.  This causes
	 * any single, large process to start random page
	 * replacement once it fills memory.
	 */
	p->p_rlimit[RLIMIT_STACK].rlim_cur = DFLSSIZ;
    p->p_rlimit[RLIMIT_STACK].rlim_max = MAXSSIZ;
    p->p_rlimit[RLIMIT_DATA].rlim_cur = DFLDSIZ;
    /* limit the limit to no less than 128K */
    rss_limit = max(cnt.v_free_count / 2, 32);
    p->p_rlimit[RLIMIT_DATA].rlim_max = RLIM_INFINITY;
	p->p_rlimit[RLIMIT_RSS].rlim_cur = ptoa(rss_limit);
}

#include "vm_pageout.h"

#ifdef DEBUG
int		enableswap = 1;
int		swapdebug = 0;
#define	SDB_FOLLOW	1
#define SDB_SWAPIN	2
#define SDB_SWAPOUT	4
#endif

int swapinreq;
int percentactive;

/*
 * Brutally simple:
 *	1. Attempt to swapin every swaped-out, runnable process in
 *	   order of priority.
 *	2. If not enough memory, wake the pageout daemon and let it
 *	   clear some space.
 */
void
scheduler()
{
	register struct proc *p;
	register int pri;
	struct proc *pp;
	int ppri;

	//vm_offset_t addr;
	//vm_size_t size;

	int lastidle, lastrun;
	int curidle, currun;
	int forceload;
	int percent;
	int ntries;

	lastidle = 0;
	lastrun = 0;

loop:
	ntries = 0;

	curidle = cp_time[CP_IDLE];
	currun = cp_time[CP_USER] + cp_time[CP_SYS] + cp_time[CP_NICE];
	percent = (100*(currun-lastrun)) / ( 1 + (currun-lastrun) + (curidle-lastidle));
	lastrun = currun;
	lastidle = curidle;
	if( percent > 100)
		percent = 100;
	percentactive = percent;

	if( percentactive < 25)
		forceload = 1;
	else
		forceload = 0;

loop1:
	pp = NULL;
	ppri = INT_MIN;
	for (p = (struct proc *)allproc; p != NULL; p = p->p_nxt) {
		if (p->p_stat == SRUN && (p->p_flag &  (P_INMEM|P_SWAPPING)) == 0) {
			int mempri;
			pri = p->p_swtime + p->p_slptime - p->p_nice * 8;
			mempri = pri > 0 ? pri : 0;
			/*
			 * if this process is higher priority and there is
			 * enough space, then select this process instead
			 * of the previous selection.
			 */
			if (pri > ppri &&
					(((cnt.v_free_count + (mempri * (4*PAGE_SIZE) / PAGE_SIZE) >= (p->p_vmspace->vm_swrss)) || (ntries > 0 && forceload)))) {
				pp = p;
				ppri = pri;
			}
		}
	}


	if ((pp == NULL) && (ntries == 0) && forceload) {
		++ntries;
		goto loop1;
	}

	/*
	 * Nothing to do, back to sleep
	 */
	if ((p = pp) == NULL) {
		tsleep((caddr_t)&proc0, PVM, "scheduler", 0);
		goto loop;
	}

	/*
	 * We would like to bring someone in.
	 * This part is really bogus cuz we could deadlock on memory
	 * despite our feeble check.
	 * XXX should require at least vm_swrss / 2
	 */

	(void) splhigh();
	if ((forceload && (cnt.v_free_count > (cnt.v_free_reserved + UPAGES + 1))) ||
		    (cnt.v_free_count >= cnt.v_free_min)) {
		spl0();
		faultin(p);
		p->p_swtime = 0;
		goto loop;
	}
	/*
	 * log the memory shortage
	 */
	swapinreq += p->p_vmspace->vm_swrss;
	/*
	 * Not enough memory, jab the pageout daemon and wait til the
	 * coast is clear.
	 */
	if( cnt.v_free_count < cnt.v_free_min) {
		VM_WAIT;
	} else {
		tsleep((caddr_t)&proc0, PVM, "scheduler", 0);
	}
	(void) spl0();
	goto loop;
}

#define	swappable(p)							\
	(((p)->p_flag &								\
			(P_TRACED|P_NOSWAP|P_SYSTEM|P_INMEM|P_WEXIT|P_PHYSIO|P_SWAPPING)) == P_INMEM)

/*
 * Swapout is driven by the pageout daemon.  Very simple, we find eligible
 * procs and unwire their u-areas.  We try to always "swap" at least one
 * process in case we need the room for a swapin.
 * If any procs have been sleeping/stopped for at least maxslp seconds,
 * they are swapped.  Else, we swap the longest-sleeping or stopped process,
 * if any, otherwise the longest-resident process.
 */
void
swapout_threads()
{
	register struct proc *p;
	struct proc *outp, *outp2;
	int outpri, outpri2;
	int tpri;
	int didswap = 0;
	int swapneeded = swapinreq;
	extern int maxslp;
	int runnablenow;


	outp = outp2 = NULL;
	outpri = outpri2 = 0;
	for (p = (struct proc *)allproc; p != NULL; p = p->p_nxt) {
		if (!swappable(p))
			continue;
		switch (p->p_stat) {
		case SRUN:
			if (p->p_swtime > outpri2) {
				outp2 = p;
				outpri2 = p->p_swtime;
			}
			continue;
			
		case SSLEEP:
		case SSTOP:
			if (!lock_try_write( &p->p_vmspace->vm_map.lock)) {
				continue;
			}
			vm_map_unlock( &p->p_vmspace->vm_map);
			if ((p->p_slptime > maxslp) && (p->p_vmspace->vm_pmap.pm_stats.resident_count <= 6)) {
				swapout(p);
				didswap++;
			} else if ((tpri = p->p_slptime + p->p_nice * 8) > outpri) {
				outp = p;
				outpri = p->p_slptime;
			}
			continue;
		}
	}
	/*
	 * We swapout only if there are more than two runnable processes or if
	 * another process needs some space to swapin.
	 */
	if ((swapinreq || ((percentactive > 90) && (runnablenow > 2))) &&
				(((cnt.v_free_count + cnt.v_inactive_count) <= (cnt.v_free_target + cnt.v_inactive_target)) ||
				(cnt.v_free_count < cnt.v_free_min))) {
		if ((p = outp) == 0) {
			p = outp2;
		}

		/*
		 * Only swapout processes that have already had most
		 * of their pages taken away.
		 */
		if (p && (p->p_vmspace->vm_pmap.pm_stats.resident_count <= 6)) {
					swapout(p);
					didswap = 1;
		}
	}

	/*
	 * if we previously had found a process to swapout, and we need to swapout
	 * more then try again.
	 */
#if 0
	if(p && swapinreq)
		goto swapmore;
#endif

	/*
	 * If we swapped something out, and another process needed memory,
	 * then wakeup the sched process.
	 */
	if (didswap) {
		if (swapneeded)
			wakeup((caddr_t)&proc0);
		swapinreq = 0;
	}
}

void
swapout(p)
	register struct proc *p;
{
	vm_offset_t addr;
	vm_size_t size;

	vm_map_t map = &p->p_vmspace->vm_map;

	++p->p_stats->p_ru.ru_nswap;
	/*
	 * remember the process resident count
	 */
	p->p_vmspace->vm_swrss =
			p->p_vmspace->vm_pmap.pm_stats.resident_count;
	/*
	 * and decrement the amount of needed space
	 */
	swapinreq -= min(swapinreq, p->p_vmspace->vm_pmap.pm_stats.resident_count);

	(void) splhigh();
	p->p_flag &= ~P_INMEM;
	if (p->p_stat == SRUN)
		remrq(p);
	(void) spl0();

	p->p_flag |= P_SWAPPING;
/* let the upages be paged */
	pmap_remove(vm_map_pmap(kernel_map),
		(vm_offset_t) p->p_addr, ((vm_offset_t) p->p_addr) + UPAGES * NBPG);

	vm_map_pageable(map, (vm_offset_t) kstack,
		(vm_offset_t) kstack + UPAGES * NBPG, TRUE);

	p->p_flag &= ~P_SWAPPING;
	p->p_swtime = 0;
}

/*
 * The rest of these routines fake thread handling
 */

void
assert_wait(event, ruptible)
	void *event;
	boolean_t ruptible;
{
#ifdef lint
	ruptible++;
#endif
	curproc->p_thread = event;
}

void
thread_block()
{
	int s = splhigh();

	if (curproc->p_thread)
		tsleep(curproc->p_thread, PVM, "thrd_block", 0);
	splx(s);
}

void
thread_sleep(event, lock, ruptible)
	void *event;
	simple_lock_t lock;
	boolean_t ruptible;
{
	int s = splhigh();

#ifdef lint
	ruptible++;
#endif
	curproc->p_thread = event;
	simple_unlock(lock);
	if (curproc->p_thread)
		tsleep(event, PVM, "thrd_sleep", 0);
	splx(s);
}

void
thread_wakeup(event)
	void *event;
{
	int s = splhigh();

	wakeup(event);
	splx(s);
}

/*
 * DEBUG stuff
 */

int indent = 0;

#include <machine/stdarg.h>		/* see subr_prf.c */

/*ARGSUSED2*/
void
#if __STDC__
iprintf(const char *fmt, ...)
#else
iprintf(fmt /* , va_alist */)
	char *fmt;
	/* va_dcl */
#endif
{
	register int i;
	va_list ap;

	for (i = indent; i >= 8; i -= 8)
		printf("\t");
	while (--i >= 0)
		printf(" ");
	va_start(ap, fmt);
	printf("%r", fmt, ap);
	va_end(ap);
}
