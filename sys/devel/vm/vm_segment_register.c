/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_mman.c	1.3 (2.11BSD) 2000/2/20
 */
/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/map.h>
#include <devel/vm/include/vm_segment.h>
#include <devel/vm/include/vm_param.h>

/*
 * use a deferred startup of pseudo segments
 * - setup pseudo segments during vmspace allocation (vm_map)
 * - Initialize pseudo segments in vm_segment
 */

/*
 * Process Segmentation:
 * Order of Change:
 * Process/Kernel:
 * 1. Process requests a change is size of segment regions
 * 2. Run X kernel call (vmcmd most likely)
 * 3. Runs through vmspace execution
 * 4. return result
 *
 * VMSpace execution:
 * 1. vm_segment copies old segments to ovlspace.
 * 2. vm_segment resizes segments, while retaining original segments in ovlspace
 * 3. Determine if segment resize was a success
 * 3a. Yes (successful). vm_segment notifies ovlspace to delete original segments.
 * 3b. No (unsuccessful). vm_segments notifies ovlspace to copy original segments back and before removing from ovlspace
 */

/* allocate segment registers according to vmspace */
void
vmspace_alloc_vm_segment_register(vm, segment, flags)
	struct vmspace 	*vm;
	vm_segment_t 	segment;
	int 			flags;
{
	RMALLOC3(segment->sg_register, union segment_register *, segment->sg_data, segment->sg_stack, segment->sg_text, sizeof(union segment_register *)); /* XXX */
	DATA_SEGMENT(segment->sg_data, vm->vm_dsize, vm->vm_daddr, flags);
	STACK_SEGMENT(segment->sg_stack, vm->vm_ssize, vm->vm_saddr, flags);
	TEXT_SEGMENT(segment->sg_text, vm->vm_tsize, vm->vm_taddr, flags);
	segment->sg_flags = flags;
}

/* free vm_segment registers from vmspace */
void
vmspace_free_vm_segment_register(vm, segment)
	struct vmspace 	*vm;
	vm_segment_t 	segment;
{
	RMFREE(segment->sg_data, segment->sg_data.sp_dsize, segment->sg_data.sp_daddr);
	RMFREE(segment->sg_stack, segment->sg_stack.sp_ssize, segment->sg_stack.sp_saddr);
	RMFREE(segment->sg_text, segment->sg_text.sp_tsize, segment->sg_text.sp_taddr);
}

/* set vm_segment register regions */
void
vm_segment_set_segment_register(segment, size, addr, flag)
	vm_segment_t 	segment;
	segsz_t			size;
	caddr_t			addr;
	int 			flag;
{
	if(segment->sg_type == SEG_DATA) {
		DATA_SEGMENT(segment->sg_data, size, addr, flag);
	}
	if(segment->sg_type == SEG_STACK) {
		STACK_SEGMENT(segment->sg_stack, size, addr, flag);
	}
	if(segment->sg_type == SEG_TEXT) {
		TEXT_SEGMENT(segment->sg_text, size, addr, flag);
	}
}

/* unset vm_segment register regions */
void
vm_segment_unset_segment_register(segment)
	vm_segment_t 	segment;
{
	if (segment->sg_type == SEG_DATA) {
		segment->sg_data = NULL;
	}
	if (segment->sg_type == SEG_STACK) {
		segment->sg_stack = NULL;
	}
	if (segment->sg_type == SEG_TEXT) {
		segment->sg_text = NULL;
	}
}

/*
 * Expands a vm_segment register region if not null.
 */
void
vm_segment_register_expand(segment, newsize, newaddr)
	vm_segment_t 	segment;
	segsz_t 		newsize;
	caddr_t 		newaddr;
{
	if (segment != NULL && segment->sg_register != NULL) {
		if (segment->sg_type == SEG_DATA) {
			DATA_EXPAND(segment->sg_data, newsize, newaddr);
			printf("vm_segment_register_expand: data segment expanded: newsize %l newaddr %s", newsize, newaddr);
		}
		if (segment->sg_type == SEG_STACK) {
			STACK_EXPAND(segment->sg_stack, newsize, newaddr);
			printf("vm_segment_register_expand: stack segment expanded: newsize %l newaddr %s", newsize, newaddr);
		}
		if (segment->sg_type == SEG_TEXT) {
			TEXT_EXPAND(segment->sg_text, newsize, newaddr);
			printf("vm_segment_register_expand: text segment expanded: newsize %l newaddr %s", newsize, newaddr);
		}
	} else {
		panic("vm_segment_register_expand: segments could not be expanded, restoring original segments size");
	}
}

