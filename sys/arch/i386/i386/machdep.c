/*-
 * Copyright (c) 1982, 1987, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)machdep.c	8.3 (Berkeley) 5/9/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/sysctl.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>

#include <net/netisr.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

vm_map_t buffer_map;
extern vm_offset_t avail_end;

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/specialreg.h>
#include <machine/sigframe.h>
#include <machine/bootinfo.h>
#include <machine/gdt.h>

#include <i386/isa/rtc.h>
#include <i386/i386/cons.h>

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif
int	msgbufmapped;		/* set when safe to use msgbuf */


/*
 * Machine-dependent startup code
 */
int boothowto = 0, Maxmem = 0;
long dumplo;
int physmem, maxmem;
extern int bootdev;
#ifdef SMALL
extern int forcemaxmem;
#endif
int biosmem;

extern cyloffset;
extern int kstack[];

union descriptor gdt[NGDT];
struct gate_descriptor idt[32+16];
union descriptor ldt[NLDT];

struct soft_segment_descriptor gdt_segs[];
struct soft_segment_descriptor ldt_segs[];

int lcr0(), lcr3(), rcr0(), rcr2();
int _udatasel, _ucodesel, _gsel_tss, _exit_tss;

struct i386tss inv_tss, dbl_tss, exit_tss;//tss, panic_tss;
char alt_stack[1024];

extern struct user *proc0paddr;

startup(firstaddr)
	int firstaddr;
{
	register int unixsize;
	register unsigned i;
	register struct pte *pte;
	int mapaddr, j;
	register caddr_t v;
	int maxbufs, base, residual;
	extern long Usrptsize;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;

	/*
	 * Initialize error message buffer (at end of core).
	 */

	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(sizeof(struct msgbuf)); i++)
		pmap_enter(kernel_pmap, msgbufp, avail_end + i * NBPG,
		VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

#ifdef KDB
	kdb_init();			/* startup kernel debugger */
#endif
	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("real mem  = %d\n", ctob(physmem));

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * As pages of memory are allocated and cleared,
	 * "firstaddr" is incremented.
	 * An index into the kernel page table corresponding to the
	 * virtual memory address maintained in "v" is kept in "mapaddr".
	 */

	/*
	 * Make two passes.  The first pass calculates how much memory is
	 * needed and allocates it.  The second pass assigns virtual
	 * addresses to the various data structures.
	 */
	firstaddr = 0;
	again: v = (caddr_t) firstaddr;

#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))
	valloc(cfree, struct cblock, nclist);
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
	/*
	 * Determine how many buffers to allocate.
	 * Use 10% of memory for the first 2 Meg, 5% of the remaining
	 * memory. Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		if (physmem < (2 * 1024 * 1024))
			bufpages = physmem / 10 / CLSIZE;
		else
			bufpages = ((2 * 1024 * 1024 + physmem) / 20) / CLSIZE;
	if (nbuf == 0) {
		nbuf = bufpages / 2;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) & ~1; /* force even */
		if (nswbuf > 256)
			nswbuf = 256; /* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);

	/*
	 * End of first pass, size has been calculated so allocate memory
	 */
	if (firstaddr == 0) {
		size = (vm_size_t) (v - firstaddr);
		firstaddr = (int) kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0)
			panic("startup: no room for tables");
		goto again;
	}
	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t) (v - firstaddr) != size)
		panic("startup: table size inconsistency");
	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t) &buffers, &maxaddr,
			size, TRUE);
	minaddr = (vm_offset_t) buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t) 0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vm_offset_t) buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base + 1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf + curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
	}
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr, 16 * NCARGS, TRUE);
	/*
	 * Allocate a submap for physio
	 */
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr, VM_PHYS_SIZE, TRUE);

	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	mclrefcnt = (char*) malloc(NMBCLUSTERS + CLBYTES / MCLBYTES,
	M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS + CLBYTES / MCLBYTES);
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t) &mbutl, &maxaddr,
			VM_MBUF_SIZE, FALSE);
	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i - 1].c_next = &callout[i];
	callout[i - 1].c_next = NULL;

	/*printf("avail mem = %d\n", ptoa(vm_page_free_count));*/
	printf("using %d buffers containing %d bytes of memory\n", nbuf,
			bufpages * CLBYTES);

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	configure();

	cpu_setregs();
}


