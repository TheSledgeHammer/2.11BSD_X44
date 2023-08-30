/*-
 * Copyright (c) 1989, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software developed by the Computer Systems
 * Engineering group at Lawrence Berkeley Laboratory under DARPA contract
 * BG 91-66 and contributed to Berkeley.
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)kvm_hp300.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <unistd.h>
#include <nlist.h>
#include <kvm.h>

#include <vm/include/vm.h>
#include <vm/include/vm_param.h>

#include <limits.h>
#include <db.h>

#include "kvm_private.h"

#include <i386/include/pmap.h>
#include <i386/include/pte.h>
#include <i386/include/param.h>
#include <i386/include/ram.h>
#include <i386/include/vmparam.h>

#ifndef btop
#define	btop(x)		(i386_btop(x)) //(((unsigned)(x)) >> PGSHIFT)	/* XXX */
#define	ptob(x)		(i386_ptob(x)) //((caddr_t)((x) << PGSHIFT))		/* XXX */
#endif

off_t _kvm_pa2off(kvm_t *, u_long *);

struct vmstate {
	pd_entry_t 		*ptd;
	int				pae;
	u_long 			kernbase;
};

#define KREAD(kd, addr, p)	\
	(kvm_read(kd, addr, (char *)(p), sizeof(*(p))) != sizeof(*(p)))

void
_kvm_freevtop(kd)
	kvm_t *kd;
{
	if (kd->vmst != 0) {
		if (kd->vmst->ptd) {
			free(kd->vmst->ptd);
		}
		free(kd->vmst);
	}
}

static int
i386_initvtop(kd, vm, nlist)
	kvm_t *kd;
	struct vmstate *vm;
	struct nlist *nlist;
{
	u_long pa;
	pd_entry_t ptd;

	if (KREAD(kd, (nlist[0].n_value - vm->kernbase), &pa)) {
		_kvm_err(kd, kd->program, "cannot read IdlePTD");
		return (-1);
	}
	ptd = _kvm_malloc(kd, PAGE_SIZE);
	if (KREAD(kd, pa, ptd) ) {
		_kvm_err(kd, kd->program, "cannot read PTD");
		return (-1);
	}
	vm->ptd = ptd;
	vm->pae = 1;
	return (0);
}

static int
i386_initvtop_pae(kd, vm, nlist)
	kvm_t *kd;
	struct vmstate *vm;
	struct nlist *nlist;
{
	u_long pa;
	pd_entry_t ptd;
	uint64_t pa64;
	int i;

	if (KREAD(kd, (nlist[0].n_value - vm->kernbase), &pa)) {
		_kvm_err(kd, kd->program, "cannot read IdlePDPT");
		return (-1);
	}
	pa = le32toh(pa);
	ptd = _kvm_malloc(kd, 4 * I386_PAGE_SIZE);
	if (ptd == NULL) {
		_kvm_err(kd, kd->program, "cannot allocate PTD");
		return (-1);
	}
	for (i = 0; i < 4; i++) {
		if (KREAD(kd, pa + (i * sizeof(pa64)), &pa64)) {
			_kvm_err(kd, kd->program, "cannot read PDPT");
			free(ptd);
			return (-1);
		}
	}
	vm->ptd = ptd;
	vm->pae = 0;
	return (0);
}

