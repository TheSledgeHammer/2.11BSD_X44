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
 *	@(#)vm_map.h	8.9 (Berkeley) 5/17/95
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
 *	Virtual memory map module definitions.
 */

#ifndef	_VM_MAP_
#define	_VM_MAP_

struct vm_map_clist;
struct vm_map_rb_tree;

/*
 *	Types defined:
 *
 *	vm_map_t		the high-level address map data structure.
 *	vm_map_entry_t		an entry in an address map.
 *	vm_map_version_t	a timestamp of a map, for use with vm_map_lookup
 */

/*
 *	Objects which live in maps may be either VM objects, or
 *	another map (called a "sharing map") which denotes read-write
 *	sharing with other maps.
 */

union vm_map_object {
	struct vm_object			*vm_object;	/* object object */
	struct vm_map				*share_map;	/* share map */
	struct vm_map				*sub_map;	/* belongs to another map */
};

/*
 *	Address map entries consist of start and end addresses,
 *	a VM object (or sharing map) and offset into that object,
 *	and user-exported inheritance and protection information.
 *	Also included is control information for virtual copy operations.
 */
struct vm_map_entry {
	CIRCLEQ_ENTRY(vm_map_entry) cl_entry;		/* entries in a circular list */
	RB_ENTRY(vm_map_entry) 		rb_entry;		/* tree information */
	vm_offset_t					start;			/* start address */
	vm_offset_t					end;			/* end address */
	vm_offset_t					ownspace;		/* free space after */
	vm_offset_t					space;			/* space in subtree */
	union vm_map_object			object;			/* object I point to */
	vm_offset_t					offset;			/* offset into object */
	bool_t						is_a_map;		/* Is "object" a map? */
	bool_t						is_sub_map;		/* Is "object" a submap? Only in sharing maps: */
	bool_t						copy_on_write;	/* is data copy-on-write */
	bool_t						needs_copy;		/* does object need to be copied Only in task maps: */
	vm_prot_t					protection;		/* protection code */
	vm_prot_t					max_protection;	/* maximum protection */
	vm_inherit_t				inheritance;	/* inheritance */
	int							wired_count;	/* can be paged if = 0 */
	int							advice;			/* madvise advice */
	struct vm_aref				aref;			/* anonymous overlay */
};

/*
 *	Maps are doubly-linked lists of map entries, kept sorted
 *	by address.  A single hint is provided to start
 *	searches again from the last successful search,
 *	insertion, or removal.
 */
CIRCLEQ_HEAD(vm_map_clist, vm_map_entry);
RB_HEAD(vm_map_rb_tree, vm_map_entry);
struct vm_map {
	struct vm_map_clist         cl_header;          /* Circular List of entries */
	struct vm_map_rb_tree 		rb_root;			/* Tree of entries */
	struct pmap 				*pmap;				/* Physical map */
	lock_data_t					lock;				/* Lock for map data */
	int							nentries;			/* Number of entries */
	vm_size_t					size;				/* virtual size */
	bool_t						is_main_map;		/* Am I a main map? */
	int							ref_count;			/* Reference count */
	simple_lock_data_t			ref_lock;			/* Lock for ref_count field */
	vm_map_entry_t				hint;				/* hint for quick lookups */
	simple_lock_data_t			hint_lock;			/* lock for hint storage */
	vm_map_entry_t				first_free;			/* First free space hint */
	bool_t						entries_pageable; 	/* map entries pageable?? */
	unsigned int				timestamp;			/* Version number */
#define	min_offset				cl_header.cqh_first->start
#define max_offset				cl_header.cqh_first->end
};

/*
 *	Map versions are used to validate a previous lookup attempt.
 *
 *	Since lookup operations may involve both a main map and
 *	a sharing map, it is necessary to have a timestamp from each.
 *	[If the main map timestamp has changed, the share_map and
 *	associated timestamp are no longer valid; the map version
 *	does not include a reference for the imbedded share_map.]
 */
typedef struct {
	int							main_timestamp;
	vm_map_t					share_map;
	int							share_timestamp;
} vm_map_version_t;

/*
 *	Macros:		vm_map_lock, etc.
 *	Function:
 *		Perform locking on the data portion of a map.
 */

#include <sys/proc.h>	/* XXX for curproc and p_pid */

#define	vm_map_lock_drain_interlock(map) { 						\
	lockmgr(&(map)->lock, LK_DRAIN|LK_INTERLOCK, 				\
		&(map)->ref_lock, curproc->p_pid); 						\
	(map)->timestamp++; 										\
}
#ifdef DIAGNOSTIC
#define	vm_map_lock(map) { 										\
	if (lockmgr(&(map)->lock, LK_EXCLUSIVE, (void *)0, curproc->p_pid) != 0) { \
		panic("vm_map_lock: failed to get lock"); 				\
	} 															\
	(map)->timestamp++; 										\
}
#else
#define	vm_map_lock(map) { 										\
	lockmgr(&(map)->lock, LK_EXCLUSIVE, (void *)0, curproc->p_pid); 	\
	(map)->timestamp++; 										\
}
#endif /* DIAGNOSTIC */
#define	vm_map_unlock(map) 										\
		lockmgr(&(map)->lock, LK_RELEASE, (void *)0, curproc->p_pid)