void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = splhigh();

	*tvp = time;
	tvp->tv_usec += tick;
	while (tvp->tv_usec > 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	splx(s);
}

physstrat(bp, strat, prio)
	struct buf *bp;
	int (*strat)(), prio;
{
	register int s;
	caddr_t baddr;

	/*
	 * vmapbuf clobbers b_addr so we must remember it so that it
	 * can be restored after vunmapbuf.  This is truely rude, we
	 * should really be storing this in a field in the buf struct
	 * but none are available and I didn't want to add one at
	 * this time.  Note that b_addr for dirty page pushes is
	 * restored in vunmapbuf. (ugh!)
	 */
	baddr = bp->b_un.b_addr;
	vmapbuf(bp);
	(*strat)(bp);
	/* pageout daemon doesn't wait for pushed pages */
	if (bp->b_flags & B_DIRTY)
		return;
	s = splbio();
	while ((bp->b_flags & B_DONE) == 0)
		sleep((caddr_t) bp, prio);
	splx(s);
	vunmapbuf(bp);
	bp->b_un.b_addr = baddr;
}
initcpu()
{

}

/*
 * Set up proc0's PCB and LDT.
 */
static void
i386_proc0_pcb_ldt_init(void)
{
	register struct proc *p = curproc;
}

static void
tss_init(tss, stack, func)
	struct i386tss *tss;
	void *stack;
	void *func;
{
	KASSERT(curcpu()->ci_pmap == pmap_kernel());

	memset(tss, 0, sizeof *tss);
	tss->tss_esp0 = tss->tss_esp = (int)((char *)stack + USPACE - 16);
	tss->tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	tss->tss_cs = GSEL(GCODE_SEL, SEL_KPL);
	tss->tss_fs = GSEL(GCPU_SEL, SEL_KPL);
	tss->tss_gs = tss->__tss_es = tss->__tss_ds =
	    tss->tss_ss = GSEL(GDATA_SEL, SEL_KPL);
	/* %cr3 contains the value associated to pmap_kernel */
	tss->tss_cr3 = rcr3();
	tss->tss_esp = (int)((char *)stack + USPACE - 16);
	tss->tss_ldt = GSEL(GLDT_SEL, SEL_KPL);
	tss->tss_eflags = PSL_MBO | PSL_NT;	/* XXX not needed? */
	tss->tss_eip = (int)func;
}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the  current time and a previous
 * time as represented  by the arguments.
 * If there is a pending clock interrupt
 * which has not been serviced due to high
 * ipl, return error code.
 */
