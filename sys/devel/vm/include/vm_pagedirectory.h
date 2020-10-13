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

#ifndef _VM_PAGE_TABLE_H_
#define _VM_PAGE_TABLE_H_

#include <sys/queue.h>
#include <sys/tree.h>

struct vm_pagedirectory {
	struct pglist                   		pd_pglist;

	RB_ENTRY(vm_pagedirectory)				pd_tree;
    u_short									pd_flags;
    u_short									pd_paging_in_progress;		/* Paging (in or out) so don't collapse or destroy */

	simple_lock_data_t						pd_lock;					/* Synchronization */
    int 									pd_ref_count;				/* How many refs?? */
    vm_size_t								pd_size;					/* Segment size */
    int										pd_resident_page_count;		/* number of resident pages */

	struct vm_pagedirectory					*pd_copy;					/* page table that holds copies of my changed pages */

	vm_segment_t							pd_segment;
	vm_offset_t		 						pd_paging_offset;			/* Offset into paging space */
	vm_offset_t                   			pd_offset;					/* offset in segment */

	struct vm_pagedirectory					*pd_shadow;					/* My shadow */
	vm_offset_t								pd_shadow_offset;			/* Offset in shadow */

	vm_pager_t								pd_pager;

    TAILQ_ENTRY(vm_pagedirectory)			pd_cache_list;				/* for persistence */
};

RB_HEAD(vm_pagedirectory_hash_root, vm_pagedirectory_hash_entry);
struct vm_pagedirectory_hash_entry {
	RB_ENTRY(vm_pagedirectory_hash_entry) 	pde_hlinks;
	vm_pagedirectory_t						pde_pdtable;
};
typedef struct vm_pagedirectory_hash_entry *vm_pagedirectory_hash_entry_t;

struct pdtree;
RB_HEAD(pdtree, vm_pagedirectory);
struct pdlist;
TAILQ_HEAD(pdlist, vm_pagedirectory);

struct pdtree 		vm_pagedirectory_tree;
long				vm_pagedirectory_count;			/* count of all pagedirectory */
simple_lock_data_t	vm_pagedirectory_tree_lock;		/* lock for pagedirectory rbtree and count */

struct pdlist		vm_pagedirectory_cache_list;
int					vm_pagedirectory_cached;		/* size of cached list */
simple_lock_data_t	vm_pagedirectory_cache_lock;

#define	vm_pagedirectory_cache_lock()				simple_lock(&vm_pagedirectory_cache_lock)
#define	vm_pagedirectory_cache_unlock()				simple_unlock(&vm_pagedirectory_cache_lock)

#define	vm_pagedirectory_lock_init(pagedirectory)	simple_lock_init(&(pagedirectory)->pd_lock)
#define	vm_pagedirectory_lock(pagedirectory)		simple_lock(&(pagedirectory)->pd_lock)
#define	vm_pagedirectory_unlock(pagedirectory)		simple_unlock(&(pagedirectory)->pd_lock)
#define	vm_pagedirectory_lock_try(pagedirectory)	simple_lock_try(&(pagedirectory)->pd_lock)
#define	vm_pagedirectory_sleep(event, pagedirectory, interruptible) \
			thread_sleep((event), &(pagedirectory)->pd_lock, (interruptible))

void				vm_pagedirectory_init(vm_offset_t *, vm_offset_t *);
vm_pagedirectory_t	vm_pagedirectory_allocate(vm_size_t);
void				vm_pagedirectory_deallocate(vm_pagedirectory_t);
void				vm_pagedirectory_reference(vm_pagedirectory_t);
void				vm_pagedirectory_terminate(vm_pagedirectory_t);
boolean_t			vm_pagedirectory_page_clean(vm_pagedirectory_t, vm_offset_t, vm_offset_t, boolean_t, boolean_t);
void				vm_pagedirectory_deactivate_pages(vm_pagedirectory_t);
void				vm_pagedirectory_pmap_copy(vm_pagedirectory_t, vm_offset_t, vm_offset_t);
void				vm_pagedirectory_pmap_remove(vm_pagedirectory_t, vm_offset_t, vm_offset_t);
vm_pager_t			vm_pagedirectory_getpager(vm_pagedirectory_t);
void				vm_pagedirectory_setpager(vm_pagedirectory_t, boolean_t);
void				vm_pagedirectory_shadow(vm_pagedirectory_t, vm_offset_t, vm_size_t);
void				vm_pagedirectory_enter(vm_pagedirectory_t, vm_segment_t, vm_offset_t);
vm_pagedirectory_t	vm_pagedirectory_lookup(vm_segment_t, vm_offset_t);
void 				vm_pagedirectory_remove(vm_pagedirectory_t);
void				vm_pagedirectory_cache_clear();
void				vm_pagedirectory_collapse(vm_pagedirectory_t);
void				vm_pagedirectory_page_remove(vm_pagedirectory_t, vm_offset_t, vm_offset_t);
boolean_t			vm_pagedirectory_coalesce(vm_pagedirectory_t, vm_pagedirectory_t, vm_offset_t, vm_offset_t, vm_size_t, vm_size_t);

#endif /* _VM_PAGE_TABLE_H_ */
