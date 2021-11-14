/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#ifndef _I386_PMAP_BASE_H_
#define	_I386_PMAP_BASE_H_

struct pmap_args {
	void (*pmap_cold_map)(u_long pa, u_long va, u_long cnt);
	void (*pmap_cold_mapident)(u_long pa, u_long cnt);
	void (*pmap_remap_lower)(boolean_t enable);
	void (*pmap_cold)(void);
	void (*pmap_set_nx)(void);
	void (*pmap_bootstrap)(vm_offset_t firstaddr, vm_offset_t loadaddr);
	int (*pmap_isvalidphys)(int addr);
	void *(*pmap_bootstrap_alloc)(u_long size);
	void (*pmap_init_pat)(void);
	void (*pmap_init)(vm_offset_t phys_start, vm_offset_t phys_end);
	vm_offset_t (*pmap_map)(vm_offset_t virt, vm_offset_t start, vm_offset_t end, int prot);
	pmap_t (*pmap_create)(vm_size_t	size);
	void (*pmap_pinit)(struct pmap *pmap);
	void (*pmap_destroy)(struct pmap pmap);
	void (*pmap_release)(struct pmap *pmap);
	void (*pmap_reference)(pmap_t pmap);
	void (*pmap_remove)(struct pmap *pmap, vm_offset_t sva, vm_offset_t eva);
	void (*pmap_remove_all)(vm_offset_t pa);
	void (*pmap_copy_on_write)(vm_offset_t pa);
	void (*pmap_protect)(pmap_t pmap, vm_offset_t sva, vm_offset_t eva, vm_prot_t prot);
	void (*pmap_enter)(pmap_t pmap, vm_offset_t va, vm_offset_t pa, vm_prot_t prot, boolean_t wired);
	void (*pmap_page_protect)(vm_offset_t phys, vm_prot_t prot);
	void (*pmap_change_wiring)(pmap_t pmap, vm_offset_t va, boolean_t wired);
	struct pte *(*pmap_pte)(pmap_t pmap, vm_offset_t va);
	vm_offset_t (*pmap_extract)(pmap_t pmap, vm_offset_t va);
	void (*pmap_copy)(pmap_t dst_pmap, pmap_t src_pmap, vm_offset_t dst_addr, vm_size_t	len, vm_offset_t src_addr);
	void (*pmap_update)();
	void (*pmap_collect)(pmap_t pmap);
	void (*pmap_activate)(pmap_t pmap, struct pcb *pcbp);
	void (*pmap_zero_page)(vm_offset_t phys);
	void (*pmap_copy_page)(vm_offset_t src, vm_offset_t dst);
	void (*pmap_pageable)(pmap_t pmap, vm_offset_t sva, vm_offset_t	eva, boolean_t pageable);
	void (*pmap_clear_modify)(vm_offset_t pa);
	void (*pmap_clear_reference)(vm_offset_t pa);
	boolean_t (*pmap_is_referenced)(vm_offset_t pa);
	boolean_t (*pmap_is_modified)(vm_offset_t pa);
	vm_offset_t (*pmap_phys_address)(int ppn);
	void (*i386_protection_init)();
	boolean_t (*pmap_testbit)(vm_offset_t pa, int bit);
	void (*pmap_changebit)(vm_offset_t pa, int bit, boolean_t setem);
	void (*pmap_pvdump)(vm_offset_t pa);
	void (*pmap_check_wiring)(char *str, vm_offset_t va);
	void (*pmap_pads)(pmap_t pm);
	u_int (*pmap_get_kcr3)(void);
	u_int (*pmap_get_cr3)(pmap_t pmap);
	caddr_t (*pmap_cmap3)(caddr_t pa, u_int pte_flags);
	void *(*pmap_bios16_enter)(void);
	void (*pmap_bios16_leave)(void *handle);
	/* SMP & TLB */
	void (*pmap_invalidate_page)(pmap_t, vm_offset_t);
	void (*pmap_invalidate_range)(pmap_t, vm_offset_t, vm_offset_t);
	void (*pmap_invalidate_all)(pmap_t);
	void (*pmap_tlb_init)(void);
	void (*pmap_tlb_shootnow)(pmap_t, int32_t);
	void (*pmap_tlb_shootdown)(pmap_t, vm_offset_t, vm_offset_t, int32_t *);
	void (*pmap_do_tlb_shootdown)(pmap_t, struct cpu_info *);
	void (*pmap_tlb_shootdown_q_drain)(struct pmap_tlb_shootdown_q *);
	struct pmap_tlb_shootdown_job *(*pmap_tlb_shootdown_job_get)(struct pmap_tlb_shootdown_q *);
	void (*pmap_tlb_shootdown_job_put)(struct pmap_tlb_shootdown_q *, struct pmap_tlb_shootdown_job *);
};

void	pmap_cold(void);
void	pmap_pae_cold(void);
void	pmap_nopae_cold(void);

#endif /* _I386_PMAP_BASE_H_ */
