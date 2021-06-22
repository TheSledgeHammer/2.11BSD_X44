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
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_segment.h>
#include <devel/vm/include/vm_stack.h>
#include <devel/vm/include/vm_text.h>
#include <devel/vm/include/vm_param.h>

#include <devel/sys/malloctypes.h>

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
vm_psegment_t 	psegment;

void
vmspace_psegment_init(vm, flags)
	struct vmspace *vm;
	int 			flags;
{
	register vm_psegment_t 	*pseg;
	register vm_data_t 		data;
	register vm_stack_t 	stack;
	register vm_text_t 		text;

	/* allocated pseudo-segment manager */
	memaddr psize = (sizeof(vm->vm_data) + sizeof(vm->vm_stack) + sizeof(vm->vm_text));
	RMALLOC3(vm->vm_psegment, union vm_pseudo_segment *, sizeof(vm->vm_data), sizeof(vm->vm_stack), sizeof(vm->vm_text), sizeof(union vm_pseudo_segment *) + psize);
	pseg = &psegment = vm->vm_psegment;
	pseg->ps_vmspace = vm;
	pseg->ps_size = psize;
	pseg->ps_flags = flags;
	pseg->ps_data = data = vm->vm_data;
	pseg->ps_stack = stack = vm->vm_stack;
	pseg->ps_text = text = vm->vm_text;
}

/*
 * allocate pseudo-segments according to vmspace
 * TODO: Deal with I & D seperation.
 */
void
vmspace_segmentation_alloc(vm, flags)
	struct vmspace 	*vm;
	int 			flags;
{
	/* allocate segments */
	RMALLOC(vm->vm_data, vm_data_t, sizeof(vm->vm_data));
	RMALLOC(vm->vm_stack, vm_stack_t, sizeof(vm->vm_stack));
	RMALLOC(vm->vm_text, vm_text_t, sizeof(vm->vm_text));

	/* setup pseudo-segments to vmspace */
	DATA_SEGMENT(vm->vm_data, vm->vm_dsize, vm->vm_daddr, flags);
	STACK_SEGMENT(vm->vm_stack, vm->vm_ssize, vm->vm_saddr, flags);
	TEXT_SEGMENT(vm->vm_text, vm->vm_tsize, vm->vm_taddr, flags);

	vmspace_psegment_init(vm, flags);
}

/*
 * free pseudo-segments from vmspace.
 * TODO: Deal with I & D seperation.
 */
void
vmspace_segmentation_free(vm)
	struct vmspace 	*vm;
{
	RMFREE(vm->vm_data, vm->vm_dsize, vm->vm_daddr);
	RMFREE(vm->vm_stack, vm->vm_ssize, vm->vm_saddr);
	RMFREE(vm->vm_text, vm->vm_tsize, vm->vm_taddr);
}

/* set pseudo-segments */
void
vm_psegment_set(pseg, type, size, addr, flag)
	vm_psegment_t 	*pseg;
	segsz_t			size;
	caddr_t			addr;
	int 			type, flag;
{
	switch(type) {
	case PSEG_DATA:
		DATA_SEGMENT(pseg->ps_data, size, addr, flag);
		break;
	case PSEG_STACK:
		STACK_SEGMENT(pseg->ps_stack, size, addr, flag);
		break;
	case PSEG_TEXT:
		TEXT_SEGMENT(pseg->ps_text, size, addr, flag);
		break;
	}
}

/* unset pseudo-segments */
void
vm_psegment_unset(pseg, type)
	vm_psegment_t 	*pseg;
	int type;
{
	switch(type) {
	case PSEG_DATA:
		pseg->ps_data = NULL;
		break;
	case PSEG_STACK:
		pseg->ps_stack = NULL;
		break;
	case PSEG_TEXT:
		pseg->ps_text = NULL;
		break;
	}
}

/*
 * Expands a pseudo-segment if not null.
 */
void
vm_psegment_expand(pseg, type, newsize, newaddr)
	vm_psegment_t 	*pseg;
	int 			type;
	segsz_t 		newsize;
	caddr_t 		newaddr;
{
	if(pseg != NULL) {
		switch (type) {
		case PSEG_DATA:
			DATA_EXPAND(pseg->ps_data, newsize, newaddr);
			printf("vm_segment_register_expand: data segment expanded: newsize %l newaddr %s", newsize, newaddr);
			break;
		case PSEG_STACK:
			STACK_EXPAND(pseg->ps_stack, newsize, newaddr);
			printf("vm_segment_register_expand: stack segment expanded: newsize %l newaddr %s", newsize, newaddr);
			break;
		case PSEG_TEXT:
			TEXT_EXPAND(pseg->ps_text, newsize, newaddr);
			printf("vm_segment_register_expand: text segment expanded: newsize %l newaddr %s", newsize, newaddr);
			break;
		}
	} else {
		panic("vm_segment_register_expand: segments could not be expanded, restoring original segments size");
	}
}

/*
 * Shrinks a pseudo-segment if not null.
 */
void
vm_psegment_shrink(pseg, type, newsize, newaddr)
	vm_psegment_t 	*pseg;
	int	 			type;
	segsz_t 		newsize;
	caddr_t 		newaddr;
{
	if(pseg != NULL) {
		switch (type) {
		case PSEG_DATA:
			DATA_SHRINK(pseg->ps_data, newsize, newaddr);
			printf("vm_psegment_shrink: data segment shrunk: newsize %l newaddr %s", newsize, newaddr);
			break;
		case PSEG_STACK:
			STACK_SHRINK(pseg->ps_stack, newsize, newaddr);
			printf("vm_psegment_shrink: stack segment shrunk: newsize %l newaddr %s", newsize, newaddr);
			break;
		case PSEG_TEXT:
			TEXT_SHRINK(pseg->ps_text, newsize, newaddr);
			printf("vm_psegment_shrink: text segment shrunk: newsize %l newaddr %s", newsize, newaddr);
			break;
		}
	} else {
		panic("vm_psegment_shrink: segments could not be shrunk, restoring original segments size");
	}
}

