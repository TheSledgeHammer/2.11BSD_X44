/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm.h	7.1 (Berkeley) 6/4/86
 */
/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vm.h	8.5 (Berkeley) 5/11/95
 */

#ifndef _VM_VMSPACE_H_
#define _VM_VMSPACE_H_

/*
 * Shareable process virtual address space.
 * May eventually be merged with vm_map.
 * Several fields are temporary (text, data stuff).
 */
struct vmspace {
	struct	vm_map 	 vm_map;		/* VM address map */
	struct	pmap 	 vm_pmap;		/* private physical map */
	struct	extent	 *vm_extent;	/* extent manager */

	int				 vm_refcnt;		/* number of references */
	caddr_t			 vm_shm;		/* SYS5 shared memory private data XXX */
/* we copy from vm_startcopy to the end of the structure on fork */
#define vm_startcopy vm_rssize
	segsz_t 		 vm_rssize; 	/* current resident set size in pages */
	segsz_t 		 vm_swrss;		/* resident set size before last swap */
	segsz_t 		 vm_tsize;		/* text size (pages) XXX */
	segsz_t 		 vm_dsize;		/* data size (pages) XXX */
	segsz_t 		 vm_ssize;		/* stack size (pages) */
	caddr_t			 vm_taddr;		/* user virtual address of text XXX */
	caddr_t			 vm_daddr;		/* user virtual address of data XXX */
	caddr_t 		 vm_minsaddr;	/* user VA at min stack growth */
	caddr_t 		 vm_maxsaddr;	/* user VA at max stack growth */
};

/*
 * Shareable process anonymous virtual address space.
 */
struct avmspace {
	struct	vm_amap  avm_amap;		/* AVM anon address map */

	int				 avm_refcnt;	/* number of references */

	segsz_t 		 avm_rssize; 	/* current resident set size in pages */
	segsz_t 		 avm_swrss;		/* resident set size before last swap */
	segsz_t 		 avm_tsize;		/* text size (pages) XXX */
	segsz_t 		 avm_dsize;		/* data size (pages) XXX */
	segsz_t 		 avm_ssize;		/* stack size (pages) */
	caddr_t			 avm_taddr;		/* user virtual address of text XXX */
	caddr_t			 avm_daddr;		/* user virtual address of data XXX */
	caddr_t 		 avm_minsaddr;	/* user VA at min stack growth */
	caddr_t 		 avm_maxsaddr;	/* user VA at max stack growth */
};

/*
 * Shareable overlay address space.
 */
struct ovlspace {
	struct ovl_map 	ovl_map;		/* overlay address map */
	struct pmap 	ovl_pmap;		/* private physical map */

	struct koverlay ovl_kovl;		/* kernel overlay space */
	struct voverlay ovl_vovl;		/* virtual overlay space */
};

#ifdef _KERNEL
#include <machine/vmparam.h>
#endif /* _KERNEL */
#endif /* _VM_VMSPACE_H_ */
