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
#include <sys/bootinfo.h>

#include <net/netisr.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/gdt.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/bootinfo.h>

#include <dev/cons.h>
#include <dev/isa/isareg.h>
#include <dev/isa/rtc.h>

#include <dev/ic/i8042reg.h>
#include <i386/isa/isa_machdep.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#ifdef VM86
#include <machine/vm86.h>
#endif

#include "apm.h"
#include "bioscall.h"

#if NBIOSCALL > 0
#include <machine/bioscall.h>
#endif

#if NAPM > 0
#include <machine/apmvar.h>
#endif

#include "isa.h"
#include "isadma.h"
#include "npx.h"
#if NNPX > 0
extern struct proc *npxproc;
#endif

/* the following is used externally (sysctl_hw) */
char machine[] = "i386";			/* cpu "architecture" */
char machine_arch[] = "i386";		/* machine == machine_arch */

int want_resched;					/* resched() was called */

char bootinfo[BOOTINFO_MAXSIZE];

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

vm_map_t buffer_map;
extern vm_offset_t avail_end;

/*
 * Machine-dependent startup code
 */
int boothowto = 0, Maxmem = 0;
long dumplo;
int physmem, maxmem;
int biosmem;

extern int kstack[];

struct pcb *curpcb;			/* our current running pcb */

int	i386_use_fxsave;

union descriptor gdt[NGDT];
struct gate_descriptor idt[32+16];
union descriptor ldt[NLDT];

struct soft_segment_descriptor gdt_segs[];
struct soft_segment_descriptor ldt_segs[];

int _udatasel, _ucodesel, _gsel_tss;

extern struct user *proc0paddr;

void
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
		pmap_enter(kernel_pmap, msgbufp, avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

#ifdef KDB
	kdb_init();			/* startup kernel debugger */
#endif
	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
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

	i386_bus_space_mallocok();
}

/*
 * Set up proc0's TSS and LDT.
 */
