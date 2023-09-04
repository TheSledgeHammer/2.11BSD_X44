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

/*
 * CPUID instruction 7 Structured Extended Features, leaf 0 ecx info
 */
#define	CPUID_STDEXT2_LA57		0x00010000
#define	CR4_LA57                0x00001000	/* Enable 5-level paging */

uint64_t 		KPML5phys;					/* phys addr of kernel level 5 */
pml5_entry_t 	*IdlePML5;

extern const char la57_trampoline[], la57_trampoline_gdt_desc[], la57_trampoline_gdt[], la57_trampoline_end[];

static __inline bool_t
pmap_emulate_ad_bits(pmap_t pmap)
{
	return ((pmap->pm_flags & PMAP_EMULATE_AD_BITS) != 0);
}

static __inline pt_entry_t
pmap_valid_bit(pmap_t pmap)
{
	pt_entry_t mask;

	switch (pmap->pm_type) {
	case PT_X86:
	case PT_RVI:
		mask = PG_V;
		break;
	case PT_EPT:
		if (pmap_emulate_ad_bits(pmap))
			mask = EPT_PG_EMUL_V;
		else
			mask = EPT_PG_READ;
		break;
	default:
		panic("pmap_valid_bit: invalid pm_type %d", pmap->pm_type);
	}

	return (mask);
}

static bool_t
pmap_is_la57(pmap)
	pmap_t pmap;
{
	if (pmap->pm_type == PT_X86) {
		return (la57);
	}
	return (FALSE);		/* XXXKIB handle EPT */
}

static void
pmap_cold_la57(physfree)
	vm_offset_t		physfree;
{
	uint64_t cr4;

    /* Check if CR4_LA57 is present */
	cr4 = rcr4();
	cr4 |= CR4_LA57;
	if (cr4 == 0) {
		KPML5phys = NULL;
		return;
	}

	/* Allocated Page Table Level 5 */
	KPML5phys = allocpages(1, &physfree);
}

static void
create_pagetables_la57(firstaddr)
	vm_offset_t *firstaddr;
{
	int i;

	if (KPML5phys != NULL) {
		/* And recursively map PML5 to itself in order to get PTmap */
		IdlePML5 = (pml5_entry_t *)KPML5phys;
		IdlePML5[PDIR_SLOT_KERN] = KPML5phys;
		IdlePML5[PDIR_SLOT_KERN] |= PG_RW | PG_V | pg_nx;

		/* Connect the KVA slots up to the PML5 */
		for (i = 0; i < NKL5_MAX_ENTRIES; i++) {
			IdlePML5[PDIR_SLOT_KERNBASE + i] = IdlePML4 + ptoa(i);
			IdlePML5[PDIR_SLOT_KERNBASE + i] |= PG_RW | PG_V;
		}
	}
}

