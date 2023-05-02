/*	$NetBSD: pmap.h,v 1.22 2008/10/26 00:08:15 mrg Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * All rights reserved.
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
 *    must display the following acknowledgment:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AMD64_PMAP_H_
#define _AMD64_PMAP_H_

/*
 * Place below in amd64 param.h (i.e. NPML5EPG)
 */
/* Size of the level 5 page-map level-5 table units */
#define	NPML5EPG		(NBPG/(sizeof(pml5_entry_t)))

/* Implementing 5-Level Page Tables */

#define L1_SHIFT		12
#define	L2_SHIFT		21
#define	L3_SHIFT		30
#define	L4_SHIFT		39
#define L5_SHIFT		48

#define	NBPD_L1			(1ULL << L1_SHIFT) 		/* # bytes mapped by L1 ent (4K) */
#define	NBPD_L2			(1ULL << L2_SHIFT) 		/* # bytes mapped by L2 ent (2MB) */
#define	NBPD_L3			(1ULL << L3_SHIFT) 		/* # bytes mapped by L3 ent (1G) */
#define	NBPD_L4			(1ULL << L4_SHIFT) 		/* # bytes mapped by L4 ent (512G) */
#define	NBPD_L5			(1ULL << L5_SHIFT)		/* # bytes mapped by L5 ent (256T) */

#define L5_MASK			0x01fe000000000000		/* 127PB */
#define L4_MASK			0x0000ff8000000000		/* 255TB */
#define L3_MASK			0x0000007fc0000000		/* 511GB */
#define L2_MASK			0x000000003fe00000		/* 1GB */
#define L1_MASK			0x00000000001ff000		/* 2MB */

#define L5_FRAME		L5_MASK
#define L4_FRAME		(L5_FRAME|L4_MASK)
#define L3_FRAME		(L4_FRAME|L3_MASK)
#define L2_FRAME		(L3_FRAME|L2_MASK)
#define L1_FRAME		(L2_FRAME|L1_MASK)

typedef uint64_t 		pt_entry_t;				/* PTE  (L1) */
typedef uint64_t 		pd_entry_t;				/* PDE  (L2) */
typedef uint64_t 		pdp_entry_t;			/* PDP  (L3) */
typedef uint64_t 		pml4_entry_t;			/* PML4 (L4) */
typedef uint64_t 		pml5_entry_t;			/* PML5 (L5) */

#define L1_BASE			((pt_entry_t *)(L4_SLOT_PTE * NBPD_L5))
#define L2_BASE 		((pd_entry_t *)((char *)L1_BASE + L4_SLOT_PTE * NBPD_L4))
#define L3_BASE 		((pdp_entry_t *)((char *)L2_BASE + L4_SLOT_PTE * NBPD_L3))
#define L4_BASE 		((pml4_entry_t *)((char *)L3_BASE + L4_SLOT_PTE * NBPD_L2))
#define L5_BASE			((pml5_entry_t *)((char *)L4_BASE + L4_SLOT_PTE * NBPD_L1))

#define AL1_BASE		((pt_entry_t *)(VA_SIGN_NEG((L4_SLOT_APTE * NBPD_L5))))
#define AL2_BASE 		((pd_entry_t *)((char *)AL1_BASE + L4_SLOT_PTE * NBPD_L4))
#define AL3_BASE 		((pdp_entry_t *)((char *)AL2_BASE + L4_SLOT_PTE * NBPD_L3))
#define AL4_BASE 		((pml4_entry_t *)((char *)AL3_BASE + L4_SLOT_PTE * NBPD_L2))
#define AL5_BASE		((pml5_entry_t *)((char *)AL4_BASE + L4_SLOT_PTE * NBPD_L1))

#define NKL5_MAX_ENTRIES	(unsigned long)1
#define NKL4_MAX_ENTRIES	(unsigned long)(NKL5_MAX_ENTRIES * 512)

#define NKL5_START_ENTRIES	0

#define PL5_I(VA)		(((VA_SIGN_POS(VA)) & L5_FRAME) >> L5_SHIFT)

#define PTP_MASK_INITIALIZER	{ L1_FRAME, L2_FRAME, L3_FRAME, L4_FRAME, L5_FRAME }
#define PTP_SHIFT_INITIALIZER	{ L1_SHIFT, L2_SHIFT, L3_SHIFT, L4_SHIFT, L5_SHIFT }
#define NBPD_INITIALIZER	{ NBPD_L1, NBPD_L2, NBPD_L3, NBPD_L4, NBPD_L5 }
#define PDES_INITIALIZER	{ L2_BASE, L3_BASE, L4_BASE, L5_BASE }
#define APDES_INITIALIZER	{ AL2_BASE, AL3_BASE, AL4_BASE, AL5_BASE }

#define NKPTP_INITIALIZER	{ NKL1_START_ENTRIES, NKL2_START_ENTRIES, NKL3_START_ENTRIES, NKL4_START_ENTRIES, NKL5_START_ENTRIES }
#define NKPTPMAX_INITIALIZER	{ NKL1_MAX_ENTRIES, NKL2_MAX_ENTRIES, NKL3_MAX_ENTRIES, NKL4_MAX_ENTRIES, NKL5_MAX_ENTRIES }

#define PTP_LEVELS		5

struct pmap {
	pml5_entry_t   			*pm_pml5;       /* KVA of page map level 5 (top level) */
};

#define	PML5_ENTRY_NULL		((pml5_entry_t)0)

#endif /* _AMD64_PMAP_H_ */
