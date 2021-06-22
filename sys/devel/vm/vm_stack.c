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
		vm_data_t data = pseg->ps_data;
		DATA_SEGMENT(data, size, addr, flag);
		pseg->ps_data = data;
		break;
	case PSEG_STACK:
		vm_stack_t stack = pseg->ps_stack;
		STACK_SEGMENT(stack, size, addr, flag);
		pseg->ps_stack = stack;
		break;
	case PSEG_TEXT:
		vm_text_t text = pseg->ps_text;
		TEXT_SEGMENT(text, size, addr, flag);
		pseg->ps_text = text;
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
		vm_data_t data = pseg->ps_data;
		data = NULL;
		pseg->ps_data = data;
		break;
	case PSEG_STACK:
		vm_stack_t stack = pseg->ps_stack;
		stack = NULL;
		pseg->ps_stack = stack;
		break;
	case PSEG_TEXT:
		vm_text_t text = pseg->ps_text;
		text = NULL;
		pseg->ps_text = text;
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
			vm_data_t data = pseg->ps_data;
			DATA_EXPAND(data, newsize, newaddr);
			printf("vm_segment_register_expand: data segment expanded: newsize %l newaddr %s", data->sp_dsize, data->sp_daddr);
			pseg->ps_data = data;
			break;
		case PSEG_STACK:
			vm_stack_t stack = pseg->ps_stack;
			STACK_EXPAND(stack, newsize, newaddr);
			printf("vm_segment_register_expand: stack segment expanded: newsize %l newaddr %s", stack->sp_ssize, stack->sp_saddr);
			pseg->ps_stack = stack;
			break;
		case PSEG_TEXT:
			vm_text_t text = pseg->ps_text;
			TEXT_EXPAND(text, newsize, newaddr);
			printf("vm_segment_register_expand: text segment expanded: newsize %l newaddr %s", text->sp_tsize, text->sp_taddr);
			pseg->ps_text = text;
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
			vm_data_t data = pseg->ps_data;
			DATA_SHRINK(data, newsize, newaddr);
			printf("vm_psegment_shrink: data segment shrunk: newsize %l newaddr %s", newsize, newaddr);
			pseg->ps_data = data;
			break;
		case PSEG_STACK:
			vm_stack_t stack = pseg->ps_stack;
			STACK_SHRINK(stack, newsize, newaddr);
			printf("vm_psegment_shrink: stack segment shrunk: newsize %l newaddr %s", newsize, newaddr);
			pseg->ps_stack = stack;
			break;
		case PSEG_TEXT:
			vm_text_t text = pseg->ps_text;
			TEXT_SHRINK(text, newsize, newaddr);
			printf("vm_psegment_shrink: text segment shrunk: newsize %l newaddr %s", newsize, newaddr);
			pseg->ps_text = text;
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
vm_psegment_extent_alloc(pseg, start, size, flags)
	vm_psegment_t 	*pseg;
	u_long 			start, size;
	int 			flags;
{
	register struct extent ex;
	int error;

	ex = pseg->ps_extent;

	error = extent_alloc_region(ex, start, size, flags);
	if(error == 0) {
		printf("vm_psegment_extent_alloc: pseudo-segment extent allocated: start %l size %s", start, size);
	} else {
		goto out;
	}

out:
	extent_free(ex, start, size, flags);
	panic("vm_psegment_extent_alloc: pseudo-segment extent unable to be allocated, extent freed");
}

void
vm_psegment_extent_suballoc(pseg, size, boundary, type, flags)
	vm_psegment_t 	*pseg;
	u_long 			size;
	u_long 			boundary;
	int 			type, flags;
{
	register struct extent ex;
	int 	error;
	u_long 	alignment;

	ex = pseg->ps_extent;
	alignment = SEGMENT_SIZE;

	switch (type) {
	case PSEG_DATA:
		vm_data_t data = pseg->ps_data;
		if (data != NULL) {
			error = extent_alloc(ex, size, alignment, boundary, flags, data->sp_dresult);
			if (error == 0) {
				printf(
						"vm_psegment_extent_suballoc: data extent allocated: size %s", size);
			} else {
				goto out;
			}
		}
		break;
	case PSEG_STACK:
		vm_stack_t stack = pseg->ps_stack;
		if (stack != NULL) {
			error = extent_alloc(ex, size, alignment, boundary, flags, stack->sp_sresult);
			if (error == 0) {
				printf("vm_psegment_extent_suballoc: stack extent allocated: size %s", size);
			} else {
				goto out;
			}
		}
		break;
	case PSEG_TEXT:
		vm_text_t text = pseg->ps_text;
		if (text != NULL) {
			error = extent_alloc(ex, size, alignment, boundary, flags, text->sp_tresult);
			if (error == 0) {
				printf("vm_psegment_extent_suballoc: text extent allocated: size %s", size);
			} else {
				goto out;
			}
		}
		break;
	}

out:
	extent_free(ex, ex->ex_start, size, flags);
	panic("vm_psegment_extent_suballoc: was unable to be allocated");
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
			printf("vm_psegment_extent_free: extent freed: start %l size %s", start, size);
		} else {
			printf("vm_psegment_extent_free: extent could not be freed");
		}
	} else {
		panic("vm_psegment_extent_free: no segment extent found");
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
	} else {
		printf("vm_psegment_extent_destroy: no extent to destroy");
	}
}
