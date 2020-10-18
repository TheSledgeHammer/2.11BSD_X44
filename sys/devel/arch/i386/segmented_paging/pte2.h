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

#ifndef SYS_DEVEL_ARCH_I386_PTE_H_
#define SYS_DEVEL_ARCH_I386_PTE_H_

/* pte.h */
/* segment table */
struct ste {
unsigned int
		sg_v:2,			/* valid bits */
		sg_prot:1,		/* write protect bit */
		sg_mbz1:2,		/* reserved, must be zero */
		sg_u:1,			/* hardware modified (dirty) bit */
		:8,				/* reserved at 0 */
		:1,				/* reserved at 1 */
		sg_pfnum:20,	/* page table frame number of pde's */
		sg_ptaddr:24;	/* page table page addr */
};

struct pde {
unsigned int
		pd_v:1,			/* valid bit */
		pd_prot:2,		/* access control */
		pd_mbz1:2,		/* reserved, must be zero */
		pd_u:1,			/* hardware maintained 'used' bit */
		:1,				/* not used */
		pd_mbz2:2,		/* reserved, must be zero */
		:3,				/* reserved for software */
		pd_pfnum:20;	/* physical page frame number of pte's*/
};

struct pte {
unsigned int
		pg_v:1,			/* valid bit */
		pg_prot:2,		/* access control */
		pg_mbz1:2,		/* reserved, must be zero */
		pg_u:1,			/* hardware maintained 'used' bit */
		pg_m:1,			/* hardware maintained modified bit */
		pg_mbz2:2,		/* reserved, must be zero */
		pg_w:1,			/* software, wired down page */
		pg_fod:1,		/* is fill on demand (= 0) */
		:1,				/* must write back to swap (unused) */
		pg_nc:1,		/* 'uncacheable page' bit */
		pg_pfnum:20;	/* physical page frame number */
};

#define	SG_V			0x00000002	/* segment is valid */
#define	SG_NV			0x00000000
#define	SG_PROT			0x00000004	/* access protection mask */
#define	SG_RO			0x00000004
#define	SG_RW			0x00000000
#define	SG_U			0x00000008

#define	SG_FRAME		0xfffff000
#define	SG_IMASK		0xffc00000
#define	SG_ISHIFT		22
#define	SG_PMASK		0x003ff000
#define	SG_PSHIFT		12

#define	SG4_LEV1SIZE	128
#define	SG4_LEV2SIZE	128
#define	SG4_LEV3SIZE	64

#define	PG_V			0x00000001
#define	PG_RO			0x00000000
#define	PG_RW			0x00000002
#define	PG_u			0x00000004
#define	PG_PROT			0x00000006 /* all protection bits . */
#define	PG_W			0x00000200
#define	PG_SWAPM		0x00000400
#define	PG_FOD			0x00000600
#define PG_N			0x00000800 /* Non-cacheable */
#define	PG_M			0x00000040
#define PG_U			0x00000020
#define PG_A			0x00000060
#define	PG_FRAME		0xfffff000

#define	PG_NOACC		0
#define	PG_KR			0x00000000
#define	PG_KW			0x00000002
#define	PG_URKR			0x00000004
#define	PG_URKW			0x00000004
#define	PG_UW			0x00000006

#define	PG_FZERO		0
#define	PG_FTEXT		1
#define	PG_FMAX			(PG_FTEXT)

/*
 * Page Protection Exception bits
 */
#define PGEX_P			0x01	/* Protection violation vs. not present */
#define PGEX_W			0x02	/* during a Write cycle */
#define PGEX_U			0x04	/* access from User mode (UPL) */
#define PGEX_RSV		0x08	/* reserved PTE field is non-zero */
#define PGEX_I			0x10	/* during an instruction fetch */

/*
 * Pte related macros
 */
#define	dirty(pte)	((pte)->pg_m)

extern struct pte	*CMAP1, *CMAP2;

#ifdef KERNEL
extern struct pte	PTmap[], APTmap[], Upte;
extern struct pde	PTD[], APTD[], PTDpde, APTDpde, Upde;
extern struct ste	STE[], ASTE[], STEptd, ASTEptd, Uste;
extern	pt_entry_t	*Sysmap;
extern	st_entry_t	*Sysseg;

extern int			IdlePTD;	/* physical address of "Idle" state directory */
#endif
#ifndef LOCORE
#ifdef KERNEL
/* utilities defined in pmap.c */
extern	struct ste *Sysseg;
extern	struct pte *Sysmap;
#endif
#endif
#endif /* SYS_DEVEL_ARCH_I386_PTE_H_ */