void
i386_proc0_tss_ldt_init()
{
	struct pcb *pcb;
	int x;

	curpcb = pcb = &proc0.p_addr->u_pcb;
	pcb->pcb_flags = 0;
	pcb->pcb_tss.tss_ioopt =
	    ((caddr_t)pcb->pcb_iomap - (caddr_t)&pcb->pcb_tss) << 16;
	for (x = 0; x < sizeof(pcb->pcb_iomap) / 4; x++)
		pcb->pcb_iomap[x] = 0xffffffff;

	pcb->pcb_ldt_sel = pmap_kernel()->pm_ldt_sel = GSEL(GLDT_SEL, SEL_KPL);
	pcb->pcb_cr0 = rcr0();
	pcb->pcb_tss.tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	pcb->pcb_tss.tss_esp0 = (int)proc0.p_addr + USPACE - 16;

	ltr(proc0.p_md.md_tss_sel);
	lldt(pcb->pcb_ldt_sel);

	proc0.p_md.md_regs = (struct trapframe *)pcb->pcb_tss.tss_esp0 - 1;
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
	u_long  code;
{
	register struct proc *p = curproc;
	struct trapframe *tf;
	register struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int oonstack;
	extern char sigcode[], esigcode[];

	/*
	 * Build the argument list for the signal handler.
	 */
	frame.sf_signum = sig;

	tf = p->p_md.md_regs;
	oonstack = psp->ps_sigstk.ss_flags & SA_ONSTACK;

	/*
	 * Allocate space for the signal handler context.
	 */
	if ((psp->ps_flags & SAS_ALTSTACK) && !oonstack &&
	    (psp->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(psp->ps_sigstk2.ss_sp +
		    psp->ps_sigstk.ss_size - sizeof(struct sigframe));
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
	} else {
		fp = (struct sigframe *)tf->tf_esp - 1;
	}

	fp->sf_signum = sig;
	fp->sf_code = code;
	fp->sf_scp = &fp->sf_sc;
	fp->sf_handler = catcher;

	/*
	 * Build the signal context to be used by sigreturn.
	 */
#ifdef VM86
	if (tf->tf_eflags & PSL_VM) {
		frame.sf_sc.sc_gs = tf->tf_vm86_gs;
		frame.sf_sc.sc_fs = tf->tf_vm86_fs;
		frame.sf_sc.sc_es = tf->tf_vm86_es;
		frame.sf_sc.sc_ds = tf->tf_vm86_ds;
		frame.sf_sc.sc_eflags = get_vflags(p);
	} else
#endif
	{
		frame.sf_sc.sc_gs = tf->tf_gs;
		frame.sf_sc.sc_fs = tf->tf_fs;
		frame.sf_sc.sc_es = tf->tf_es;
		frame.sf_sc.sc_ds = tf->tf_ds;
		frame.sf_sc.sc_eflags = tf->tf_eflags;
	}

	frame.sf_sc.sc_edi = tf->tf_edi;
	frame.sf_sc.sc_esi = tf->tf_esi;
	frame.sf_sc.sc_ebp = tf->tf_ebp;
	frame.sf_sc.sc_ebx = tf->tf_ebx;
	frame.sf_sc.sc_edx = tf->tf_edx;
	frame.sf_sc.sc_ecx = tf->tf_ecx;
	frame.sf_sc.sc_eax = tf->tf_eax;
	frame.sf_sc.sc_eip = tf->tf_eip;
	frame.sf_sc.sc_cs = tf->tf_cs;
	frame.sf_sc.sc_esp = tf->tf_esp;
	frame.sf_sc.sc_ss = tf->tf_ss;
	frame.sf_sc.sc_trapno = tf->tf_trapno;
	frame.sf_sc.sc_err = tf->tf_err;

	fp->sf_sc.sc_onstack = oonstack;
	fp->sf_sc.sc_mask = mask;

	if (copyout(&frame, fp, sizeof(frame)) != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(p, SIGILL);
		/* NOTREACHED */
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_gs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_eip = (int)(((char *)PS_STRINGS) - (esigcode - sigcode));
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_eflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_esp = (int)fp;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);

	/* Remember that we're now on the signal stack. */
	if (oonstack)
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
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
	register struct sigcontext *scp, context;
	register struct trapframe *tf;

	tf = p->p_md.md_regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);


#ifdef VM86
	if (context.sc_eflags & PSL_VM) {
		tf->tf_vm86_gs = context.sc_gs;
		tf->tf_vm86_fs = context.sc_fs;
		tf->tf_vm86_es = context.sc_es;
		tf->tf_vm86_ds = context.sc_ds;
		set_vflags(p, context.sc_eflags);
	} else
#endif
	{
		/*
		 * Check for security violations.  If we're returning to
		 * protected mode, the CPU will validate the segment registers
		 * automatically and generate a trap on violations.  We handle
		 * the trap, rather than doing all of the checking here.
		 */
		if (((context.sc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0
				|| !USERMODE(context.sc_cs, context.sc_eflags))
			return (EINVAL);

		/* %fs and %gs were restored by the trampoline. */
		tf->tf_es = context.sc_es;
		tf->tf_ds = context.sc_ds;
		tf->tf_eflags = context.sc_eflags;
	}
	tf->tf_edi = context.sc_edi;
	tf->tf_esi = context.sc_esi;
	tf->tf_ebp = context.sc_ebp;
	tf->tf_ebx = context.sc_ebx;
	tf->tf_edx = context.sc_edx;
	tf->tf_ecx = context.sc_ecx;
	tf->tf_eax = context.sc_eax;
	tf->tf_eip = context.sc_eip;
	tf->tf_cs = context.sc_cs;
	tf->tf_esp = context.sc_esp;
	tf->tf_ss = context.sc_ss;

	if (context.sc_onstack & 01)
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	else
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	p->p_sigmask = context.sc_mask & ~sigcantmask;

	return (EJUSTRETURN);
}

int	waittime = -1;

void
boot(arghowto)
	int arghowto;
{
	register long dummy; 	/* r12 is reserved */
	register int howto; 	/* r11 == how to boot */
	register int devtype; 	/* r10 == major of root dev */
	extern char *panicstr;
	extern int cold;

	howto = arghowto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		register struct buf *bp;
		int iter, nbusy;

		waittime = 0;
		(void) splnet();
		printf("syncing disks... ");
		/*
		 * Release inodes held by texts before update.
		 */
		if (panicstr == 0)
			vnode_pager_umount(NULL);
		sync((struct sigcontext*) 0);
		/*
		 * Unmount filesystems
		 */
		if (panicstr == 0)
			vfs_unmountall();

		for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf;)
				if ((bp->b_flags & (B_BUSY | B_INVAL)) == B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf("%d ", nbusy);
			DELAY(40000 * iter);
		}
		if (nbusy)
			printf("giving up\n");
		else
			printf("done\n");
		DELAY(10000); /* wait for printf to finish */
	}
	splhigh();
	devtype = major(rootdev);
	if (howto & RB_HALT) {
		printf("halting (in tight loop); hit reset\n\n");
		splx(0xfffd); /* all but keyboard XXX */
		for (;;)
			;
	} else {
		if (howto & RB_DUMP) {
			dumpsys();
			/*NOTREACHED*/
		}
	}
#ifdef lint
	dummy = 0; dummy = dummy;
	printf("howto %d, devtype %d\n", arghowto, devtype);
#endif
#ifdef	notdef
	pg("pausing (hit any key to reset)");
#endif
	reset_cpu();
	for (;;)
		;
	/*NOTREACHED*/
}

u_long	dumpmag = 	0x8fca0101;	/* magic number for savecore */
int		dumpsize = 	0;			/* also for savecore */
long	dumplo = 0; 			/* blocks */

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
void
dumpsys()
{

	if (dumpdev == NODEV)
		return;
	if ((minor(dumpdev) & 07) != 1)
		return;
	dumpsize = physmem;
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);
	printf("dump ");
	switch ((*bdevsw[major(dumpdev)].d_dump)(dumpdev)) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	default:
		printf("succeeded\n");
		break;
	}
	printf("\n\n");
	DELAY(1000);
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

