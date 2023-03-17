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
#include <sys/sysdecl.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/extent.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/exec_linker.h>
#include <sys/kenv.h>
#include <sys/ksyms.h>
#include <sys/power.h>
#include <sys/ucontext.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

#include <net/netisr.h>

#include <dev/core/ic/i8042reg.h>
#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>
#include <dev/misc/cons/cons.h>
#include <dev/core/isa/rtc.h>
#include <dev/core/ic/mc146818reg.h>

#include <machine/bus.h>
#include <machine/bios.h>
#include <machine/bootinfo.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/cpuvar.h>
#include <machine/gdt.h>
#include <machine/intr.h>
#include <machine/pic.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/vm86.h>
#include <machine/npx.h>
#include <machine/proc.h>
#include <machine/pmap.h>

#include <machine/apic/lapicvar.h>
#include <machine/isa/isa_machdep.h>

#include <i386/isa/nvram.h>

#ifdef SMP
#include <machine/smp.h>
#endif

/* the following is used externally (sysctl_hw) */
char machine[] = "i386";			/* cpu "architecture" */
char machine_arch[] = "i386";		/* machine == machine_arch */

void i386_microtime(struct timeval *);
void cpu_dumpconf(void);
int	 cpu_dump(void);
void dumpsys(void);
void init386_bootinfo(struct bootinfo *);
int	bootinfo_check(struct bootinfo *);
void cpu_reset(void);
void cpu_halt(void);
void identify_cpu(void);
caddr_t allocsys(caddr_t);

void (*delay_func)(int) = i8254_delay;
void (*microtime_func)(struct timeval *) = i386_microtime;
void (*initclocks_func)(void) = i8254_initclocks;

/*
 * Declare these as initialized data so we can patch them.
 */
/*
extern int	nswbuf = 0;
#ifdef	NBUF
extern int	nbuf = NBUF;
#else
extern int	nbuf = 0;
#endif
*/
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif
int	msgbufmapped;		/* set when safe to use msgbuf */

vm_map_t buffer_map;
vm_map_t exec_map;
vm_map_t mb_map;
vm_map_t phys_map;
extern vm_offset_t avail_end;

/*
 * Machine-dependent startup code
 */
long Maxmem = 0;
long dumplo;
int physmem, maxmem;
int biosmem;
struct bootinfo i386boot;
char bootsize[BOOTINFO_MAXSIZE];
extern int *esym;

extern int biosbasemem;
extern int biosextmem;

#ifdef CPURESET_DELAY
int cpureset_delay = CPURESET_DELAY;
#else
int cpureset_delay = 2000; /* default to 2s */
#endif

#define PHYSSEG_MAX 32
typedef struct {
	u_quad_t	start;		/* Physical start address	*/
	u_quad_t	size;		/* Size in bytes		*/
} phys_ram_seg_t;
phys_ram_seg_t mem_clusters[PHYSSEG_MAX];
int	mem_cluster_cnt = 0;

struct pcb *curpcb;			/* our current running pcb */

int	i386_fpu_present;
int	i386_fpu_exception;
int	i386_fpu_fdivbug;

int	i386_use_fxsave;

union descriptor 		gdt[NGDT];
struct gate_descriptor 	idt[NIDT];
union descriptor 		ldt[NLDT];

struct soft_segment_descriptor *gdt_segs;
struct soft_segment_descriptor *ldt_segs;

int _udatasel, _ucodesel, _gsel_tss;

struct user u;
struct user *proc0paddr;
vm_offset_t proc0kstack;

void
startup(void)
{
	int firstaddr, sz;
	register int unixsize;
	register unsigned i;
	int mapaddr, j;
	register caddr_t v;
	int maxbufs, base, residual;
	extern long Usrptsize;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;
	
	/* Initialize user */
	u = u;
	proc0paddr = &u;

	/*
	 * Initialize error message buffer (at end of core).
	 */

	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(sizeof(struct msgbuf)); i++) {
		pmap_enter(kernel_pmap, (vm_offset_t)msgbufp, avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	}
	msgbufmapped = 1;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */

	printf(version);
	printcpuinfo();
	panicifcpuunsupported();

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

again:
	v = (caddr_t)firstaddr;
	sz = (int)allocsys(v);

	/*
	 * End of first pass, size has been calculated so allocate memory
	 */
	if(firstaddr == 0) {
		size = (vm_size_t)(sz - (int)v);
		firstaddr = (int) kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0) {
			panic("startup: no room for tables");
		}
		goto again;
	}
	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t)(sz - (int)v) != size) {
		panic("startup: table size inconsistency");
	}

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers, &maxaddr, size, TRUE);
	minaddr = (vm_offset_t) buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t) 0, &minaddr, size, FALSE) != KERN_SUCCESS) {
		panic("startup: cannot allocate buffers");
	}
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
	mclrefcnt = (char *) malloc(NMBCLUSTERS + CLBYTES / MCLBYTES, M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS + CLBYTES / MCLBYTES);

	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr, VM_MBUF_SIZE, FALSE);

	/*printf("avail mem = %d\n", ptoa(vm_page_free_count));*/
	printf("using %d buffers containing %d bytes of memory\n", nbuf, bufpages * CLBYTES);

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	configure();

	/* Safe for i/o port / memory space allocation to use malloc now. */
	i386_bus_space_mallocok();
}

/*
 * Allocate space for system data structures.  We are given
 * a starting virtual address and we return a final virtual
 * address; along the way we set each data structure pointer.
 *
 * We call allocsys() with 0 to find out how much space we want,
 * allocate that much and fill it with zeroes, and then call
 * allocsys() again with the correct base virtual address.
 */
