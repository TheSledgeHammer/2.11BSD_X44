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

#ifndef _VM_H_
#define _VM_H_

typedef char 				vm_inherit_t;		/* XXX: inheritance codes */

union vm_map_object;
typedef union vm_map_object vm_map_object_t;

struct vm_map_entry;
typedef struct vm_map_entry *vm_map_entry_t;

struct vm_map;
typedef struct vm_map 		*vm_map_t;

struct vm_object;
typedef struct vm_object 	*vm_object_t;

struct vm_segment;
typedef struct vm_segment 	*vm_segment_t;

struct vm_page;
typedef struct vm_page  	*vm_page_t;

struct pager_struct;
typedef struct pager_struct *vm_pager_t;

/*
 * VM Anonymous Memory Management
 */
struct vm_aobject;
typedef struct vm_aobject 	*vm_aobject_t;

struct vm_amap;
typedef struct vm_amap 		*vm_amap_t;

struct vm_anon;
typedef struct vm_anon 		*vm_anon_t;

struct vm_aref;
typedef struct vm_aref 		*vm_aref_t;

/*
 * VM Pseudo-Segmentation with Data, Stack & Text Management
 */
struct vm_pseudo_segment;
//typedef struct vm_pseudo_segment *vm_psegment_t;
typedef struct vm_pseudo_segment *vm_pseudo_segment_t;

struct vm_data;
typedef struct vm_data  		*vm_data_t;

struct vm_stack;
typedef struct vm_stack  		*vm_stack_t;

struct vm_text;
typedef struct vm_text 			*vm_text_t;

/*
 *	MACH VM locking type mappings to kernel types
 */
typedef struct lock_object	simple_lock_data_t;
typedef struct lock_object	*simple_lock_t;
typedef struct lock			lock_data_t;
typedef struct lock			*lock_t;

#include <sys/vmmeter.h>
#include <sys/vmsystm.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <vm/include/vm_param.h>
#include <sys/lock.h>
#include <vm/include/vm_mac.h>
#include <vm/include/vm_prot.h>
#include <vm/include/vm_inherit.h>
#include <vm/include/vm_amap.h>
#include <vm/include/vm_anon.h>
#include <vm/include/vm_map.h>
#include <vm/include/vm_object.h>
#include <vm/include/pmap.h>
#include <vm/include/vm_extern.h>
#include <vm/include/vm_text.h>
#include <vm/include/vm_psegment.h>

/*
 * Shareable process virtual address space.
 * May eventually be merged with vm_map.
 * Several fields are temporary (text, data stuff).
 */
struct vmspace {
	struct vm_map			vm_map;			/* VM address map */
	struct pmap 			vm_pmap;		/* private physical map */
	struct vm_pseudo_segment 	vm_psegment;		/* VM pseudo segment */

	int				vm_refcnt;		/* number of references */

/* we copy from vm_startcopy to the end of the structure on fork */
#define vm_startcopy 			vm_rssize
	segsz_t 			vm_rssize; 		/* current resident set size in pages */
	segsz_t 			vm_swrss;		/* resident set size before last swap */

#define vm_dsize 			vm_psegment.ps_dsize	/* data size (pages) XXX */
#define vm_ssize 			vm_psegment.ps_ssize	/* stack size (pages) */
#define vm_tsize 			vm_psegment.ps_tsize	/* text size (pages) XXX */
#define vm_daddr 			vm_psegment.ps_daddr	/* user virtual address of data XXX */
#define vm_saddr 			vm_psegment.ps_saddr	/* user virtual address of stack XXX */
#define vm_taddr 			vm_psegment.ps_taddr	/* user virtual address of text XXX */
#define vm_minsaddr			vm_psegment.ps_minsaddr	/* user VA at min stack growth */
#define vm_maxsaddr			vm_psegment.ps_maxsaddr	/* user VA at max stack growth */
};

#endif /* _VM_H_ */