static void
pmap_enable_la57(cr4)
	uint64_t cr4;
{
	cr4 = rcr4();
	cr4 |= CR4_LA57;
	if (cr4 != 0) {
		load_cr4(cr4);
		load_cr3(IdlePML5);
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
	pdpt_entry_t *v_pdpt;
	pd_entry_t *v_pde;
	pt_entry_t *v_pte;
	struct region_descriptor region;
	char *v_code;
	vm_offset_t *m_code, *temp;
	void (*la57_tramp)(uint64_t pml5);

	if ((cpu_stdext_feature2 & CPUID_STDEXT2_LA57) == 0) {
		return;
	}

	if (!la57) {
		return;
	}

	setregion(&region, gdt, NGDT * sizeof(gdt)-1);

	v_code = kmem_alloc(kernel_map, (la57_trampoline_end - la57_trampoline));

	create_pagetables_la57(firstaddr);

	/*
	 * Map m_code 1:1, it appears below 4G in KVA due to physical
	 * address being below 4G.  Since kernel KVA is in upper half,
	 * the pml4e should be zero and free for temporary use.
	 */
	v_pml4 = pmap_extract(kernel_pmap, (vm_offset_t)IdlePML4);
	v_pdpt = pmap_extract(kernel_pmap, (vm_offset_t)IdlePDPT);
	v_pde = pmap_extract(kernel_pmap, (vm_offset_t)IdlePTD);
	v_pte = pmap_extract(kernel_pmap, (vm_offset_t)KPTmap);

	v_pml4[PL4_E(v_code)] = v_pdpt | PG_V | PG_RW | PG_A | PG_M;
	v_pdpt[PL3_E(v_code)] = v_pde | PG_V | PG_RW | PG_A | PG_M;
	v_pde[PL2_E(v_code)] = v_pte | PG_V | PG_RW | PG_A | PG_M;
	v_pte[PL1_E(v_code)] = v_code | PG_V | PG_RW | PG_A | PG_M;

	/*
	 * Add pml5 entry at top of KVA pointing to existing pml4 table,
	 * entering all existing kernel mappings into level 5 table.
	 */
	v_pml5[l4etol5e(PL4_E(UPT_MAX_ADDRESS))] = IdlePML4 | PG_V | PG_RW | PG_A | PG_M | pg_g;

	/*
	 * Add pml5 entry for 1:1 trampoline mapping after LA57 is turned on.
	 */
	v_pml5[l4etol5e(PL4_E(m_code))] = v_pml4 | PG_V | PG_RW | PG_A | PG_M;
	v_pml4[PL4_E(m_code)] = v_pdpt | PG_V | PG_RW | PG_A | PG_M;

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
	 * gdt was necessary reset, switch back to our gdt.
	 */
	lgdt(&region);
	wrmsr(MSR_GSBASE, (uint64_t)&__pcpu[0]);
	load_ds(_udatasel);
	load_es(_udatasel);
	load_fs(_ufssel);
	//ssdtosyssd(&gdt_segs[GPROC0_SEL], (struct system_segment_descriptor *)&__pcpu[0].pc_gdt[GPROC0_SEL]);
	//ltr(GSEL(GPROC0_SEL, SEL_KPL));

	/*
	 * Now unmap the trampoline, and free the pages.
	 * Clear pml5 entry used for 1:1 trampoline mapping.
	 */
	//pte_clear(&v_pml5[l4etol5e(PL4_E(m_code))]);
	invlpg(m_code);

	/*
	 * Recursively map PML5 to itself in order to get PTmap and
	 * PDmap.
	 */
	v_pml5[PDIR_SLOT_KERN] = IdlePML5 | PG_RW | PG_V | pg_nx;

	IdlePML5 = (pml5_entry_t *)v_pml5;
	IdlePML4 = (pml4_entry_t *)v_pml4;
	IdlePDPT = (pdpt_entry_t *)v_pdpt;
	IdlePTD = (pd_entry_t *)v_pde;
	KPTmap = (pt_entry_t *)v_pte;

	kernel_pmap->pm_pdir = (pd_entry_t *)(KERNBASE + IdlePTD);
	kernel_pmap->pm_pml4 = (pml4_entry_t *)(KERNBASE + IdlePML4);
	kernel_pmap->pm_pml5 = (pml5_entry_t *)(KERNBASE + IdlePML5);
	kernel_pmap->pm_pdirpa[0] = (vm_offset_t)IdlePTD;
}

static void
pmap_bootstrap_la57_v2(firstaddr)
	vm_offset_t firstaddr;
{
	pml5_entry_t *v_pml5;
	pml4_entry_t *v_pml4;
	pdpt_entry_t *v_pdpt;
	pd_entry_t *v_pde;
	pt_entry_t *v_pte;
	vm_offset_t m_pml5, m_pml4, m_pdpt, m_pde, m_pte;
	char *v_code;
	vm_offset_t *m_code, *temp;
	struct region_descriptor region;
	void (*la57_tramp)(uint64_t pml5);

	if ((cpu_stdext_feature2 & CPUID_STDEXT2_LA57) == 0) {
		return;
	}

	if (!la57) {
		return;
	}

	create_pagetables_la57(firstaddr);

	setregion(&region, gdt, NGDT * sizeof(gdt)-1);
	m_code =  kmem_alloc(kernel_map, (la57_trampoline_end - la57_trampoline));
	v_code = (char *)PHYS_TO_DMAP(m_code);

	/*
	 * Map m_code 1:1, it appears below 4G in KVA due to physical
	 * address being below 4G.  Since kernel KVA is in upper half,
	 * the pml4e should be zero and free for temporary use.
	 */
	m_pml5 = pmap_extract(kernel_pmap, (vm_offset_t)IdlePML5);
	v_pml5 = (pml5_entry_t *)PHYS_TO_DMAP(m_pml5);

	m_pml4 = pmap_extract(kernel_pmap, (vm_offset_t)IdlePML4);
	v_pml4 = (pml4_entry_t *)PHYS_TO_DMAP(m_pml4);

	m_pdpt = pmap_extract(kernel_pmap, (vm_offset_t)IdlePDPT);
	v_pdpt = (pdpt_entry_t *)PHYS_TO_DMAP(m_pdpt);

	m_pde = pmap_extract(kernel_pmap, (vm_offset_t)IdlePTD);
	v_pde = (pd_entry_t *)PHYS_TO_DMAP(m_pde);

	m_pte = pmap_extract(kernel_pmap, (vm_offset_t)KPTmap);
	v_pte = (pt_entry_t *)PHYS_TO_DMAP(m_pte);

	v_pml4[PL4_E(m_code)] = m_pdpt | PG_V | PG_RW | PG_A | PG_M;
	v_pdpt[PL3_E(m_code)] = m_pde | PG_V | PG_RW | PG_A | PG_M;
	v_pde[PL2_E(m_code)] = m_pte | PG_V | PG_RW | PG_A | PG_M;
	v_pte[PL1_E(m_code)] = m_code | PG_V | PG_RW | PG_A | PG_M;

	/*
	 * Add pml5 entry at top of KVA pointing to existing pml4 table,
	 * entering all existing kernel mappings into level 5 table.
	 */
	v_pml5[l4etol5e(PL4_E(UPT_MAX_ADDRESS))] = IdlePML4 | PG_V | PG_RW | PG_A | PG_M | pg_g;

	/*
	 * Add pml5 entry for 1:1 trampoline mapping after LA57 is turned on.
	 */
	v_pml5[l4etol5e(PL4_E(m_code))] = m_pml4 | PG_V | PG_RW | PG_A | PG_M;
	v_pml4[PL4_E(m_code)] = m_pdpt | PG_V | PG_RW | PG_A | PG_M;

	/*
	 * Copy and call the 48->57 trampoline, hope we return there, alive.
	 */
	bcopy(la57_trampoline, v_code, (la57_trampoline_end - la57_trampoline));
	m_code = (v_code + 2 + (la57_trampoline_gdt_desc - la57_trampoline));
	temp = (la57_trampoline_gdt - la57_trampoline) + m_code;
	m_code = temp;
	temp = NULL;
	la57_tramp = (void (*)(uint64_t))pmap_extract(kernel_pmap, m_code);
	invlpg((vm_offset_t)la57_tramp);
	la57_tramp(KPML5phys);
	/*
	 * gdt was necessary reset, switch back to our gdt.
	 */
	lgdt(&region);
	wrmsr(MSR_GSBASE, (uint64_t)&__percpu[0]);
	load_ds(_udatasel);
	load_es(_udatasel);
	load_fs(_ufssel);
	//ssdtosyssd(&gdt_segs[GPROC0_SEL], (struct system_segment_descriptor *)&__pcpu[0].pc_gdt[GPROC0_SEL]);
	//ltr(GSEL(GPROC0_SEL, SEL_KPL));

	/*
	 * Now unmap the trampoline, and free the pages.
	 * Clear pml5 entry used for 1:1 trampoline mapping.
	 */
	//pte_clear(&v_pml5[l4etol5e(PL4_E(m_code))]);
	invlpg((vm_offset_t)v_code);
	kmem_free(kernel_pmap, m_code, sizeof(m_code));
	kmem_free(kernel_pmap, m_pml4, sizeof(m_pml4));
	kmem_free(kernel_pmap, m_pdpt, sizeof(m_pdpt));
	kmem_free(kernel_pmap, m_pde, sizeof(m_pde));
	kmem_free(kernel_pmap, m_pte, sizeof(m_pte));

	/*
	 * Recursively map PML5 to itself in order to get PTmap and
	 * PDmap.
	 */
	v_pml5[PDIR_SLOT_KERN] = IdlePML5 | PG_RW | PG_V | pg_nx;

	IdlePML5 = (pml5_entry_t *)v_pml5;
	IdlePML4 = (pml4_entry_t *)v_pml4;
	IdlePDPT = (pdpt_entry_t *)v_pdpt;
	IdlePTD = (pd_entry_t *)v_pde;
	KPTmap = (pt_entry_t *)v_pte;

	kernel_pmap->pm_pdir = (pd_entry_t *)(KERNBASE + IdlePTD);
	kernel_pmap->pm_pml4 = (pml4_entry_t *)(KERNBASE + IdlePML4);
	kernel_pmap->pm_pml5 = (pml5_entry_t *)(KERNBASE + IdlePML5);
	kernel_pmap->pm_pdirpa[0] = (vm_offset_t)IdlePTD;
}

void
pmap_pinit_pml5(pml5)
	pml5_entry_t *pml5;
{
	int i;

	pml5 = (pml5_entry_t *)kmem_alloc(kernel_map, (vm_offset_t)(NKL5_MAX_ENTRIES * sizeof(pml5_entry_t)));

	/*
	 * Add pml5 entry at top of KVA pointing to existing pml4 table,
	 * entering all existing kernel mappings into level 5 table.
	 */
	for (i = 0; i < NKL5_MAX_ENTRIES; i++) {
		pml5[PDIR_SLOT_KERNBASE + i] = pmap_extract(kernel_map, (vm_offset_t)(IdlePML4 + ptoa(i))) | PG_V | PG_RW | PG_A | PG_M | pg_g;
	}

	/* install self-referential address mapping entry(s) */
	pml5[PDIR_SLOT_KERN] = pmap_extract(kernel_map, (vm_offset_t)pml5) | PG_RW | PG_V | PG_A | PG_M;
}

/*
 * Initialize a preallocated and zeroed pmap structure,
 * such as one in a vmspace structure.
 */
void
pmap_pinit_type(pmap, pm_type)
	pmap_t pmap;
	enum pmap_type pm_type;
{
	pmap->pm_type = pm_type;

	pmap_pinit_pdir(pmap->pm_pdir, pmap->pm_pdirpa);

	switch (pm_type) {
	case PT_X86:
		if (pmap_is_la57(pmap)) {
			pmap_pinit_pml5(pmap->pm_pml5);
		} else {
			pmap_pinit_pml4(pmap->pm_pml4);
		}
		break;

	case PT_EPT:
	case PT_RVI:
	}
}

void
pmap_release(pmap)
	register pmap_t pmap;
{
	int i;

	kmem_free(kernel_map, (vm_offset_t)pmap->pm_pdir, NBPTD);

	if (pmap_is_la57(pmap)) {
		kmem_free(kernel_map, (vm_offset_t)pmap->pm_pml5, NBPTD * sizeof(pml5_entry_t));
	} else {
		kmem_free(kernel_map, (vm_offset_t)pmap->pm_pml4, NBPTD * sizeof(pml4_entry_t));
	}
}

void
pmap_activate(pmap, pcbp)
	register pmap_t pmap;
	struct pcb *pcbp;
{
	if (pmap != NULL && pmap->pm_pdchanged) {
		if (pmap_is_la57(pmap)) {
			pcbp->pcb_cr3 = pmap_extract(kernel_pmap, (vm_offset_t)pmap->pm_pml5); /* PML5?? */
		} else {
			pcbp->pcb_cr3 = pmap_extract(kernel_pmap, (vm_offset_t)pmap->pm_pml4); /* PML4?? */
		}
		if (pmap == &curproc->p_vmspace->vm_pmap) {
			lcr3(pcbp->pcb_cr3);
		}
		pmap->pm_pdchanged = FALSE;
	}
}

/* Direct Map */
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

pml4_entry_t *
pmap_pml5e_to_pml4e(pml5_entry_t *pml5e, vm_offset_t va)
{
	pml4_entry_t *pml4e;

	/* XXX MPASS(pmap_is_la57(pmap); */
	pml4e = (pml4_entry_t *)PHYS_TO_DMAP(*pml5e & PG_FRAME);
	return (&pml4e[PL4_E(va)]);
}

pml5_entry_t *
pmap_pml4e_to_pml5e(pmap_t pmap, vm_offset_t va)
{
	pml4_entry_t *pml4;
	pml5_entry_t *pml5;

	pml4 = pmap_pml4e(pmap, va);
	if (pmap_is_la57(pmap)) {
		pml5 = &pml4[l4etol5e(PL4_E(va))];
		PG_V = pmap_valid_bit(pmap);
	}

	return (pml5);
}

