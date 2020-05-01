/*
 * vm_seg.h
 *
 *  Created on: 28 Apr 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_VM_VM_SEG_H_
#define SYS_DEVEL_VM_VM_SEG_H_

#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/tree.h>

extern struct vm_seg *ksegs[];

struct vm_seg_rbtree;
RB_HEAD(vm_seg_rbtree, vm_segspace);
struct vm_seg {
	struct vm_seg_rbtree 	seg_rbtree;			/* tree of segments space entries */
	struct pmap				seg_pmap;			/* private physical map */
	struct extent 			*seg_extent;		/* segment extent manager */

	/* VM Segment Space */
	char 					*seg_name;			/* segment name */
	vm_offset_t 			seg_start;			/* start address */
	vm_offset_t 			seg_end;			/* end address */
	caddr_t					seg_addr;			/* virtual address */
	vm_size_t				seg_size;			/* virtual size */
	int						seg_mtype;			/* segments malloc type */
	int						seg_mflags;			/* segments malloc flags */
};


/* Segmented Space */
struct vm_segspace {
	RB_ENTRY(vm_segspace) 	seg_rbentry;		/* tree entries */

	char 					*segs_name;
	vm_offset_t 			segs_start;			/* start address */
	vm_offset_t 			segs_end;			/* end address */
	caddr_t					segs_addr;
	vm_size_t				segs_size;			/* virtual size */
	int						segs_mtype;			/* segments malloc type */
	int						segs_mflags;		/* segments malloc flags */

	/* VM Space Related Address Segments & Sizes */

	segsz_t 				segs_tsize;			/* text size (pages) XXX */
	segsz_t 				segs_dsize;			/* data size (pages) XXX */
	segsz_t 				segs_ssize;			/* stack size (pages) */
	caddr_t					segs_taddr;			/* user virtual address of text XXX */
	caddr_t					segs_daddr;			/* user virtual address of data XXX */
	caddr_t 				segs_minsaddr;		/* user VA at min stack growth */
	caddr_t 				segs_maxsaddr;		/* user VA at max stack growth */
};

extern struct extent *vm_extents;

#define VMSEG_ALLOCATED  (1 < 0)

extern struct vm_seg 	*vm_seg_extent_create(seg, name, start, end, addr, size);
extern void				vm_seg_destroy(seg);
extern int				vm_seg_extent_alloc_region(seg, size);
extern int				vm_seg_extent_alloc_subregion(seg, name, start, end, size, alignment, boundary, result);
extern void				vm_seg_free(seg, start, size);

#endif /* SYS_DEVEL_VM_VM_SEG_H_ */
