/*-
 * Copyright (c) 1990, 1993
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
 *	@(#)param.h	8.3 (Berkeley) 5/14/95
 */

#ifndef _AMD64_PARAM_H_
#define _AMD64_PARAM_H_

/*
 * Machine dependent constants for AMD64.
 */

#ifdef _KERNEL
#ifdef _LOCORE
#include <machine/psl.h>
#else
#include <machine/cpu.h>
#endif
#endif

#ifndef MACHINE
#define MACHINE			"amd64"
#endif
#ifndef MACHINE_ARCH
#define	MACHINE_ARCH	"x86_64"
#endif
#define MID_MACHINE		MID_X86_64

/* Maximum Number of CPU's */
#ifdef SMP
#ifndef NCPUS
#define NCPUS 			256
#endif
#else
#define NCPUS 			1
#endif

/* segments */
#define	NBSG			4194304				/* bytes/segment (SEGMENT SIZE) */

#define	SGOFSET			(NBSG-1)			/* byte offset into segment */
#define	SGSHIFT			22					/* LOG2(NBSG) */
#define	SGSIZE			(1 << SGSHIFT)		/* bytes/segment (SEGMENT SIZE) */
#define SGMASK			SGOFSET				/* SGOFSET (SGSIZE - 1) */

/* pages */
#define	NBPG			4096				/* bytes/page (PAGE SIZE) */

#define	PGOFSET			(NBPG-1)			/* byte offset into page */
#define PGSHIFT			12					/* LOG2(PAGE_SIZE) */
#define PGSIZE			(1<<PGSHIFT)		/* bytes/page */
#define PGMASK			PGOFSET				/* PGOFSET (PGSIZE - 1) */

#define NPGPTD			1					/* Num of pages for page directory */
/* Size in bytes of the page directory */
#define NBPTD			(NPGPTD << PGSHIFT)
/* Number of PDEs in page directory, AMD64: 512, X86 PAE: 2048, NOPAE: 1024 */
#define NPDEPTD			(NBPTD/(sizeof(pd_entry_t)))

/* Size of the level 1 page table units */
#define NPTEPG			(NBPG/(sizeof(pt_entry_t)))
/* Size of the level 2 page directory units */
#define	NPDEPG			(NBPG/(sizeof(pd_entry_t)))
/* Size of the level 3 page directory pointer table units */
#define	NPDPTEPG		(NBPG/(sizeof(pdpt_entry_t)))
/* Size of the level 4 page-map level-4 table units */
#define	NPML4EPG		(NBPG/(sizeof(pml4_entry_t)))
/* Size of the level 5 page-map level-5 table units */
#define	NPML5EPG		(NBPG/(sizeof(pml5_entry_t)))

#define	PD_SHIFT		(SGSHIFT-1)			/* LOG2(NBPDR) (21) */
#define NBPDR			(1 << PD_SHIFT)		/* bytes/page dir */

#define	KERNBASE		0xFFFFFFFF80000000	/* start of kernel virtual (i.e. SYSTEM) */
#define KERNLOAD		KERNBASE			/* Kernel physical load address */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#define amd64_round_pdr(x)		((((unsigned long)(x)) + NBPDR - 1) & ~(NBPDR - 1))
#define amd64_trunc_pdr(x)		((unsigned long)(x) & ~(NBPDR - 1))
#define amd64_btod(x)			((unsigned long)(x) >> (PD_SHIFT))
#define amd64_dtob(x)			((unsigned long)(x) << (PD_SHIFT))

#define amd64_round_segment(x)	((((unsigned long)(x)) + NBSG - 1) & ~(NBSG-1))
#define amd64_trunc_segment(x)	((unsigned long)(x) & ~(NBSG-1))
#define amd64_round_page(x)		((((unsigned long)(x)) + NBPG - 1) & ~(NBPG-1))
#define amd64_trunc_page(x)		((unsigned long)(x) & ~(NBPG-1))
#define amd64_btos(x)			((unsigned long)(x) >> SGSHIFT)
#define amd64_stob(x)			((unsigned long)(x) << SGSHIFT)
#define amd64_btop(x)			((unsigned long)(x) >> PGSHIFT)
#define amd64_ptob(x)			((unsigned long)(x) << PGSHIFT)
#endif /* _AMD64_PARAM_H_ */
