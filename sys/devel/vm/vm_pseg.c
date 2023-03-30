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

#include <sys/param.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/extent.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_text.h>
#include <vm/include/vm_param.h>

/*
 * TODO:
 * - map the memory of each data, stack, text segment to each non-empty vm_segment.
 * - adjust pseudo-segments extent subregion to only need the addr and size of that segment (i.e. data, stack, text)
 */

void  vm_psegment_alloc(vm_psegment_t, vm_offset_t, vm_offset_t, int, int);
void  vm_psegment_free(vm_psegment_t, vm_offset_t, segsz_t, caddr_t, int, int);

void vm_pseg_extent_create(vm_psegment_t, u_long, u_long, caddr_t, size_t, int);
void vm_pseg_extent_alloc_region(vm_psegment_t, u_long, u_long, int);
void vm_pseg_extent_alloc_subregion(vm_psegment_t, u_long, u_long, u_long, int, u_long *);
void vm_pseg_extent_free(vm_psegment_t, u_long, u_long, int);
void vm_pseg_extent_destroy(vm_psegment_t);

struct vmspace *
vmspace_alloc(min, max, pageable)
	vm_offset_t min, max;
	int pageable;
{
	register struct vmspace *vm;

	MALLOC(vm, struct vmspace *, sizeof(struct vmspace), M_VMMAP, M_WAITOK);
	bzero(vm, (caddr_t) &vm->vm_startcopy - (caddr_t) vm);
	vm_map_init(&vm->vm_map, min, max, pageable);
	pmap_pinit(&vm->vm_pmap);
	vm_psegment_init(&vm->vm_psegment, min, max);
	vm->vm_map.pmap = &vm->vm_pmap;		/* XXX */
	vm->vm_psegment.ps_vmspace = vm;
	vm->vm_refcnt = 1;
	return (vm);
}

void
vmspace_free(vm)
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
		vm_psegment_destroy(&vm->vm_psegment);
		FREE(vm, M_VMMAP);
	}
}

/*
 * initializes the pseudo-segment parameters.
 */
void
vm_psegment_init(pseg, min, max)
	vm_psegment_t pseg;
	vm_offset_t min, max;
{
	pseg->ps_nentries = 0;
	pseg->ps_size = 0;
	pseg->ps_start = min;
	pseg->ps_end = max;
}

/*
 * allocates data, stack and text pseudo-segments, creates
 * and allocates the extent map for those pseudo-segments.
 */
vm_psegment_t
vm_psegment_create(min, max)
	vm_offset_t min, max;
{
	register vm_psegment_t pseg;

	pseg = (vm_psegment_t)rmalloc(coremap, sizeof(struct vm_pseudo_segment));
	if (pseg != NULL) {
		pseg->ps_data = (struct vm_data *)rmalloc(coremap, sizeof(struct vm_data *));
		pseg->ps_stack = (struct vm_stack *)rmalloc(coremap, sizeof(struct vm_stack *));
		pseg->ps_text = (struct vm_text *)rmalloc(coremap, sizeof(struct vm_text *));
		vm_pseg_extent_create(pseg, min, max, NULL, 0, EX_WAITOK | EX_MALLOCOK);
		if (pseg->ps_extent != NULL) {
			vm_pseg_extent_alloc_region(pseg, min, max - min, EX_WAITOK | EX_MALLOCOK);
		}
	}

	vm_psegment_init(pseg, min, max);
	vm_text_init(&pseg->ps_text);
	return (pseg);
}

void
vm_psegment_destroy(pseg)
	vm_psegment_t pseg;
{
	vm_pseg_extent_destroy(pseg);
	rmfree(coremap, sizeof(struct vm_pseudo_segment), (long)pseg);
}

