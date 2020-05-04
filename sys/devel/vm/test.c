/*
 * test.c
 *
 *  Created on: 4 May 2020
 *      Author: marti
 */

//#include <devel/vm/include/vm_seg.h>
#include <devel/vm/include/vm_extent.h>
#include <devel/vm/include/vm_extops.h>

struct vm_segments {
	struct vm_extops *seg_extops;
};

struct vm_extops vm_segment_extops = {
	.extops_create =	vm_segment_create,
	.extops_mallocok = 	vm_segment_mallocok,
	.extops_alloc =		vm_segment_malloc,
	.extops_suballoc = 	vm_segmentspace_malloc,
	.extops_free = 		vm_segment_free,
	.extops_destroy	= 	vm_segment_destroy
};

vm_segment_create(args, name, start, end, mtype, storage, storagesize, flags)
	struct vm_extops_create_args *args;
	char *name;
	vm_offset_t start, end;
	int mtype, flags;
	caddr_t	storage;
	vm_size_t storagesize;
{
	union vm_extentloader *vmel;
	struct extent *ext = vmel->vmel_ext;
	if(ext == NULL) {
		ext = extent_load(vmel, name, start, end, mtype, storage, storagesize, flags);
	}
	struct vm_extops *extops;
	int error = extops_create(args->a_head.a_ops, ext, name, start, end, mtype, storage, storagesize, flags);
}

vm_segment_mallocok()
{

}


vm_segment_malloc()
{

}

vm_segmentspace_malloc()
{

}

vm_segment_free()
{

}

vm_segment_destroy()
{

}
