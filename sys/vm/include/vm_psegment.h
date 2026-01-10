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

#ifndef _VM_PSEGMENT_H_
#define _VM_PSEGMENT_H_

/* pseudo segment registers */
struct vm_pseudo_segment {
	/* vmspace */
    struct vmspace			*ps_vmspace;			/* vmspace pointer */

	/* pseudo segments */
    struct vm_data      	*ps_data;
    struct vm_stack     	*ps_stack;
    struct vm_text      	*ps_text;

#define ps_dsize 			ps_data->psx_dsize		/* data size (pages) XXX */
#define ps_ssize 			ps_stack->psx_ssize		/* stack size (pages) */
#define ps_tsize 			ps_text->psx_tsize		/* text size (pages) XXX */
#define ps_daddr 			ps_data->psx_daddr		/* user virtual address of data XXX */
#define ps_saddr 			ps_stack->psx_saddr		/* user virtual address of stack XXX */
#define ps_taddr 			ps_text->psx_taddr		/* user virtual address of text XXX */

	caddr_t 				ps_minsaddr;				/* user VA at min stack growth */
	caddr_t 				ps_maxsaddr;				/* user VA at max stack growth */
};

/* pseudo-segment types */
#define PSEG_DATA			S_DATA					/* data segment */
#define PSEG_STACK			S_STACK					/* stack segment */
#define PSEG_TEXT			2						/* text segment */
#define	PSEG_OVLY			3						/* overlay segment */

/* pseudo-segment flags */
#define PSEG_SEP			(PSEG_DATA | PSEG_STACK)				/* I&D seperation */
#define PSEG_NOSEP			(PSEG_DATA | PSEG_STACK | PSEG_TEXT)	/* No I&D seperation */

struct map;

#ifdef _KERNEL

void vm_psegment_init(vm_pseudo_segment_t);
void vm_psegment_release(vm_pseudo_segment_t);
void vm_psegment_expand(vm_pseudo_segment_t, segsz_t, caddr_t, int);
void vm_psegment_shrink(vm_pseudo_segment_t, segsz_t, caddr_t, int);
void vm_psegment_alloc(vm_pseudo_segment_t, struct map *, segsz_t, caddr_t, int, int);
void vm_psegment_free(vm_pseudo_segment_t, struct map *, segsz_t, caddr_t, int);

#endif /* _KERNEL */

#endif /* _VM_PSEGMENT_H_ */