void
vm_psegment_alloc(pseg, start, end, type, flags)
	vm_psegment_t pseg;
	vm_offset_t start, end;
	int type, flags;
{
	vm_data_t data;
	vm_stack_t stack;
	vm_text_t text;

	if (pseg == NULL) {
		return;
	}

	if (start < end && start >= pseg->ps_start && end < pseg->ps_end) {
		switch (type) {
		case PSEG_DATA:
			data = &pseg->ps_data;
			if (data != NULL) {
				vm_pseg_extent_alloc_subregion(pseg, pseg->ps_start, pseg->ps_end, sizeof(data), flags, &data->psx_dresult);
				pseg->ps_nentries++;
			}
			break;

		case PSEG_STACK:
			stack = &pseg->ps_stack;
			if (stack != NULL) {
				vm_pseg_extent_alloc_subregion(pseg, pseg->ps_start, pseg->ps_end, sizeof(stack), flags, &stack->psx_sresult);
				pseg->ps_nentries++;
			}
			break;

		case PSEG_TEXT:
			text = &pseg->ps_text;
			if (text != NULL) {
				vm_pseg_extent_alloc_subregion(pseg, pseg->ps_start, pseg->ps_end, sizeof(text), flags, &text->psx_tresult);
				pseg->ps_nentries++;
			}
			break;
		}
	}
}

void
vm_psegment_free(pseg, start, size, addr, type, flags)
	vm_psegment_t pseg;
	vm_offset_t start;
	segsz_t size;
	caddr_t addr;
	int type, flags;
{
	vm_data_t data;
	vm_stack_t stack;
	vm_text_t text;

	if (pseg == NULL) {
		return;
	}

	switch (type) {
	case PSEG_DATA:
		data = &pseg->ps_data;
		if (data != NULL) {
			if (data->psx_dsize == size && data->psx_daddr == addr) {
				vm_pseg_extent_free(pseg, start, size, flags);
				pseg->ps_nentries--;
				printf("vm_psegment_extent_free: data extent freed: size %l addr %s\n", size, addr);
			}
		} else {
			panic("vm_psegment_extent_free: no data extent found");
		}
		break;

	case PSEG_STACK:
		stack = &pseg->ps_stack;
		if (stack != NULL) {
			if (stack->psx_ssize == size && stack->psx_saddr == addr) {
				vm_pseg_extent_free(pseg, start, size, flags);
				pseg->ps_nentries--;
				printf("vm_psegment_extent_free: stack extent freed: size %l addr %s\n", size, addr);
			}
		} else {
			panic("vm_psegment_extent_free: no stack extent found");
		}
		break;

	case PSEG_TEXT:
		text = &pseg->ps_text;
		if (text != NULL) {
			if (text->psx_tsize == size && text->psx_taddr == addr) {
				vm_pseg_extent_free(pseg, start, size, flags);
				pseg->ps_nentries--;
				printf("vm_psegment_extent_free: text extent freed: size %l addr %s\n", size, addr);
			}
		} else {
			panic("vm_psegment_extent_free: no text extent found");
		}
		break;
	}
}

/*
 * Expands a pseudo-segment if not null.
 */
void
vm_psegment_expand(pseg, newsize, newaddr, type)
	vm_psegment_t 	pseg;
	segsz_t 		newsize;
	caddr_t 		newaddr;
	int 			type;
{
	vm_data_t data;
	vm_stack_t stack;
	vm_text_t text;

	if (pseg == NULL) {
		panic("vm_segment_register_expand: segments could not be expanded\n");
		return;
	}

	switch (type) {
	case PSEG_DATA:
		data = &pseg->ps_data;
		DATA_EXPAND(data, newsize, newaddr);
		pseg->ps_data = *data;
		printf("vm_psegment_expand: data segment expanded: newsize %l newaddr %s\n", newsize, newaddr);
		break;

	case PSEG_STACK:
		stack = &pseg->ps_stack;
		STACK_EXPAND(stack, newsize, newaddr);
		pseg->ps_stack = *stack;
		printf("vm_psegment_expand: stack segment expanded: newsize %l newaddr %s\n", newsize, newaddr);
		break;

	case PSEG_TEXT:
		text = &pseg->ps_text;
		TEXT_EXPAND(text, newsize, newaddr);
		pseg->ps_text = *text;
		printf("vm_psegment_expand: text segment expanded: newsize %l newaddr %s\n", newsize, newaddr);
		break;
	}
}

