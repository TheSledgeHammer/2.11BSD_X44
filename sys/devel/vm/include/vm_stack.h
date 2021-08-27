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

#ifndef _VM_STACK_H_
#define _VM_STACK_H_

/*
 * Data structure
 */
struct vm_data {
    segsz_t 			psx_dsize;		/* data size */
	caddr_t				psx_daddr;		/* data addr */
	int 				psx_dflag;		/* data flags */
	u_long				*psx_dresult;	/* data extent */
};

/*
 * Stack structure
 */
struct vm_stack {
	segsz_t 			psx_ssize;		/* stack size */
	caddr_t				psx_saddr;		/* stack addr */
    int 				psx_sflag;		/* stack flags */
    u_long				*psx_sresult;	/* stack extent */
};

/*
 * Text structure (vm_text.h placeholder)
 */


/* pseudo segment registers */
union vm_pseudo_segment {
    struct vm_data      ps_data;
    struct vm_stack     ps_stack;
    struct vm_text      ps_text;

    struct extent 		*ps_extent;		/* segments extent allocator */
    vm_offset_t         *ps_start;		/* start of space */
    vm_offset_t         *ps_end;		/* end of space */
    size_t				ps_size;		/* total size (data + stack + text) */
    int 				ps_flags;		/* flags */
};

/* pseudo-segment types */
#define PSEG_DATA			1			/* data segment */
#define PSEG_STACK			2			/* stack segment */
#define PSEG_TEXT			3			/* text segment */

/* pseudo-segment flags */
#define PSEG_SEP			(PSEG_DATA | PSEG_STACK)				/* I&D seperation */
#define PSEG_NOSEP			(PSEG_DATA | PSEG_STACK | PSEG_TEXT)	/* No I&D seperation */

/* pseudo-segment macros */
#define DATA_SEGMENT(data, dsize, daddr, dflag) {		\
	(data)->psx_dsize = (dsize);						\
	(data)->psx_daddr = (daddr);						\
	(data)->psx_dflag = (dflag);						\
};

#define DATA_EXPAND(data, dsize, daddr) {				\
	(data)->psx_dsize += (dsize);						\
	(data)->psx_daddr += (daddr);						\
};

#define DATA_SHRINK(data, dsize, daddr) {				\
	(data)->psx_dsize -= (dsize);						\
	(data)->psx_daddr -= (daddr);						\
};

#define STACK_SEGMENT(stack, ssize, saddr, sflag) {		\
	(stack)->psx_ssize = (ssize);						\
	(stack)->psx_saddr = (saddr);						\
	(stack)->psx_sflag = (sflag);						\
};

#define STACK_EXPAND(stack, ssize, saddr) {				\
	(stack)->psx_ssize += (ssize);						\
	(stack)->psx_saddr += (saddr);						\
};

#define STACK_SHRINK(stack, ssize, saddr) {				\
	(stack)->psx_ssize -= (ssize);						\
	(stack)->psx_saddr -= (saddr);						\
};

#define TEXT_SEGMENT(text, tsize, taddr, tflag) {		\
	(text)->psx_tsize = (tsize);						\
	(text)->psx_taddr = (taddr);						\
	(text)->psx_tflag = (tflag);						\
};

#define TEXT_EXPAND(text, tsize, taddr) {				\
	(text)->psx_tsize += (tsize);						\
	(text)->psx_taddr += (taddr);						\
};

#define TEXT_SHRINK(text, tsize, taddr) {				\
	(text)->psx_tsize -= (tsize);						\
	(text)->psx_taddr -= (taddr);						\
};

void	vm_psegment_init(vm_segment_t, vm_offset_t *, vm_offset_t *);
void	vm_psegment_expand(vm_psegment_t *, int, segsz_t, caddr_t);
void	vm_psegment_shrink(vm_psegment_t *, int, segsz_t, caddr_t);
void	vm_psegment_extent_create(vm_psegment_t *, char *, u_long, u_long, int, caddr_t, size_t, int);
void	vm_psegment_extent_alloc(vm_psegment_t *, u_long, u_long, int, int);
void	vm_psegment_extent_suballoc(vm_psegment_t *, u_long, u_long, int, u_long *);
void	vm_psegment_extent_free(vm_psegment_t *, caddr_t, u_long, int, int);
void	vm_psegment_extent_destroy(vm_psegment_t *);
#endif /* _VM_STACK_H_ */