/*ARGSUSED*/
vmtime(otime, olbolt, oicr)
	register int otime, olbolt, oicr;
{

	return (((time.tv_sec-otime)*60 + lbolt-olbolt)*16667);
}
#endif

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	unsigned code;
{
	register struct proc *p = curproc;
	register int *regs;
	register struct sigframe *fp;
	struct sigacts *psp = p->p_sigacts;
	int oonstack, frmtrap;

	regs = p->p_md.md_regs;
	oonstack = psp->ps_sigstk.ss_flags & SA_ONSTACK;
	frmtrap = curpcb->pcb_flags & FM_TRAP;

	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	if ((psp->ps_flags & SAS_ALTSTACK)
			&& (psp->ps_sigstk.ss_flags & SA_ONSTACK) == 0
			&& (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe*) (psp->ps_sigstk.ss_base + psp->ps_sigstk.ss_size
				- sizeof(struct sigframe));
		psp->ps_sigstk.ss_flags |= SA_ONSTACK;
	} else {
		if (frmtrap)
			fp = (struct sigframe*) (regs[tESP] - sizeof(struct sigframe));
		else
			fp = (struct sigframe*) (regs[sESP] - sizeof(struct sigframe));
	}

	if ((unsigned) fp <= USRSTACK - ctob(p->p_vmspace->vm_ssize))
		(void) grow(p, (unsigned) fp);

	if (useracc((caddr_t) fp, sizeof(struct sigframe), B_WRITE) == 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		SIGACTION(p, SIGILL) = SIG_DFL;
		sig = sigmask(SIGILL);
		p->p_sigignore &= ~sig;
		p->p_sigcatch &= ~sig;
		p->p_sigmask &= ~sig;
		psignal(p, SIGILL);
		return;
	}

	/*
	 * Build the argument list for the signal handler.
	 */
	fp->sf_signum = sig;
	fp->sf_code = code;
	fp->sf_scp = &fp->sf_sc;
	fp->sf_handler = catcher;
	/* save scratch registers */
	if (frmtrap) {
		fp->sf_eax = regs[tEAX];
		fp->sf_edx = regs[tEDX];
		fp->sf_ecx = regs[tECX];
	} else {
		fp->sf_eax = regs[sEAX];
		fp->sf_edx = regs[sEDX];
		fp->sf_ecx = regs[sECX];
	}
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	fp->sf_sc.sc_onstack = oonstack;
	fp->sf_sc.sc_mask = mask;
	if (frmtrap) {
		fp->sf_sc.sc_sp = regs[tESP];
		fp->sf_sc.sc_fp = regs[tEBP];
		fp->sf_sc.sc_pc = regs[tEIP];
		fp->sf_sc.sc_ps = regs[tEFLAGS];
		regs[tESP] = (int) fp;
		regs[tEIP] = (int) ((struct pcb*) kstack)->pcb_sigc;
	} else {
		fp->sf_sc.sc_sp = regs[sESP];
		fp->sf_sc.sc_fp = regs[sEBP];
		fp->sf_sc.sc_pc = regs[sEIP];
		fp->sf_sc.sc_ps = regs[sEFLAGS];
		regs[sESP] = (int) fp;
		regs[sEIP] = (int) ((struct pcb*) kstack)->pcb_sigc;
	}
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper priviledges or to cause
 * a machine fault.
 */
struct sigreturn_args {
	struct sigcontext *sigcntxp;
};
sigreturn(p, uap, retval)
	struct proc *p;
	struct sigreturn_args *uap;
	int *retval;
{
	register struct sigcontext *scp;
	register struct sigframe *fp;
	register int *regs = p->p_md.md_regs;

	fp = (struct sigframe*) regs[sESP];

	if (useracc((caddr_t) fp, sizeof(*fp), 0) == 0)
		return (EINVAL);

	/* restore scratch registers */
	regs[sEAX] = fp->sf_eax;
	regs[sEDX] = fp->sf_edx;
	regs[sECX] = fp->sf_ecx;

	scp = fp->sf_scp;
	if (useracc((caddr_t) scp, sizeof(*scp), 0) == 0)
		return (EINVAL);
#ifdef notyet
		if ((scp->sc_ps & PSL_MBZ) != 0 || (scp->sc_ps & PSL_MBO) != PSL_MBO) {
			return(EINVAL);
		}
#endif
	if (scp->sc_onstack & 01)
		p->p_sigacts->ps_sigstk.ss_flags |= SA_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SA_ONSTACK;
	p->p_sigmask = scp->sc_mask
			& ~(sigmask(SIGKILL) | sigmask(SIGCONT) | sigmask(SIGSTOP));
	regs[sEBP] = scp->sc_fp;
	regs[sESP] = scp->sc_sp;
	regs[sEIP] = scp->sc_pc;
	regs[sEFLAGS] = scp->sc_ps;
	return (EJUSTRETURN);
}

/*
 * Clear registers on exec
 */
void
setregs(p, elp, stack)
	struct proc *p;
	struct exec_linker *elp;
	u_long stack;
{
	struct pmap *pmap = vm_map_pmap(&p->p_vmspace->vm_map);
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct trapframe *tf;
#if NNPX > 0
	npxinit(0x262);
#endif

	p->p_md.md_flags &= ~MDL_USEDFPU;

	tf = p->p_md.md_regs;
	tf->tf_gs = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_fs = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_es = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_ds = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_edi = 0;
	tf->tf_esi = 0;
	tf->tf_ebp = 0;
	tf->tf_ebx = (int)p->p_psstrp;
	tf->tf_edx = 0;
	tf->tf_ecx = 0;
	tf->tf_eax = 0;
	tf->tf_eip = elp->el_entry;
	tf->tf_cs = 0;//pmap->pm_hiexec > I386_MAX_EXE_ADDR ? LSEL(LUCODEBIG_SEL, SEL_UPL) : LSEL(LUCODE_SEL, SEL_UPL);
	tf->tf_eflags = PSL_USERSET;
	tf->tf_esp = stack;
	tf->tf_ss = LSEL(LUDATA_SEL, SEL_UPL);
}

void
cpu_setregs(void)
{
	unsigned int cr0;
	cr0 = rcr0();
	cr0 |= CR0_MP | CR0_NE | CR0_TS | CR0_WP | CR0_AM;
	load_cr0(cr0);
	load_gs(_udatasel);
}

/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR); /* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &cn_tty->t_dev,
				sizeof cn_tty->t_dev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Initialize segments & interrupt table
 */

