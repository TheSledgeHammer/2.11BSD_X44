/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Copyright (c) 2018 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
 *
 * Portions of this software were developed by
 * Konstantin Belousov <kib@FreeBSD.org> under sponsorship from
 * the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 *	from: hp300: @(#)pmap.h	7.2 (Berkeley) 12/16/90
 *	from: @(#)pmap.h	7.4 (Berkeley) 5/12/91
 * $FreeBSD$
 */

#ifndef _I386_PMAP_REG_H_
#define _I386_PMAP_REG_H_

/*
 * 386 page table entry and page table directory
 * W.Jolitz, 8/89
 */

#ifndef NKPDE
#define NKPDE				(KVA_PAGES)								/* number of page tables/pde's */
#endif

/*
 * One page directory, shared between
 * kernel and user modes.
 */
#define I386_PAGE_SIZE		NBPG
#define I386_PDR_SIZE		NBPDR

#define I386_KPDES			8 										/* KPT page directory size */
#define I386_UPDES			(NBPDR/sizeof(struct pde) - I386_KPDES) /* UPT page directory size */

#define	UPTDI				0x3f6									/* ptd entry for u./kernel&user stack */
#define	PTDPTDI				0x3f7									/* ptd entry that points to ptd! */
#define	KPTDI_FIRST			0x3f8									/* start of kernel virtual pde's */
#define	KPTDI_LAST			0x3fA									/* last of kernel virtual pde's */

//#define	KPTDI				0										/* start of kernel virtual pde's */

/*
 * Input / Output Memory Physical Addresses
 */
/*
 * XXX doesn't really belong here I guess...
 */
#define ISA_HOLE_START    	0xa0000
#define ISA_HOLE_END    	0x100000
#define ISA_HOLE_LENGTH 	(ISA_HOLE_END-ISA_HOLE_START)

#ifndef PMAP_PAE_COMP
/* NOPAE Constants */
#define	PD_SHIFT			22
//#define	PG_FRAME		(~PAGE_MASK)
#define	PG_PS_FRAME			(0xffc00000)							/* PD_MASK_NOPAE */

#define	NTRPPTD				1
#define	LOWPTDI				1
#define	KERNPTDI			2

#define NPGPTD				1
#define NPGPTD_SHIFT		10
#undef	PDRSHIFT
#define	PDRSHIFT			PD_SHIFT
#undef	NBPDR
#define NBPDR				(1 << PD_SHIFT)							/* bytes/page dir */

#define KVA_PAGES			(256*4)

#ifndef NKPT
#define	NKPT				30
#endif

typedef uint32_t	 		pd_entry_t;
typedef uint32_t 			pt_entry_t;

#else
#define	PMAP_PAE_COMP

/* PAE Constants  */
#define	PD_SHIFT			21									/* LOG2(NBPDR) */
#define	PG_FRAME			(0x000ffffffffff000ull)
#define	PG_PS_FRAME			(0x000fffffffe00000ull)				/* PD_MASK_PAE */

#define	NTRPPTD				2									/* Number of PTDs for trampoline mapping */
#define	LOWPTDI				2									/* low memory map pde */
#define	KERNPTDI			4									/* start of kernel text pde */

#define NPGPTD				4									/* Num of pages for page directory */
#define NPGPTD_SHIFT		9
#undef	PDRSHIFT
#define	PDRSHIFT			PD_SHIFT
#undef	NBPDR
#define NBPDR				(1 << PD_SHIFT)						/* bytes/page dir */

/*
 * Size of Kernel address space.  This is the number of page table pages
 * (4MB each) to use for the kernel.  256 pages == 1 Gigabyte.
 * This **MUST** be a multiple of 4 (eg: 252, 256, 260, etc).
 * For PAE, the page table page unit size is 2MB.  This means that 512 pages
 * is 1 Gigabyte.  Double everything.  It must be a multiple of 8 for PAE.
 */
#define KVA_PAGES			(512*4)

/*
 * The initial number of kernel page table pages that are constructed
 * by pmap_cold() must be sufficient to map vm_page_array.  That number can
 * be calculated as follows:
 *     			max_phys / PAGE_SIZE * sizeof(struct vm_page) / NBPDR
 * PAE:      	max_phys 16G, sizeof(vm_page) 76, NBPDR 2M, 152 page table pages.
 * PAE_TABLES: 	max_phys 4G,  sizeof(vm_page) 68, NBPDR 2M, 36 page table pages.
 * Non-PAE:  	max_phys 4G,  sizeof(vm_page) 68, NBPDR 4M, 18 page table pages.
 */
#ifndef NKPT
#define	NKPT				240
#endif

typedef uint64_t 			pdpt_entry_t;
typedef uint64_t 			pd_entry_t;
typedef uint64_t 			pt_entry_t;
#endif

#define	PT_ENTRY_NULL		((pt_entry_t) 0)
#define	PD_ENTRY_NULL		((pd_entry_t) 0)

#ifdef _KERNEL
extern pt_entry_t 			PTmap[], APTmap[];
extern pd_entry_t 			PTD[], APTD[];
extern pd_entry_t 			PTDpde[], APTDpde[];
extern pd_entry_t 			*IdlePTD;
extern pt_entry_t 			*KPTmap;
extern pdpt_entry_t 		*IdlePDPT;
#endif	/* _KERNEL */
#endif /* _I386_PMAP_REG_H_ */
