/*
 * ovl_map.h
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 */

#ifndef	OVL_MAP_
#define	OVL_MAP_


struct vovl_map_object {
	struct koverlay	*kovl_ob;
};

struct vovl_map_entry {
    CIRCLEQ_ENTRY(ovl_map_entry)        vovl_entry;
    vm_offset_t		                    vovle_start;		    /* start address */
	vm_offset_t		                    vovle_end;		        /* end address */
};

CIRCLEQ_HEAD(vovllist, ovl_map_entry);
struct vovl_map {
    struct ovllist                      vovl_header;        /* Head of overlay List */
    struct pmap *		                pmap;		        /* Physical map */
    lock_data_t		                    lock;		        /* Lock for overlay data */
    int			                        nentries;	        /* Number of entries */
    vm_size_t		                    size;		        /* virtual size */
    int			                        ref_count;	        /* Reference count */
	simple_lock_data_t	                ref_lock;	        /* Lock for ref_count field */
    simple_lock_data_t	                hint_lock;	        /* lock for hint storage */
    unsigned int		                timestamp;	        /* Version number */

#define	min_offset		                header.cqh_first->start
#define max_offset		                header.cqh_first->end
};

#define	vovl_lock_drain_interlock(vovl) { 			\
	lockmgr(&(vovl)->lock, LK_DRAIN|LK_INTERLOCK, 	\
		&(vovl)->ref_lock, curproc); 				\
	(vovl)->timestamp++; 							\
}

#ifdef DIAGNOSTIC
#define	vovl_lock(ovl) { \
	if (lockmgr(&(ovl)->lock, LK_EXCLUSIVE, (void *)0, curproc) != 0) { \
		panic("ova_lock: failed to get lock"); \
	} \
	(ova)->timestamp++; \
}
#else
#define	vovl_lock(vovl) {  \
    (lockmgr(&(vovl)->lock, LK_EXCLUSIVE, (void *)0, curproc) != 0); \
    (vovl)->timestamp++; \
}
#endif /* DIAGNOSTIC */

#define	vovl_unlock(vovl) \
		lockmgr(&(vovl)->lock, LK_RELEASE, (void *)0, curproc)
#define	vovl_lock_read(vovl) \
		lockmgr(&(vovl)->lock, LK_SHARED, (void *)0, curproc)
#define	vovl_unlock_read(ovl) \
		lockmgr(&(vovl)->lock, LK_RELEASE, (void *)0, curproc)

#endif /* OVL_MAP_ */