void
setidt(idx, func, typ, dpl, selec)
	int idx, typ, dpl, selec;
	void *func;
{
	struct gate_descriptor *ip;

	ip = idt + idx;
	ip->gd_looffset = (u_int)func;
	ip->gd_selector = selec;
	ip->gd_stkcpy = 0;
	ip->gd_xx = 0;
	ip->gd_type = typ;
	ip->gd_dpl = dpl;
	ip->gd_p = 1;
	ip->gd_hioffset = ((u_int)func) >> 16 ;
}

void
setirq(idx, func)
	int idx;
	void *func;
{
	setidt(idx, func, SDT_SYS386IGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
}

#define	IDTVEC(name)	__CONCAT(X, name)
extern 	IDTVEC(div), IDTVEC(dbg), IDTVEC(nmi), IDTVEC(bpt), IDTVEC(ofl),
		IDTVEC(bnd), IDTVEC(ill), IDTVEC(dna), IDTVEC(dble), IDTVEC(fpusegm),
		IDTVEC(tss), IDTVEC(missing), IDTVEC(stk), IDTVEC(prot),
		IDTVEC(page), IDTVEC(rsvd), IDTVEC(fpu), IDTVEC(rsvd0),
		IDTVEC(rsvd1), IDTVEC(rsvd2), IDTVEC(rsvd3), IDTVEC(rsvd4),
		IDTVEC(rsvd5), IDTVEC(rsvd6), IDTVEC(rsvd7), IDTVEC(rsvd8),
		IDTVEC(rsvd9), IDTVEC(rsvd10), IDTVEC(rsvd11), IDTVEC(rsvd12),
		IDTVEC(rsvd13), IDTVEC(rsvd14), IDTVEC(rsvd14), IDTVEC(syscall);

void
sdtossd(sd, ssd)
	struct segment_descriptor *sd;
	struct soft_segment_descriptor *ssd;
{
	ssd->ssd_base  = (sd->sd_hibase << 24) | sd->sd_lobase;
	ssd->ssd_limit = (sd->sd_hilimit << 16) | sd->sd_lolimit;
	ssd->ssd_type  = sd->sd_type;
	ssd->ssd_dpl   = sd->sd_dpl;
	ssd->ssd_p     = sd->sd_p;
	ssd->ssd_def32 = sd->sd_def32;
	ssd->ssd_gran  = sd->sd_gran;
}

void
ssdtosd(ssd, sd)
	struct soft_segment_descriptor *ssd;
	struct segment_descriptor *sd;
{
	sd->sd_lobase = (ssd->ssd_base) & 0xffffff;
	sd->sd_hibase = (ssd->ssd_base >> 24) & 0xff;
	sd->sd_lolimit = (ssd->ssd_limit) & 0xffff;
	sd->sd_hilimit = (ssd->ssd_limit >> 16) & 0xf;
	sd->sd_type = ssd->ssd_type;
	sd->sd_dpl = ssd->ssd_dpl;
	sd->sd_p = ssd->ssd_p;
	sd->sd_def32 = ssd->ssd_def32;
	sd->sd_gran = ssd->ssd_gran;
}

void
init386(first)
	int first;
{
	int x, *pi;
	unsigned biosbasemem, biosextmem;
	struct gate_descriptor *gdp;
	extern int sigcode, szsigcode;
	struct region_descriptor r_gdt, r_idt;

	proc0.p_addr = proc0paddr;

	allocate_gdt(&gdt_segs);
	allocate_ldt(&ldt_segs);

	/*
	 * Initialize the console before we print anything out.
	 */

	cninit (KERNBASE+0xa0000);

	/* make gdt memory segments */
	gdt_segs[GCODE_SEL].ssd_limit = btoc((int) &etext + NBPG);
	for (x=0; x < NGDT; x++) ssdtosd(gdt_segs+x, gdt+x);
	/* make ldt memory segments */
	ldt_segs[LUCODE_SEL].ssd_limit = btoc(UPT_MIN_ADDRESS);
	ldt_segs[LUDATA_SEL].ssd_limit = btoc(UPT_MIN_ADDRESS);
	/* Note. eventually want private ldts per process */
	for (x=0; x < 5; x++) ssdtosd(ldt_segs+x, ldt+x);

	/* exceptions */
	setidt(0, &IDTVEC(div),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(1, &IDTVEC(dbg),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(2, &IDTVEC(nmi),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(3, &IDTVEC(bpt),  SDT_SYS386TGT, SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(4, &IDTVEC(ofl),  SDT_SYS386TGT, SEL_UPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(5, &IDTVEC(bnd),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(6, &IDTVEC(ill),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(7, &IDTVEC(dna),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(8, &IDTVEC(dble),  SDT_SYSTASKGT, SEL_KPL, GSEL(GDBLFLT_SEL, SEL_KPL));
	setidt(9, &IDTVEC(fpusegm),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(10, &IDTVEC(tss),  SDT_SYSTASKGT, SEL_KPL, GSEL(GINVTSS_SEL, SEL_KPL));
	setidt(11, &IDTVEC(missing),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(12, &IDTVEC(stk),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(13, &IDTVEC(prot),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(14, &IDTVEC(page),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(15, &IDTVEC(rsvd),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(16, &IDTVEC(fpu),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(17, &IDTVEC(rsvd0),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(18, &IDTVEC(rsvd1),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(19, &IDTVEC(rsvd2),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(20, &IDTVEC(rsvd3),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(21, &IDTVEC(rsvd4),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(22, &IDTVEC(rsvd5),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(23, &IDTVEC(rsvd6),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(24, &IDTVEC(rsvd7),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(25, &IDTVEC(rsvd8),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(26, &IDTVEC(rsvd9),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(27, &IDTVEC(rsvd10),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(28, &IDTVEC(rsvd11),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(29, &IDTVEC(rsvd12),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(30, &IDTVEC(rsvd13),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	setidt(31, &IDTVEC(rsvd14),  SDT_SYS386TGT, SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));

	r_gdt.rd_limit = NGDT * sizeof(union descriptor ) - 1;
	r_gdt.rd_base = (int) gdt;
	lgdt(r_gdt);

	r_idt.rd_limit = sizeof(idt) - 1;
	r_idt.rd_base = (int) idt;
	lidt(r_idt);
	lldt(GSEL(GLDT_SEL, SEL_KPL));

	/*
	 * This memory size stuff is a real mess.  Here is a simple
	 * setup that just believes the BIOS.  After the rest of
	 * the system is a little more stable, we'll come back to
	 * this and deal with issues if incorrect BIOS information,
	 * and when physical memory is > 16 megabytes.
	 */
	biosbasemem = rtcin(RTC_BASELO) + (rtcin(RTC_BASEHI) << 8);
	biosextmem = rtcin(RTC_EXTLO) + (rtcin(RTC_EXTHI) << 8);
	Maxmem = btoc((biosextmem + 1024) * 1024);
	maxmem = Maxmem - 1;
	physmem = btoc(biosbasemem * 1024 + (biosextmem - 1) * 1024);
	printf("bios %dK+%dK. maxmem %x, physmem %x\n", biosbasemem, biosextmem,
			ctob(maxmem), ctob(physmem));

	vm_set_page_size();
	/* call pmap initialization to make new kernel address space */
	pmap_bootstrap(first, 0);
	/* now running on new page tables, configured,and u/iom is accessible */

	/* make a initial tss so microp can get interrupt stack on syscall! */
	proc0.p_addr->u_pcb.pcb_tss.tss_esp0 = (int) kstack + UPAGES * NBPG;
	proc0.p_addr->u_pcb.pcb_tss.tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	proc0.p_addr->u_pcb.pcb_tss.tss_esp = (int) kstack + UPAGES * NBPG;
	proc0.p_addr->u_pcb.pcb_tss.tss_ss = GSEL(GDATA_SEL, SEL_KPL) ;
	_gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	ltr(_gsel_tss);

	proc0.p_addr->u_pcb.pcb_tss.tss_ldt =  GSEL(GLDT_SEL, SEL_KPL);
	proc0.p_addr->u_pcb.pcb_tss.tss_cs =  GSEL(GCODE_SEL, SEL_KPL);
	proc0.p_addr->u_pcb.pcb_tss.tss_ds =  GSEL(GDATA_SEL, SEL_KPL);
	proc0.p_addr->u_pcb.pcb_tss.tss_es =  GSEL(GDATA_SEL, SEL_KPL);
	proc0.p_addr->u_pcb.pcb_tss.tss_ss =  GSEL(GDATA_SEL, SEL_KPL);
	proc0.p_addr->u_pcb.pcb_tss.tss_fs =  GSEL(GDATA_SEL, SEL_KPL);
	proc0.p_addr->u_pcb.pcb_tss.tss_gs =  GSEL(GDATA_SEL, SEL_KPL);


	/* make a call gate to reenter kernel with */
	gdp = &ldt[LSYS5CALLS_SEL].gd;
	x = (int) &IDTVEC(syscall);
	gdp->gd_looffset = x++;
	gdp->gd_selector = GSEL(GCODE_SEL, SEL_KPL);
	gdp->gd_stkcpy = 0;
	gdp->gd_type = SDT_SYS386CGT;
	gdp->gd_dpl = SEL_UPL;
	gdp->gd_p = 1;
	gdp->gd_hioffset = ((int) &IDTVEC(syscall)) >>16;

	/* transfer to user mode */
	_ucodesel = LSEL(LUCODE_SEL, SEL_UPL);
	_udatasel = LSEL(LUDATA_SEL, SEL_UPL);

	/* setup proc 0's pcb */
	bcopy(&sigcode, proc0.p_addr->u_pcb.pcb_sigc, szsigcode);
	proc0.p_addr->u_pcb.pcb_flags = 0;
	proc0.p_addr->u_pcb.pcb_ptd = IdlePTD;

	/* setup fault tss's */
	inv_tss = proc0.p_addr->u_pcb.pcb_tss;
	inv_tss.tss_esp0 = (int) (alt_stack + 1020);
	inv_tss.tss_esp = (int) (alt_stack + 1020);
	inv_tss.tss_ss =  GSEL(GDATA_SEL, SEL_KPL);
	inv_tss.tss_cs =  GSEL(GCODE_SEL, SEL_KPL);
	inv_tss.tss_eip =  (int) &invtss;
	exit_tss = dbl_tss = inv_tss;
	dbl_tss.tss_eip =  (int) &dbl;
	_exit_tss =  GSEL(GEXIT_SEL, SEL_KPL);
}

/*
 * Construct a PCB from a trapframe. This is called from kdb_trap() where
 * we want to start a backtrace from the function that caused us to enter
 * the debugger. We have the context in the trapframe, but base the trace
 * on the PCB. The PCB doesn't have to be perfect, as long as it contains
 * enough for a backtrace.
 */
void
makectx(tf, pcb)
	struct trapframe *tf;
	struct pcb *pcb;
{
	pcb->pcb_tss.tss_edi = tf->tf_edi;
	pcb->pcb_tss.tss_esi = tf->tf_esi;
	pcb->pcb_tss.tss_ebp = tf->tf_ebp;
	pcb->pcb_tss.tss_ebx = tf->tf_ebx;
	pcb->pcb_tss.tss_eip = tf->tf_eip;
	pcb->pcb_tss.tss_esp = (ISPL(tf->tf_cs)) ? tf->tf_esp : (int)(tf + 1) - 8;
	pcb->pcb_tss.tss_gs = rgs();
}


void
need_resched(struct cpu_info *ci)
{

}

extern struct pte	*CMAP1, *CMAP2;
extern caddr_t		CADDR1, CADDR2;

/*
 * zero out physical memory
 * specified in relocation units (NBPG bytes)
 */
clearseg(n)
{
	*(int*) CMAP2 = PG_V | PG_KW | ctob(n);
	load_cr3(rcr3());
	bzero(CADDR2, NBPG);
	*(int*) CADDR2 = 0;
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
copyseg(frm, n)
{
	*(int*) CMAP2 = PG_V | PG_KW | ctob(n);
	load_cr3(rcr3());
	bcopy((void*) frm, (void*) CADDR2, NBPG);
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
physcopyseg(frm, to)
{
	*(int*) CMAP1 = PG_V | PG_KW | ctob(frm);
	*(int*) CMAP2 = PG_V | PG_KW | ctob(to);
	load_cr3(rcr3());
	bcopy(CADDR1, CADDR2, NBPG);
}

/*aston() {
	schednetisr(NETISR_AST);
}*/

setsoftclock() {
	//schednetisr(NETISR_SCLK);
}

/*
 * insert an element into a queue
 */
#undef insque
_insque(element, head)
	register struct prochd *element, *head;
{
	element->ph_link = head->ph_link;
	head->ph_link = (struct proc*) element;
	element->ph_rlink = (struct proc*) head;
	((struct prochd*) (element->ph_link))->ph_rlink = (struct proc*) element;
}

/*
 * remove an element from a queue
 */
#undef remque
_remque(element)
	register struct prochd *element;
{
	((struct prochd*) (element->ph_link))->ph_rlink = element->ph_rlink;
	((struct prochd*) (element->ph_rlink))->ph_link = element->ph_link;
	element->ph_rlink = (struct proc*) 0;
}

vmunaccess() {}

/*
 * Below written in C to allow access to debugging code
 */
copyinstr(fromaddr, toaddr, maxlength, lencopied)
	u_int *lencopied, maxlength;
	void *toaddr, *fromaddr;
{
	int c, tally;

	tally = 0;
	while (maxlength--) {
		c = fubyte(fromaddr++);
		if (c == -1) {
			if (lencopied)
				*lencopied = tally;
			return (EFAULT);
		}
		tally++;
		*(char*) toaddr++ = (char) c;
		if (c == 0) {
			if (lencopied)
				*lencopied = tally;
			return (0);
		}
	}
	if (lencopied)
		*lencopied = tally;
	return (ENAMETOOLONG);
}

copyoutstr(fromaddr, toaddr, maxlength, lencopied)
	u_int *lencopied, maxlength;
	void *fromaddr, *toaddr;
{
	int c;
	int tally;

	tally = 0;
	while (maxlength--) {
		c = subyte(toaddr++, *(char*) fromaddr);
		if (c == -1)
			return (EFAULT);
		tally++;
		if (*(char*) fromaddr++ == 0) {
			if (lencopied)
				*lencopied = tally;
			return (0);
		}
	}
	if (lencopied)
		*lencopied = tally;
	return (ENAMETOOLONG);
}

copystr(fromaddr, toaddr, maxlength, lencopied)
	u_int *lencopied, maxlength;
	void *fromaddr, *toaddr;
{
	u_int tally;

	tally = 0;
	while (maxlength--) {
		*(u_char*) toaddr = *(u_char*) fromaddr++;
		tally++;
		if (*(u_char*) toaddr++ == 0) {
			if (lencopied)
				*lencopied = tally;
			return (0);
		}
	}
	if (lencopied)
		*lencopied = tally;
	return (ENAMETOOLONG);
}
