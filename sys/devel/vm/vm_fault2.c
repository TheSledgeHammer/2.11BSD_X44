/*
 * vm_fault2.c
 *
 *  Created on: 31 Oct 2022
 *      Author: marti
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

struct vm_advice_range {
	int 		npages;
	int 		nback;
	int 		nforw;
	int 		centeridx;
	vm_offset_t startva;
	vm_offset_t currva;
};

void
vm_fault_advice(vfi)
	struct vm_faultinfo *vfi;
{
	bool_t narrow;
	int npages, nback, nforw, centeridx;
	vm_offset_t startva, currva;

	if (narrow == FALSE) {
		KASSERT(vmadvice[vfi->entry->advice].advice == vfi->entry->advice);
		nback = MIN(vmadvice[vfi->entry->advice].nback,
				(vfi->orig_rvaddr - vfi->entry->start) >> PAGE_SHIFT);
		startva = vfi->orig_rvaddr - (nback << PAGE_SHIFT);
		nforw = MIN(vmadvice[vfi->entry->advice].nforw,
				((vfi->entry->end - vfi->orig_rvaddr) >> PAGE_SHIFT) - 1);

		npages = nback + nforw + 1;
		centeridx = nback;

		narrow = TRUE; /* ensure only once per-fault */
	} else {

		/* narrow fault! */
		nback = nforw = 0;
		startva = vfi->orig_rvaddr;
		npages = 1;
		centeridx = 0;
	}



	/*
	 * for MADV_SEQUENTIAL mappings we want to deactivate the back pages
	 * now and then forget about them (for the rest of the fault).
	 */
	if (vfi->entry->advice == MADV_SEQUENTIAL && nback != 0) {
		if (amap) {
			vm_fault_anonflush(anons, nback);
		}
		if (vfi->object) {
			objaddr = (startva - vfi->entry->start) + vfi->entry->offset;
			simple_lock(&vfi->object->Lock);
			//(void)vm_pager_put(obj->pager, );
		}
		if (amap) {
			anons += nback;
		}
		startva += (nback << PAGE_SHIFT);
		npages -= nback;
		nback = centeridx = 0;
	}
}

void
vm_fault_amap(vfi)
	struct vm_faultinfo *vfi;
{
	vm_amap_t amap;
	vm_anon_t anons_store[VM_MAXRANGE], *anons;
	vm_offset_t startva, currva;
	int lcv, npages;

	startva = vfi->orig_rvaddr;
	npages = 1;

	if (amap == NULL) {
		vm_fault_unlockmaps(vfi, FALSE);
		return (KERN_INVALID_ADDRESS);
	}
	if (amap) {
		amap_lock(amap);
		anons = anons_store;
		vm_amap_lookups(vfi->entry->aref, startva - vfi->entry->start, anons, npages);
	} else {
		anons = NULL;
	}

	vfi->anon = anons[centeridx];
	simple_lock(vfi->anon->an_lock);

	error = vm_fault_anonget(vfi, vfi->amap, vfi->anon);
	vfi->object = vfi->anon->u.an_page->object;
	cnt.v_flt_przero++;
	vm_fault_zero_fill(vfi);
	vm_amap_add(vfi->entry->aref, vfi->orig_rvaddr - vfi->entry->start, vfi->anon, 0);
}

void
vm_fault_anon(vfi, fault_type)
	struct vm_faultinfo *vfi;
	vm_prot_t fault_type;
{
	vm_object_t 	object;
	vm_segment_t 	segment;
	vm_page_t 		page;
	vm_amap_t 		amap;
	vm_anon_t 		anon, oanon;
	vm_prot_t 		enter_prot;

	object = vfi->object;
	segment = vfi->segment;
	page = vfi->page;
	amap = vfi->amap;
	anon = vfi->anon;

	if ((fault_type & VM_PROT_WRITE) != 0 && anon->an_ref > 1) {
		oanon = anon;
		anon = vm_anon_alloc();
		if (anon) {
			segment = vm_segment_anon_alloc(vfi->object, vfi->offset, anon);
			if (segment) {
				page = vm_page_anon_alloc(vfi->segment, vfi->segment->sg_offset, anon);
			}
		}
		if (anon == NULL || segment == NULL || page == NULL) {
			if (anon) {
				vm_anon_free(anon);
			}
			vm_fault_unlockall(vfi, amap, object, oanon);
			if (anon == NULL || cnt.v_swpgonly == cnt.v_swpages) {
				cnt.v_fltnoanon++;
				return (KERN_RESOURCE_SHORTAGE);
			}
			cnt.v_fltnoram++;
			VM_WAIT;
			goto ReFault;
		}
		vm_page_copy(oanon->u.an_page, page);
		amap_add(vfi->entry->aref, vfi->orig_rvaddr - vfi->entry->start, anon, 1);
		oanon->an_ref--;
	} else {
		oanon = anon;
		segment = anon->u.an_segment;
		page = anon->u.an_page;
		if (anon->an_ref > 1) {
			enter_prot = enter_prot & ~VM_PROT_WRITE;
		}
	}
}
