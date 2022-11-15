/*	$NetBSD: uvm_fault.c,v 1.87.2.1.2.1 2005/05/11 19:15:41 riz Exp $	*/

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
 *    must display the following acknowledgement:
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
 *
 * from: Id: uvm_fault.c,v 1.1.2.23 1998/02/06 05:29:05 chs Exp
 */

/*
 * uvm_fault.c: fault handler
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mman.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_fault.h>
#include <devel/vm/include/vm_page.h>
#include <devel/vm/include/vm_pageout.h>
#include <devel/vm/include/vm_segment.h>

static struct vm_advice vmadvice[] = {
		{ MADV_NORMAL, 3, 4 },
		{ MADV_RANDOM, 0, 0 },
		{ MADV_SEQUENTIAL, 8, 7},
};

#define VM_MAXRANGE 16

void
vm_fault_advice(vfi)
	struct vm_faultinfo *vfi;
{
	KASSERT(vmadvice[vfi->entry->advice].advice == vfi->entry->advice);
	vfi->nback = min(vmadvice[vfi->entry->advice].nback, (vfi->orig_rvaddr - vfi->entry->start) >> PAGE_SHIFT);
	vfi->startva = vfi->orig_rvaddr - (vfi->nback << PAGE_SHIFT);
	vfi->nforw = min(vmadvice[vfi->entry->advice].nforw, ((vfi->entry->end - vfi->orig_rvaddr) >> PAGE_SHIFT) - 1);
	vfi->npages = vfi->nback + vfi->nforw + 1;
    vfi->nsegments = num_segments(vfi->npages); /* converts npages to nsegments */
    vfi->centeridx = vfi->nback;
}

void
vm_fault_amap(vfi)
	struct vm_faultinfo *vfi;
{
	vm_anon_t anons_store[VM_MAXRANGE], *anons;
	vm_page_t pages[VM_MAXRANGE];
	vm_offset_t start, end;
	int lcv;

	if(vfi->amap && vfi->object == NULL) {
		vm_fault_unlockmaps(vfi, FALSE);
		//return (EFAULT);
	}

	vm_fault_advice(vfi);

	if (vfi->amap) {
		amap_lock(vfi->amap);
		anons = anons_store;
		vm_amap_lookups(vfi->entry->aref, vfi->startva - vfi->entry->start, anons, vfi->npages);
	} else {
		anons = NULL;
	}

	/*
	 * for MADV_SEQUENTIAL mappings we want to deactivate the back pages
	 * now and then forget about them (for the rest of the fault).
	 */
	if (vfi->entry->advice == MADV_SEQUENTIAL && vfi->nback != 0) {
		if (vfi->amap) {
			vm_fault_anonflush(anons, vfi->nback);
		}
		/* flush object? */
		if (vfi->object) {
			start = (vfi->startva - vfi->entry->start) + vfi->entry->offset;
			end = start + (vfi->nback << PAGE_SHIFT);
			simple_lock(&vfi->object->Lock);
			(void)vm_object_page_clean(vfi->object, start, end, TRUE, TRUE);
		}
		if (vfi->amap) {
			anons += vfi->nback;
		}
		vfi->startva += (vfi->nback << PAGE_SHIFT);
		vfi->npages -= vfi->nback;
		vfi->nback = vfi->centeridx = 0;
	}

	vfi->currva = vfi->startva;
	for (lcv = 0 ; lcv < vfi->npages ; lcv++, vfi->currva += PAGE_SIZE) {
		vfi->anon = anons[lcv];
		simple_lock(&vfi->anon->an_lock);
		if (vfi->anon->u.an_page && (vfi->anon->u.an_page->flags & PG_BUSY) == 0) {
			vm_page_lock_queues();
			vm_page_activate(vfi->anon->u.an_page);
			vm_page_unlock_queues();
			pmap_enter(vfi->orig_map->pmap, vfi->currva, VM_PAGE_TO_PHYS(vfi->anon->u.an_page), (vfi->anon->an_ref > 1) ? (vfi->prot & ~VM_PROT_WRITE) : vfi->prot, vfi->wired);
		}
		simple_unlock(&vfi->anon->an_lock);
		pmap_update();
	}

	vfi->anon = anons[vfi->centeridx];
	simple_lock(vfi->anon->an_lock);
}

void
vm_fault_anon(vfi, fault_type)
	struct vm_faultinfo *vfi;
	vm_prot_t fault_type;
{
	vm_anon_t 		oanon;
	vm_prot_t 		enter_prot;

	if ((fault_type & VM_PROT_WRITE) != 0 && vfi->anon->an_ref > 1) {
		cnt.v_flt_acow++;
		oanon = vfi->anon;
		vfi->anon = vm_anon_alloc();
		if (vfi->anon) {
			vfi->segment = vm_segment_anon_alloc(vfi->object, vfi->offset, vfi->anon);
			if (vfi->segment) {
				vfi->page = vm_page_anon_alloc(vfi->segment, vfi->segment->sg_offset, vfi->anon);
			}
		}
		if (vfi->anon == NULL || vfi->segment == NULL || vfi->page == NULL) {
			if (vfi->anon) {
				vm_anon_free(vfi->anon);
			}
			vm_fault_unlockall(vfi, vfi->amap, vfi->object, oanon);
			if (vfi->anon == NULL || cnt.v_swpgonly == cnt.v_swpages) {
				cnt.v_fltnoanon++;
				return (KERN_RESOURCE_SHORTAGE);
			}
			cnt.v_fltnoram++;
			VM_WAIT;
			goto ReFault;
		}
		vm_page_copy(oanon->u.an_page, vfi->page);
		vm_page_lock_queues();
		vm_page_activate(vfi->page);
		vfi->page->flags &= ~(PG_BUSY|PG_FAKE);
		vm_page_lock_queues();
		amap_add(vfi->entry->aref, vfi->orig_rvaddr - vfi->entry->start, vfi->anon, 1);
		oanon->an_ref--;
	} else {
		cnt.v_flt_anon++;
		oanon = vfi->anon;
		vfi->segment = vfi->anon->u.an_segment;
		vfi->page = vfi->anon->u.an_page;
		if (vfi->anon->an_ref > 1) {
			enter_prot = enter_prot & ~VM_PROT_WRITE;
		}
	}
}


