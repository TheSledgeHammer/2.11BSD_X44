/*
 * vm_seg.c
 *
 *  Created on: 28 Apr 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <vm_seg.h>

struct extent *vm_extents;
static int vm_extent_malloc_safe;

void
vm_extent_init(start, end)
{
	vm_extents = extent_create("vm extents", start, end, M_VMSEG, NULL, 0, EX_NOWAIT | EX_MALLOCOK);
}

void
vm_extent_alloc(start)
	vm_offset_t start;
{
	int error;
	extent_alloc_region(&vm_extents, start, EX_NOWAIT | EX_MALLOCOK);
}

void
vm_extent_mallocok(void)
{
	vm_extent_malloc_safe = 1;
}

struct extent
vm_seg_extent(seg)
	struct vm_seg *seg;
{
	return (seg->seg_extent);
}

struct vm_seg *
vm_seg_extent_create(seg, start, end, mflags)
	struct vm_seg *seg;
	vm_offset_t start, end;
	int mflags;
{
	char *name = "vm segments allocator";
	vm_seg_init(&seg, name, start, end, NULL, 0);
	vm_seg_extent(&seg) = extent_create(name, start, end, M_VMSEG, NULL, 0, EX_NOWAIT | EX_MALLOCOK);
	return (seg);
}

void
vm_seg_init(seg, name, start, end, addr, size)
	struct vm_seg *seg;
	char *name;
	vm_offset_t start, end;
	caddr_t addr;
	vm_size_t size;
{
	RB_INIT(&seg->seg_rbtree);
	seg->seg_name = name;
	seg->seg_start = start;
	seg->seg_end = end;
	seg->seg_mtype = 0;
	seg->seg_mflags = 0;
	seg->seg_addr = addr;
	seg->seg_size = size;
}

int
vm_seg_alloc(seg, size, mflags)
	struct vm_seg *seg;
	size_t size;
	int mflags;
{
	struct extent *ex = seg->seg_extent;
	int error;

	if (ex == NULL) {
		return (0);
	}

	seg->seg_addr += ex->ex_start;
	error = extent_alloc_region(ex, seg->seg_addr, size, EX_NOWAIT | EX_MALLOCOK);

	if (error) {
		return (error);
	}

	return (0);
}

int
vm_segspace_alloc(seg, name, start, end, size, mtype, mflags, alignment, boundary, result)
	struct vm_seg *seg;
	char *name;
	vm_offset_t start, end;
	int mtype, mflags;
	u_long size, alignment, boundary;
	u_long *result;
{
	struct extent *ex = seg->seg_extent;
	register struct vm_segspace *segs;
	int error;

	if(start > seg->seg_start && start < seg->seg_end) {
		if(end > start) {
			error = extent_alloc_subregion(ex, start, end, size, alignment, boundary, EX_FAST | EX_NOWAIT | EX_MALLOCOK, result);
		} else {
			printf("the segment space end address must be greater than the segment space start address");
		}
	} else {
		printf("segment space must be with segment extent region allocated");
	}

	if(error) {
		return (error);
	}

	segs = vm_segspace_insert(seg, segs, name, start, end, mtype, mflags, size);
	seg->seg_mtype = mtype;
	seg->seg_mflags = mflags;
	return (0);
}

struct vm_segspace *
vm_segspace_insert(seg, segs, name, start, end, mtype, mflags, size)
	struct vm_seg *seg;
	struct vm_segspace *segs;
	char *name;
	vm_offset_t start, end;
	int mtype, mflags;
{
	segs->segs_name = name;
	segs->segs_start = start;
	segs->segs_end = end;
	segs->segs_mtype = mtype;
	segs->segs_mflags = mflags;
	//segs->segs_addr;
	segs->segs_size;
	segs = RB_INSERT(vm_seg_rbtree, seg->seg_rbtree, segs);
	return (segs);
}

void
vm_seg_destroy(seg)
	struct vm_seg *seg;
{
	struct extent *ex = vm_seg_extent(&seg);

	if(ex != NULL) {
		extent_destroy(ex);
	}

	if(VMSEG_ALLOCATED) {
		free(seg, M_VMSEG);
	}
}

void
vm_seg_free(seg, start, size)
	struct vm_seg *seg;
	u_long start, size;
{
	struct extent *ex = seg->seg_extent;
	if (ex != NULL) {
		vm_seg_unmap(seg, start, size);
	}
}

static void
vm_seg_unmap(seg, start, size)
	struct vm_seg *seg;
	u_long start, size;
{
	struct extent *ex = seg->seg_extent;
	int error;

	if(ex == NULL) {
		return;
	}

	error = extent_free(ex, start, size, EX_NOWAIT);

	if (error) {
		printf("%#lx-%#lx of %s space lost\n", start, start + size, ex->ex_name);
	}
}


