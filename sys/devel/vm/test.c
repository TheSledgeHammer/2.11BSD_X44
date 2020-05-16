/*
 * test.c
 *
 *  Created on: 4 May 2020
 *      Author: marti
 */

//#include <devel/vm/include/vm_seg.h>
#include <devel/vm/include/vm_extent.h>
#include <devel/vm/include/vm_extops.h>

struct vm_extentops vm_segment_extentops[] = {
	.vm_extent_create =	vm_segment_create,
	.vm_extent_mallocok = vm_segment_mallocok,
	.vm_extent_alloc = vm_segment_malloc,
	.vm_extent_suballoc = vm_segmentspace_malloc,
	.vm_extent_free = vm_segment_free,
	.vm_extent_destroy = vm_segment_destroy
};

int
vm_segment_create(ap)
	struct vm_extentops_create_args *ap;
{
	struct vm_extent *vext = ap->a_vext;
	struct extent *ext = vext->vext_ext;
	int error;

	EXTENTOPS(ap, vm_extent_create);
	return (0);
}

int
vm_segment_mallocok(ap)
	struct vm_extentops_mallocok_args *ap;
{
	int error;

	EXTENTOPS(ap, vm_extent_mallocok);
	return (0);
}

int
vm_segment_malloc(ap)
	struct vm_extentops_alloc_args *ap;
{
	int error;

	EXTENTOPS(ap, vm_extent_alloc);
	return (0);
}

int
vm_segmentspace_malloc(ap)
	struct vm_extentops_suballoc_args *ap;
{
	int error;

	EXTENTOPS(ap, vm_extent_suballoc);
	return (0);
}

int
vm_segment_free(ap)
	struct vm_extentops_free_args *ap;
{
	int error;

	EXTENTOPS(ap, vm_extent_free);
	return (0);
}

int
vm_segment_destroy(ap)
	struct vm_extentops_destroy_args *ap;
{
	int error;

	EXTENTOPS(ap, vm_extent_destroy);
	return (0);
}
