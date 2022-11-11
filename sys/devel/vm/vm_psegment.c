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
#include <sys/malloc.h>
#include <sys/map.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_segment.h>
#include <devel/vm/include/vm_text.h>
#include <devel/vm/include/vm_param.h>

#include <devel/sys/malloctypes.h>

void
vm_psegment_startup(pseg, start, end)
	vm_psegment_t 	*pseg;
	vm_offset_t 	*start, *end;
{
	pseg->ps_start = start;
	pseg->ps_end = end;
	pseg->ps_size = end - start;

	pseg->ps_data = (struct vm_data *)rmalloc(&coremap, sizeof(struct vm_data *));
	pseg->ps_stack = (struct vm_stack *)rmalloc(&coremap, sizeof(struct vm_stack *));
	pseg->ps_text = (struct vm_text *)rmalloc(&coremap, sizeof(struct vm_text *));

	/* extents */
	vm_psegment_extent_create(pseg, "vm_psegment", pseg->ps_start, pseg->ps_end, M_VMPSEG, NULL, 0, EX_WAITOK | EX_MALLOCOK);
	vm_psegment_extent_alloc(pseg, pseg->ps_start, pseg->ps_size, EX_WAITOK | EX_MALLOCOK);

	/* extent regions (data, stack & text) */
	vm_psegment_extent_suballoc(pseg, sizeof(pseg->ps_data), 0, PSEG_DATA, EX_WAITOK | EX_MALLOCOK);
	vm_psegment_extent_suballoc(pseg, sizeof(pseg->ps_stack), 0, PSEG_STACK, EX_WAITOK | EX_MALLOCOK);
	vm_psegment_extent_suballoc(pseg, sizeof(pseg->ps_text), 0, PSEG_TEXT, EX_WAITOK | EX_MALLOCOK);

	/* initialize vm_text */
	vm_text_init(pseg->ps_text);
}

void
vm_psegment_init(seg, start, end)
	vm_segment_t 	seg;
	vm_offset_t 	*start, *end;
{
	if (&seg->sg_psegment == NULL) {
		&seg->sg_psegment = vm_psegment_allocate();
	}
	vm_psegment_startup(&seg->sg_psegment, start, end);
}

vm_psegment_t *
vm_psegment_alloc(void)
{
	register vm_psegment_t 	*pseg;

	pseg = (union vm_pseudo_segment *)rmalloc(&coremap, sizeof(*pseg));
	return (pseg);
}

void
vm_psegment_free(pseg)
	vm_psegment_t 	*pseg;
{
	rmfree(&coremap, sizeof(*pseg), pseg);
}

/*
 * Expands a pseudo-segment if not null.
 */
void
vm_psegment_expand(pseg, newsize, newaddr, type)
	vm_psegment_t 	*pseg;
	segsz_t 		newsize;
	caddr_t 		newaddr;
	int 			type;
{
	if(pseg != NULL) {
		switch (type) {
		case PSEG_DATA:
			vm_data_t data = pseg->ps_data;
			DATA_EXPAND(data, newsize, newaddr);
			printf("vm_segment_register_expand: data segment expanded: newsize %l newaddr %s", data->psx_dsize, data->psx_daddr);
			pseg->ps_data = data;
			break;
		case PSEG_STACK:
			vm_stack_t stack = pseg->ps_stack;
			STACK_EXPAND(stack, newsize, newaddr);
			printf("vm_segment_register_expand: stack segment expanded: newsize %l newaddr %s", stack->psx_ssize, stack->psx_saddr);
			pseg->ps_stack = stack;
			break;
		case PSEG_TEXT:
			vm_text_t text = pseg->ps_text;
			TEXT_EXPAND(text, newsize, newaddr);
			printf("vm_segment_register_expand: text segment expanded: newsize %l newaddr %s", text->psx_tsize, text->psx_taddr);
			pseg->ps_text = text;
			break;
		}
	} else {
		panic("vm_segment_register_expand: segments could not be expanded");
	}
}

/*
 * Shrinks a pseudo-segment if not null.
 */
void
vm_psegment_shrink(pseg, newsize, newaddr, type)
	vm_psegment_t 	*pseg;
	segsz_t 		newsize;
	caddr_t 		newaddr;
	int	 			type;
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
		panic("vm_psegment_shrink: segments could not be shrunk");
	}
}

void
vm_psegment_extent_create(pseg, name, start, end, mtype, storage, storagesize, flags)
	vm_psegment_t *pseg;
	char *name;
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
	u_long			size;
	u_long 			boundary;
	int 			type, flags;
{
	caddr_t addr;
	int 	error;

	switch (type) {
	case PSEG_DATA:
		vm_data_t data = pseg->ps_data;
		if (data != NULL) {
			error = extent_alloc(pseg->ps_extent, size, SEGMENT_SIZE, boundary, flags, data->psx_dresult);
			if (error) {
				printf("vm_psegment_extent_suballoc: data extent allocated: addr %s size %l", data->psx_daddr, size);
			} else {
				addr = data->psx_daddr;
				goto out;
			}
		}
		break;
	case PSEG_STACK:
		vm_stack_t stack = pseg->ps_stack;
		if (stack != NULL) {
			error = extent_alloc(pseg->ps_extent, size, SEGMENT_SIZE, boundary, flags, stack->psx_sresult);
			if (error) {
				printf("vm_psegment_extent_suballoc: stack extent allocated: addr %s size %l", stack->psx_saddr, size);
			} else {
				addr = stack->psx_saddr;
				goto out;
			}
		}
		break;
	case PSEG_TEXT:
		vm_text_t text = pseg->ps_text;
		if (text != NULL) {
			error = extent_alloc(pseg->ps_extent, size, SEGMENT_SIZE, boundary, flags, text->psx_tresult);
			if (error) {
				printf("vm_psegment_extent_suballoc: text extent allocated: addr %s size %l", text->psx_taddr, size);
			} else {
				addr = text->psx_taddr;
				goto out;
			}
		}
		break;
	}

out:
	vm_psegment_extent_free(pseg, size, addr, type, flags);
	panic("vm_psegment_extent_suballoc: was unable to be allocated");
}

void
vm_psegment_extent_free(pseg, size, addr, type, flags)
	vm_psegment_t 	*pseg;
	caddr_t addr;
	u_long	size;
	int type, flags;
{
	register struct extent ex;
	int error;

	ex = pseg->ps_extent;

	switch(type) {
	case PSEG_DATA:
		vm_data_t data = pseg->ps_data;
		if (data != NULL && data->psx_daddr == addr && data->psx_dsize == size) {
			error = extent_free(ex, ex->ex_start, size, flags);
			if (error) {
				printf("vm_psegment_extent_free: data extent freed: addr %s size %l", addr, size);
			} else {
				goto out;
			}
		}
		break;
	case PSEG_STACK:
		vm_stack_t stack = pseg->ps_stack;
		if (stack != NULL && stack->psx_saddr == addr && stack->psx_ssize == size) {
			error = extent_free(ex, ex->ex_start, size, flags);
			if (error) {
				printf("vm_psegment_extent_free: stack extent freed: addr %s size %l", addr, size);
			} else {
				goto out;
			}
		}
		break;
	case PSEG_TEXT:
		vm_text_t text = pseg->ps_text;
		if (text != NULL && text->psx_taddr == addr && text->psx_tsize == size) {
			error = extent_free(ex, ex->ex_start, size, flags);
			if (error) {
				printf("vm_psegment_extent_free: text extent freed: addr %s size %l", addr, size);
			} else {
				goto out;
			}
		}
		break;
	}

out:
	panic("vm_psegment_extent_free: no segment extent found");
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
