/*-
 * Copyright (c) 1993
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
 *	@(#)pcb.h	8.2 (Berkeley) 1/21/94
 */

/*
 * Intel 386 process control block
 */

#include <machine/segments.h>
#include <machine/tss.h>
#include <machine/npx.h>
#include <machine/vm86.h>

#define	NPORT					1024					/* # of ports we allow to be mapped */

struct pcb {
	struct	i386tss 			pcb_tss;
#define pcb_edi					pcb_tss.tss_edi
#define pcb_esi					pcb_tss.tss_esi
#define	pcb_ebp					pcb_tss.tss_ebp
#define pcb_esp					pcb_tss.tss_esp
#define pcb_ebx					pcb_tss.tss_ebx
#define	pcb_eib					pcb_tss.tss_eip

#define	pcb_ptd					pcb_tss.tss_cr3
	struct	savefpu				pcb_savefpu;			/* floating point state (context) for 287/387 */
	struct	emcsts				pcb_saveemc;			/* Cyrix EMC state */
	u_long						pcb_iomap[NPORT/32];	/* i/o port bitmap */
#define	pcb_cr0					pcb_tss.tss_cr0			/* saved image of CR0 */
#define	pcb_cr2					pcb_tss.tss_cr2
#define	pcb_cr3					pcb_tss.tss_cr3
#define	pcb_cr4					pcb_tss.tss_cr4
	int							pcb_fsd[2];				/* %fs descriptor */
	int							pcb_gsd[2];				/* %gs descriptor */
	union descriptor			*pcb_desc;				/* gate & segment descriptors */
	int							pcb_ldt_sel;
	int							pcb_ldt_len;

/* Software pcb (extension) */
	int							pcb_flags;
#define	PCB_USER_LDT			0x01					/* has user-set LDT */
#define	PCB_VM86CALL			0x02					/* in vm86 call */
#define	FM_TRAP					0x04					/* process entered kernel on a trap frame */
	short						pcb_iml;				/* interrupt mask level */
	caddr_t						pcb_onfault;			/* copyin/out fault recovery */
	long						pcb_sigc[8];			/* XXX signal code trampoline */
	int							pcb_cmap2;				/* XXX temporary PTE - will prefault instead */
	struct segment_descriptor 	pcb_tssd;				/* tss descriptor */
	int							vm86_eflags;			/* virtual eflags for vm86 mode */
//	u_long						pcb_scvm86[2];			/* vm86bios scratch space */
	struct vm86_kernel 			pcb_vm86;				/* vm86 area */
};


/*
 * The pcb is augmented with machine-dependent additional data for
 * core dumps. For the i386: ???
 */
struct md_coredump {
	int     pad;		/* XXX? -- cgd */
};

#ifdef _KERNEL
struct pcb *curpcb;		/* our current running pcb */
struct trapframe;
#endif
