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

#ifndef	VM_OBJECT_
#define	VM_OBJECT_

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_segment.h>
#include <devel/vm/include/vm_pager.h>

struct vm_object {
	struct seglist					seglist;				/* resident memory segments */

	RB_ENTRY(vm_object)				object_tree;
	u_short							flags;					/* see below */
	u_short							segment_active;
	simple_lock_data_t				Lock;					/* Synchronization */
	int								ref_count;				/* How many refs?? */
	vm_size_t						size;					/* Object size */
	int								resident_segment_count;	/* number of resident segments */
	vm_pager_t						pager;					/* Where to get data */
	vm_offset_t						segment_offset;			/* Offset into segment */
	struct vm_object				*shadow;				/* My shadow */
	vm_offset_t						shadow_offset;			/* Offset in shadow */
	TAILQ_ENTRY(vm_object)			cached_list;			/* for persistence */

	/* avm */
	struct simplelock				*vmobjlock;				/* lock on memq */
	int								uo_npages;				/* # of pages in memq */
	int								uo_refs;				/* reference count */
	struct vm_pagerops				*pgops;					/* pager ops */
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

RB_HEAD(vm_object_hash_head, vm_object_hash_entry);
struct vm_object_hash_entry {
	RB_ENTRY(vm_object_hash_entry)  hash_links;	/* hash chain links */
	vm_object_t			   			object;		/* object represented */
};
typedef struct vm_object_hash_entry	*vm_object_hash_entry_t;

#ifdef	KERNEL
struct objecttree;
RB_HEAD(objecttree, vm_object);
struct objectlist;
TAILQ_HEAD(objectlist, vm_object);

struct objectlist	vm_object_cached_list;	/* rbtree of objects persisting */
int					vm_object_cached;		/* size of cached list */
simple_lock_data_t	vm_cache_lock;			/* lock for object cache */

struct objecttree	vm_object_tree;			/* rbtree of allocated objects */
long				vm_object_count;		/* count of all objects */
simple_lock_data_t	vm_object_tree_lock;	/* lock for object rbtree and count */

vm_object_t			kernel_object;			/* the single kernel object */
vm_object_t			kmem_object;

#define	vm_object_cache_lock()		simple_lock(&vm_cache_lock)
#define	vm_object_cache_unlock()	simple_unlock(&vm_cache_lock)
#endif /* KERNEL */

#define	vm_object_lock_init(object)	simple_lock_init(&(object)->Lock)
#define	vm_object_lock(object)		simple_lock(&(object)->Lock)
#define	vm_object_unlock(object)	simple_unlock(&(object)->Lock)
#define	vm_object_lock_try(object)	simple_lock_try(&(object)->Lock)
#define	vm_object_sleep(event, object, interruptible) \
			thread_sleep((event), &(object)->Lock, (interruptible))

#ifdef KERNEL
vm_object_t	 vm_object_allocate (vm_size_t);
void		 vm_object_cache_clear (void);
void		 vm_object_cache_trim (void);
/*
boolean_t	 vm_object_coalesce (vm_object_t, vm_object_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_size_t);
void		 vm_object_collapse (vm_object_t);
void		 vm_object_copy (vm_object_t, vm_offset_t, vm_size_t, vm_object_t *, vm_offset_t *, boolean_t *);
*/
void		 vm_object_deallocate (vm_object_t);
void		 vm_object_enter (vm_object_t, vm_pager_t);
void		 vm_object_init (vm_size_t);
vm_object_t	 vm_object_lookup (vm_pager_t);
void		 vm_object_reference (vm_object_t);
void		 vm_object_remove (vm_pager_t);
void		 vm_object_shadow (vm_object_t *, vm_offset_t *, vm_size_t);
void		 vm_object_terminate (vm_object_t);

/* XXX INTEREST: NetBSD: uvm_loan:
 * XXX
 */
void		vm_object_copy_to_overlay();
void		vm_object_copy_from_overlay();

void		vm_object_coalesce_segment();
void		vm_object_split_segment();
void		vm_object_shrink_segment();
void		vm_object_expand_segment();
#endif
#endif /* _VM_OBJECT_ */