void
vm_psegment_init(start, end)
	vm_offset_t *start, *end;
{
	register vm_psegment_t *pseg;

	pseg = &psegment;
	if(pseg == NULL) {
		RMALLOC(pseg, union vm_pseudo_segment *, sizeof(union vm_pseudo_segment *));
	}

	pseg->ps_start = start;
	pseg->ps_end = end;
	vm_psegment_extent_create(pseg, "vm_psegment", start, end, M_VMPSEG, NULL, 0, EX_WAITOK | EX_MALLOCOK);
	vm_psegment_extent_alloc(pseg, pseg->ps_start, pseg->ps_size + sizeof(union vm_pseudo_segment *), 0, EX_WAITOK | EX_MALLOCOK);
}

void
vm_psegment_extent_create(pseg, name, start, end, mtype, storage, storagesize, flags)
	vm_psegment_t *pseg;
	u_long start, end;
	caddr_t storage;
	size_t storagesize;
	int mtype, flags;
{
	pseg->ps_extent = extent_create(name, start, end, mtype, storage, storagesize, flags);
	printf("vm_region_create: new pseudo-segment extent created");
}

void
vm_psegment_extent_alloc(pseg, start, size, type, flags)
	vm_psegment_t 	*pseg;
	u_long 			start, size;
	int 			type, flags;
{
	register struct extent ex;
	int error;

	ex = pseg->ps_extent;

	switch(type) {
	case PSEG_DATA:
		vm_data_t *data = pseg->ps_data;
		if (data != NULL) {
			error = extent_alloc_region(ex, start, size, flags);
			if(error == 0) {
				printf("vm_region_alloc: data extent allocated: start %l size %s", start, size);
			} else {
				goto out;
			}
		}
		break;
	case PSEG_STACK:
		vm_stack_t *stack = pseg->ps_stack;
		if (stack != NULL) {
			error = extent_alloc_region(ex, start, size, flags);
			if(error == 0) {
				printf("vm_region_alloc: stack extent allocated: start %l size %s", start, size);
			} else {
				goto out;
			}
		}
		break;
	case PSEG_TEXT:
		vm_text_t *text = pseg->ps_text;
		if (text != NULL) {
			error = extent_alloc_region(ex, start, size, flags);
			if(error == 0) {
				printf("vm_region_alloc: text extent allocated: start %l size %s", start, size);
			} else {
				goto out;
			}
		}
		break;
	default:
		error = extent_alloc_region(ex, start, size, flags);
		if(error == 0) {
			printf("vm_region_alloc: pseudo-segment extent allocated: start %l size %s", start, size);
		} else {
			goto out;
		}
	}

out:
	extent_free(ex, start, size, flags);
	panic("vm_region_alloc: region unable to be allocated, extent freed");
}

void
vm_psegment_extent_suballoc(pseg, size, boundary, flags, result)
	vm_psegment_t 	*pseg;
	u_long 			size;
	u_long 			boundary;
	int 			flags;
	u_long 			*result;
{
	register struct extent ex;
	int error;

	ex = pseg->ps_extent;

	error = extent_alloc(ex, size, SEGMENT_SIZE, boundary, flags, result);

	if(error == 0) {
		printf("vm_region_suballoc: sub_region allocated: size %l", size);
	} else {
		panic("vm_region_suballoc: sub_region unable to be allocated");
	}
}

void
vm_psegment_extent_free(pseg, start, size, flags)
	vm_psegment_t 	*pseg;
	u_long 			start, size;
	int 			flags;
{
	register struct extent ex;
	int error;

	if(pseg != NULL) {
		ex = pseg->ps_extent;
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
vm_psegment_extent_destroy(pseg)
	vm_psegment_t 	*pseg;
{
	register struct extent ex;

	ex = pseg->ps_extent;
	if(ex != NULL) {
		extent_destroy(ex);
	}
	printf("vm_region_destroy: no extent to destroy");
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
estabur(pseg, data, stack, text, type, sep, flags)
	vm_psegment_t 	*pseg;
	vm_data_t 		data;
	vm_stack_t 		stack;
	vm_text_t 		text;
	int 			sep, flags;
{

	if(pseg == NULL || data == NULL || stack == NULL || text == NULL) {
		return (1);
	}
	if(!sep && (type = PSEG_DATA | PSEG_STACK | PSEG_TEXT)) {
		if(flags == SEG_RO) {
			vm_psegment_set(pseg, text->sp_tsize, text->sp_taddr, SEG_RO);
		} else {
			vm_psegment_set(pseg, text->sp_tsize, text->sp_taddr, SEG_RW);
		}
		vm_psegment_set(pseg, data->sp_dsize, data->sp_daddr, SEG_RW);
		vm_psegment_set(pseg, stack->sp_ssize, stack->sp_saddr, SEG_RW);
	}
	if(sep && (type == PSEG_DATA | PSEG_STACK)) {
		if (flags == SEG_RO) {
			vm_psegment_set(pseg, text->sp_tsize, text->sp_taddr, SEG_RO);
		} else {
			vm_psegment_set(pseg, text->sp_tsize, text->sp_taddr, SEG_RW);
		}
		vm_psegment_set(pseg, data->sp_dsize, data->sp_daddr, SEG_RW);
		vm_psegment_set(pseg, stack->sp_ssize, stack->sp_saddr, SEG_RW);
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