caddr_t
allocsys(v)
	register caddr_t v;
{
#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))

	valloc(cfree, struct cblock, nclist);

	//valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif

	/*
	 * Determine how many buffers to allocate.
	 * Use 10% of memory for the first 2 Meg, 5% of the remaining
	 * memory. Insure a minimum of 16 buffers.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0) {
		if (physmem < (2 * 1024 * 1024)) {
			bufpages = physmem / 10 / CLSIZE;
		} else {
			bufpages = ((2 * 1024 * 1024 + physmem) / 20) / CLSIZE;
		}
	}
	if (nbuf == 0) {
		nbuf = bufpages / 2;
		if (nbuf < 16) {
			nbuf = 16;
		}
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) & ~1; /* force even */
		if (nswbuf > 256) {
			nswbuf = 256; /* sanity */
		}
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);

	return (v);
}

/*
 * Set up proc0's TSS and LDT.
 */
void
i386_proc0_tss_ldt_init(void)
{
	struct pcb *pcb;
	int x;
	unsigned int cr0;

	proc0paddr->u_kstack = proc0kstack;
	proc0paddr->u_kstack_pages = KSTACK_PAGES;
	proc0paddr->u_uspace = USPACE;

	proc0.p_addr = proc0paddr;
	curpcb = pcb = &proc0.p_addr->u_pcb;

	pcb->pcb_flags = 0;
	pcb->pcb_tss.tss_ioopt = ((caddr_t)pcb->pcb_iomap - (caddr_t)&pcb->pcb_tss) << 16;
	for (x = 0; x < sizeof(pcb->pcb_iomap) / 4; x++) {
		pcb->pcb_iomap[x] = 0xffffffff;
	}

	cr0 = rcr0();
#ifdef SMP
	cr0 |= CR0_NE;				/* Done by npxinit() */
#endif
	cr0 |= CR0_MP | CR0_TS;		/* Done at every execve() too. */
#ifndef I386_CPU
	cr0 |= CR0_WP | CR0_AM;
#endif
	pcb->pcb_cr0 = cr0;
	lcr0(pcb->pcb_cr0);

	/* make a initial tss so microp can get interrupt stack on syscall! */

	proc0.p_addr->u_pcb.pcb_tss.tss_esp0 = proc0paddr->u_kstack + proc0paddr->u_kstack_pages * proc0paddr->u_uspace - VM86_STACK_SPACE;
	proc0.p_addr->u_pcb.pcb_tss.tss_ss0 = GSEL(GDATA_SEL, SEL_KPL);
	_gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	ltr(_gsel_tss);

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
int
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
	struct proc *p = curproc;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp = p->p_sigacts;
	int oonstack;
	extern int szsigcode;
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
	if (tf->tf_eflags & PSL_VM) {
		struct trapframe_vm86 *tf = (struct trapframe_vm86 *) tf;
		struct vm86_kernel *vm86 =  &p->p_addr->u_pcb.pcb_vm86;

		frame.sf_sc.sc_gs = tf->tf_vm86_gs;
		frame.sf_sc.sc_fs = tf->tf_vm86_fs;
		frame.sf_sc.sc_es = tf->tf_vm86_es;
		frame.sf_sc.sc_ds = tf->tf_vm86_ds;
		if (vm86->vm86_has_vme == 0) {
			frame.sf_sc.sc_eflags = get_vm86flags(tf, vm86);
		}
		/*
		 * We should never have PSL_T set when returning from vm86
		 * mode.  It may be set here if we deliver a signal before
		 * getting to vm86 mode, so turn it off.
		 */
		tf->tf_eflags &= ~(PSL_VM | PSL_T | PSL_VIF | PSL_VIP);
	} else {
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
		SIGACTION(p, SIGILL) = SIG_DFL;
		sig = sigmask(SIGILL);
		p->p_sigignore &= ~sig;
		p->p_sigcatch &= ~sig;
		p->p_sigmask &= ~sig;
		psignal(p, SIGILL);
		return;
	}

	/*
	 * Build context to run handler in.
	 */
	tf->tf_gs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_eip = (int)(((char *)PS_STRINGS) - (szsigcode));
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_eflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_esp = (int)fp;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);

	/* Remember that we're now on the signal stack. */
	if (oonstack) {
		psp->ps_sigstk.ss_flags |= SS_ONSTACK;
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

int
sigreturn()
{
	register struct sigreturn_args {
		syscallarg(struct sigcontext *) sigcntxp;
	} *uap = (struct sigreturn_args *) u.u_ap;

	struct proc *p;
	struct sigcontext *scp, context;
	struct trapframe *tf;
	int eflags;

	p = u.u_procp;
	tf = p->p_md.md_regs;

	/*
	 * The trampoline code hands us the context.
	 * It is unsafe to keep track of it ourselves, in the event that a
	 * program jumps out of a signal handler.
	 */
	scp = SCARG(uap, sigcntxp);
	if (copyin((caddr_t)scp, &context, sizeof(*scp)) != 0)
		return (EFAULT);

	eflags = scp->sc_ps;
	if (context.sc_eflags & PSL_VM) {
		struct trapframe_vm86 *tf = (struct trapframe_vm86 *)tf;
		struct vm86_kernel *vm86;
		/*
		 * if pcb_ext == 0 or vm86_inited == 0, the user hasn't
		 * set up the vm86 area, and we can't enter vm86 mode.
		 */
		if(&p->p_addr->u_pcb == NULL) {
			return (EINVAL);
		}
		vm86 = &p->p_addr->u_pcb.pcb_vm86;
		if (vm86->vm86_inited == 0) {
			return (EINVAL);
		}
		/* go back to user mode if both flags are set */
		if ((context.sc_eflags & PSL_VIP) && (context.sc_eflags & PSL_VIF)) {
			trapsignal(p, SIGBUS, 0);
		}
		set_vm86flags(tf, vm86, context.sc_eflags);
		tf->tf_vm86_gs = context.sc_gs;
		tf->tf_vm86_fs = context.sc_fs;
		tf->tf_vm86_es = context.sc_es;
		tf->tf_vm86_ds = context.sc_ds;
	} else {
		/*
		 * Check for security violations.  If we're returning to
		 * protected mode, the CPU will validate the segment registers
		 * automatically and generate a trap on violations.  We handle
		 * the trap, rather than doing all of the checking here.
		 */
		if (((context.sc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC) != 0	|| !USERMODE(context.sc_cs, context.sc_eflags))
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
struct pcb dumppcb;

void
boot(arghowto)
	int arghowto;
{
	register int howto; 	/* r11 == how to boot */
	extern int cold;

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	howto = arghowto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
	}
	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP) {
		dumpsys();
	}

haltsys:
	doshutdownhooks();

#ifdef SMP
	i386_broadcast_ipi(I386_IPI_HALT);
#endif

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");

#ifdef BEEP_ONHALT
		{
			int c;
			for (c = BEEP_ONHALT_COUNT; c > 0; c--) {
				sysbeep(BEEP_ONHALT_PITCH, BEEP_ONHALT_PERIOD * hz / 1000);
				delay(BEEP_ONHALT_PERIOD * 1000);
				sysbeep(0, BEEP_ONHALT_PERIOD * hz / 1000);
				delay(BEEP_ONHALT_PERIOD * 1000);
			}
		}
#endif

		cnpollc(1); /* for proper keyboard command handling */
		if (cngetc() == 0) {
			cpu_halt();
		}
		cnpollc(0);
	}
	printf("rebooting...\n");
	if (cpureset_delay > 0) {
		delay(cpureset_delay * 1000);
	}
	cpu_reset();
	for(;;) ;
	/*NOTREACHED*/
}

u_long	dumpmag = 	0x8fca0101;	/* magic number for savecore */
int		dumpsize = 	0;			/* also for savecore */
long	dumplo = 	0; 			/* blocks */

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
int
cpu_dumpsize(void)
{
	int size;
	size = ALIGN(mem_cluster_cnt * sizeof(phys_ram_seg_t));
	if(roundup(size, dbtob(1)) != dbtob(1)) {
		return (-1);
	}
	return (1);
}

/*
 * cpu_dump_mempagecnt: calculate the size of RAM (in pages) to be dumped.
 */
u_long
cpu_dump_mempagecnt(void)
{
	u_long i, n;

	n = 0;
	for (i = 0; i < mem_cluster_cnt; i++) {
		n += atop(mem_clusters[i].size);
	}
	return (n);
}

/*
 * This is called by configure to set dumplo and dumpsize.
 * Dumps always skip the first block of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	const struct bdevsw *bdev;
	int nblks, dumpblks;	/* size of dump area */

	bdev = bdevsw_lookup(dumpdev);
	if (dumpdev == NODEV){
		dumpsize = 0;
		return;
	}
	if(bdev == NULL) {
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
		return;
	}
	nblks = (*bdev->d_psize)(dumpdev);
	if (nblks <= ctod(1)) {
		dumpsize = 0;
		return;
	}
	dumpblks = cpu_dumpsize();
	if (dumpblks < 0) {
		dumpsize = 0;
		return;
	}
	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1))) {
		dumpsize = 0;
		return;
	}

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = cpu_dump_mempagecnt();
	return;
}