int
_kvm_initvtop(kd)
	kvm_t *kd;
{
	struct vmstate *vm;
	struct nlist nlist[2];
	u_long pa;
	u_long kernbase;
	pd_entry_t	*ptd;

	vm = (struct vmstate *)_kvm_malloc(kd, sizeof(*vm));
	if (vm == 0) {
		_kvm_err(kd, kd->program, "cannot allocate vm");
		return (-1);
	}
	kd->vmst = vm;
	vm->ptd = 0;

	nlist[0].n_name = "kernbase";
	nlist[3].n_name = 0;

	if (kvm_nlist(kd, nlist) != 0) {
		vm->kernbase = KERNBASE; 			/* for old kernels */
	} else {
		vm->kernbase = nlist[0].n_value;
	}

	if (kvm_nlist(kd, nlist) != 0) {
		_kvm_err(kd, kd->program, "bad namelist");
		return (-1);
	}
#ifdef notyet
	if (kvm_nlist(kd, nlist) == 0) {

		nlist[0].n_name = "IdlePDPT";
		nlist[1].n_name = 0;
		return (i386_initvtop_pae(kd, vm, nlist));
#endif
	//} else {
	if (kvm_nlist(kd, nlist) != 0) {
		nlist[0].n_name = "IdlePTD";
		nlist[1].n_name = 0;
		return (i386_initvtop(kd, vm, nlist));
	}
	return (0);
}

static int
_kvm_vatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	register struct vmstate *vm;
	u_long offset;
	u_long pte_pa;
	pd_entry_t pde;
	pt_entry_t pte;
	u_long pdeindex;
	u_long pteindex;
	size_t s;
	uint32_t a;
	int i;
	pd_entry_t *ptd;

	vm = kd->vmst;
	ptd = (pd_entry_t *)vm->ptd;
	offset = va & PGOFSET;

	/*
	 * If we are initializing (kernel page table descriptor pointer
	 * not yet set) then return pa == va to avoid infinite recursion.
	 */
	if (vm->ptd == 0) {
		*pa = va;
		return (PAGE_SIZE - offset);
	}
	pdeindex = va >> PDRSHIFT;
	pde = vm->ptd[pdeindex];
	if (((u_long)pde & PG_V) == 0) {
		goto invalid;
	}

	pte_pa = ((u_long)pde & PG_FRAME) + (PL1_PI(va) * sizeof(pt_entry_t));
	s = _kvm_pa2off(kd, pte_pa);
	if (s < sizeof(pte)) {
		_kvm_err(kd, kd->program, "_i386_vatop: pte_pa not found");
		goto invalid;
	}

	/* XXX This has to be a physical address read, kvm_read is virtual */
	if (pread(kd->pmfd, &pte, sizeof(pte), s) != sizeof(pte)) {
		_kvm_syserr(kd, kd->program, "_i386_vatop: pread");
		goto invalid;
	}
	pte = le32toh(pte);
	if ((pte & PG_V) == 0) {
		_kvm_err(kd, kd->program, "_kvm_kvatop: pte not valid");
		goto invalid;
	}

	a = (pte & PG_FRAME) + offset;
	s = _kvm_pa2off(kd, a);
	if (s == 0) {
		_kvm_err(kd, kd->program, "_i386_vatop: address not in dump");
		goto invalid;
	} else {
		return (I386_PAGE_SIZE - offset);
	}

invalid:
	_kvm_err(kd, 0, "invalid address (0x%jx)", (uintmax_t)va);
	return (0);
}

