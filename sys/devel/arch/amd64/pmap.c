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


#include <devel/arch/amd64/pmap.h>

static u_int64_t KPTphys;					/* phys addr of kernel level 1 */
static u_int64_t KPDphys;					/* phys addr of kernel level 2 */
static u_int64_t KPDPphys;					/* phys addr of kernel level 3 */
u_int64_t 		 KPML4phys;					/* phys addr of kernel level 4 */
u_int64_t 		 KPML5phys;					/* phys addr of kernel level 5 */


static pml5_entry_t *pmap_pml5(pmap_t, vm_offset_t);

pml5_entry_t *
pmap_pml5(pmap, va)
	register pmap_t pmap;
	vm_offset_t va;
{
	pml5_entry_t *pml5;

	pml5 = (pml5_entry_t *)pmap_pde(pmap, va, 5);
	if (pmap_valid_entry(pmap, (pml5_entry_t *)pml5, va)) {
		return (pml5);
	}
	return (NULL);
}

void
pmap_pinit_pml4(pml4)
	pml4_entry_t *pml4;
{
	int i;

	pml4 = (pml4_entry_t *)kmem_alloc(kernel_map, (vm_offset_t)(NKL4_MAX_ENTRIES * sizeof(pml4_entry_t)));

	for (i = 0; i < NKL4_MAX_ENTRIES; i++) {
		pml4[L4_SLOT_KERNBASE + i] = pmap_extract(kernel_map, (vm_offset_t)(KPDPphys + ptoa(i))) | PG_RW | PG_V;
	}
	/* install self-referential address mapping entry(s) */
	pml4[L4_SLOT_KERN] = pmap_extract(kernel_map, (vm_offset_t)pml4) | PG_RW | PG_V | PG_A | PG_M;
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
	pml5[PL5_I(UPT_MAX_ADDRESS)] = pmap_extract(kernel_map, (vm_offset_t)(KPML4phys)) | PG_V | PG_RW | PG_A | PG_M | pg_g;

	/* install self-referential address mapping entry(s) */
	pml5[L4_SLOT_KERN] = pmap_extract(kernel_map, (vm_offset_t)pml5) | PG_RW | PG_V | PG_A | PG_M;
}
