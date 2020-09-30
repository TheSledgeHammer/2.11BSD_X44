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

#ifndef _VM_H
#define _VM_H

typedef char 					vm_inherit_t;		/* XXX: inheritance codes */

union vm_map_object;
typedef union vm_map_object 	vm_map_object_t;

struct vm_map_entry;
typedef struct vm_map_entry 	*vm_map_entry_t;

struct vm_map;
typedef struct vm_map 			*vm_map_t;

struct vm_object;
typedef struct vm_object 		*vm_object_t;

struct vm_segment;
typedef struct vm_segment 		*vm_segment_t;

struct vm_page_table;
typedef struct vm_page_table 	*vm_page_table_t;

struct vm_page;
typedef struct vm_page  		*vm_page_t;

struct pager_struct;
typedef struct pager_struct 	*vm_pager_t;

#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/user.h>
#include <sys/vmmeter.h>

#include <devel/vm/include/pmap.h>
#include <devel/vm/include/swap_pager.h>
#include <devel/vm/include/vm_extern.h>
#include <devel/vm/include/vm_inherit.h>
#include <devel/vm/include/vm_map.h>
#include <devel/vm/include/vm_object.h>
#include <devel/vm/include/vm_param.h>
#include <devel/vm/include/vm_prot.h>
#include <devel/vm/include/vm_swap.h>

#include <devel/vm/extents/vm_extent.h>		/* Work in Progress */
#include <devel/vm/segment/vm_page_table.h>	/* Work in Progress */
#include <devel/vm/segment/vm_segment.h>	/* Work in Progress */

#include <devel/vm/avm/avm.h>				/* Work in Progress */
#include <devel/vm/ovl/ovl.h>				/* Work in Progress */

/*
 *	MACH VM locking type mappings to kernel types
 */
typedef struct simplelock	simple_lock_data_t;
typedef struct simplelock	*simple_lock_t;
typedef struct lock			lock_data_t;
typedef struct lock			*lock_t;

/*
 * Shareable process virtual address space.
 * May eventually be merged with vm_map.
 * Several fields are temporary (text, data stuff).
 */
struct vmspace {
	struct	vm_map 	 	vm_map;			/* VM address map */
	struct	pmap 	 	vm_pmap;		/* private physical map */
	struct	extent	 	*vm_extent;		/* extent manager */

	int				 	vm_refcnt;		/* number of references */
	caddr_t			 	vm_shm;			/* SYS5 shared memory private data XXX */
/* we copy from vm_startcopy to the end of the structure on fork */
#define vm_startcopy 	vm_rssize
	segsz_t 		 	vm_rssize; 		/* current resident set size in pages */
	segsz_t 		 	vm_swrss;		/* resident set size before last swap */
	segsz_t 		 	vm_tsize;		/* text size (pages) XXX */
	segsz_t 		 	vm_dsize;		/* data size (pages) XXX */
	segsz_t 		 	vm_ssize;		/* stack size (pages) */
	caddr_t			 	vm_taddr;		/* user virtual address of text XXX */
	caddr_t			 	vm_daddr;		/* user virtual address of data XXX */
	caddr_t 		 	vm_minsaddr;	/* user VA at min stack growth */
	caddr_t 		 	vm_maxsaddr;	/* user VA at max stack growth */
};

struct vm {
	/* vm_page queues */
	struct pglist 		page_active; 	/* allocated pages, in use */
	struct pglist 		page_inactive; 	/* pages between the clock hands */
	struct simplelock 	pageqlock; 		/* lock for active/inactive page q */
	struct simplelock 	fpageqlock; 	/* lock for free page q */

	/* page hash */
	struct pglist 		*page_hash; 	/* page hash table (vp/off->page) */
	int 				page_nhash; 	/* number of buckets */
	int 				page_hashmask; 	/* hash mask */
	struct simplelock 	hashlock; 		/* lock on page_hash array */

	/* anon stuff */
	struct avm_anon 	*afree; 		/* anon free list */
	struct simplelock 	afreelock; 		/* lock on anon free list */

	/* static kernel map entry pool */
	struct vm_map_entry *kentry_free; 	/* free page */
	struct simplelock 	kentry_lock;

	/* swap-related items */
	struct simplelock 	swap_data_lock;

	/* kernel object: to support anonymous pageable kernel memory */
	struct vm_object 	*kernel_object;
};
extern struct vm vm;

/*
 * vmexp: global data structures that are exported to parts of the kernel
 * other than the vm system.
 */
struct vmexp {
	/* vm_page constants */
	int pagesize; 		/* size of a page (PAGE_SIZE): must be power of 2 */
	int pagemask; 		/* page mask */
	int pageshift; 		/* page shift */

	/* vm_page counters */
	int npages; 		/* number of pages we manage */
	int free; 			/* number of free pages */
	int active; 		/* number of active pages */
	int inactive; 		/* number of pages that we free'd but may want back */
	int paging; 		/* number of pages in the process of being paged out */
	int wired; 			/* number of wired pages */

	/* swap */
	int nswapdev; 		/* number of configured swap devices in system */
	int swpages; 		/* number of PAGE_SIZE'ed swap pages */
	int swpginuse; 		/* number of swap pages in use */
	int swpgonly; 		/* number of swap pages in use, not also in RAM */
	int nswget; 		/* number of times fault calls uvm_swap_get() */
	int nanon; 			/* number total of anon's in system */
	int nanonneeded;	/* number of anons currently needed */
	int nfreeanon; 		/* number of free anon's */
};
extern struct vmexp vmexp;

/*
 * vm_map_entry etype bits:
 */
#define VM_ET_OBJ				0x01	/* it is a uvm_object */
#define VM_ET_SUBMAP			0x02	/* it is a vm_map submap */
#define VM_ET_COPYONWRITE 		0x04	/* copy_on_write */
#define VM_ET_NEEDSCOPY			0x08	/* needs_copy */

#define VM_ET_ISOBJ(E)			(((E)->etype & VM_ET_OBJ) != 0)
#define VM_ET_ISSUBMAP(E)		(((E)->etype & VM_ET_SUBMAP) != 0)
#define VM_ET_ISCOPYONWRITE(E)	(((E)->etype & VM_ET_COPYONWRITE) != 0)
#define VM_ET_ISNEEDSCOPY(E)	(((E)->etype & VM_ET_NEEDSCOPY) != 0)

#ifdef _KERNEL
#include <machine/vmparam.h>
#endif /* _KERNEL */
#endif /* _VM_H */