/*
 * cpu_dump: dump the machine-dependent kernel core dump headers.
 */
int
cpu_dump(void)
{
	const struct bdevsw *bdev;
	phys_ram_seg_t *memsegp;
	int (*dump)(dev_t, daddr_t, caddr_t, size_t);
	long buf[dbtob(1) / sizeof (long)];
	int i;

	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		return (ENXIO);
	}
	dump = bdev->d_dump;

	memset(buf, 0, sizeof buf);
	memsegp = (phys_ram_seg_t *)&buf;

	for (i = 0; i < mem_cluster_cnt; i++) {
		memsegp[i].start = mem_clusters[i].start;
		memsegp[i].size = mem_clusters[i].size;
	}
	return (dump(dumpdev, dumplo, (caddr_t)buf, dbtob(1)));
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
static vm_offset_t dumpspace;

vm_offset_t
reserve_dumppages(vm_offset_t p)
{
	dumpspace = p;
	return (p + PAGE_SIZE);
}

void
dumpsys(void)
{
	u_long totalbytesleft, bytes, i, n, m, memseg;
	u_long maddr;
	int psize;
	daddr_t blkno;
	const struct bdevsw *bdev;
	int (*dump)(dev_t, daddr_t, caddr_t, size_t);
	int error;

	/* Save registers. */
	savectx(&dumppcb);

	if (dumpdev == NODEV) {
		return;
	}

	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL) {
		return;
	}
	if (dumpsize == 0) {
		cpu_dumpconf();
	}
	if(dumplo <= 0 || dumpsize == 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev), minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev), minor(dumpdev), dumplo);

	psize = (*bdev->d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	if ((error = cpu_dump()) != 0) {
		goto err;
	}

	totalbytesleft = ptoa(cpu_dump_mempagecnt());
	blkno = dumplo + cpu_dumpsize();

	for (memseg = 0; memseg < mem_cluster_cnt; memseg++) {
		maddr = mem_clusters[memseg].start;
		bytes = mem_clusters[memseg].size;
		for (i = 0; i < bytes; i += n, totalbytesleft -= n) {
			/* Print out how many MBs we have left to go. */
			if ((totalbytesleft % (1024*1024)) == 0) {
				printf("%ld ", totalbytesleft / (1024 * 1024));
			}
			/* Limit size for next transfer. */
			n = bytes - i;
			pmap_enter(kernel_pmap, dumpspace, maddr, VM_PROT_READ, TRUE);
			error = (*dump)(dumpdev, blkno, (caddr_t)dumpspace, n);
			if(error) {
				goto err;
			}
		}
	}
err:
	switch (error) {
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
	delay(5000000);
}

void
i386_microtime(tvp)
	struct timeval *tvp;
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
	/* If we were using the FPU, forget about it. */
	if (npxproc == p)
		npxdrop();
#endif
	p->p_md.md_flags &= ~MDP_USEDFPU;
	if (i386_use_fxsave) {
		pcb->pcb_savefpu.sv_fx.fxv_env.fx_cw = ___NPX87___;
		pcb->pcb_savefpu.sv_fx.fxv_env.fx_mxcsr = ___MXCSR___;
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
		if (cn_tab != NULL) {
			consdev = cn_tab->cn_dev;
		} else {
			consdev = NODEV;
		}
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev, sizeof(consdev)));
	case CPU_BIOSBASEMEM:
		return (sysctl_rdint(oldp, oldlenp, newp, biosbasemem));
	case CPU_BIOSEXTMEM:
		return (sysctl_rdint(oldp, oldlenp, newp, biosextmem));

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Initialize 386 and configure to run kernel
 */
