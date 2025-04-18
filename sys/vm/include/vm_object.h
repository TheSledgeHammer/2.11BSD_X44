/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	@(#)vm_object.h	8.4 (Berkeley) 1/9/95
 *
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 *	Virtual memory object module definitions.
 */

#ifndef	VM_OBJECT_H_
#define	VM_OBJECT_H_

#include <vm/include/vm_segment.h>
#include <vm/include/vm_pager.h>

struct vm_object {
	struct seglist			seglist;				/* Resident memory */
	RB_ENTRY(vm_object)		object_tree;			/* tree of all objects */
	u_short					flags;					/* see below */
	u_short					paging_in_progress; 	/* Paging (in or out) so don't collapse or destroy */
	simple_lock_data_t		Lock;					/* Synchronization */
	int						ref_count;				/* How many refs?? */
	vm_size_t				size;					/* Object size */

	int						resident_segment_count;	/* number of resident segments */
	int						resident_page_count;	/* number of resident pages */

	struct vm_object		*copy;					/* Object that holds copies of my changed pages */
	vm_pager_t				pager;					/* Where to get data */
	vm_offset_t				paging_offset;			/* Offset into paging space */
	struct vm_object		*shadow;				/* My shadow */
	vm_offset_t				shadow_offset;			/* Offset in shadow */
	TAILQ_ENTRY(vm_object)	cached_list;			/* for persistence */
};

/* flags */
#define OBJ_CANPERSIST	0x0001	/* allow to persist */
#define OBJ_INTERNAL	0x0002	/* internally created object */
#define OBJ_ACTIVE		0x0004	/* used to mark active objects */
#define OBJ_SHADOW		0x0006	/* mark to shadow object */
#define OBJ_COPY		0x0008	/* mark to copy object */
#define OBJ_COALESCE	0x0010	/* mark to coalesce object */
#define OBJ_COLLAPSE	0x0012	/* mark to collapse object */
#define OBJ_OVERLAY		0x0014	/* mark to transfer object to overlay */

#define VM_OBJ_KERN		(-2)

RB_HEAD(vm_object_hash_head, vm_object_hash_entry);
struct vm_object_hash_entry {
	RB_ENTRY(vm_object_hash_entry)  hash_links;	/* hash chain links */
	vm_object_t			   			object;		/* object represented */
};
typedef struct vm_object_hash_entry	*vm_object_hash_entry_t;

#ifdef	_KERNEL
struct object_rbt;
RB_HEAD(object_rbt, vm_object);
struct object_q;
TAILQ_HEAD(object_q, vm_object);

extern
struct object_q		vm_object_cached_list;	/* list of objects persisting */
extern
int					vm_object_cached;		/* size of cached list */
extern
simple_lock_data_t	vm_cache_lock;			/* lock for object cache */
extern
struct object_rbt	vm_object_tree;			/* tree of allocated objects */
extern
long				vm_object_count;		/* count of all objects */
extern
simple_lock_data_t	vm_object_tree_lock;	/* lock for object tree and count */
extern
vm_object_t			kernel_object;			/* the single kernel object */
extern
vm_object_t			kmem_object;

#define	vm_object_cache_lock()		simple_lock(&vm_cache_lock)
#define	vm_object_cache_unlock()	simple_unlock(&vm_cache_lock)
#define	vm_object_tree_lock()		simple_lock(&vm_object_tree_lock)
#define	vm_object_tree_unlock()		simple_unlock(&vm_object_tree_lock)
#endif /* KERNEL */

#define	vm_object_lock_init(object)	simple_lock_init(&(object)->Lock, "vm_object_lock")
#define	vm_object_lock(object)		simple_lock(&(object)->Lock)
#define	vm_object_unlock(object)	simple_unlock(&(object)->Lock)
#define	vm_object_lock_try(object)	simple_lock_try(&(object)->Lock)
#define	vm_object_sleep(event, object, interruptible) \
			vm_thread_sleep((event), &(object)->Lock, (interruptible))

#ifdef _KERNEL
vm_object_t	 vm_object_allocate(vm_size_t);
void		 vm_object_cache_clear(void);
void		 vm_object_cache_trim(void);
bool_t	 	 vm_object_coalesce(vm_object_t, vm_object_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_size_t);
void		 vm_object_collapse(vm_object_t);
void		 vm_object_copy(vm_object_t, vm_offset_t, vm_size_t, vm_object_t *, vm_offset_t *, bool_t *);
void 		 vm_object_deactivate_segment_pages(vm_object_t);
void		 vm_object_deallocate(vm_object_t);
void		 vm_object_enter(vm_object_t, vm_pager_t);
void		 vm_object_init(vm_size_t, int);
vm_object_t	 vm_object_lookup(vm_pager_t);
bool_t		 vm_object_segment_page_clean(vm_object_t, vm_offset_t, vm_offset_t, bool_t, bool_t);
void		 vm_object_segment_page_remove(vm_object_t, vm_offset_t, vm_offset_t);
void		 vm_object_pmap_copy(vm_object_t, vm_offset_t, vm_offset_t);
void		 vm_object_pmap_remove(vm_object_t, vm_offset_t, vm_offset_t);
void		 vm_object_print(vm_object_t, bool_t);
void		 _vm_object_print(vm_object_t, bool_t, void (*)(const char *, ...));
void		 vm_object_reference(vm_object_t);
void		 vm_object_remove(vm_pager_t);
void		 vm_object_setpager(vm_object_t, vm_pager_t, vm_offset_t, bool_t);
void		 vm_object_shadow(vm_object_t *, vm_offset_t *, vm_size_t);
void		 vm_object_terminate(vm_object_t);
void		 vm_object_mem_stats(struct vmtotal *, vm_object_t);
void		 vm_object_mark_inactive(vm_object_t);
#endif
#endif /* _VM_OBJECT_H_ */
