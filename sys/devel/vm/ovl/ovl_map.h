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

#ifndef	OVL_MAP_
#define	OVL_MAP_

#include "ovl.h"

struct ovl_map_clist;
struct ovl_map_rb_tree;
RB_PROTOTYPE(ovl_map_rb_tree, ovl_map_entry, ovl_rb_entry, ovl_rb_compare);

union ovl_map_object {
	struct ovl_object					*ovl_object;		/* overlay_object object */
	struct ovl_map						*ovl_sub_map;
};

struct ovl_map_entry {
  CIRCLEQ_ENTRY(ovl_map_entry)  		ovl_cl_entry;		/* entries in a circular list */
  RB_ENTRY(ovl_map_entry) 				ovl_rb_entry;		/* tree information */
  vm_offset_t		                	ovle_start;			/* start address */
  vm_offset_t		                	ovle_end;			/* end address */
  caddr_t								ovle_ownspace;		/* free space after */
  caddr_t								ovle_space;			/* space in subtree */
  union ovl_map_object			   		ovle_object;		/* object I point to */
  vm_offset_t				          	ovle_offset;		/* offset into object */

  boolean_t								ovle_is_a_map;		/* Is "object" a map? */
  boolean_t								ovle_is_sub_map;	/* Is "object" a submap? */

  vm_prot_t								ovle_protection;	/* protection code */
  vm_prot_t								ovle_max_protection;/* maximum protection */
  vm_inherit_t							ovle_inheritance;	/* inheritance */
};

CIRCLEQ_HEAD(ovl_map_clist, ovl_map_entry);
RB_HEAD(ovl_map_rb_tree, ovl_map_entry);
struct ovl_map {
	struct ovl_map_clist         		ovl_header;        	/* Circular List of entries */
	struct ovl_map_rb_tree				ovl_root;			/* Tree of entries */
	struct pmap *		           		ovl_pmap;		    /* Physical map */
    lock_data_t		                    ovl_lock;		    /* Lock for overlay data */
    int			                        ovl_nentries;	    /* Number of entries */
    vm_size_t		                    ovl_size;		    /* virtual size */

    boolean_t							ovl_is_main_map;	/* Am I a main map? */

    int			                        ovl_ref_count;	    /* Reference count */
	simple_lock_data_t	                ovl_ref_lock;	    /* Lock for ref_count field */
	ovl_map_entry_t						ovl_hint;			/* hint for quick lookups */
    simple_lock_data_t	                ovl_hint_lock;	    /* lock for hint storage */
	ovl_map_entry_t						ovl_first_free;		/* First free space hint */
    unsigned int		                ovl_timestamp;	    /* Version number */

#define	min_offset			    		ovl_header.cqh_first->ovle_start
#define max_offset			    		ovl_header.cqh_first->ovle_end
};

#define	ovl_lock_drain_interlock(ovl) { 								\
	lockmgr(&(ovl)->lock, LK_DRAIN|LK_INTERLOCK, 						\
		&(ovl)->ref_lock, curproc); 									\
	(ovl)->timestamp++; 												\
}

#ifdef DIAGNOSTIC
#define	ovl_lock(ovl) { 												\
	if (lockmgr(&(ovl)->lock, LK_EXCLUSIVE, (void *)0, curproc) != 0) { \
		panic("ova_lock: failed to get lock"); 							\
	} 																	\
	(ova)->timestamp++; 												\
}
#else
#define	ovl_lock(ovl) {  												\
    (lockmgr(&(ovl)->lock, LK_EXCLUSIVE, (void *)0, curproc) != 0); 	\
    (ovl)->timestamp++; 												\
}
#endif /* DIAGNOSTIC */

#define	ovl_unlock(ovl) \
		lockmgr(&(ovl)->lock, LK_RELEASE, (void *)0, curproc)
#define	ovl_lock_read(ovl) \
		lockmgr(&(ovl)->lock, LK_SHARED, (void *)0, curproc)
#define	ovl_unlock_read(ovl) \
		lockmgr(&(ovl)->lock, LK_RELEASE, (void *)0, curproc)

/*
 *	Functions implemented as macros
 */
#define	ovl_map_min(map)	((map)->min_offset)
#define	ovl_map_max(map)	((map)->max_offset)
#define	ovl_map_pmap(map)	((map)->ovl_pmap)

/* XXX: number of overlay maps and entries to statically allocate */
#define MAX_OMAP	64
#define	NKOVLE		(32)			/* number of kernel overlay entries */
#define	NVOVLE		(32)			/* number of virtual (vm) overlay entries */

#ifdef _KERNEL
struct pmap;
vm_map_t		ovl_map_create (struct pmap *, vm_offset_t, vm_offset_t, boolean_t);
void			ovl_map_deallocate (ovl_map_t);
int		 		ovl_map_delete (ovl_map_t, vm_offset_t, vm_offset_t);
vm_map_entry_t	ovl_map_entry_create (ovl_map_t);
void			ovl_map_entry_delete (ovl_map_t, ovl_map_entry_t);
void			ovl_map_entry_dispose (ovl_map_t, ovl_map_entry_t);
int		 		ovl_map_find (ovl_map_t, ovl_object_t, vm_offset_t, vm_offset_t *, vm_size_t, boolean_t);
int		 		ovl_map_findspace (ovl_map_t, vm_offset_t, vm_size_t, vm_offset_t *);
void			ovl_map_init (ovl_map_t, vm_offset_t, vm_offset_t, boolean_t);
int				ovl_map_insert (ovl_map_t, ovl_object_t, vm_offset_t, vm_offset_t, vm_offset_t);
boolean_t		ovl_map_lookup_entry (ovl_map_t, vm_offset_t, ovl_map_entry_t *);
//void			ovl_map_print (ovl_map_t, boolean_t);
void			ovl_map_reference (ovl_map_t);
int		 		ovl_map_remove (ovl_map_t, vm_offset_t, vm_offset_t);
void			ovl_map_startup (void);

void 			ovl_map_swapin(ovl_map_t, vm_offset_t, ovl_map_entry_t *);
void 			ovl_map_swapout(ovl_map_t, vm_offset_t, ovl_map_entry_t *);
#endif
#endif /* OVL_MAP_ */
