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

#include <devel/arch/amd64/vmparam.h>
#include <devel/arch/amd64/pmap.h>

uint64_t 		KPML5phys;					/* phys addr of kernel level 5 */
pml5_entry_t 	*IdlePML5;

#define	CR4_LA57                0x00001000	/* Enable 5-level paging */
/*
 * CPUID instruction 7 Structured Extended Features, leaf 0 ecx info
 */
#define	CPUID_STDEXT2_LA57		0x00010000

static void
create_5_level_pagetable(firstaddr)
	vm_offset_t *firstaddr;
{
	int i;

	KPML5phys = allocpages(1, &physfree);			/* recursive PML5 map */

	/* And recursively map PML5 to itself in order to get PTmap */
	IdlePML5 = (pml5_entry_t *)KPML5phys;
	IdlePML5[PDIR_SLOT_KERN] = KPML5phys;
	IdlePML5[PDIR_SLOT_KERN] |= PG_RW | PG_V | pg_nx;

	/* Connect the KVA slots up to the PML5 */
	for (i = 0; i < NKL5_MAX_ENTRIES; i++) {
		IdlePML5[PDIR_SLOT_KERNBASE + i] = KPML4phys + ptoa(i);
		IdlePML5[PDIR_SLOT_KERNBASE + i] |= PG_RW | PG_V;
	}
}

extern const char la57_trampoline[], la57_trampoline_gdt_desc[], la57_trampoline_gdt[], la57_trampoline_end[];

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


/* Direct Map */

static int ndmpdp;
static vm_offset_t dmaplimit;

static uint64_t	DMPDphys;	/* phys addr of direct mapped level 2 */
static uint64_t	DMPDPTphys;	/* phys addr of direct mapped level 3 */

void
pmap_direct_map(void)
{
	int i;

	ndmpdp = (ptoa(Maxmem) + NBPD_L3 - 1) >> L2_SHIFT;
	if (ndmpdp < 4) {		/* Minimum 4GB of dirmap */
		ndmpdp = 4;
	}

	DMPDPTphys = allocpages(L4_DMAP_SLOTS, &physfree);
	DMPDphys = allocpages(ndmpdp, &physfree);
	dmaplimit = (vm_offset_t)ndmpdp << L3_SHIFT;

	/* Now set up the direct map space using 2MB pages */
	for (i = 0; i < NPDEPG * ndmpdp; i++) {
		((pd_entry_t *)DMPDphys)[i] = (vm_offset_t)i << L3_SHIFT;
		((pd_entry_t *)DMPDphys)[i] |= PG_RW | PG_V | PG_PS | PG_G;
	}

	/* And the direct map space's PDP */
	for (i = 0; i < ndmpdp; i++) {
		((pdpt_entry_t *)DMPDPTphys)[i] = DMPDphys + (i << L1_SHIFT);
		((pdpt_entry_t *)DMPDPTphys)[i] |= PG_RW | PG_V | PG_U;
	}

	/* Connect the Direct Map slot up to the PML4 */
	((pdpt_entry_t *)KPML4phys)[PDIR_SLOT_DIRECT] = DMPDPTphys;
	((pdpt_entry_t *)KPML4phys)[PDIR_SLOT_DIRECT] |= PG_RW | PG_V | PG_U;
}

/* FreeBSD / DragonFlyBSD compatibility */

pml4_entry_t *
pmap_pml4e(pmap_t pmap, vm_offset_t va)
{
	pml4_entry_t *pml4;

	pml4 = pmap_table(pmap, va, 4);
	return (&pml4[PL4_E(va)]);
}

pdpt_entry_t *
pmap_pdpt(pmap_t pmap, vm_offset_t va)
{
	pml4_entry_t *pml4e;
	pdpt_entry_t *pdpt;

	pml4e = pmap_pml4e(pmap, va);
	if (pml4e == NULL) {
		return (NULL);
	}
	pdpt = (pdpt_entry_t *)PHYS_TO_DMAP(*pml4e & PG_FRAME);
	return (&pdpt[PL3_E(va)]);
}

pd_entry_t *
pmap_pde(pmap_t pmap, vm_offset_t va)
{
	pdpt_entry_t *pdpt;
	pd_entry_t *pde;

	pdpt = pmap_pdpt(pmap, va);
	if (pdpt == NULL) {
		return (NULL);
	}
	pde = (pd_entry_t *)PHYS_TO_DMAP(*pdpt & PG_FRAME);
	return (&pde[PL2_E(va)]);
}

pt_entry_t *
pmap_pte(pmap_t pmap, vm_offset_t va)
{
	pd_entry_t *pde;
	pt_entry_t *pte;

	pde = pmap_pde(pmap, va);
	if (pde == NULL) {
		return (NULL);
	}
	if ((*pde & PG_PS) != 0) {
		return ((pt_entry_t *)pde);
	}
	pte = (pt_entry_t *)PHYS_TO_DMAP(*pde & PG_FRAME);
	return (&pte[PL1_E(va)]);
}

pt_entry_t *
pmap_pte_pde(pmap_t pmap, vm_offset_t va, pd_entry_t *ptepde)
{
	pd_entry_t *pde;
	pt_entry_t *pte;

	pde = pmap_pde(pmap, va);
	if (pde == NULL) {
		return NULL;
	}
	*ptepde = *pde;
	if ((*pde & PG_PS) != 0) {	/* compat with i386 pmap_pte() */
		return ((pt_entry_t *)pde);
	}
	pte = (pt_entry_t *)PHYS_TO_DMAP(*pde & PG_FRAME);
	return (&pte[PL1_E(va)]);
}