void
setregion(rd, base, limit)
	struct region_descriptor *rd;
	void *base;
	size_t limit;
{
	rd->rd_base = (int)base;
	rd->rd_limit = (int)limit;
}

/*
 * Initialize segments & interrupt table
 */
void
setidt_nodisp(ip, idx, off, sel, args, typ, dpl, p, hi)
	struct gate_descriptor *ip;
	int idx, off, args, sel, typ, dpl, p, hi;
{
	ip->gd_looffset = off;
	ip->gd_selector = sel;
	ip->gd_stkcpy = args;
	ip->gd_xx = 0;
	ip->gd_type = typ;
	ip->gd_dpl = dpl;
	ip->gd_p = p;
	ip->gd_hioffset = hi;
}

void
setidt(idx, func, args, typ, dpl)
	int idx, args, typ, dpl;
	void *func;
{
	struct gate_descriptor *ip;
	int off, sel, p, hi;

	ip = &idt[idx];
	off = func != NULL ? (int)func : 0;
	sel = GSEL(GCODE_SEL, SEL_KPL);
	p = 1;
	hi = ((u_int)off) >> 16;
	setidt_nodisp(ip, idx, off, sel, args, typ, dpl, p, hi);
}

void
unsetidt(idx)
	int idx;
{
	struct gate_descriptor *ip;

	ip = &idt[idx];
	setidt_nodisp(ip, idx, 0, 0, 0, 0, 0, 0, 0);
}

void
init_descriptors(void)
{
	gdt_allocate(gdt_segs);
	ldt_allocate(ldt_segs);
}

void
make_memory_segments(void)
{
	int x;

	/* make GDT memory segments */
	gdt_segs[GCODE_SEL].ssd_limit = btoc((int) &etext + NBPG);
	for (x = 0; x < NGDT; x++) {
		ssdtosd(&gdt_segs[x], &gdt[x].sd);
	}

	/* make LDT memory segments */
	ldt_segs[LUCODE_SEL].ssd_limit = btoc(UPT_MIN_ADDRESS);
	ldt_segs[LUDATA_SEL].ssd_limit = btoc(UPT_MIN_ADDRESS);
	/* Note. eventually want private ldts per process */
	for (x = 0; x < NLDT; x++) {
		ssdtosd(&ldt_segs[x], &ldt[x].sd);
	}
}

#define	KBTOB(x) ((size_t)(x) * 1024UL)

int
add_mem_cluster(seg_start, seg_end)
	u_int64_t seg_start, seg_end;
{
	extern struct extent *iomem_ex;
	int error, i;

	if (seg_end > 0x100000000ULL) {
		printf("WARNING: skipping large "
				"memory map entry: "
				"0x%qx/0x%qx\n", seg_start, (seg_end - seg_start));
		return (0);
	}

	if (seg_end == 0x100000000ULL) {
		seg_end -= PAGE_SIZE;
	}

	if (seg_end <= seg_start) {
		return (0);
	}

	for (i = 0; i < mem_cluster_cnt; i++) {
		if ((mem_clusters[i].start == round_page(seg_start)) && (mem_clusters[i].size == trunc_page(seg_end) - mem_clusters[i].start)) {
			return (0);
		}
	}

	/*
	 * Allocate the physical addresses used by RAM
	 * from the iomem extent map.  This is done before
	 * the addresses are page rounded just to make
	 * sure we get them all.
	 */
	error = extent_alloc_region(iomem_ex, seg_start, seg_end - seg_start, EX_NOWAIT);
	if (error) {
		printf("WARNING: CAN'T ALLOCATE "
				"MEMORY SEGMENT "
				"(0x%qx/0x%qx) FROM "
				"IOMEM EXTENT MAP!\n", seg_start, seg_end - seg_start);
		return (0);
	}

	if (mem_cluster_cnt >= PHYSSEG_MAX) {
		panic("init386: too many memory segments");
	}

	seg_start = round_page(seg_start);
	seg_end = trunc_page(seg_end);

	if (seg_start == seg_end) {
		return (0);
	}
	mem_clusters[mem_cluster_cnt].start = seg_start;
	mem_clusters[mem_cluster_cnt].size = seg_end - seg_start;

	physmem += atop(mem_clusters[mem_cluster_cnt].size);
	mem_cluster_cnt++;

	return (1);
}

