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

struct vm_segment_stack {

};

/* pseudo segment registers */
union segment_register {
	struct extent 			*sp_extent;
	long					sp_sregions;
	struct segr_data {
		segsz_t 			sp_dsize;
		caddr_t				sp_daddr;
		int 				sp_dflag;
	} sp_data;
	struct segr_stack {
		segsz_t 			sp_ssize;
		caddr_t				sp_saddr;
		int 				sp_sflag;
	} sp_stack;
	struct segr_text {
		segsz_t 			sp_tsize;
		caddr_t				sp_taddr;
		int 				sp_tflag;
	} sp_text;
/*
#define sg_daddr			sp_data.sp_daddr
#define sg_dsize			sp_data.sp_dsize
#define sg_dflag			sp_data.sp_dflag
#define sg_saddr			sp_stack.sp_saddr
#define sg_ssize			sp_stack.sp_ssize
#define sg_sflag			sp_stack.sp_sflag
#define sg_taddr			sp_text.sp_taddr
#define sg_tsize			sp_text.sp_tsize
#define sg_tflag			sp_text.sp_tflag
	*/
};

typedef struct segr_data  	segr_data_t;
typedef struct segr_stack  	segr_stack_t;
typedef struct segr_text  	segr_text_t;

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
