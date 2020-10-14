/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */

#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_segment.h>

/* to be merge into vm_segment.c */

/*
 * Change the size of the data+stack regions of the process.
 * If the size is shrinking, it's easy -- just release the extra core.
 * If it's growing, and there is core, just allocate it and copy the
 * image, taking care to reset registers to account for the fact that
 * the system's stack has moved.  If there is no core, arrange for the
 * process to be swapped out after adjusting the size requirement -- when
 * it comes in, enough core will be allocated.  After the expansion, the
 * caller will take care of copying the user's stack towards or away from
 * the data area.  The data and stack segments are separated from each
 * other.  The second argument to expand specifies which to change.  The
 * stack segment will not have to be copied again after expansion.
 */
void
vm_segment_expand(vm, segment, newsize)
	struct vmspace 	*vm;
	vm_segment_t 	segment;
	vm_size_t 	 	newsize;
{
	register vm_size_t i, n;
	caddr_t a1, a2;

	if (segment->sg_types == SEG_DATA) {
		n = vm->vm_dsize;
		vm->vm_dsize = newsize;
		a1 = vm->vm_daddr;
		if (n >= newsize) {
			n -= newsize;
			rmfree(coremap, n, a1+newsize);
			return;
		}
	} else {
		n = vm->vm_ssize;
		vm->vm_ssize = newsize;
		a1 = vm->vm_saddr;
		if (n >= newsize) {
			n -= newsize;
			vm->vm_saddr += n;
			rmfree(coremap, n, a1);
			return;
		}
	}
	if (segment->sg_types == SEG_STACK) {
		a1 = vm->vm_saddr;
		i = newsize - n;
		a2 = a1 + i;
		/*
		 * i is the amount of growth.  Copy i clicks
		 * at a time, from the top; do the remainder
		 * (n % i) separately.
		 */
		while (n >= i) {
			n -= i;
			vm_segment_copy(a1+n, a2+n, i);
		}
		vm_segment_copy(a1, a2, n);
	}
	a2 = rmalloc(coremap, newsize);
	if(a2 == NULL) {
		if (segment == SEG_DATA) {
			swapout();
		}
	}
	if (segment->sg_types == SEG_STACK) {
		vm->vm_saddr = a2;
		a2 += newsize - n;
	} else {
		vm->vm_daddr = a2;
	}
	vm_segment_copy(a1, a2, n);
	rmfree(coremap, n, a1);
}

void
vm_segment_shrink(segment, size)
	vm_segment_t segment;
	vm_size_t 	 size;
{

}

void
vm_segment_copy(src_m, dest_m, n)
	vm_segment_t src_m, dest_m;
	vm_size_t n;
{
	VM_SEGMENT_CHECK(src_m);
	VM_SEGMENT_CHECK(dest_m);

	dest_m->sg_flags &= ~SEG_CLEAN;

	pmap_copy_segment(src_m, dest_m);
}

void
pmap_copy_segment(src, dst)
	register vm_offset_t	src, dst;
{
	register int ix;

#ifdef DEBUG
	if (pmapdebug & PDB_FOLLOW)
		printf("pmap_copy_segment(%x, %x)", src, dst);
#endif
	src >>= SG_SHIFT;
	dst >>= SG_SHIFT;
	ix = 0;
	do {
		physcopyseg(src++, dst++);
	} while (++ix != i386pagesperpage);
}