/*
 * Shrinks a pseudo-segment if not null.
 */
void
vm_psegment_shrink(pseg, newsize, newaddr, type)
	vm_psegment_t 	pseg;
	segsz_t 		newsize;
	caddr_t 		newaddr;
	int	 			type;
{
	vm_data_t 	data;
	vm_stack_t 	stack;
	vm_text_t 	text;

	if (pseg == NULL) {
		panic("vm_psegment_shrink: segments could not be shrunk\n");
		return;
	}

	switch (type) {
	case PSEG_DATA:
		data = &pseg->ps_data;
		DATA_SHRINK(data, newsize, newaddr);
		pseg->ps_data = *data;
		printf("vm_psegment_shrink: data segment shrunk: newsize %l newaddr %s\n", newsize, newaddr);
		break;

	case PSEG_STACK:
		stack = &pseg->ps_stack;
		STACK_SHRINK(stack, newsize, newaddr);
		pseg->ps_stack = *stack;
		printf("vm_psegment_shrink: stack segment shrunk: newsize %l newaddr %s\n", newsize, newaddr);
		break;

	case PSEG_TEXT:
		text = &pseg->ps_text;
		TEXT_SHRINK(text, newsize, newaddr);
		pseg->ps_text = *text;
		printf("vm_psegment_shrink: text segment shrunk: newsize %l newaddr %s\n", newsize, newaddr);
		break;
	}
}

/* vm pseudo-segment extent memory management */
void
vm_pseg_extent_create(pseg, start, end, storage, storagesize, flags)
	vm_psegment_t pseg;
	u_long start, end;
	caddr_t storage;
	size_t storagesize;
	int flags;
{
	pseg->ps_extent = extent_create("vm_pseudo_segment", start, end, M_VMPSEG, storage, storagesize, flags);
	if (pseg->ps_extent == NULL) {
		vm_pseg_extent_free(pseg, start, end - start, flags);
	} else {
		printf("vm_extent_create: extent created\n");
	}
}

void
vm_pseg_extent_alloc_region(pseg, start, size, flags)
	vm_psegment_t 	pseg;
	u_long start, size;
	int flags;
{
	int error;

	error = extent_alloc_region(pseg->ps_extent, start, size, flags);
	if (error) {
		printf("vm_extent_alloc_region: extent allocated\n");
	} else {
		vm_pseg_extent_free(pseg, start, size, flags);
	}
}

void
vm_pseg_extent_alloc_subregion(pseg, start, end, size, flags, result)
	vm_psegment_t 	pseg;
	u_long 			start, end, size;
	int 			flags;
	u_long 			*result;
{
	int error;

	error = extent_alloc_subregion(pseg->ps_extent, start, end, size, PAGE_SIZE, SEGMENT_SIZE, flags, result);
	if (error) {
		printf("vm_extent_alloc_subregion: extent subregion allocated\n");
	} else {
		vm_pseg_extent_free(pseg, start, size, flags);
	}
}

void
vm_pseg_extent_free(pseg, start, size, flags)
	vm_psegment_t pseg;
	u_long start, size;
	int flags;
{
	int error;

	if (pseg != NULL) {
		if (pseg->ps_extent != NULL) {
			error = extent_free(pseg->ps_extent, start, size, flags);
			if (error) {
				printf("vm_extent_free: extent freed\n");
			} else {
				panic("vm_extent_free: no extent found\n");
			}
		}
	}
}

void
vm_pseg_extent_destroy(pseg)
	vm_psegment_t pseg;
{
	int error;

	if (pseg != NULL) {
		if (pseg->ps_extent == NULL) {
			extent_destroy(pseg);
		} else {
			panic("vm_extent_destroy: extent contains contents\n");
		}
	}
}
