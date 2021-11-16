/*-
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, and William Jolitz.
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
 *	@(#)vm_machdep.c	8.3 (Berkeley) 1/21/94
 *	Utah $Hdr: vm_machdep.c 1.16.1.1 89/06/23$
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/user.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>

#include <machine/cpu.h>
#include <machine/gdt.h>
#include <machine/reg.h>
#include <machine/specialreg.h>

#include "npx.h"

/*
 * Finish a fork operation, with process p2 nearly set up.
 * Copy and update the kernel stack and pcb, making the child
 * ready to run, and marking it so that it can return differently
 * than the parent.  Returns 1 in the child process, 0 in the parent.
 * We currently double-map the user area so that the stack is at the same
 * address in each process; in the future we will probably relocate
 * the frame pointers on the stack after copying.
 */
int
cpu_fork(p1, p2)
	register struct proc *p1, *p2;
{
	register struct user *up = p2->p_addr;
	int foo, offset, addr, i;
	extern char kstack[];
	extern int mvesp();

	/*
	 * Copy pcb and stack from proc p1 to p2.
	 * We do this as cheaply as possible, copying only the active
	 * part of the stack.  The stack and pcb need to agree;
	 * this is tricky, as the final pcb is constructed by savectx,
	 * but its frame isn't yet on the stack when the stack is copied.
	 * mi_switch compensates for this when the child eventually runs.
	 * This should be done differently, with a single call
	 * that copies and updates the pcb+stack,
	 * replacing the bcopy and savectx.
	 */
	p2->p_addr->u_pcb = p1->p_addr->u_pcb;
	offset = mvesp() - (int)kstack;
	bcopy((caddr_t)kstack + offset, (caddr_t)p2->p_addr + offset, (unsigned) ctob(UPAGES) - offset);
	p2->p_md.md_regs = p1->p_md.md_regs;

	/*
	 * Wire top of address space of child to it's u.
	 * First, fault in a page of pte's to map it.
	 */
        addr = trunc_page((u_int)vtopte(kstack));
	(void)vm_map_pageable(&p2->p_vmspace->vm_map, addr, addr+NBPG, FALSE);
	for (i=0; i < UPAGES; i++)
		pmap_enter(&p2->p_vmspace->vm_pmap, (vm_offset_t)kstack+i*NBPG, pmap_extract(kernel_pmap, ((int)p2->p_addr)+i*NBPG), VM_PROT_READ, 1);
	pmap_activate(&p2->p_vmspace->vm_pmap, &up->u_pcb);

	/*
	 *
	 * Arrange for a non-local goto when the new process
	 * is started, to resume here, returning nonzero from setjmp.
	 */
	if (savectx(up, 1)) {
		/*
		 * Return 1 in child.
		 */
		return (1);
	}
	return (0);
}

#ifdef notyet
/*
 * cpu_exit is called as the last action during exit.
 *
 * We change to an inactive address space and a "safe" stack,
 * passing thru an argument to the new stack. Now, safely isolated
 * from the resources we're shedding, we release the address space
 * and any remaining machine-dependent resources, including the
 * memory for the user structure and kernel stack.
 *
 * Next, we assign a dummy context to be written over by mi_switch,
 * calling it to send this process off to oblivion.
 * [The nullpcb allows us to minimize cost in mi_switch() by not having
 * a special case].
 */
struct proc *switch_to_inactive();

void
cpu_exit(p)
	register struct proc *p;
{
	static struct pcb nullpcb;	/* pcb to overwrite on last switch */

#if NNPX > 0
	/* free cporcessor (if we have it) */
	if( p == npxproc()) {
		npxproc() = 0;
	}
#endif

	/* move to inactive space and stack, passing arg accross */
	p = switch_to_inactive(p);

	/* drop per-process resources */
	vmspace_free(p->p_vmspace);
	kmem_free(kernel_map, (vm_offset_t)p->p_addr, ctob(UPAGES));

	p->p_addr = (struct user *) &nullpcb;
	swtch();
	/* NOTREACHED */
}
#else

void
cpu_exit(p)
	register struct proc *p;
{
	/* free coprocessor (if we have it) */
#if NNPX > 0
	if( p == npxproc()) {
		npxproc() = 0;
	}
#endif

	curproc() = p;
	swtch();
}

void
cpu_wait(p)
struct proc *p;
{

	/* drop per-process resources */
	vmspace_free(p->p_vmspace);
	kmem_free(kernel_map, (vm_offset_t)p->p_addr, ctob(UPAGES));
}
#endif

/*
 * Dump the machine specific header information at the start of a core dump.
 */
int
cpu_coredump(p, vp, cred)
	struct proc *p;
	struct vnode *vp;
	struct ucred *cred;
{
	return (vn_rdwr(UIO_WRITE, vp, (caddr_t) p->p_addr, ctob(UPAGES), (off_t)0, UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred, (int *)NULL, p));
}