void
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

void
initcpu()
{

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
	if (i386_use_fxsave) {
		pcb->pcb_savefpu.sv_fx.fxv_env.fx_cw = ___NPX87___;
		pcb->pcb_savefpu.sv_fx.fxv_env.fx_mxcsr = __MXCSR__;
	} else {
		pcb->pcb_savefpu.sv_87.sv_env.en_cw = ___NPX87___;
	}
	tf = p->p_md.md_regs;
	tf->tf_gs = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_fs = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_es = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_ds = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_edi = 0;
	tf->tf_esi = 0;
	tf->tf_ebp = 0;
	tf->tf_ebx = (int)PS_STRINGS;
	tf->tf_edx = 0;
	tf->tf_ecx = 0;
	tf->tf_eax = 0;
	tf->tf_eip = elp->el_entry;
	tf->tf_cs = 0;
	tf->tf_eflags = PSL_USERSET;
	tf->tf_esp = stack;
	tf->tf_ss = LSEL(LUDATA_SEL, SEL_UPL);
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
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR); /* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev, sizeof consdev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Initialize 386 and configure to run kernel
 */

/*
 * Initialize segments & interrupt table
 */
void
setidt(idx, func, args, typ, dpl)
	int idx, args, typ, dpl;
	void *func;
{
	struct gate_descriptor *ip;

	ip = &idt[idx];
	ip->gd_looffset = (u_int)func;
	ip->gd_selector = GSEL(GCODE_SEL, SEL_KPL);
	ip->gd_stkcpy = args;
	ip->gd_xx = 0;
	ip->gd_type = typ;
	ip->gd_dpl = dpl;
	ip->gd_p = 1;
	ip->gd_hioffset = ((u_int)func) >> 16 ;
}

void
init_descriptors()
{
	allocate_gdt(&gdt_segs);
	allocate_ldt(&ldt_segs);
}

#define	IDTVEC(name)	__CONCAT(X, name)
extern 	IDTVEC(div), IDTVEC(dbg), IDTVEC(nmi), IDTVEC(bpt), IDTVEC(ofl),
		IDTVEC(bnd), IDTVEC(ill), IDTVEC(dna), IDTVEC(dble), IDTVEC(fpusegm),
		IDTVEC(tss), IDTVEC(missing), IDTVEC(stk), IDTVEC(prot), IDTVEC(page),
		IDTVEC(rsvd), IDTVEC(fpu), IDTVEC(align), IDTVEC(syscall), IDTVEC(osyscall);