void
setmemsize_common(basemem, extmem)
	int *basemem, *extmem;
{
	u_int64_t base_start, base_end, ext_start, ext_end;
	int error;

	/* BIOS_BASEMEM */
	base_start = (u_int64_t)*basemem;
	base_end = KBTOB(basemem);
	error = extent_alloc_region(iomem_ex, base_start, base_end, EX_NOWAIT);
	if (error) {
		printf("WARNING: CAN'T ALLOCATE BASE MEMORY FROM IOMEM EXTENT MAP!\n");
	}
	mem_clusters[0].start = base_start;
	mem_clusters[0].size = trunc_page(base_end);
	physmem += atop(mem_clusters[0].size);

	/* BIOS_EXTMEM */
	ext_start = IOM_END;
	ext_end = KBTOB(extmem);
	error = extent_alloc_region(iomem_ex, ext_start, ext_end, EX_NOWAIT);
	if (error) {
		printf("WARNING: CAN'T ALLOCATE EXTENDED MEMORY FROM IOMEM EXTENT MAP!\n");
	}
#if NISADMA > 0
	/*
	 * Some motherboards/BIOSes remap the 384K of RAM that would
	 * normally be covered by the ISA hole to the end of memory
	 * so that it can be used.  However, on a 16M system, this
	 * would cause bounce buffers to be allocated and used.
	 * This is not desirable behaviour, as more than 384K of
	 * bounce buffers might be allocated.  As a work-around,
	 * we round memory down to the nearest 1M boundary if
	 * we're using any isadma devices and the remapped memory
	 * is what puts us over 16M.
	 */
	if (extmem > (15*1024) && extmem < (16*1024)) {
		extmem = (15*1024);
	}
#endif

	mem_clusters[1].start = ext_start;
	mem_clusters[1].size = trunc_page(ext_end);
	physmem += atop(mem_clusters[1].size);

	mem_cluster_cnt = 2;

	Maxmem = atop(physmem);
	avail_end = ext_start + trunc_page(ext_end);
	maxmem = Maxmem - 1;

#ifdef SMP
	alloc_ap_trampoline(basemem, base_start, base_end);
#endif
}

void
setmemsize(boot, basemem, extmem)
	struct bootinfo *boot;
	int *basemem, *extmem;
{
	int error;

	vm86_initial_bioscalls(basemem, extmem);

	error = bootinfo_check(boot);
	if (boot != NULL && error) {
		setmemsize_common(boot->bi_basemem, boot->bi_extmem);
	} else {
		setmemsize_common(basemem, extmem);
	}
}

void
getmemsize(boot)
	struct bootinfo *boot;
{
	setmemsize(boot, &biosbasemem, &biosextmem);
}

void
proc0pcb_setup(p)
	struct proc *p;
{
	int sigcode, szsigcode;

	bcopy(&sigcode, p->p_addr->u_pcb.pcb_sigc, szsigcode);
	p->p_addr->u_pcb.pcb_flags = 0;
	p->p_addr->u_pcb.pcb_ptd = (int)IdlePTD;
}

typedef void *vector_t;
#define	IDTVEC(name)	__CONCAT(X, name)
extern vector_t IDTVEC(div), IDTVEC(dbg), IDTVEC(nmi), IDTVEC(bpt), IDTVEC(ofl),
		IDTVEC(bnd), IDTVEC(ill), IDTVEC(dna), IDTVEC(dble), IDTVEC(fpusegm),
		IDTVEC(tss), IDTVEC(missing), IDTVEC(stk), IDTVEC(prot), IDTVEC(page),
		IDTVEC(fpu), IDTVEC(align), IDTVEC(rsvd), IDTVEC(syscall), IDTVEC(osyscall);

