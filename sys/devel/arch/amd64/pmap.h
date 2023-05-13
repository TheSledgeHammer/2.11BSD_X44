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

#define L4ETOL5E_SHIFT  (PTP_SHIFT * 2)-3)

#define l4etol4e(idx)   (l4tol4(idx) & (L4_REALMASK >> L4_SHIFT))
#define l4etol5e(idx)   (((l4tol4(idx) + l4tol5(idx)) + L4ETOL5E_SHIFT) & ((1UL << PTP_SHIFT)-1))

#define PL5_I(VA)		(((VA_SIGN_POS(VA)) & L5_FRAME) >> L5_SHIFT)

/*
 * Return a non-clipped indexes for a given VA, which are page table
 * pages indexes at the corresponding level.
 */
#define PL1_E(va)       ((((va)) >> L1_SHIFT) & ((1UL << PTP_SHIFT)-1))
#define PL2_E(va)       ((((va)) >> L2_SHIFT) & ((1UL << PTP_SHIFT)-1))
#define PL3_E(va)       ((((va)) >> L3_SHIFT) & ((1UL << PTP_SHIFT)-1))
#define PL4_E(va)       ((((va)) >> L4_SHIFT) & ((1UL << PTP_SHIFT)-1))
#define PL5_E(va)       ((((va)) >> L5_SHIFT) & ((1UL << PTP_SHIFT)-1))
#define PL_E(va, lvl)   ((((va)) >> ptp_shifts[(lvl)-1]) & ((1UL << PTP_SHIFT)-1))

#define PTP_LEVELS_LA57	5						/* 5 level page tables (la57 enabled) */
#define PTP_LEVELS_LA48 4						/* 4 level page tables (la57 disabled) */

#define PTP_MASK_LA57_INITIALIZER	{ L1_FRAME, L2_FRAME, L3_FRAME, L4_FRAME, L5_FRAME }
#define PTP_SHIFT_LA57_INITIALIZER	{ L1_SHIFT, L2_SHIFT, L3_SHIFT, L4_SHIFT, L5_SHIFT }

#define PTP_MASK_LA48_INITIALIZER	{ L1_FRAME, L2_FRAME, L3_FRAME, L4_FRAME }
#define PTP_SHIFT_LA48_INITIALIZER	{ L1_SHIFT, L2_SHIFT, L3_SHIFT, L4_SHIFT }

extern int la57; /* amd64 md_var.h */

extern uint64_t KPML5phys;	/* physical address of kernel level 5 */

#endif /* _AMD64_PMAP_H_ */