/*
 * Shrinks a vm_segment register region if not null.
 */
void
vm_segment_register_shrink(segment, newsize, newaddr)
	vm_segment_t 	segment;
	segsz_t 		newsize;
	caddr_t 		newaddr;
{
	if (segment != NULL && segment->sg_register != NULL) {
		if (segment->sg_type == SEG_DATA) {
			DATA_SHRINK(segment->sg_data, newsize, newaddr);
			printf("vm_segment_register_shrink: data segment shrunk: newsize %l newaddr %s", newsize, newaddr);
		}
		if (segment->sg_type == SEG_STACK) {
			STACK_SHRINK(segment->sg_stack, newsize, newaddr);
			printf("vm_segment_register_shrink: stack segment shrunk: newsize %l newaddr %s", newsize, newaddr);
		}
		if (segment->sg_type == SEG_TEXT) {
			TEXT_SHRINK(segment->sg_text, newsize, newaddr);
			printf("vm_segment_register_shrink: text segment shrunk: newsize %l newaddr %s", newsize, newaddr);
		}
	} else {
		panic("vm_segment_register_shrink: segments could not be shrunk, restoring original segments size");
	}
}

int
sbrk(p, uap, retval)
	struct proc *p;
	struct sbrk_args  {
		syscallarg(int) 	type;
		syscallarg(segsz_t) size;
		syscallarg(caddr_t) addr;
		syscallarg(int) 	sep;
		syscallarg(int) 	flags;
		syscallarg(int) 	incr;
	} *uap;
	register_t *retval;
{
	register segsz_t n, d;

	n = btoc(uap->size);
	if (!uap->sep) {

	} else {
		n -= ctos(p->p_tsize) * stoc(1);
	}
	if (n < 0) {
		n = 0;
	}
	p->p_tsize;
	if (estabur(n, p->p_ssize, p->p_tsize, uap->sep, SEG_RO)) {
		return (0);
	}
	expand(n, S_DATA);
	/* set d to (new - old) */
	d = n - p->p_dsize;
	if (d > 0) {
		clear(p->p_daddr + p->p_dsize, d);
	}
	p->p_dsize = n;
	/* Not yet implemented */
	return (EOPNOTSUPP);
}

/*
 * Set up software prototype segmentation registers to implement the 3
 * pseudo text, data, stack segment sizes passed as arguments.  The
 * argument sep specifies if the text and data+stack segments are to be
 * separated.  The last argument determines whether the text segment is
 * read-write or read-only.
 */
int
estabur(seg, data, stack, text, sep, flags)
	vm_segment_t 	seg;
	segr_data_t 	*data;
	segr_stack_t 	*stack;
	segr_text_t 	*text;
	int 			sep, flags;
{

	if(seg == NULL || data == NULL || stack == NULL || text == NULL) {
		return (1);
	}
	if(!sep && (seg->sg_type = SEG_DATA | SEG_STACK | SEG_TEXT)) {
		if(flags == SEG_RO && seg->sg_flags == flags) {
			vm_segment_set_segment_register(seg, text->sp_tsize, text->sp_taddr, SEG_RO);
		} else {
			vm_segment_set_segment_register(seg, text->sp_tsize, text->sp_taddr, SEG_RW);
		}
		vm_segment_set_segment_register(seg, data->sp_dsize, data->sp_daddr, SEG_RW);
		vm_segment_set_segment_register(seg, stack->sp_ssize, stack->sp_saddr, SEG_RW);
	}
	if(sep && (seg->sg_type == SEG_DATA | SEG_STACK)) {
		if (flags == SEG_RO && seg->sg_flags == flags) {
			vm_segment_set_segment_register(seg, text->sp_tsize, text->sp_taddr, SEG_RO);
		} else {
			vm_segment_set_segment_register(seg, text->sp_tsize, text->sp_taddr, SEG_RW);
		}
		vm_segment_set_segment_register(seg, data->sp_dsize, data->sp_daddr, SEG_RW);
		vm_segment_set_segment_register(seg, stack->sp_ssize, stack->sp_saddr, SEG_RW);
	}
	return (0);
}

