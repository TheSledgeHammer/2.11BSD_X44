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
 *	@(#)pte.h	8.1 (Berkeley) 6/11/93
 */

/*
 * 386 page table entry and page table directory
 * W.Jolitz, 8/89
 *
 * There are two major kinds of pte's: those which have ever existed (and are
 * thus either now in core or on the swap device), and those which have
 * never existed, but which will be filled on demand at first reference.
 * There is a structure describing each.  There is also an ancillary
 * structure used in page clustering.
 */

#ifndef _I386_PTE_H_
#define _I386_PTE_H_

#define	PG_RO		0x000		/* R/O	Read-Only		*/
#define	PG_V		0x001		/* P	Valid			*/
#define	PG_RW		0x002		/* R/W	Read/Write		*/
#define	PG_u		0x004
#define	PG_PROT		0x006 		/* all protection bits .*/
#define	PG_NC_PWT	0x008		/* PWT	Write through	*/
#define	PG_NC_PCD	0x010		/* PCD	Cache disable	*/
#define PG_U		0x020		/* U/S  User/Supervisor	*/
#define	PG_M		0x040		/* D	Dirty			*/
#define PG_A		0x060		/* A	Accessed		*/
#define	PG_PS		0x080		/* PS	Page size (0=4k,1=4M)	*/
#define	PG_PTE_PAT	0x080		/* PAT	PAT index		*/
#define	PG_G		0x100		/* G	Global			*/
#define	PG_W		0x200		/* "Wired" pseudoflag 	*/
#define	PG_SWAPM	0x400
#define	PG_FOD		0x600
#define PG_N		0x800 		/* Non-cacheable 		*/
#define	PG_PDE_PAT	0x1000		/* PAT	PAT index		*/
#define	PG_NX		(1ull<<63) 	/* No-execute 			*/

#define	PG_FRAME	0xfffff000

#define	PG_NOACC	0
#define	PG_KR		0x2000
#define	PG_KW		0x4000
#define	PG_URKR		0x6000
#define	PG_URKW		0x6000
#define	PG_UW		0x8000

#define	PG_FZERO	0
#define	PG_FTEXT	1
#define	PG_FMAX		(PG_FTEXT)

/*
 * Page Protection Exception bits
 */
#define PGEX_P		0x01		/* Protection violation vs. not present */
#define PGEX_W		0x02		/* during a Write cycle */
#define PGEX_U		0x04		/* access from User mode (UPL) */
#define PGEX_RSV	0x08		/* reserved PTE field is non-zero */
#define PGEX_I		0x10		/* during an instruction fetch */

#ifndef _LOCORE
/*
 * Get PDEs and PTEs for user/kernel address space
 */
/*
 * Pte related macros
 */
#define pmap_pde_v(pte)		    ((*(int *)pte & PG_V) != 0)

#define pmap_pte_pa(pte)		((*(int *)pte & PG_FRAME) != 0)
#define pmap_pte_ci(pte)		((*(int *)pte & PG_CI) != 0)
#define pmap_pte_w(pte)			((*(int *)pte & PG_W) != 0)
#define pmap_pte_m(pte)			((*(int *)pte & PG_M) != 0)
#define pmap_pte_u(pte)			((*(int *)pte & PG_A) != 0)
#define pmap_pte_v(pte)			((*(int *)pte & PG_V) != 0)

#define pmap_pte_set_w(pte, v) 		\
	if (v) {						\
		*(int *)(pte) |= PG_W;		\
	} else {						\
		*(int *)(pte) &= ~PG_W;		\
	}								\

#define pmap_pte_set_prot(pte, v) 	\
	if (v) {						\
		*(int *)(pte) |= PG_PROT;   \
	} else { 						\
		*(int *)(pte) &= ~PG_PROT;	\
	}								\

#endif /* !_LOCORE */
#endif /* _I386_PTE_H_ */