void
init386(first)
	int first;
{
	int x;
	struct region_descriptor region;

	i386_bus_space_init();
	init_descriptors();
	init386_bootinfo(&i386boot);

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit(/*KERNBASE+0xa0000*/);

	/* make GDT & LDT memory segments */
	make_memory_segments();

	pmap_kenter(local_apic_va, local_apic_pa);
	memset((void *)local_apic_va, 0, PAGE_SIZE);

	/* exceptions */
	for(x = 0; x < NIDT; x++) {
		setidt(x, &IDTVEC(rsvd), 0, SDT_SYS386TGT, SEL_KPL);
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

	setregion(&region, gdt, sizeof(gdt)-1);
	lgdt(&region);
	setregion(&region, idt, sizeof(idt)-1);
	lidt(&region);

	finishidentcpu();		/* Final stage of CPU initialization */
	pmap_set_nx();
	initializecpu();		/* Initialize CPU registers */
	initializecpucache();

	softpic_init();			/* Initialize softpic pic selector */
	intr_default_setup();	/* Initialize vectors */

	splhigh();
	enable_intr();

	i386_bus_space_check(avail_end, biosbasemem, biosextmem);
	vm86_initialize();
	getmemsize(&i386boot);
	vm_set_page_size();

	/* call pmap initialization to make new kernel address space */
	pmap_bootstrap(first);
	/* now running on new page tables, configured,and u/iom is accessible */

	/* make a call gate to reenter kernel with */
	setidt(LSYS5CALLS_SEL, &IDTVEC(osyscall), 1, SDT_SYS386CGT, SEL_UPL);

	/* transfer to user mode */
	_ucodesel = LSEL(LUCODE_SEL, SEL_UPL);
	_udatasel = LSEL(LUDATA_SEL, SEL_UPL);

	/* setup proc0's pcb */
	proc0pcb_setup(&proc0);
}

void
init386_ksyms(boot)
	struct bootinfo *boot;
{
	extern int  end;
	vm_offset_t addend;
	
	if (boot->bi_environment != 0) {
		ksyms_addsyms_elf(*(int*) &end, ((int*) &end) + 1, esym);
		addend = (vm_offset_t)(boot->bi_environment < KERNBASE ? PMAP_MAP_LOW : 0);
	} else {
		ksyms_addsyms_elf(*(int*) &end, ((int*) &end) + 1, esym);
	}
	boot->bi_symtab += KERNBASE;
	boot->bi_esymtab += KERNBASE;
	ksyms_addsyms_elf(boot->bi_nsymtab, (int*) boot->bi_symtab, (int*) boot->bi_esymtab);
}

void
init386_bootinfo(boot)
	struct bootinfo *boot;
{
	vm_offset_t addend;

	if (i386_ksyms_addsyms_elf(boot)) {
		init386_ksyms(boot);
	} else {
		if (boot->bi_environment != 0) {
			addend = (vm_offset_t)(boot->bi_environment < KERNBASE ? PMAP_MAP_LOW : 0);
		}
	}
	identify_cpu();
}

int
bootinfo_check(boot)
	struct bootinfo *boot;
{
	if(boot->bi_version != BOOTINFO_VERSION) {
		printf("Bootinfo Check: Boot version does not match!\n");
		return (1);
	}
	if(boot->bi_memsizes_valid == 0) {
		printf("Bootinfo Check: Boot is not valid!\n");
		return (1);
	}
	if(boot->bi_size > BOOTINFO_MAXSIZE || boot->bi_size != sizeof(struct bootinfo *)) {
		printf("Bootinfo Check: Boot size is too big!\n");
		return (1);
	}
	return (0);
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

#if defined(I586_CPU) && !defined(NO_F00F_HACK)

void
f00f_hack(void)
{
	struct region_descriptor region;
	struct gate_descriptor 	 *new_idt;
	vm_offset_t tmp;
	void *p;
	
	if (!has_f00f_bug) {
		return;
	}
	
	printf("Intel Pentium detected, installing workaround for F00F bug\n");
	
	new_idt = idt;
	tmp = kmem_alloc(kernel_map, PAGE_SIZE * 2);
	if (tmp == 0) {
		panic("kmem_alloc returned 0");
	}
	if (((unsigned int)tmp & (PAGE_SIZE-1)) != 0) {
		panic("kmem_alloc returned non-page-aligned memory");
	}
	/* Put the first seven entries in the lower page */
	p = (void *)(tmp + PAGE_SIZE - (7 * 8));
	bcopy(new_idt, p, sizeof(idt));
	new_idt = p;
	
	/* Reload idtr */
	setregion(&region, new_idt, sizeof(idt) - 1);
	lidt(&region);
	
	if (vm_map_protect(kernel_map, tmp, tmp + PAGE_SIZE, VM_PROT_READ, FALSE) != KERN_SUCCESS) {
		panic("vm_map_protect failed");
	}
	return;
}
#endif /* defined(I586_CPU) && !NO_F00F_HACK */

void
cpu_need_proftick(p)
	struct proc *p;
{
	p->p_pflag |= MDP_OWEUPC;
	aston(p);
}

/*
 * Shutdown the CPU as much as possible
 */
void
cpu_halt(void)
{
	for (;;) {
		__asm__ ("hlt");
	}
}

void
need_resched(p)
	struct proc *p;
{
	want_resched(p);

	if (p) {
		aston(p);
	}
}

void
cpu_reset(void)
{
	struct region_descriptor region;

	disable_intr();
	/*
	 * Ensure the NVRAM reset byte contains something vaguely sane.
	 */

	outb(IO_RTC, NVRAM_RESET);
	outb(IO_RTC + 1, NVRAM_RESET_RST);

	/*
	 * The keyboard controller has 4 random output pins, one of which is
	 * connected to the RESET pin on the CPU in many PCs.  We tell the
	 * keyboard controller to pulse this line a couple of times.
	 */
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);

	/*
	 * Try to cause a triple fault and watchdog reset by making the IDT
	 * invalid and causing a fault.
	 */
	memset((caddr_t) idt, 0, sizeof(idt));
	setregion(&region, idt, sizeof(idt) - 1);
	lidt(&region);
	__asm __volatile("divl %0,%1" : : "q" (0), "a" (0));

#if 0
	/*
	 * Try to cause a triple fault and watchdog reset by unmapping the
	 * entire address space and doing a TLB flush.
	 */
	memset((caddr_t)PTD, 0, PAGE_SIZE);
	tlbflush();
#endif

	for (;;);
}

void
cpu_getmcontext(p, mcp, flags)
	struct proc *p;
	mcontext_t *mcp;
	unsigned int *flags;
{
	const struct trapframe *tf;
	gregset_t *gr;

	tf = p->p_md.md_regs;
	gr = &mcp->mc_gregs;
	/* Save register context. */
	if (tf->tf_eflags & PSL_VM) {
		struct trapframe_vm86 *tf86 = (struct trapframe_vm86 *)tf;
		struct vm86_kernel 	  *vm86 = &p->p_addr->u_pcb.pcb_vm86;

		gr->mc_gs  = tf86->tf_vm86_gs;
		gr->mc_fs  = tf86->tf_vm86_fs;
		gr->mc_es  = tf86->tf_vm86_es;
		gr->mc_ds  = tf86->tf_vm86_ds;
		gr->mc_eflags = get_vm86flags(tf86, vm86);
	}  else {
		gr->mc_gs = tf->tf_gs;
		gr->mc_fs = tf->tf_fs;
		gr->mc_es = tf->tf_es;
		gr->mc_ds = tf->tf_ds;
		gr->mc_eflags = tf->tf_eflags;
	}
	gr->mc_edi = tf->tf_edi;
	gr->mc_esi = tf->tf_esi;
	gr->mc_ebp = tf->tf_ebp;
	gr->mc_ebx = tf->tf_ebx;
	gr->mc_edx = tf->tf_edx;
	gr->mc_ecx = tf->tf_ecx;
	gr->mc_eax = tf->tf_eax;
	gr->mc_eip = tf->tf_eip;
	gr->mc_cs  = tf->tf_cs;
	gr->mc_esp = tf->tf_esp;
	gr->mc_ss  = tf->tf_ss;
	gr->mc_trapno = tf->tf_trapno;
	gr->mc_err = tf->tf_err;

	*flags |= _UC_CPU;

	/* Save floating point register context, if any. */
	if ((p->p_md.md_flags & MDP_USEDFPU) != 0) {
#if NNPX > 0
		/*
		 * If this process is the current FP owner, dump its
		 * context to the PCB first.
		 * XXX npxsave() also clears the FPU state; depending on the
		 * XXX application this might be a penalty.
		 */
		if (p->p_addr->u_pcb.pcb_fpcpu) {
			npxsave_proc(p, 1);
		}
#endif
		if (i386_use_fxsave) {
			memcpy(&mcp->mc_fpregs.mc_fp_reg_set.mc_fp_xmm_state.fp_xmm,
					&p->p_addr->u_pcb.pcb_savefpu.sv_fx,
					sizeof(mcp->mc_fpregs.mc_fp_reg_set.mc_fp_xmm_state.fp_xmm));
			*flags |= _UC_FXSAVE;
		} else {
			memcpy(&mcp->mc_fpregs.mc_fp_reg_set.mc_fpchip_state.fp_state,
					&p->p_addr->u_pcb.pcb_savefpu.sv_87,
					sizeof(mcp->mc_fpregs.mc_fp_reg_set.mc_fpchip_state.fp_state));
		}
		*flags |= _UC_FPU;
	}
}

int
cpu_setmcontext(p, mcp, flags)
	struct proc *p;
	const mcontext_t *mcp;
	unsigned int flags;
{
	struct trapframe *tf;
	const gregset_t *gr;

	tf = p->p_md.md_regs;
	gr = &mcp->mc_gregs;
	/* Restore register context, if any. */
	if ((flags & _UC_CPU) != 0) {
		if (gr->mc_eflags & PSL_VM) {
			struct trapframe_vm86 *tf86 = (struct trapframe_vm86 *)tf;
			struct vm86_kernel *vm86 = &p->p_addr->u_pcb.pcb_vm86;

			tf86->tf_vm86_gs = gr->mc_gs;
			tf86->tf_vm86_fs = gr->mc_fs;
			tf86->tf_vm86_es = gr->mc_es;
			tf86->tf_vm86_ds = gr->mc_ds;
			set_vm86flags(tf86, vm86, gr->mc_eflags);
		} else {
			/*
			 * Check for security violations.  If we're returning
			 * to protected mode, the CPU will validate the segment
			 * registers automatically and generate a trap on
			 * violations.  We handle the trap, rather than doing
			 * all of the checking here.
			 */
			if (((gr->mc_eflags ^ tf->tf_eflags) & PSL_USERSTATIC)
					|| !USERMODE(gr->mc_cs, gr->mc_eflags)) {
				printf("cpu_setmcontext error: uc EFL: 0x%08x"
						" tf EFL: 0x%08x uc CS: 0x%x\n", gr->mc_eflags,
						tf->tf_eflags, gr->mc_cs);
				return (EINVAL);
			}
			tf->tf_gs = gr->mc_gs;
			tf->tf_fs = gr->mc_fs;
			tf->tf_es = gr->mc_es;
			tf->tf_ds = gr->mc_ds;
			/* Only change the user-alterable part of eflags */
			tf->tf_eflags &= ~PSL_USER;
			tf->tf_eflags |= (gr->mc_eflags & PSL_USER);
		}
	}
	tf->tf_edi = gr->mc_edi;
	tf->tf_esi = gr->mc_esi;
	tf->tf_ebp = gr->mc_ebp;
	tf->tf_ebx = gr->mc_ebx;
	tf->tf_edx = gr->mc_edx;
	tf->tf_ecx = gr->mc_ecx;
	tf->tf_eax = gr->mc_eax;
	tf->tf_eip = gr->mc_eip;
	tf->tf_cs  = gr->mc_cs;
	tf->tf_esp = gr->mc_esp;
	tf->tf_ss  = gr->mc_ss;

	/* Restore floating point register context, if any. */
	if ((flags & _UC_FPU) != 0) {
#if NNPX > 0
		/*
		 * If we were using the FPU, forget that we were.
		 */
		if (p->p_addr->u_pcb.pcb_fpcpu != NULL) {
			npxsave_proc(p, 0);
		}
#endif
		if (flags & _UC_FXSAVE) {
			if (i386_use_fxsave) {
				memcpy(&p->p_addr->u_pcb.pcb_savefpu.sv_fx,
						&mcp->mc_fpregs.mc_fp_reg_set.mc_fp_xmm_state.fp_xmm,
						sizeof(&p->p_addr->u_pcb.pcb_savefpu.sv_fx));
			} else {
				/* This is a weird corner case */
				process_xmm_to_s87(
						(struct fxsave*) &mcp->mc_fpregs.mc_fp_reg_set.mc_fp_xmm_state.fp_xmm,
						&p->p_addr->u_pcb.pcb_savefpu.sv_87);
			}
		} else {
			if (i386_use_fxsave) {
				process_s87_to_xmm(
						(struct save87*) &mcp->mc_fpregs.mc_fp_reg_set.mc_fpchip_state.fp_state,
						&p->p_addr->u_pcb.pcb_savefpu.sv_fx);
			} else {
				memcpy(&p->p_addr->u_pcb.pcb_savefpu.sv_87,
						&mcp->mc_fpregs.mc_fp_reg_set.mc_fpchip_state.fp_state,
						sizeof(p->p_addr->u_pcb.pcb_savefpu.sv_87));
			}
		}
	}
	/* If not set already. */
	p->p_md.md_flags |= MDP_USEDFPU;

	if (flags & _UC_SETSTACK) {
		p->p_sigacts->ps_sigstk.ss_flags |= SS_ONSTACK;
	}
	if (flags & _UC_CLRSTACK) {
		p->p_sigacts->ps_sigstk.ss_flags &= ~SS_ONSTACK;
	}
	return (0);
}

#ifdef notyet
/* bios smap */
static int
add_smap_entry(struct bios_smap *smap)
{
	if (boothowto & RB_VERBOSE) {
		printf("SMAP type=%02x base=%016llx len=%016llx\n", smap->type, smap->base, smap->length);
	}

	if (smap->type != SMAP_TYPE_MEMORY) {
		return (1);
	}

	return (add_mem_cluster(smap->base, smap->length));
}

static void
add_smap_entries(struct bios_smap *smapbase)
{
	struct bios_smap *smap, *smapend;
	u_int32_t smapsize;

	/*
	 * Memory map from INT 15:E820.
	 *
	 * subr_module.c says:
	 * "Consumer may safely assume that size value precedes data."
	 * ie: an int32_t immediately precedes SMAP.
	 */
	smapsize = *((u_int32_t *)smapbase - 1);
	smapend = (struct bios_smap *)((uintptr_t)smapbase + smapsize);

	for (smap = smapbase; smap < smapend; smap++) {
		if (!add_smap_entry(smap)) {
			break;
		}
	}
}

int
has_smapbase(smapbase)
	struct bios_smap *smapbase;
{
	int has_smap;

	has_smap = 0;
	if (smapbase != NULL) {
		add_smap_entries(smapbase);
		has_smap = 1;
		return (has_smap);
	}
	return (has_smap);
}
#endif

/*
 * Add a mask to cpl, and return the old value of cpl.
 */
int
splraise(ncpl)
	register int ncpl;
{
	register int ocpl = cpl;

	cpl = ocpl | ncpl;
	return (ocpl);
}

/*
 * Restore a value to cpl (unmasking interrupts).  If any unmasked
 * interrupts are pending, call Xspllower() to process them.
 */
void
splx(ncpl)
	register int ncpl;
{
	cpl = ncpl;
	if (ipending & ~ncpl) {
		Xspllower();
	}
}

/*
 * Same as splx(), but we return the old value of spl, for the
 * benefit of some splsoftclock() callers.
 */
int
spllower(ncpl)
	register int ncpl;
{
	register int ocpl = cpl;

	cpl = ncpl;
	if (ipending & ~ncpl)
		Xspllower();
	return (ocpl);
}

/*
 * Software interrupt registration
 *
 * We hand-code this to ensure that it's atomic.
 */
void
softintr(mask)
	register int mask;
{
	__asm __volatile("orl %0, ipending" : : "ir" (1 << mask));
}

#ifdef notyet
/*
 * Copy a binary buffer from user space to kernel space.
 *
 * Returns 0 on success, EFAULT on failure.
 */
int
copyin(to, from, len)
	const void *to;
	void *from;
	size_t len;
{
	int error;
	size_t n;

	error = 0;
	while (len) {
		error = user_write_fault(to);
		if (error) {
			break;
		}
		n = PAGE_SIZE - ((vm_offset_t)to & PAGE_MASK);
		if (n > len) {
			n = len;
		}
		bcopy(to & PAGE_MASK, from, n);
		len -= n;
		to = (const char *)to + n;
		from = (char *)from + n;
	}
	if (error) {
		error = EFAULT;
	}
	return (error);
}

/*
 * Copy a binary buffer from kernel space to user space.
 *
 * Returns 0 on success, EFAULT on failure.
 */
int
copyout(from, to, len)
	const void *from;
	void *to;
	size_t len;
{
	int error;
	size_t n;

	error = 0;
	while (len) {
		error = user_write_fault(to);
		if (error) {
			break;
		}
		n = PAGE_SIZE - ((vm_offset_t)to & PAGE_MASK);
		if (n > len) {
			n = len;
		}
		bcopy(from, to & PAGE_MASK, n);
		len -= n;
		to = (char *)to + n;
		from = (const char *)from + n;
	}
	if (error) {
		error = EFAULT;
	}
	return (error);
}
#endif

/*
 * Below written in C to allow access to debugging code
 */
int
copyinstr(fromaddr, toaddr, maxlength, lencopied)
	const void *fromaddr;
	void *toaddr;
	size_t *lencopied, maxlength;
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

int
copyoutstr(fromaddr, toaddr, maxlength, lencopied)
	const void *fromaddr;
	void *toaddr;
	size_t *lencopied, maxlength;
{
	int c, tally;

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

int
copystr(fromaddr, toaddr, maxlength, lencopied)
	const void *fromaddr;
	void *toaddr;
	size_t *lencopied, maxlength;
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

/*
 * Provide inb() and outb() as functions.  They are normally only available as
 * inline functions, thus cannot be called from the debugger.
 */

/* silence compiler warnings */

u_char 	inb_(u_short);
void 	outb_(u_short, u_char);

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
