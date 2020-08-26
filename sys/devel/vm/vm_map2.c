/*
 * vm_map2.c
 *
 *  Created on: 25 Aug 2020
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <devel/vm/include/vm.h>

/* Allocate & Free Vmspace using extents */
struct vmspace *
vmspace_extent_alloc(min, max, pageable)
	vm_offset_t min, max;
	int pageable;
{
	register struct vmspace *vm;

	MALLOC(vm, struct vmspace *, sizeof(struct vmspace), M_VMMAP, M_WAITOK);
	bzero(vm, (caddr_t) &vm->vm_startcopy - (caddr_t) vm);
	vm->vm_extent = extent_create("vm_map", min, max, M_VMMAP, NULL, 0, EX_WAITOK | EX_MALLOCOK);
	if(vm->vm_extent) {
		if(extent_alloc_region(vm->vm_extent, min, max, EX_WAITOK | EX_MALLOCOK)) {
			vm_map_init(&vm->vm_map, min, max, pageable);
			pmap_pinit(&vm->vm_pmap);
			vm->vm_map.pmap = &vm->vm_pmap;		/* XXX */
			vm->vm_refcnt = 1;
		}
	}
	return (vm);
}

void
vmspace_extent_free(vm)
	register struct vmspace *vm;
{
	if (--vm->vm_refcnt == 0) {
		/*
		 * Lock the map, to wait out all other references to it.
		 * Delete all of the mappings and pages they hold,
		 * then call the pmap module to reclaim anything left.
		 */
		vm_map_lock(&vm->vm_map);
		(void) vm_map_delete(&vm->vm_map, vm->vm_map.min_offset, vm->vm_map.max_offset);
		pmap_release(&vm->vm_pmap);
		extent_free(vm->vm_extent, vm, sizeof(struct vmspace *), EX_WAITOK);
		FREE(vm, M_VMMAP);
	}
}