/*
 * Set a red zone in the kernel stack after the u. area.
 */
void
setredzone(pte, vaddr)
	u_short *pte;
	caddr_t vaddr;
{
/* eventually do this by setting up an expand-down stack segment
   for ss0: selector, allowing stack access down to top of u.
   this means though that protection violations need to be handled
   thru a double fault exception that must do an integral task
   switch to a known good context, within which a dump can be
   taken. a sensible scheme might be to save the initial context
   used by sched (that has physical memory mapped 1:1 at bottom)
   and take the dump while still in mapped mode */
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
void
pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	register struct pte *fpte, *tpte;

	if (size % CLBYTES)
		panic("pagemove");
	fpte = kvtopte(from);
	tpte = kvtopte(to);
	while (size > 0) {
		*tpte++ = *fpte;
		*(int *)fpte++ = 0;
		from += NBPG;
		to += NBPG;
		size -= NBPG;
	}
	tlbflush();
}

/*
 * Convert kernel VA to physical address
 */
u_long
kvtop(addr)
	register caddr_t addr;
{
	vm_offset_t va;

	va = pmap_extract(kernel_pmap, (vm_offset_t)addr);
	if (va == 0)
		panic("kvtop: zero page frame");
	return((int)va);
}

#ifdef notdef
/*
 * The probe[rw] routines should probably be redone in assembler
 * for efficiency.
 */
int
prober(addr)
	register u_int addr;
{
	register int page;
	register struct proc *p;

	if (addr >= USRSTACK)
		return(0);
	p = u->u_procp;
	page = btop(addr);
	if (page < dptov(p, p->p_dsize) || page > sptov(p, p->p_ssize))
		return(1);
	return(0);
}

int
probew(addr)
	register u_int addr;
{
	register int page;
	register struct proc *p;

	if (addr >= USRSTACK)
		return(0);
	p = u->u_procp;
	page = btop(addr);
	if (page < dptov(p, p->p_dsize) || page > sptov(p, p->p_ssize))
		return((*(int *)vtopte(p, page) & PG_PROT) == PG_UW);
	return(0);
}
#endif


extern vm_map_t phys_map;

/*
 * Map an IO request into kernel virtual address space.  Requests fall into
 * one of five catagories:
 *
 *	B_PHYS|B_UAREA:	User u-area swap.
 *			Address is relative to start of u-area (p_addr).
 *	B_PHYS|B_PAGET:	User page table swap.
 *			Address is a kernel VA in usrpt (Usrptmap).
 *	B_PHYS|B_DIRTY:	Dirty page push.
 *			Address is a VA in proc2's address space.
 *	B_PHYS|B_PGIN:	Kernel pagein of user pages.
 *			Address is VA in user's address space.
 *	B_PHYS:		User "raw" IO request.
 *			Address is VA in user's address space.
 *
 * All requests are (re)mapped into kernel VA space via the useriomap
 * (a name with only slightly more meaning than "kernelmap")
 */
void
vmapbuf(bp)
	register struct buf *bp;
{
	register int npf;
	register caddr_t addr;
	register long flags = bp->b_flags;
	struct proc *p;
	int off;
	vm_offset_t kva;
	register vm_offset_t pa;

	if ((flags & B_PHYS) == 0)
		panic("vmapbuf");
	addr = bp->b_saveaddr = bp->b_un.b_addr;
	off = (int)addr & PGOFSET;
	p = bp->b_proc;
	npf = btoc(round_page(bp->b_bcount + off));
	kva = kmem_alloc_wait(phys_map, ctob(npf));
	bp->b_un.b_addr = (caddr_t) (kva + off);
	while (npf--) {
		pa = pmap_extract(&p->p_vmspace->vm_pmap, (vm_offset_t)addr);
		if (pa == 0)
			panic("vmapbuf: null page frame");
		pmap_enter(vm_map_pmap(phys_map), kva, trunc_page(pa), VM_PROT_READ|VM_PROT_WRITE, TRUE);
		addr += PAGE_SIZE;
		kva += PAGE_SIZE;
	}
}

/*
 * Free the io map PTEs associated with this IO operation.
 * We also invalidate the TLB entries and restore the original b_addr.
 */
void
vunmapbuf(bp)
	register struct buf *bp;
{
	register int npf;
	register caddr_t addr = bp->b_un.b_addr;
	vm_offset_t kva;

	if ((bp->b_flags & B_PHYS) == 0)
		panic("vunmapbuf");
	npf = btoc(round_page(bp->b_bcount + ((int)addr & PGOFSET)));
	kva = (vm_offset_t)((int)addr & ~PGOFSET);
	kmem_free_wakeup(phys_map, kva, ctob(npf));
	bp->b_un.b_addr = bp->b_saveaddr;
	bp->b_saveaddr = NULL;
}
