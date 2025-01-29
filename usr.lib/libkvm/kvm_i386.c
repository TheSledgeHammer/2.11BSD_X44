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
#if 0
static char sccsid[] = "@(#)kvm_hp300.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <nlist.h>
#include <kvm.h>

#include <vm/include/vm.h>
#include <vm/include/vm_param.h>

#include <limits.h>
#include <db.h>

#include "kvm_private.h"

#include <i386/pmap.h>
#include <i386/pte.h>
#include <i386/param.h>
#include <i386/ram.h>
#include <i386/vmparam.h>

static int  _i386_initvtop(kvm_t *, struct vmstate *, struct nlist *);
static int  _i386_initvtop_pae(kvm_t *, struct vmstate *, struct nlist *);
static int  _i386_vatop(kvm_t *, u_long, u_long *);
static int  _i386_vatop_pae(kvm_t *, u_long, u_long *);
static int  _i386_uvatop(kvm_t *, struct vmspace *, u_long, u_long *);
static int  _i386_uvatop_pae(kvm_t *, struct vmspace *, u_long, u_long *);
static int  _kvm_vatop(kvm_t *, u_long, u_long *);
off_t       _kvm_pa2off(kvm_t *, u_long);

#ifdef PMAP_PAE_COMP
static int i386_use_pae = 0;
#else
static int i386_use_pae = 1;
#endif

struct vmstate {
	pd_entry_t 		*ptd;
	int				pae;
	u_long 	        kernbase;
};

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
_i386_initvtop(kd, vm, nlist)
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
	ptd = (pd_entry_t)_kvm_malloc(kd, I386_PAGE_SIZE);
	if (KREAD(kd, pa, &ptd) ) {
		_kvm_err(kd, kd->program, "cannot read PTD");
		return (-1);
	}
	vm->ptd = &ptd;
	vm->pae = 1;
	return (0);
}

static int
_i386_initvtop_pae(kd, vm, nlist)
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
	ptd = (pd_entry_t)_kvm_malloc(kd, 4 * I386_PAGE_SIZE);
	if (ptd == NULL) {
		_kvm_err(kd, kd->program, "cannot allocate PTD");
		return (-1);
	}
	for (i = 0; i < 4; i++) {
		if (KREAD(kd, pa + (i * sizeof(pa64)), &pa64)) {
			_kvm_err(kd, kd->program, "cannot read PDPT");
			free(&ptd);
			return (-1);
		}
		pa64 = le64toh(pa64);
        ptd = (pd_entry_t)(ptd + (i * I386_PAGE_SIZE));
		if (KREAD(kd, pa64 & L2_FRAME, &ptd)) {
			free(&ptd);
			return (-1);
		}
	}
	vm->ptd = &ptd;
	vm->pae = 0;
	return (0);
}

int
_kvm_initvtop(kd)
	kvm_t *kd;
{
	struct vmstate *vm;
	struct nlist nlist[2];

	vm = (struct vmstate *)_kvm_malloc(kd, sizeof(*vm));
	if (vm == 0) {
		_kvm_err(kd, kd->program, "cannot allocate vm");
		return (-1);
	}
	kd->vmst = vm;
	vm->ptd = 0;

	nlist[0].n_name = "kernbase";
	nlist[1].n_name = 0;

	if (kvm_nlist(kd, nlist) != 0) {
		vm->kernbase = KERNBASE; 			/* for old kernels */
	} else {
		vm->kernbase = nlist[0].n_value;
	}

	if (kvm_nlist(kd, nlist) != 0) {
		_kvm_err(kd, kd->program, "bad namelist");
		return (-1);
	}

	if (kvm_nlist(kd, nlist) == 0) {
		if (i386_use_pae == 0) {
			nlist[0].n_name = "IdlePDPT";
			nlist[1].n_name = 0;
			return (_i386_initvtop_pae(kd, vm, nlist));
		}
		if (i386_use_pae == 1) {
			nlist[0].n_name = "IdlePTD";
			nlist[1].n_name = 0;
			return (_i386_initvtop(kd, vm, nlist));
		}
	}
	return (0);
}

