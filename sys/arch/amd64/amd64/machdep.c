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

union descriptor		gdt[NGDT];
struct gate_descriptor 	idt[NIDT];
union descriptor 		ldt[NLDT];

void
startup(void)
{

}

/*
 * Initialize 386 and configure to run kernel
 */
void
setregion(rd, base, limit)
	struct region_descriptor *rd;
	void *base;
	uint16_t limit;
{
	rd->rd_base = (uint64_t)base;
	rd->rd_limit = (uint64_t)limit;
}

void
proc0pcb_setup(p)
	struct proc *p;
{
	int sigcode, szsigcode;

	bcopy(&sigcode, p->p_addr->u_pcb.pcb_sigc, szsigcode);
	p->p_addr->u_pcb.pcb_flags = 0;
	p->p_addr->u_pcb.pcb_ptd = (int)IdlePML4;
}

void
initamd64(first)
	uint64_t first;
{
	vm_set_segment_size();
	vm_set_page_size();

	/* call pmap initialization to make new kernel address space */
	pmap_bootstrap(first);

	/* transfer to user mode */
	_ucodesel = LSEL(LUCODE_SEL, SEL_UPL);
	_udatasel = LSEL(LUDATA_SEL, SEL_UPL);

	/* setup proc0's pcb */
	proc0pcb_setup(&proc0);
}