void
init386(first)
	int first;
{
	int x;
	unsigned biosbasemem, biosextmem;
	struct gate_descriptor *gdp;
	extern int sigcode, szsigcode;

	proc0.p_addr = proc0paddr;
	curpcb = &proc0.p_addr->u_pcb;

	i386_bus_space_init();
	init_descriptors();

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

	/* make gdt memory segments */
	gdt_segs[GCODE_SEL].ssd_limit = btoc((int) &etext + NBPG);
	for (x=0; x < NGDT; x++)
		ssdtosd(gdt_segs+x, gdt+x);

	/* make ldt memory segments */
	ldt_segs[LUCODE_SEL].ssd_limit = btoc(UPT_MIN_ADDRESS);
	ldt_segs[LUDATA_SEL].ssd_limit = btoc(UPT_MIN_ADDRESS);
	/* Note. eventually want private ldts per process */
	for (x=0; x < 5; x++)
		ssdtosd(ldt_segs+x, ldt+x);

	/* exceptions */
	for(x = 0; x < NIDT; x++) {
		setidt(x, &IDTVEC(rsvd), SDT_SYS386TGT, SEL_KPL);
	}
	setidt(0, &IDTVEC(div), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(1, &IDTVEC(dbg), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(2, &IDTVEC(nmi), 0, SDT_SYS386TGT, SEL_KPL);
 	setidt(3, &IDTVEC(bpt), 0, SDT_SYS386TGT, SEL_UPL);
	setidt(4, &IDTVEC(ofl), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(5, &IDTVEC(bnd), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(6, &IDTVEC(ill), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(7, &IDTVEC(dna), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(8, &IDTVEC(dble), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(9, &IDTVEC(fpusegm), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(10, &IDTVEC(tss), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(11, &IDTVEC(missing), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(12, &IDTVEC(stk), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(13, &IDTVEC(prot), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(14, &IDTVEC(page), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(16, &IDTVEC(fpu), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(17, &IDTVEC(align), 0, SDT_SYS386TGT, SEL_KPL);
	setidt(128, &IDTVEC(syscall), 0, SDT_SYS386TGT, SEL_UPL);

	lgdt(gdt, sizeof(gdt)-1);
	lidt(idt, sizeof(idt)-1);

#if	NISA > 0
	isa_defaultirq();
#endif

	splhigh();
	enable_intr();

	i386_bus_space_check(avail_end, biosbasemem, biosextmem);

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
	_gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	ltr(_gsel_tss);

	/* make a call gate to reenter kernel with */
	setidt(LSYS5CALLS_SEL, &IDTVEC(osyscall), 1, SDT_SYS386CGT, SEL_UPL);

	/* transfer to user mode */
	_ucodesel = LSEL(LUCODE_SEL, SEL_UPL);
	_udatasel = LSEL(LUDATA_SEL, SEL_UPL);

	/* setup proc 0's pcb */
	bcopy(&sigcode, proc0.p_addr->u_pcb.pcb_sigc, szsigcode);
	proc0.p_addr->u_pcb.pcb_flags = 0;
	proc0.p_addr->u_pcb.pcb_ptd = IdlePTD;
}

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

void *
lookup_bootinfo(type)
	int type;
{
	union bootinfo *help;
	int n = *(int*)bootinfo;
	help = (union bootinfo *)(bootinfo + sizeof(int));
	while(n--) {
		if(help->bi_type == type)
			return(help);
		help = (union bootinfo *)((char*)help + help->bi_len);
	}
	return(0);
}

/*
 * Provide inb() and outb() as functions.  They are normally only available as
 * inline functions, thus cannot be called from the debugger.
 */

/* silence compiler warnings */
u_char inb_(u_short);
void outb_(u_short, u_char);

u_char
inb_(u_short port)
{
	return inb(port);
}

void
outb_(u_short port, u_char data)
{
	outb(port, data);
}
