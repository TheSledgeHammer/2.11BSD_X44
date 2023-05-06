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

extern int la57; /* amd64 md_var.h */

extern const char la57_trampoline[], la57_trampoline_gdt_desc[], la57_trampoline_gdt[], la57_trampoline_end[];

/*
 * Copy and call the 48->57 trampoline, hope we return there, alive.
 */
static void
pmap_la57_tramp(void)
{
	char *v_code;
	vm_offset_t *m_code, *temp;
	void (*la57_tramp)(uint64_t pml5);

	v_code = kmem_alloc(kernel_map, (la57_trampoline_end - la57_trampoline));

	bcopy(la57_trampoline, v_code, (la57_trampoline_end - la57_trampoline));

	*m_code = pmap_extract(kernel_pmap, v_code);

	m_code = (v_code + 2 + (la57_trampoline_gdt_desc - la57_trampoline));
	temp = (la57_trampoline_gdt - la57_trampoline) + m_code;
	m_code = temp;
	la57_tramp = (void (*)(uint64_t))pmap_extract(kernel_pmap, m_code);
	invlpg((vm_offset_t)la57_tramp);
	la57_tramp(KPML5phys);
}

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

static void
pmap_bootstrap_la57(firstaddr)
	vm_offset_t firstaddr;
{
	pml5_entry_t *v_pml5;
	pml4_entry_t *v_pml4;
	pdpt_entry_t *v_pdp;
	pd_entry_t *v_pd;
	pt_entry_t *v_pt;

	void (*la57_tramp)(uint64_t pml5);

	if ((cpu_stdext_feature2 & CPUID_STDEXT2_LA57) == 0) {
		return;
	}

	if (!la57) {
		return;
	}

	create_5_level_pagetable(firstaddr);

	kernel_pmap->pm_pml5 = (pml5_entry_t *)(KERNBASE + KPML5phys);

	pmap_la57_tramp();
}

/*
 * FreeBSD Ported with a few modifications (Not correct)
 * - Also relies on sysinit. which we do not implement.
 */
static void
pmap_bootstrap_la57(void *arg/* __unused*/)
{
	pml5_entry_t *v_pml5;
	pml4_entry_t *v_pml4;
	pdpt_entry_t *v_pdp;
	pd_entry_t *v_pd;
	pt_entry_t *v_pt;
	vm_offset_t firstaddr;

	firstaddr = (vm_offset_t)arg;
	void (*la57_tramp)(uint64_t pml5);

	if ((cpu_stdext_feature2 & CPUID_STDEXT2_LA57) == 0) {
		return;
	}

	if (!la57) {
		return;
	}

	v_pml5 = (pml5_entry_t *)kmem_alloc(kernel_map, sizeof(pml5_entry_t));
	v_pml4 = (pml4_entry_t *)kmem_alloc(kernel_map, sizeof(pml4_entry_t));
	v_pdp = (pdpt_entry_t *)kmem_alloc(kernel_map, sizeof(pdp_entry_t));
	v_pd = (pd_entry_t *)kmem_alloc(kernel_map, sizeof(pd_entry_t));
	v_pt = (pt_entry_t *)kmem_alloc(kernel_map, sizeof(pt_entry_t));

	kernel_pmap->pm_pml5 = (pml5_entry_t *)(KERNBASE + KPML5phys);

	/*
	 * Map m_code 1:1, it appears below 4G in KVA due to physical
	 * address being below 4G.  Since kernel KVA is in upper half,
	 * the pml4e should be zero and free for temporary use.
	 */
	v_pml4 = pmap_extract(kernel_pmap, (vm_offset_t)KPML4phys) | PG_V | PG_RW | PG_A | PG_M;
	v_pdp = pmap_extract(kernel_pmap, (vm_offset_t)KPDPphys) | PG_V | PG_RW | PG_A | PG_M;
	v_pd = pmap_extract(kernel_pmap, (vm_offset_t)KPDphys) | PG_V | PG_RW | PG_A | PG_M;
	v_pt = pmap_extract(kernel_pmap, (vm_offset_t)KPTphys) | PG_V | PG_RW | PG_A | PG_M;

	/*
	 * Add pml5 entry at top of KVA pointing to existing pml4 table,
	 * entering all existing kernel mappings into level 5 table.
	 */
	v_pml5[pmap_pml5e_index(UPT_MAX_ADDRESS)] = KPML4phys | PG_V | PG_RW | PG_A | PG_M | pg_g;

	/*
	 * Add pml5 entry for 1:1 trampoline mapping after LA57 is turned on.
	 */
	v_pml5[pmap_pml5e_index(VM_PAGE_TO_PHYS(m_code))] = VM_PAGE_TO_PHYS(m_pml4) | PG_V | PG_RW | PG_A | PG_M;
	v_pml4[pmap_pml4e_index(VM_PAGE_TO_PHYS(m_code))] = VM_PAGE_TO_PHYS(m_pdp) | PG_V | PG_RW | PG_A | PG_M;

	pmap_la57_tramp();

	v_pml5[PML5PML5I] = KPML5phys | PG_RW | PG_V | pg_nx;
}
