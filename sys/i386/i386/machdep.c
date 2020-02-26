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

cpu_startup(firstaddr)
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
}


initcpu()
{

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

struct sigframe {
	int	sf_signum;
	int	sf_code;
	struct	sigcontext *sf_scp;
	sig_t	sf_handler;
	int	sf_eax;
	int	sf_edx;
	int	sf_ecx;
	struct	sigcontext sf_sc;
};

extern int kstack[];

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

void *
getframe(p, sig, onstack)
	struct proc *p;
	int sig, *onstack;
{
	struct trapframe *tf = p->p_md.md_regs;

	/* Do we need to jump onto the signal stack? */
	*onstack = (u->u_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0
			&& (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;
	if (*onstack)
		return (char*) u->u_sigstk.ss_base + u->u_sigstk.ss_size;
	return (void*) tf->tf_esp;
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

	//load_cr0(rcr0() | CR0_MP | CR0_TS);		/* start emulating */
}

void
cpu_setregs(void)
{
	unsigned int cr0;
	cr0 = rcr0();
	cr0 |= CR0_MP | CR0_NE | CR0_TS | CR0_WP | CR0_AM;
	load_cr0(cr0);
//	load_gs(_udatasel);
}

void
cpu_sysctl()
{

}

/*
 * Initialize segments and descriptor tables
 */
void
setgate(gd, func, args, type, dpl, sel)
	struct gate_descriptor *gd;
	void *func;
	int args, type, dpl, sel;
{
	gd->gd_looffset = (int)func;
	gd->gd_selector = sel;
	gd->gd_stkcpy = args;
	gd->gd_xx = 0;
	gd->gd_type = type;
	gd->gd_dpl = dpl;
	gd->gd_p = 1;
	gd->gd_hioffset = (int)func >> 16;
}

void
unsetgate(gd)
	struct gate_descriptor *gd;
{
	gd->gd_p = 0;
	gd->gd_hioffset = 0;
	gd->gd_looffset = 0;
	gd->gd_selector = 0;
	gd->gd_xx = 0;
	gd->gd_stkcpy = 0;
	gd->gd_type = 0;
	gd->gd_dpl = 0;
}

void
setregion(rd, base, limit)
	struct region_descriptor *rd;
	void *base;
	size_t limit;
{
	rd->rd_limit = (int)limit;
	rd->rd_base = (int)base;
}

void
setsegment(sd, base, limit, type, dpl, def32, gran)
	struct segment_descriptor *sd;
	const void *base;
	size_t limit;
	int type, dpl, def32, gran;
{
	sd->sd_lolimit = (int)limit;
	sd->sd_lobase = (int)base;
	sd->sd_type = type;
	sd->sd_dpl = dpl;
	sd->sd_p = 1;
	sd->sd_hilimit = (int)limit >> 16;
	sd->sd_xx = 0;
	sd->sd_def32 = def32;
	sd->sd_gran = gran;
	sd->sd_hibase = (int)base >> 24;
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
