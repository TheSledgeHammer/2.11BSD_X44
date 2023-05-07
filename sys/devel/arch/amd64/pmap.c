/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 * Copyright (c) 1994 John S. Dyson
 * All rights reserved.
 * Copyright (c) 1994 David Greenman
 * All rights reserved.
 * Copyright (c) 2003 Peter Wemm
 * All rights reserved.
 * Copyright (c) 2005-2010 Alan L. Cox <alc@cs.rice.edu>
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
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
 *	from:	@(#)pmap.c	7.7 (Berkeley)	5/12/91
 */
/*-
 * Copyright (c) 2003 Networks Associates Technology, Inc.
 * Copyright (c) 2014-2020 The FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Jake Burkholder,
 * Safeport Network Services, and Network Associates Laboratories, the
 * Security Research Division of Network Associates, Inc. under
 * DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA
 * CHATS research program.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arch/amd64/include/pte.h>
#include <devel/arch/amd64/pmap.h>

static u_int64_t KPTphys;					/* phys addr of kernel level 1 */
static u_int64_t KPDphys;					/* phys addr of kernel level 2 */
static u_int64_t KPDPphys;					/* phys addr of kernel level 3 */
u_int64_t 		 KPML4phys;					/* phys addr of kernel level 4 */
u_int64_t 		 KPML5phys;					/* phys addr of kernel level 5 */

extern const char la57_trampoline[], la57_trampoline_gdt_desc[], la57_trampoline_gdt[], la57_trampoline_end[];

static void
create_5_level_pagetable(firstaddr)
	vm_offset_t *firstaddr;
{
	pml5_entry_t *p5_p;
	int i;

	KPML5phys = allocpages(firstaddr, 1);				/* recursive PML5 map */

	/* And recursively map PML5 to itself in order to get PTmap */
	p5_p = (pml5_entry_t *)KPML5phys;
	p5_p[PDIR_SLOT_KERN] = KPML5phys;
	p5_p[PDIR_SLOT_KERN] |= PG_RW | PG_V | pg_nx;

	/* Connect the KVA slots up to the PML5 */
	for (i = 0; i < NKL5_MAX_ENTRIES; i++) {
		p5_p[PDIR_SLOT_KERNBASE + i] = KPML4phys + ptoa(i);
		p5_p[PDIR_SLOT_KERNBASE + i] |= PG_RW | PG_V;
	}
}

/*
 * FreeBSD Ported with a modifications
 */
static void
pmap_bootstrap_la57(firstaddr)
	vm_offset_t firstaddr;
{
	pml5_entry_t *v_pml5;
	pml4_entry_t *v_pml4;
	pdpt_entry_t *v_pdp;
	pd_entry_t *v_pd;
	pt_entry_t *v_pt;
	char *v_code;
	vm_offset_t *m_code, *temp;
	void (*la57_tramp)(uint64_t pml5);

	if ((cpu_stdext_feature2 & CPUID_STDEXT2_LA57) == 0) {
		return;
	}

	if (!la57) {
		return;
	}

	v_code = kmem_alloc(kernel_map, (la57_trampoline_end - la57_trampoline));

	create_5_level_pagetable(firstaddr);

	v_pml5 = (pml5_entry_t *)(KERNBASE + KPML5phys);

	/*
	 * Map m_code 1:1, it appears below 4G in KVA due to physical
	 * address being below 4G.  Since kernel KVA is in upper half,
	 * the pml4e should be zero and free for temporary use.
	 */
	v_pml4 = pmap_extract(kernel_pmap, (vm_offset_t)KPML4phys);
	v_pdp = pmap_extract(kernel_pmap, (vm_offset_t)KPDPphys);
	v_pd = pmap_extract(kernel_pmap, (vm_offset_t)KPDphys);
	v_pt = pmap_extract(kernel_pmap, (vm_offset_t)KPTphys);

	v_pml4[PL4_E(m_code)] = v_pdp | PG_V | PG_RW | PG_A | PG_M;
	v_pdp[PL3_E(m_code)] = v_pd | PG_V | PG_RW | PG_A | PG_M;
	v_pd[PL2_E(m_code)] = v_pt | PG_V | PG_RW | PG_A | PG_M;
	v_pt[PL1_E(m_code)] = m_code | PG_V | PG_RW | PG_A | PG_M;

	/*
	 * Add pml5 entry at top of KVA pointing to existing pml4 table,
	 * entering all existing kernel mappings into level 5 table.
	 */
	v_pml5[PL5_E(UPT_MAX_ADDRESS)] = KPML4phys | PG_V | PG_RW | PG_A | PG_M | pg_g;

	/*
	 * Add pml5 entry for 1:1 trampoline mapping after LA57 is turned on.
	 */
	v_pml5[PL5_E(m_code)] = v_pml4 | PG_V | PG_RW | PG_A | PG_M;
	v_pml4[PL4_E(m_code)] = v_pdp | PG_V | PG_RW | PG_A | PG_M;

	/*
	 * Copy and call the 48->57 trampoline, hope we return there, alive.
	 */
	bcopy(la57_trampoline, v_code, (la57_trampoline_end - la57_trampoline));

	*m_code = pmap_extract(kernel_pmap, v_code);

	m_code = (v_code + 2 + (la57_trampoline_gdt_desc - la57_trampoline));
	temp = (la57_trampoline_gdt - la57_trampoline) + m_code;
	m_code = temp;
	temp = NULL;
	la57_tramp = (void (*)(uint64_t))pmap_extract(kernel_pmap, m_code);
	invlpg((vm_offset_t)la57_tramp);
	la57_tramp(KPML5phys);


	/*
	 * Now unmap the trampoline, and free the pages.
	 * Clear pml5 entry used for 1:1 trampoline mapping.
	 */
	//pte_clear(&v_pml5[PL5_E(m_code)]);
	invlpg(m_code);

	/*
	 * Recursively map PML5 to itself in order to get PTmap and
	 * PDmap.
	 */
	v_pml5[PDIR_SLOT_KERN] = KPML5phys | PG_RW | PG_V | pg_nx;
	kernel_pmap->pm_pml4 = (pml4_entry_t *)v_pml4;
	kernel_pmap->pm_pml5 = (pml5_entry_t *)v_pml5;
}