#ifdef notyet
static int
_i386_vatop_pae(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	struct vmstate *vm;
	i386_physaddr_pae_t offset;
	i386_physaddr_pae_t pte_pa;
	i386_pde_pae_t pde;
	i386_pte_pae_t pte;
	kvaddr_t pdeindex;
	kvaddr_t pteindex;
	size_t s;
	i386_physaddr_pae_t a;
	off_t ofs;
	i386_pde_pae_t *PTD;

	vm = kd->vmst;
	PTD = (i386_pde_pae_t *)vm->ptd;
	offset = va & I386_PAGE_MASK;

	/*
	 * If we are initializing (kernel page table descriptor pointer
	 * not yet set) then return pa == va to avoid infinite recursion.
	 */
	if (PTD == NULL) {
		s = _kvm_pa2off(kd, va, pa);
		if (s == 0) {
			_kvm_err(kd, kd->program,
			    "_i386_vatop_pae: bootstrap data not in dump");
			goto invalid;
		} else
			return (I386_PAGE_SIZE - offset);
	}

	pdeindex = va >> I386_PDRSHIFT_PAE;
	pde = le64toh(PTD[pdeindex]);
	if ((pde & PG_V) == 0) {
		_kvm_err(kd, kd->program, "_kvm_kvatop_pae: pde not valid");
		goto invalid;
	}

	if (pde & PG_PS) {
		/*
		 * No second-level page table; ptd describes one 2MB
		 * page.  (We assume that the kernel wouldn't set
		 * PG_PS without enabling it cr0).
		 */
		offset = va & I386_PAGE_PS_MASK_PAE;
		a = (pde & I386_PG_PS_FRAME_PAE) + offset;
		s = _kvm_pa2off(kd, a, pa);
		if (s == 0) {
			_kvm_err(kd, kd->program,
			    "_i386_vatop: 2MB page address not in dump");
			goto invalid;
		}
		return (I386_NBPDR_PAE - offset);
	}

	pteindex = (va >> I386_PAGE_SHIFT) & (I386_NPTEPG_PAE - 1);
	pte_pa = (pde & I386_PG_FRAME_PAE) + (pteindex * sizeof(pde));

	s = _kvm_pa2off(kd, pte_pa, &ofs);
	if (s < sizeof(pte)) {
		_kvm_err(kd, kd->program, "_i386_vatop_pae: pdpe_pa not found");
		goto invalid;
	}

	/* XXX This has to be a physical address read, kvm_read is virtual */
	if (pread(kd->pmfd, &pte, sizeof(pte), ofs) != sizeof(pte)) {
		_kvm_syserr(kd, kd->program, "_i386_vatop_pae: read");
		goto invalid;
	}
	pte = le64toh(pte);
	if ((pte & PG_V) == 0) {
		_kvm_err(kd, kd->program, "_i386_vatop_pae: pte not valid");
		goto invalid;
	}

	a = (pte & I386_PG_FRAME_PAE) + offset;
	s = _kvm_pa2off(kd, a, pa);
	if (s == 0) {
		_kvm_err(kd, kd->program, "_i386_vatop_pae: address not in dump");
		goto invalid;
	} else {
		return (I386_PAGE_SIZE - offset);
	}

invalid:
	_kvm_err(kd, 0, "invalid address (0x%jx)", (uintmax_t)va);
	return (0);
}
#endif

int
_kvm_kvatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return (0);
	}
#ifdef notyet
	if (kd->vmst->pae) {
		return (_i386_vatop_pae(kd, va, pa));
	} else {
		return (_kvm_vatop(kd, va, pa));
	}
#endif
	return (_kvm_vatop(kd, va, pa));
}

/*
 * Translate a user virtual address to a physical address.
 */
int
_kvm_uvatop(kd, p, va, pa)
	kvm_t *kd;
	const struct proc *p;
	u_long va;
	u_long *pa;
{
	register struct vmspace *vms;
	int kva;

	vms = p->p_vmspace;

	return (_kvm_vatop(kd, kva, va, pa));
}

/*
 * Translate a physical address to a file-offset in the crash dump.
 */
off_t
_kvm_pa2off(kd, pa)
	kvm_t *kd;
	u_long *pa;
{
	phys_ram_seg_t *ramsegs;
	off_t off;
	int i, nmemsegs;

	ramsegs = (phys_ram_seg_t *)_kvm_malloc(kd, sizeof(*ramsegs) + ALIGN(sizeof(*ramsegs)));
	nmemsegs = PHYSSEG_MAX;
	off = 0;
	for (i = 0; i < nmemsegs; i++) {
		if (pa >= ramsegs[i].start && (pa - ramsegs[i].start) < ramsegs[i].size) {
			off += (pa - ramsegs[i].start);
			break;
		}
		off += ramsegs[i].size;
	}
	return (kd->dump_off + off);
}
