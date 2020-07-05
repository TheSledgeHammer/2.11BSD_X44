/*
 * ovl_map.c
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 */

#include "vm/ovl/ovl_map.h"

#include <sys/param.h>
#include <sys/systm.h>
#include "vm/ovl/ovl.h"

vm_offset_t			kovl_data;
ovl_map_entry_t 	kovl_entry_free;
ovl_map_t 			kovl_free;

//vm_map_t			overlay_map;


//#ifdef OVL
extern struct pmap	overlay_pmap_store;
#define overlay_pmap (&overlay_pmap_store)
//#endif

vm_offset_t
pmap_map_overlay(virt, start, end, prot)
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int			prot;
{
	while (start < end) {
		pmap_enter(overlay_pmap, virt, start, prot, FALSE);
		virt += PAGE_SIZE;
		start += PAGE_SIZE;
	}
	return (virt);
}

void
ovl_map_startup()
{

}

struct vovlspace *
ovlspace_alloc(min, max)
	vm_offset_t min, max;
{
	register struct vovlspace *vovl;

	vovl_map_init(&vovl->vovl_map, min, max);
	vovl->vovl_refcnt = 1;
	return (vovl);
}

ovlspace_free()
{

}

ovl_map_t
ovl_map_create(min, max)
	vm_offset_t	min, max;
{
	extern ovl_map_t kovl_rmap;

}

ovl_map_init()
{

}


/* Kernel Overlay Sub-Regions Structure */
int
kovlr_compare(kovlr1, kovlr2)
	struct koverlay_region *kovlr1, *kovlr2;
{
	if (kovlr1->kovlr_start < kovlr2->kovlr_start) {
		return (-1);
	} else if (kovlr1->kovlr_start > kovlr2->kovlr_start) {
		return (1);
	}
	return (0);
}

RB_PROTOTYPE(kovlrb, koverlay_region, kovl_region, kovlr_compare);
RB_GENERATE(kovlrb, koverlay_region, kovl_region, kovlr_compare);

insert(kovlr, addr)
	struct koverlay_region *kovlr;
	caddr_t addr;
{
	int error;

	/*
	struct koverlay_region *kovlr;
	if (kovlr == NULL) {
		kovlr = (struct koverlay*) kovl;
		kovlr->kovlr_flags = KOVLR_ALLOCATED;
	} else {
		continue;
	}

	kovlr->kovlr_name = name;
	kovlr->kovlr_start = start;
	kovlr->kovlr_end =	end;
	kovlr->kovlr_size = size;

	kovlr->kovlr_addr += ex->ex_start;
	*/


	if(kovlr != NULL) {
		error = koverlay_alloc_subregion(kovl, name, start, end, size, alignment, boundary, result);
	}

	RB_INSERT(kovlrb, kovlr, addr);
}


remove(kovlr, addr)
	struct koverlay_region *kovlr;
	caddr_t addr;
{
	int error;


	RB_REMOVE(kovlrb, kovlr, addr);
}
