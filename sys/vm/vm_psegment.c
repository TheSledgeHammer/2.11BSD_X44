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
#include <sys/map.h>

#include <vm/include/vm.h>
#include <vm/include/vm_text.h>
#include <vm/include/vm_psegment.h>
#include <vm/include/vm_param.h>

/*
 * Pseudo-Segment memory management.
 *
 * Current Implemented Features:
 * - expanding and shrinking of the data, stack and text registers
 * - allocation and freeing of the data, stack and text registers
 */
/* Temporary version! (i.e. current methods & features are likely to change) */

/*
 initialize pseudo segment structures onto coremap
*/
void
vm_psegment_init(pseg)
    vm_pseudo_segment_t pseg;
{
	pseg = (struct vm_pseudo_segment *)rmalloc(coremap, sizeof(struct vm_pseudo_segment));
	pseg->ps_data = (struct vm_data *)rmalloc(coremap, sizeof(struct vm_data));
	pseg->ps_stack = (struct vm_stack *)rmalloc(coremap, sizeof(struct vm_stack));
	pseg->ps_text = (struct vm_text *)rmalloc(coremap, sizeof(struct vm_text));

    /* zero out contents */
	bzero(pseg, sizeof(struct vm_pseudo_segment));
	bzero(pseg->ps_data, sizeof(struct vm_data));
	bzero(pseg->ps_stack, sizeof(struct vm_stack));
	bzero(pseg->ps_text, sizeof(struct vm_text));

	vm_text_init(pseg->ps_text);
}

/*
 release pseudo segment structures from coremap
*/
void
vm_psegment_release(pseg)
	vm_pseudo_segment_t pseg;
{
	if (pseg == NULL) {
		return;
	}

	rmfree(coremap, sizeof(struct vm_data), (long)pseg->ps_data);
	rmfree(coremap, sizeof(struct vm_stack), (long)pseg->ps_stack);
	rmfree(coremap, sizeof(struct vm_text), (long)pseg->ps_text);
    rmfree(coremap, sizeof(struct vm_pseudo_segment), (long)pseg);
}

/*
 * Expands a pseudo-segment if not null.
 */
void
vm_psegment_expand(pseg, newsize, newaddr, type)
	vm_pseudo_segment_t pseg;
	segsz_t 		newsize;
	caddr_t 		newaddr;
	int 			type;
{
	vm_data_t data;
	vm_stack_t stack;
	vm_text_t text;

	if (pseg == NULL) {
		panic("vm_psegment_expand: segments could not be expanded\n");
		return;
	}

	switch (type) {
	case PSEG_DATA:
		data = pseg->ps_data;
		DATA_EXPAND(data, newsize, newaddr);
		pseg->ps_data = data;
		printf("vm_psegment_expand: data segment expanded: newsize %l newaddr %s\n", newsize, newaddr);
		break;

	case PSEG_STACK:
		stack = pseg->ps_stack;
		STACK_EXPAND(stack, newsize, newaddr);
		pseg->ps_stack = stack;
		printf("vm_psegment_expand: stack segment expanded: newsize %l newaddr %s\n", newsize, newaddr);
		break;

	case PSEG_TEXT:
		text = pseg->ps_text;
		TEXT_EXPAND(text, newsize, newaddr);
		pseg->ps_text = text;
		printf("vm_psegment_expand: text segment expanded: newsize %l newaddr %s\n", newsize, newaddr);
		break;
	}
}

/*
 * Shrinks a pseudo-segment if not null.
 */
void
vm_psegment_shrink(pseg, newsize, newaddr, type)
	vm_pseudo_segment_t 	pseg;
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
		data = pseg->ps_data;
		DATA_SHRINK(data, newsize, newaddr);
		pseg->ps_data = data;
		printf("vm_psegment_shrink: data segment shrunk: newsize %l newaddr %s\n", newsize, newaddr);
		break;

	case PSEG_STACK:
		stack = pseg->ps_stack;
		STACK_SHRINK(stack, newsize, newaddr);
		pseg->ps_stack = stack;
		printf("vm_psegment_shrink: stack segment shrunk: newsize %l newaddr %s\n", newsize, newaddr);
		break;

	case PSEG_TEXT:
		text = pseg->ps_text;
		TEXT_SHRINK(text, newsize, newaddr);
		pseg->ps_text = text;
		printf("vm_psegment_shrink: text segment shrunk: newsize %l newaddr %s\n", newsize, newaddr);
		break;
	}
}

void
vm_psegment_alloc(pseg, mp, size, addr, type, mapsize)
	vm_pseudo_segment_t 	pseg;
	struct map 		*mp;
	segsz_t 		size;
	caddr_t			addr;
	int	 			type, mapsize;
{
	vm_data_t data;
	vm_stack_t stack;
	vm_text_t text;

	if (pseg == NULL) {
		return;
	}

	switch (type) {
	case PSEG_DATA:
		data = pseg->ps_data;
		if (data != NULL) {
			rmallocate(mp, (long)addr, size, mapsize);
		} else {
			data = (vm_data_t)rmalloc(mp, size);
		}
		data->psx_dsize = size;
		data->psx_daddr = addr;
		break;

	case PSEG_STACK:
		stack = pseg->ps_stack;
		if (stack != NULL) {
			rmallocate(mp, (long)addr, size, mapsize);
		} else {
			stack = (vm_stack_t)rmalloc(mp, size);
		}
		stack->psx_ssize = size;
		stack->psx_saddr = addr;
		break;

	case PSEG_TEXT:
		text = pseg->ps_text;
		if (text != NULL) {
			rmallocate(mp, (long)addr, size, mapsize);
		} else {
			text = (vm_text_t)rmalloc(mp, size);
		}
		text->psx_tsize = size;
		text->psx_taddr = addr;
		break;
	}
}

void
vm_psegment_free(pseg, mp, size, addr, type)
	vm_pseudo_segment_t 	pseg;
	struct map 		*mp;
	segsz_t 		size;
	caddr_t			addr;
	int	 			type;
{
	vm_data_t data;
	vm_stack_t stack;
	vm_text_t text;

	if (pseg == NULL) {
		return;
	}

	switch (type) {
	case PSEG_DATA:
		data = pseg->ps_data;
		if (data != NULL) {
			if (data->psx_dsize == size && data->psx_daddr == addr) {
				rmfree(mp, size, (long)addr);
			}
		}
		break;

	case PSEG_STACK:
		stack = pseg->ps_stack;
		if (stack != NULL) {
			if (stack->psx_ssize == size && stack->psx_saddr == addr) {
				rmfree(mp, size, (long)addr);
			}
		}
		break;

	case PSEG_TEXT:
		text = pseg->ps_text;
		if (text != NULL) {
			if (text->psx_tsize == size && text->psx_taddr == addr) {
				rmfree(mp, size, (long)addr);
			}
		}
		break;
	}
}
