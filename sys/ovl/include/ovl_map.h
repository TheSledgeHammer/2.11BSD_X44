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

#ifndef	_OVL_MAP_
#define	_OVL_MAP_

struct ovl_map_clist;
struct ovl_map_rb_tree;

union ovl_map_object {
	struct ovl_object					*ovl_object;	/* overlay_object object */
	struct ovl_map						*share_map;		/* share map */
	struct ovl_map						*sub_map;		/* belongs to another map */
};

struct ovl_map_entry {
  CIRCLEQ_ENTRY(ovl_map_entry)  		cl_entry;		/* entries in a circular list */
  RB_ENTRY(ovl_map_entry) 				rb_entry;		/* tree information */
  vm_offset_t		                	start;			/* start address */
  vm_offset_t		                	end;			/* end address */
  vm_offset_t							ownspace;		/* free space after */
  vm_offset_t							space;			/* space in subtree */
  union ovl_map_object			   		object;			/* object I point to */
  vm_offset_t				          	offset;			/* offset into object */
  bool_t								is_a_map;		/* Is "object" a map? */
  bool_t								is_sub_map;		/* Is "object" a submap? */
  vm_prot_t								protection;		/* protection code */
  vm_prot_t								max_protection;	/* maximum protection */
  vm_inherit_t							inheritance;	/* inheritance */
};

CIRCLEQ_HEAD(ovl_map_clist, ovl_map_entry);
RB_HEAD(ovl_map_rb_tree, ovl_map_entry);
struct ovl_map {
	struct ovl_map_clist         		cl_header;      /* Circular List of entries */
	struct ovl_map_rb_tree				rb_root;		/* Tree of entries */
	struct pmap 						*pmap;		    /* Physical map */
    lock_data_t		                    lock;		    /* Lock for overlay data */
    int			                        nentries;	    /* Number of entries */
    vm_size_t		                    size;		    /* virtual size */
    bool_t								is_main_map;	/* Am I a main map? */
    int			                        ref_count;	    /* Reference count */
	simple_lock_data_t	                ref_lock;	    /* Lock for ref_count field */
	ovl_map_entry_t						hint;			/* hint for quick lookups */
    simple_lock_data_t	                hint_lock;	    /* lock for hint storage */
	ovl_map_entry_t						first_free;		/* First free space hint */
    unsigned int		                timestamp;	    /* Version number */

#define	min_offset			    		cl_header.cqh_first->start
#define max_offset			    		cl_header.cqh_first->end
};

#include <sys/proc.h>	/* XXX for curproc and p_pid */

#define	ovl_map_lock_drain_interlock(ovl) { 								\
	lockmgr(&(ovl)->lock, LK_DRAIN|LK_INTERLOCK, 							\
		&(ovl)->ref_lock, curproc->p_pid); 									\
	(ovl)->timestamp++; 													\
}

#ifdef DIAGNOSTIC
#define	ovl_map_lock(ovl) { 												\
	if (lockmgr(&(ovl)->lock, LK_EXCLUSIVE, (void *)0, curproc->p_pid) != 0) { \
		panic("ova_lock: failed to get lock"); 								\
	} 																		\
	(ovl)->timestamp++; 													\
}
#else
#define	ovl_map_lock(ovl) {  												\
    lockmgr(&(ovl)->lock, LK_EXCLUSIVE, (void *)0, curproc->p_pid); 		\
    (ovl)->timestamp++; 													\
}
#endif /* DIAGNOSTIC */

#define	ovl_map_unlock(ovl) 												\
		lockmgr(&(ovl)->lock, LK_RELEASE, (void *)0, curproc->p_pid)
#define	ovl_map_lock_read(ovl) 												\
		lockmgr(&(ovl)->lock, LK_SHARED, (void *)0, curproc->p_pid)
#define	ovl_map_unlock_read(ovl) 											\
		lockmgr(&(ovl)->lock, LK_RELEASE, (void *)0, curproc->p_pid)
#define ovl_map_set_recursive(ovl) { 										\
	simple_lock(&(ovl)->lk_lnterlock); 										\
	(ovl)->lk_flags |= LK_CANRECURSE; 										\
	simple_unlock(&(ovl)->lk_lnterlock); 									\
}
#define ovl_map_clear_recursive(ovl) { 										\
	simple_lock(&(ovl)->lk_lnterlock); 										\
	(ovl)->lk_flags &= ~LK_CANRECURSE; 										\
	simple_unlock(&(ovl)->lk_lnterlock); 									\
}
/*
 *	Functions implemented as macros
 */
#define	ovl_map_min(ovl)	((ovl)->min_offset)
#define	ovl_map_max(ovl)	((ovl)->max_offset)
#define	ovl_map_pmap(ovl)	((ovl)->pmap)

/* XXX: number of overlay maps and entries to statically allocate */
#define MAX_OMAP			64
#define	MAX_OMAPENT			128
#define	MAX_NOVL			(32)			/* number of overlay entries */

#ifdef _KERNEL
struct pmap;
ovl_map_t		ovl_map_create(struct pmap *, vm_offset_t, vm_offset_t);
void			ovl_map_deallocate(ovl_map_t);
int		 		ovl_map_delete(ovl_map_t, vm_offset_t, vm_offset_t);
ovl_map_entry_t	ovl_map_entry_create(ovl_map_t);
void			ovl_map_entry_delete(ovl_map_t, ovl_map_entry_t);
void			ovl_map_entry_dispose(ovl_map_t, ovl_map_entry_t);
int		 		ovl_map_find(ovl_map_t, ovl_object_t, vm_offset_t, vm_offset_t *, vm_size_t, bool_t);
int		 		ovl_map_findspace(ovl_map_t, vm_offset_t, vm_size_t, vm_offset_t *);
void			ovl_map_init(ovl_map_t, vm_offset_t, vm_offset_t);
int				ovl_map_insert(ovl_map_t, ovl_object_t, vm_offset_t, vm_offset_t, vm_offset_t);
bool_t			ovl_map_lookup_entry(ovl_map_t, vm_offset_t, ovl_map_entry_t *);
void			ovl_map_reference(ovl_map_t);
int		 		ovl_map_remove(ovl_map_t, vm_offset_t, vm_offset_t);
void			ovl_map_startup(void);
int				ovl_map_submap(ovl_map_t, vm_offset_t, vm_offset_t, ovl_map_t);
#endif
#endif /* _OVL_MAP_ */
