/*	$NetBSD: db_interface.c,v 1.42 2004/02/13 11:36:13 wiz Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
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
 *
 *	db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */
/*
 * Interface to new debugger.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <vm/include/vm.h>

#include <dev/misc/cons/cons.h>

#include <machine/cpufunc.h>
#include <machine/db_machdep.h>
#include <machine/pmap.h>
#include <machine/pte.h>
//#include <machine/segments.h>
#include <machine/trap.h>

#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <ddb/ddbvar.h>

extern label_t	*db_recover;
extern char *trap_type[];
extern int trap_types;

int	db_active = 0;
db_regs_t ddb_regs;	/* register state */

void kdbprinttrap(int, int);

db_regs_t *ddb_regp = 0;

/*
 * Print trap reason.
 */
void
kdbprinttrap(type, code)
	int type, code;
{
	db_printf("kernel: ");
	if (type >= trap_types || type < 0)
		db_printf("type %d", type);
	else
		db_printf("%s", trap_type[type]);
	db_printf(" trap, code=%x\n", code);
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(type, code, regs)
	int type, code;
	db_regs_t *regs;
{
	int s, flags;
	db_regs_t dbreg;

	flags = regs->tf_err & TC_FLAGMASK;
	regs->tf_err &= ~TC_FLAGMASK;

	switch (type) {
	case T_BPTFLT:	/* breakpoint */
	case T_TRCTRAP:	/* single_step */
	case T_NMI:		/* NMI */
	case -1:		/* keyboard interrupt */
		break;
	default:
		if (!db_onpanic && db_recover==0)
			return (0);
		kdbprinttrap(type, code);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* XXX Should switch to kdb`s own stack here. */

	ddb_regs = *regs;
	if (!(flags & TC_TSS) && KERNELMODE(regs->tf_cs, regs->tf_eflags)) {
		/*
		 * Kernel mode - esp and ss not saved
		 */
		ddb_regs.tf_esp = (int)&regs->tf_esp;	/* kernel stack pointer */
		__asm("movw %%ss,%w0" : "=r" (ddb_regs.tf_ss));
	}

	ddb_regs.tf_cs &= 0xffff;
	ddb_regs.tf_ds &= 0xffff;
	ddb_regs.tf_es &= 0xffff;
	ddb_regs.tf_fs &= 0xffff;
	ddb_regs.tf_gs &= 0xffff;
	ddb_regs.tf_ss &= 0xffff;

	s = splhigh();
	db_active++;
	cnpollc(TRUE);
	db_trap(type, code);
	cnpollc(FALSE);
	db_active--;
	splx(s);

	ddb_regp = &dbreg;

	regs->tf_gs     = ddb_regs.tf_gs;
	regs->tf_fs     = ddb_regs.tf_fs;
	regs->tf_es     = ddb_regs.tf_es;
	regs->tf_ds     = ddb_regs.tf_ds;
	regs->tf_edi    = ddb_regs.tf_edi;
	regs->tf_esi    = ddb_regs.tf_esi;
	regs->tf_ebp    = ddb_regs.tf_ebp;
	regs->tf_ebx    = ddb_regs.tf_ebx;
	regs->tf_edx    = ddb_regs.tf_edx;
	regs->tf_ecx    = ddb_regs.tf_ecx;
	regs->tf_eax    = ddb_regs.tf_eax;
	regs->tf_eip    = ddb_regs.tf_eip;
	regs->tf_cs     = ddb_regs.tf_cs;
	regs->tf_eflags = ddb_regs.tf_eflags;
	if (!(flags & TC_TSS) && !KERNELMODE(regs->tf_cs, regs->tf_eflags)) {
		/* ring transit - saved esp and ss valid */
		regs->tf_esp    = ddb_regs.tf_esp;
		regs->tf_ss     = ddb_regs.tf_ss;
	}

#ifdef TRAPLOG
	wrmsr(MSR_DEBUGCTLMSR, 0x1);
#endif

	return (1);
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vm_offset_t	addr;
	register size_t	size;
	register char	*data;
{
	char *src;

	src = (char *)addr;

	if (size == 4) {
		*((int *)data) = *((int *)src);
		return;
	}

	if (size == 2) {
		*((short *)data) = *((short *)src);
		return;
	}

	while (size-- > 0)
		*data++ = *src++;
}

/*
 * Write bytes somewhere in the kernel text.  Make the text
 * pages writable temporarily.
 */
static void
db_write_text(addr, size, data)
	vm_offset_t	addr;
	register size_t	size;
	register char	*data;
{
	pt_entry_t *pte, oldpte, tmppte;
	vm_offset_t pgva;
	size_t limit;
	char *dst;

	if (size == 0)
		return;

	dst = (char*) addr;
	do {
		/*
		 * Get the PTE for the page.
		 */
		pte = kvtopte(addr);
		oldpte = *pte;

		if ((oldpte & PG_V) == 0) {
			printf(" address %p not a valid page\n", dst);
			return;
		}

		/*
		 * Get the VA for the page.
		 */
		pgva = i386_trunc_page(dst);

		/*
		 * Compute number of bytes that can be written
		 * with this mapping and subtract it from the
		 * total size.
		 */
		limit = PAGE_SIZE - ((vm_offset_t)dst & PGOFSET);
		if (limit > size)
			limit = size;
		size -= limit;

		tmppte = (oldpte & ~PG_KR) | PG_KW;
		*pte = tmppte;
		invlpg(pgva);
		/*
		 * MULTIPROCESSOR: no shootdown required as the PTE continues to
		 * map the same page and other CPU's don't need write access.
		 */

		/*
		 * Page is now writable.  Do as much access as we
		 * can in this page.
		 */
		for (; limit > 0; limit--)
			*dst++ = *data++;

		/*
		 * Restore the old PTE.
		 */
		*pte = oldpte;

#if 0
		/*
		 * XXXSMP Not clear if this is needed for 100% correctness.
		 */
		{
			int cpumask = 0;
			vm_offset_t pteva = ptetov(oldpte);

			/*
			 * shoot down in case other CPU mistakenly caches page.
			 */
			pmap_tlb_shootdown(pmap_kernel(), pgva, pteva, &cpumask);
			pmap_tlb_shootnow(pmap_kernel(), cpumask);
		}
#else
		invlpg(pgva);
#endif

	} while (size != 0);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vm_offset_t	addr;
	register size_t	size;
	register char	*data;
{
//	extern char etext;
	char 		*dst;

	dst = (char *)addr;

	/* If any part is in kernel text, use db_write_text() */
	if (addr >= KERNBASE && addr < (vm_offset_t)&etext) {
		db_write_text(addr, size, data);
		return;
	}

	dst = (char *)addr;

	if (size == 4) {
		*((int *)dst) = *((int *)data);
		return;
	}

	if (size == 2) {
		*((short *)dst) = *((short *)data);
		return;
	}

	while (size-- > 0)
		*dst++ = *data++;
}

void
Debugger()
{
	__asm("int $3");
}
