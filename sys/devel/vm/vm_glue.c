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

#include <vm/include/vm.h>

#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_pageout.h>
#include <vm/include/vm_text.h>

#include <machine/cpu.h>

/*
 * Swap in a process's u-area.
 */
void
swapin(p)
	struct proc *p;
{
	vm_offset_t addr;
	int s;

	addr = (vm_offset_t)p->p_addr;
	vm_map_pageable(kernel_map, addr, addr + USPACE, FALSE);

#ifdef IDSEPERATION
	vm_xswapin(p, addr);
#endif

	cpu_swapin(p);
	s = splstatclock();
	if (p->p_stat == SRUN) {
		setrq(p);
	}
	p->p_flag |= SLOAD | P_INMEM;
	splx(s);
	p->p_time = 0;
	p->p_swtime = 0;
	cnt.v_swpin++;
}

void
swapout(p)
	register struct proc *p;
{
	vm_offset_t addr;
	vm_size_t size;
	int s;
#ifdef IDSEPERATION
	int freecore;
	u_int odata, ostack;
#endif

#ifdef DEBUG
	if (swapdebug & SDB_SWAPOUT)
		printf("swapout: pid %d(%s)@%x, stat %x pri %d free %d\n",
		       p->p_pid, p->p_comm, p->p_addr, p->p_stat,
		       p->p_slptime, cnt.v_free_count);
#endif
	size = round_page(ctob(USIZE));
	addr = (vm_offset_t)p->p_addr;

	/*
	 * Unwire the to-be-swapped process's user struct and kernel stack.
	 */
	vm_map_pageable(kernel_map, addr, addr + addr+size, TRUE);
	pmap_collect(vm_map_pmap(&p->p_vmspace->vm_map));

#ifdef IDSEPERATION
	vm_xswapout(p, addr, size, freecore, odata, ostack);
#endif

	s = splstatclock();
	p->p_flag &= ~(P_SLOAD | P_SLOCK | P_INMEM);
	if (p->p_stat == SRUN) {
		remrq(p);
	}
	splx(s);
	p->p_time = 0;
	p->p_swtime = 0;

	cnt.v_swpout++;
	if (runout) {
		runout = 0;
		wakeup((caddr_t) &runout);
	}
}
