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

//#include <vm/include/vm.h>
#include <vm/include/vm_segment.h>
//#include <vm/include/vm_text.h>
#include <vm/include/vm_param.h>

struct vm_pseudo_segment {
	struct vmspace		*ps_vmspace;

	struct extent 		*ps_extent;

	struct vm_data		ps_data;
	struct vm_stack		ps_stack;
	struct vm_text		ps_text;

	int					ps_nentries;
	vm_size_t			ps_size;

	vm_offset_t 		ps_min;
	vm_offset_t 		ps_max;
};

typedef struct vm_pseudo_segment *vm_psegment_t;

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
	pseg->ps_min = min;
	pseg->ps_max = max;
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

	pseg = vm_psegment_malloc(sizeof(struct vm_pseudo_segment));
	if (pseg != NULL) {
		pseg->ps_data = (struct vm_data *)rmalloc(coremap, sizeof(struct vm_data *));
		pseg->ps_stack = (struct vm_stack *)rmalloc(coremap, sizeof(struct vm_stack *));
		pseg->ps_text = (struct vm_text *)rmalloc(coremap, sizeof(struct vm_text *));
		vm_psegment_extent_create(pseg, min, max, NULL, 0, EX_WAITOK | EX_MALLOCOK);
		if (pseg->ps_extent != NULL) {
			vm_psegment_extent_alloc_region(pseg, min, max - min, EX_WAITOK | EX_MALLOCOK);
		}
	}

	vm_psegment_init(pseg, min, max);
	vm_text_init(&pseg->ps_text);

	return (pseg);
}

void
vm_psegment_create_segments(pseg, start, end, type, flags)
	vm_psegment_t pseg;
	vm_offset_t start, end;
	int type, flags;
{
	vm_data_t data;
	vm_stack_t stack;
	vm_text_t text;

	if (pseg != NULL) {
		if (start < end && start >= pseg->ps_min && end < pseg->ps_max) {
			switch (type) {
			case PSEG_DATA:
				data = &pseg->ps_data;
				if (data != NULL) {
					vm_psegment_extent_alloc_subregion(pseg, start, end,
							sizeof(data), flags, &data->psx_dresult);
					pseg->ps_nentries++;
				}
				break;

			case PSEG_STACK:
				stack = &pseg->ps_stack;
				if (stack != NULL) {
					vm_psegment_extent_alloc_subregion(pseg, start, end,
							sizeof(stack), flags, &stack->psx_sresult);
					pseg->ps_nentries++;
				}
				break;

			case PSEG_TEXT:
				text = &pseg->ps_text;
				if (text != NULL) {
					vm_psegment_extent_alloc_subregion(pseg, start, end,
							sizeof(text), flags, &text->psx_tresult);
					pseg->ps_nentries++;
				}
				break;
			}
		}
	} else {
		panic("vm_psegment_create_segment: no memory map for pseudo-segments\n");
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

	if (pseg != NULL) {
		switch (type) {
		case PSEG_DATA:
			data = &pseg->ps_data;
			DATA_EXPAND(data, newsize, newaddr);
			pseg->ps_data = *data;
			printf("vm_psegment_expand: data segment expanded: newsize %l newaddr %s\n", data->psx_dsize, data->psx_daddr);
			break;

		case PSEG_STACK:
			stack = &pseg->ps_stack;
			STACK_EXPAND(stack, newsize, newaddr);
			pseg->ps_stack = *stack;
			printf("vm_psegment_expand: stack segment expanded: newsize %l newaddr %s\n", stack->psx_ssize, stack->psx_saddr);
			break;

		case PSEG_TEXT:
			text = &pseg->ps_text;
			TEXT_EXPAND(text, newsize, newaddr);
			pseg->ps_text = *text;
			printf("vm_psegment_expand: text segment expanded: newsize %l newaddr %s\n", text->psx_tsize, text->psx_taddr);
			break;
		}
	} else {
		panic("vm_segment_register_expand: segments could not be expanded\n");
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

	if (pseg != NULL) {
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
	} else {
		panic("vm_psegment_shrink: segments could not be shrunk\n");
	}
}

/* allocate a vm pseudo-segment structure */
vm_psegment_t
vm_psegment_malloc(size)
{
	vm_psegment_t pseg;
	pseg = (vm_psegment_t)rmalloc(coremap, size);
	return (pseg);
}

/* free a vm pseudo-segment structure */
void
vm_psegment_free(pseg, size)
	vm_psegment_t pseg;
	size_t size;
{
	rmfree(coremap, size, (long)pseg);
}

/* vm pseudo-segment extent memory management */
void
vm_psegment_extent_create(pseg, start, end, storage, storagesize, flags)
	vm_psegment_t pseg;
	u_long start, end;
	caddr_t storage;
	size_t storagesize;
	int flags;
{
	pseg->ps_extent = extent_create("vm_pseudo_segment", start, end, M_VMPSEG, storage, storagesize, flags);
	if (pseg->ps_extent == NULL) {
		vm_psegment_extent_free(pseg, start, end - start, flags);
	} else {
		printf("vm_extent_create: extent created\n");
	}
}

void
vm_psegment_extent_alloc_region(pseg, start, size, flags)
	vm_psegment_t 	pseg;
	u_long start, size;
	int flags;
{
	int error;

	error = extent_alloc_region(pseg->ps_extent, start, size, flags);
	if (error) {
		printf("vm_extent_alloc_region: extent allocated\n");
	} else {
		vm_psegment_extent_free(pseg, start, size, flags);
	}
}

void
vm_psegment_extent_alloc_subregion(pseg, start, end, size, flags, result)
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
		vm_psegment_extent_free(pseg, start, size, flags);
	}
}

void
vm_psegment_extent_free(pseg, start, size, flags)
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
vm_psegment_extent_destroy(pseg)
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
