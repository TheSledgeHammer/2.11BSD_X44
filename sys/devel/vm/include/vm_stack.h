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

struct vm_extent {
	struct extent 			*ve_extent;
	long					ve_result;
};

/*
 * Data structure
 */
struct vm_data {
    vm_offset_t         *sp_dstart;
    vm_offset_t         *sp_dend;
    segsz_t 			sp_dsize;
	caddr_t				sp_daddr;
	int 				sp_dflag;
};

/*
 * Stack structure
 */
struct vm_stack {
    vm_offset_t         *sp_sstart;
    vm_offset_t         *sp_send;
	segsz_t 			sp_ssize;
	caddr_t				sp_saddr;
    int 				sp_sflag;
};

/* pseudo segment registers */
union vm_pseudo_segment {
    struct vm_data      ps_data;
    struct vm_stack     ps_stack;
    struct vm_text      ps_text;
    int 				ps_flags;		/* flags */

	struct extent 		*ps_extent;		/* segments extent allocator */
	long				ps_sregions;	/* segments extent result */
};

/* pseudo-segment types */
#define SEG_DATA			1		/* data segment */
#define SEG_STACK			2		/* stack segment */
#define SEG_TEXT			3		/* text segment */

/* pseudo-segment macros */
#define DATA_SEGMENT(data, dsize, daddr, dflag) {		\
	(data)->sp_dsize = (dsize);							\
	(data)->sp_daddr = (daddr);							\
	(data)->sp_dflag = (dflag);							\
};

#define DATA_EXPAND(data, dsize, daddr) {				\
	(data)->sp_dsize += (dsize);						\
	(data)->sp_daddr += (daddr);						\
};

#define DATA_SHRINK(data, dsize, daddr) {				\
	(data)->sp_dsize -= (dsize);						\
	(data)->sp_daddr -= (daddr);						\
};

#define STACK_SEGMENT(stack, ssize, saddr, sflag) {		\
	(stack)->sp_ssize = (ssize);						\
	(stack)->sp_saddr = (saddr);						\
	(stack)->sp_sflag = (sflag);						\
};

#define STACK_EXPAND(stack, ssize, saddr) {				\
	(stack)->sp_ssize += (ssize);						\
	(stack)->sp_saddr += (ssize);						\
};

#define STACK_SHRINK(stack, ssize, saddr) {				\
	(stack)->sp_ssize -= (ssize);						\
	(stack)->sp_saddr -= (ssize);						\
};

#define TEXT_SEGMENT(text, tsize, taddr, tflag) {		\
	(text)->sp_tsize = (tsize);							\
	(text)->sp_taddr = (taddr);							\
	(text)->sp_tflag = (tflag);							\
};

#define TEXT_EXPAND(text, tsize, taddr) {				\
	(text)->sp_tsize += (tsize);						\
	(text)->sp_taddr += (taddr);						\
};

#define TEXT_SHRINK(text, tsize, taddr) {				\
	(text)->sp_tsize -= (tsize);						\
	(text)->sp_taddr -= (taddr);						\
};

#endif /* _VM_STACK_H_ */