static int
_i386_vatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	register struct vmstate *vm;
	pd_entry_t pde;
	pt_entry_t pte;
	u_long offset, a;
	u_long pte_pa;
	size_t s;

	vm = kd->vmst;
	offset = va & PGOFSET;

	/*
	 * If we are initializing (kernel page table descriptor pointer
	 * not yet set) then return pa == va to avoid infinite recursion.
	 */
	if (vm->ptd == 0) {
		*pa = va;
		return (I386_PAGE_SIZE - offset);
	}

	pde = vm->ptd[PL1_I(va)];
	if (((u_long) pde & PG_V) == 0) {
		goto invalid;
	}

	pte_pa = ((u_long) pde & PG_FRAME) + (PL1_PI(va) * sizeof(pt_entry_t));
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
		_kvm_err(kd, kd->program, "_i386_vatop: pte not valid");
		goto invalid;
	}

	a = (u_long) (pte & PG_FRAME) + offset;
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

static int
_i386_vatop_pae(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	register struct vmstate *vm;
	pd_entry_t pde;
	pt_entry_t pte;
	u_long offset, a;
	u_long pde_pa, pte_pa;
	size_t s;

	vm = kd->vmst;
	offset = va & PGOFSET;

	/*
	 * If we are initializing (kernel page table descriptor pointer
	 * not yet set) then return pa == va to avoid infinite recursion.
	 */
	if (vm->ptd == 0) {
		*pa = va;
		return (I386_PAGE_SIZE - offset);
	}
	pde = vm->ptd[PL2_I(va)];
	if (((u_long) pde & PG_V) == 0) {
		goto invalid;
	}

	pde_pa = ((u_long) pde & PG_FRAME) + (PL2_PI(va) * sizeof(pde));
	s = _kvm_pa2off(kd, pde_pa);
	if (s < sizeof(pde)) {
		_kvm_err(kd, kd->program, "_i386_vatop_pae: pde_pa not found");
		goto invalid;
	}
	if (pread(kd->pmfd, &pde, sizeof(pde), s) != sizeof(pde)) {
		_kvm_syserr(kd, kd->program, "_i386_vatop_pae: could not read PDE");
		goto invalid;
	}

	pde = le32toh(pde);
	if ((pde & PG_V) == 0) {
		_kvm_err(kd, kd->program, "_i386_vatop_pae: pde not valid");
		goto invalid;
	}

	if ((pde & PG_PS) != 0) {
		offset = va & ~PG_4MFRAME;
		return ((int) (NBPD_L2 - offset));
	}

	pte_pa = ((u_long) pde & PG_FRAME) + (PL1_PI(va) * sizeof(pt_entry_t));
	s = _kvm_pa2off(kd, pte_pa);
	if (s < sizeof(pte)) {
		_kvm_err(kd, kd->program, "_i386_vatop_pae: pte_pa not found");
		goto invalid;
	}

	if (pread(kd->pmfd, &pte, sizeof(pte), s) != sizeof(pte)) {
		_kvm_syserr(kd, kd->program, "_i386_vatop_pae: could not read PTE ");
		goto invalid;
	}

	pte = le32toh(pte);
	if ((pte & PG_V) == 0) {
		_kvm_err(kd, kd->program, "_i386_vatop_pae: pte not valid");
		goto invalid;
	}

	a = (u_long) (pte & PG_FRAME) + offset;
	s = _kvm_pa2off(kd, a);
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

static int
_kvm_vatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return (0);
	}

	switch (i386_use_pae) {
	default:
	case 0:
		return (_i386_vatop_pae(kd, va, pa));
	case 1:
		return (_i386_vatop(kd, va, pa));
	}
	return (0);
}

int
_kvm_kvatop(kd, va, pa)
	kvm_t *kd;
	u_long va;
	u_long *pa;
{
	return (_kvm_vatop(kd, va, pa));
}

/*
 * Translate a user virtual address to a physical address.
 */