#define	vm_map_lock_read(map) 									\
		lockmgr(&(map)->lock, LK_SHARED, (void *)0, curproc->p_pid)
#define	vm_map_unlock_read(map) 								\
		lockmgr(&(map)->lock, LK_RELEASE, (void *)0, curproc->p_pid)
#define vm_map_set_recursive(map) { 							\
	simple_lock(&(map)->lk_lnterlock); 							\
	(map)->lk_flags |= LK_CANRECURSE; 							\
	simple_unlock(&(map)->lk_lnterlock); 						\
}
#define vm_map_clear_recursive(map) { 							\
	simple_lock(&(map)->lk_lnterlock); 							\
	(map)->lk_flags &= ~LK_CANRECURSE; 							\
	simple_unlock(&(map)->lk_lnterlock); 						\
}

/*
 *	Functions implemented as macros
 */
#define	vm_map_min(map)		((map)->min_offset)
#define	vm_map_max(map)		((map)->max_offset)
#define	vm_map_pmap(map)	((map)->pmap)

/* XXX: number of kernel maps and entries to statically allocate */
#define MAX_KMAP	10
#define	MAX_KMAPENT	500

#ifdef _KERNEL
bool_t		vm_map_check_protection(vm_map_t, vm_offset_t, vm_offset_t, vm_prot_t);
int		 	vm_map_copy(vm_map_t, vm_map_t, vm_offset_t, vm_size_t, vm_offset_t, bool_t, bool_t);
void		vm_map_copy_entry (vm_map_t, vm_map_t, vm_map_entry_t, vm_map_entry_t);
struct pmap;
vm_map_t	vm_map_create(struct pmap *, vm_offset_t, vm_offset_t, bool_t);
void		vm_map_deallocate(vm_map_t);
int		 	vm_map_delete(vm_map_t, vm_offset_t, vm_offset_t);
vm_map_entry_t	 vm_map_entry_create(vm_map_t);
void		vm_map_entry_delete(vm_map_t, vm_map_entry_t);
void		vm_map_entry_dispose(vm_map_t, vm_map_entry_t);
void		vm_map_entry_unwire(vm_map_t, vm_map_entry_t);
int		 	vm_map_find(vm_map_t, vm_object_t, vm_offset_t, vm_offset_t *, vm_size_t, bool_t);
int		 	vm_map_findspace(vm_map_t, vm_offset_t, vm_size_t, vm_offset_t *);
int		 	vm_map_inherit(vm_map_t, vm_offset_t, vm_offset_t, vm_inherit_t);
void		vm_map_init(struct vm_map *, vm_offset_t, vm_offset_t, bool_t);
int			vm_map_insert(vm_map_t, vm_object_t, vm_offset_t, vm_offset_t, vm_offset_t);
int		 	vm_map_lookup(vm_map_t *, vm_offset_t, vm_prot_t, vm_map_entry_t *, vm_object_t *, vm_offset_t *, vm_prot_t *, bool_t *, bool_t *);
void		vm_map_lookup_done(vm_map_t, vm_map_entry_t);
bool_t		vm_map_lookup_entry(vm_map_t, vm_offset_t, vm_map_entry_t *);
int		 	vm_map_pageable(vm_map_t, vm_offset_t, vm_offset_t, bool_t);
int		 	vm_map_clean(vm_map_t, vm_offset_t, vm_offset_t, bool_t, bool_t);
void		vm_map_print (vm_map_t, bool_t);
void		 _vm_map_print (vm_map_t, bool_t, void (*)(const char *, ...));
int		 	vm_map_protect(vm_map_t, vm_offset_t, vm_offset_t, vm_prot_t, bool_t);
void		vm_map_reference(vm_map_t);
int		 	vm_map_remove(vm_map_t, vm_offset_t, vm_offset_t);
void		vm_map_simplify(vm_map_t, vm_offset_t);
void		vm_map_simplify_entry(vm_map_t, vm_map_entry_t);
void		vm_map_startup(void);
int			vm_map_submap(vm_map_t, vm_offset_t, vm_offset_t, vm_map_t);
int			vm_map_advice(vm_map_t, vm_offset_t, vm_offset_t, int);
int			vm_map_willneed(vm_map_t, vm_offset_t, vm_offset_t);
void        vm_map_clip_start(vm_map_t, vm_map_entry_t, vm_offset_t);
void        vm_map_clip_end(vm_map_t, vm_map_entry_t, vm_offset_t);

#endif
#endif /* _VM_MAP_ */
