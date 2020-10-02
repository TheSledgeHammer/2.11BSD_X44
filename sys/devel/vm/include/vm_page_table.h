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

struct vm_page_table {
	struct pglist                   	pt_pglist;

	RB_ENTRY(vm_page_table)				pt_tree;
    u_short								pt_flags;
    u_short								pt_paging_in_progress;		/* Paging (in or out) so don't collapse or destroy */

	simple_lock_data_t					pt_lock;					/* Synchronization */
    int 								pt_ref_count;				/* How many refs?? */
    vm_size_t							pt_size;					/* Segment size */
    int									pt_resident_page_count;		/* number of resident pages */

	struct vm_page_table				*pt_copy;					/* page table that holds copies of my changed pages */

	vm_segment_t						pt_segment;
	vm_offset_t		 					pt_paging_offset;			/* Offset into paging space */
	vm_offset_t                   		pt_offset;					/* offset in segment */

	struct vm_page_table				*pt_shadow;					/* My shadow */
	vm_offset_t							pt_shadow_offset;			/* Offset in shadow */

    TAILQ_ENTRY(vm_page_table)			pt_cache_list;				/* for persistence */
};

RB_HEAD(vm_page_table_hash_root, vm_page_table_hash_entry);
struct vm_page_table_hash_entry {
	RB_ENTRY(vm_page_table_hash_entry) 	pte_hlinks;
	vm_page_table_t						pte_pgtable;
};
typedef struct vm_page_table_hash_entry *vm_page_table_hash_entry_t;

struct pttree;
RB_HEAD(pttree, vm_page_table);
struct ptlist;
TAILQ_HEAD(ptlist, vm_page_table);

struct pttree 		vm_pagetable_tree;
long				vm_pagetable_count;			/* count of all pagetables */
simple_lock_data_t	vm_pagetable_tree_lock;		/* lock for pagetable rbtree and count */

struct ptlist		vm_pagetable_cache_list;
int					vm_pagetable_cached;		/* size of cached list */
simple_lock_data_t	vm_pagetable_cache_lock;

#define	vm_pagetable_cache_lock()			simple_lock(&vm_pagetable_cache_lock)
#define	vm_pagetable_cache_unlock()			simple_unlock(&vm_pagetable_cache_lock)

#define	vm_pagetable_lock_init(pagetable)	simple_lock_init(&(pagetable)->pt_lock)
#define	vm_pagetable_lock(pagetable)		simple_lock(&(pagetable)->pt_lock)
#define	vm_pagetable_unlock(pagetable)		simple_unlock(&(pagetable)->pt_lock)
#define	vm_pagetable_lock_try(pagetable)	simple_lock_try(&(pagetable)->pt_lock)
#define	vm_pagetable_sleep(event, pagetable, interruptible) \
			thread_sleep((event), &(pagetable)->pt_lock, (interruptible))

void			vm_pagetable_init(vm_offset_t *, vm_offset_t *);
vm_page_table_t	vm_pagetable_allocate(vm_size_t);
void			vm_pagetable_deallocate(vm_page_table_t);
void			vm_pagetable_reference(vm_page_table_t);
void			vm_pagetable_terminate(vm_page_table_t);
boolean_t		vm_pagetable_page_clean(vm_page_table_t, vm_offset_t, vm_offset_t, boolean_t, boolean_t);
void			vm_pagetable_deactivate_pages(vm_page_table_t);
void			vm_pagetable_pmap_copy(vm_page_table_t, vm_offset_t, vm_offset_t);
void			vm_pagetable_pmap_remove(vm_page_table_t, vm_offset_t, vm_offset_t);
vm_pager_t		vm_pagetable_getpager(vm_page_table_t);
void			vm_pagetable_setpager(vm_page_table_t, boolean_t);
void			vm_pagetable_shadow(vm_page_table_t, vm_offset_t, vm_size_t);
void			vm_pagetable_enter(vm_page_table_t, vm_segment_t, vm_offset_t);
vm_page_table_t	vm_pagetable_lookup(vm_segment_t, vm_offset_t);
void 			vm_pagetable_remove(vm_page_table_t);
void			vm_pagetable_cache_clear();
void			vm_pagetable_collapse(vm_page_table_t);
void			vm_pagetable_page_remove(vm_page_table_t, vm_offset_t, vm_offset_t);
boolean_t		vm_pagetable_coalesce(vm_page_table_t, vm_page_table_t, vm_offset_t, vm_offset_t, vm_size_t, vm_size_t);

#endif /* _VM_PAGE_TABLE_H_ */