static int
_i386_uvatop(kd, vms, va, pa)
	kvm_t *kd;
	struct vmspace *vms;
	u_long va;
	u_long *pa;
{
	pd_entry_t *pdir;
	pt_entry_t pte;
	u_long kva, index, offset;

	if (ISALIVE(kd)) {
		kva = (u_long) &vms->vm_pmap.pm_pdir;
		if (KREAD(kd, kva, &pdir)) {
			_kvm_err(kd, kd->program, "invalid address (%lx)", va);
			return (0);
		}
		index = PL1_I(va);
		kva = (u_long) &pdir[index];
		if (KREAD(kd, kva, &pte) || (pte & PG_V) == 0) {
			_kvm_err(kd, kd->program, "invalid address (%lx)", va);
			return (0);
		}
		offset = va & (NBPD_L1 - 1);
		*pa = (u_long) ((pte & L1_FRAME) + offset);
		return (NBPD_L1 - offset);
	}
	kva = (u_long) &vms->vm_pmap.pm_pdir;
	if (KREAD(kd, kva, &kva)) {
		_kvm_err(kd, kd->program, "invalid address (%lx)", va);
		return (0);
	}
	return (_i386_vatop(kd, va, pa));
}

static int
_i386_uvatop_pae(kd, vms, va, pa)
	kvm_t *kd;
	struct vmspace *vms;
	u_long va;
	u_long *pa;
{
	pdpt_entry_t *pdpt;
	pd_entry_t *pdir, pde;
	pt_entry_t 	pte;
	u_long 	kva, index, offset;

	if (ISALIVE(kd)) {
		kva = (u_long) &vms->vm_pmap.pm_pdir;
		if (KREAD(kd, kva, &pdpt)) {
			_kvm_err(kd, kd->program, "invalid address (%lx)", va);
			return (0);
		}
		index = PL2_I(va);
		kva = (u_long) &pdpt[index];
		if (KREAD(kd, kva, &pde) || (pde & PG_V) == 0) {
			_kvm_err(kd, kd->program, "invalid address (%lx)", va);
			return (0);
		}
		if ((pde & PG_PS) != 0) {
			offset = va & (NBPD_L2 - 1);
			*pa = (u_long) ((pde & L2_FRAME) + offset);
			return (NBPD_L2 - offset);
		}
		if (KREAD(kd, kva, &pdir)) {
			_kvm_err(kd, kd->program, "invalid address (%lx)", va);
			return (0);
		}
		index = PL1_I(va);
		kva = (u_long) &pdir[index];
		if (KREAD(kd, kva, &pte) || (pte & PG_V) == 0) {
			_kvm_err(kd, kd->program, "invalid address (%lx)", va);
			return (0);
		}
		offset = va & (NBPD_L1 - 1);
		*pa = (u_long) ((pte & L1_FRAME) + offset);
		return (NBPD_L1 - offset);
	}

	return (_i386_vatop_pae(kd, va, pa));
}

int
_kvm_uvatop(kd, p, va, pa)
	kvm_t *kd;
	const struct proc *p;
	u_long va;
	u_long *pa;
{
	register struct vmspace *vms;

	vms = p->p_vmspace;

	switch (i386_use_pae) {
	default:
	case 0:
		return (_i386_uvatop_pae(kd, vms, va, pa));
	case 1:
		return (_i386_uvatop(kd, vms, va, pa));
	}
	return (0);
}

/*
 * Translate a physical address to a file-offset in the crash dump.
 */
off_t
_kvm_pa2off(kd, pa)
	kvm_t *kd;
	u_long pa;
{
	phys_ram_seg_t *ramsegs;
	off_t off;
	int i, nmemsegs;
	uint64_t start, size, pa64;

	pa64 = (uint64_t) pa;
	ramsegs = (phys_ram_seg_t *)_kvm_malloc(kd, sizeof(*ramsegs) + ALIGN(sizeof(*ramsegs)));
	nmemsegs = PHYSSEG_MAX;
	off = 0;
	for (i = 0; i < nmemsegs; i++) {
		start = ramsegs[i].start;
		size = ramsegs[i].size;
		if (pa64 >= start && (pa64 - start) < size) {
			off += (off_t) (pa64 - start);
			break;
		}
		off += (off_t) size;
	}
	return (kd->dump_off + off);
}
