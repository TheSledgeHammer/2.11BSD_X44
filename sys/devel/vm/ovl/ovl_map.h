/*
 * ovl_map.h
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 */

#ifndef	OVL_MAP_
#define	OVL_MAP_

#include "ovl.h"

struct ovl_map_clist;
struct ovl_map_rb_tree;
RB_PROTOTYPE(ovl_map_rb_tree, ovl_map_entry, orb_entry, ovl_rb_compare);

union ovl_map_object {
	struct ovl_object					*ovl_object;		/* overlay object */
	//vm_object	/* vm_object */
	//avm_object /* avm_object */
};

struct ovl_map_entry {
  CIRCLEQ_ENTRY(ovl_map_entry)  		ocl_entry;			/* entries in a circular list */
  RB_ENTRY(ovl_map_entry) 				orb_entry;			/* tree information */
  vm_offset_t		                	ovle_start;			/* start address */
  vm_offset_t		                	ovle_end;			/* end address */
  caddr_t								ovle_ownspace;		/* free space after */
  caddr_t								ovle_space;			/* space in subtree */
  union ovl_map_object			   		ovle_object;		/* object I point to */
  vm_offset_t				          	ovle_offset;		/* offset into object */
};

CIRCLEQ_HEAD(ovl_map_clist, ovl_map_entry);
RB_HEAD(ovl_map_rb_tree, ovl_map_entry);
struct ovl_map {
	struct ovl_map_clist         		ovl_header;        	/* Circular List of entries */
	struct ovl_map_rb_tree				ovl_root;			/* Tree of entries */
	struct pmap *		           		pmap;		        /* Physical map */
    lock_data_t		                    lock;		        /* Lock for overlay data */
    int			                        nentries;	        /* Number of entries */
    vm_size_t		                    size;		        /* virtual size */
    int			                        ref_count;	        /* Reference count */
	simple_lock_data_t	                ref_lock;	        /* Lock for ref_count field */
	ovl_map_entry_t						hint;				/* hint for quick lookups */
    simple_lock_data_t	                hint_lock;	        /* lock for hint storage */
    unsigned int		                timestamp;	        /* Version number */

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

#endif /* OVL_MAP_ */