/*
 * Load the user hardware segmentation registers from the software
 * prototype.  The software registers must have been setup prior by
 * estabur.
 */
void
sureg()
{
	int taddr, daddr, saddr;

	taddr = daddr = u.u_procp->p_daddr;
	saddr = u.u_procp->p_saddr;
}

/*
 * Routine to change overlays.  Only hardware registers are changed; must be
 * called from sureg since the software prototypes will be out of date.
 */
void
choverlay(flags)
	int flags;
{

}

/*
 * WORK IN PROGRESS:
 * Design:
 * Create a segment allocator using the above methods with the methods below,
 * in conjunction with the revised vm_text methods. (see vm_text.c/.h)
 * - The allocation structures are setup with rmap while each segment is backed by
 * an extent allocator.
 * - With each text segment being managed via vm_text
 */

vm_sregion_t *
vm_region_create(vm, seg, name, start, end, mtype, flags)
	struct vmspace *vm;
	vm_segment_t seg;
	vm_size_t start, end;
	int mtype, flags;
{
	register vm_sregion_t *sregion;

	vmspace_alloc_vm_segment_register(vm, seg, flags);

	sregion = seg->sg_register;

	sregion->sp_extent = extent_create(name, start, end, mtype, 0, 0, flags);

	return (sregion);
}

void
vm_region_alloc(sregion, start, size, segtype, flags)
	vm_sregion_t 	*sregion;
	vm_size_t 		start;
	long 			size;
	int segtype, flags;
{
	struct extent ex;
	int error;

	ex = sregion->sp_extent;

	switch(segtype) {
	case SEG_DATA:
		segr_data_t *data = sregion->sp_data;
		if (data != NULL && segtype == SEG_DATA) {
			error = extent_alloc_region(ex, start, size, flags);
			if(error == 0) {
				printf("vm_region_alloc: data region allocated: start %l size %s", start, size);
			} else {
				goto out;
			}
		}
		break;
	case SEG_STACK:
		segr_stack_t *stack = sregion->sp_stack;
		if (stack != NULL && segtype == SEG_STACK) {
			error = extent_alloc_region(ex, start, size, flags);
			if(error == 0) {
				printf("vm_region_alloc: stack region allocated: start %l size %s", start, size);
			} else {
				goto out;
			}
		}
		break;
	case SEG_TEXT:
		segr_text_t *text = sregion->sp_text;
		if (text != NULL && segtype == SEG_TEXT) {
			error = extent_alloc_region(ex, start, size, flags);
			if(error == 0) {
				printf("vm_region_alloc: text region allocated: start %l size %s", start, size);
			} else {
				goto out;
			}
		}
		break;
	}

out:
	extent_free(ex, start, size, flags);
	panic("vm_region_alloc: region unable to be allocated, extent freed");
}

void
vm_region_suballoc(sregion, size, boundary, flags)
	vm_sregion_t 	*sregion;
	long 			size;
	u_long 			boundary;
	int 			flags;
{
	struct extent ex;
	int error;

	ex = sregion->sp_extent;

	error = extent_alloc(ex, size, SEGMENT_SIZE, boundary, flags, sregion->sp_sregions);

	if(error == 0) {
		printf("vm_region_suballoc: sub_region allocated: size %l", size);
	} else {
		panic("vm_region_suballoc: sub_region unable to be allocated");
	}
}

void
vm_region_free(sregion, start, size, flags)
	vm_sregion_t 	*sregion;
	vm_size_t 		start;
	long 			size;
	int 			flags;
{
	struct extent ex;
	int error;

	if(sregion != NULL) {
		ex = sregion->sp_extent;
		error = extent_free(ex, start, size, flags);
		if(error == 0) {
			printf("vm_region_free: region freed: start %l size %s", start, size);
		} else {
			printf("vm_region_free: region could not be freed");
		}
	} else {
		panic("vm_region_free: no segment regions found");
	}
}

void
vm_region_destroy(sregion)
	vm_sregion_t 	*sregion;
{
	struct extent ex;

	ex = sregion->sp_extent;
	if(ex != NULL) {
		extent_destroy(ex);
	}
	printf("vm_region_destroy: no extent to destroy");
}
