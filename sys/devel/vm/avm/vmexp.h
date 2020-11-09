/*	$OpenBSD: uvm.h,v 1.67 2019/12/06 08:33:25 mpi Exp $	*/

#ifndef _VMEXP_H_
#define _VMEXP_H_
struct vm {
	/* vm_page queues */
	struct pglist 		page_active; 	/* allocated pages, in use */
	struct pglist 		page_inactive; 	/* pages between the clock hands */
	struct simplelock 	pageqlock; 		/* lock for active/inactive page q */
	struct simplelock 	fpageqlock; 	/* lock for free page q */
	bool				page_init_done;

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


/*
 * vm_map_entry etype bits:
 */
#define VM_ET_OBJ				0x01	/* it is a vm_object */
#define VM_ET_SUBMAP			0x02	/* it is a vm_map submap */
#define VM_ET_COPYONWRITE 		0x04	/* copy_on_write */
#define VM_ET_NEEDSCOPY			0x08	/* needs_copy */

#define VM_ET_ISOBJ(E)			(((E)->etype & VM_ET_OBJ) != 0)
#define VM_ET_ISSUBMAP(E)		(((E)->etype & VM_ET_SUBMAP) != 0)
#define VM_ET_ISCOPYONWRITE(E)	(((E)->etype & VM_ET_COPYONWRITE) != 0)
#define VM_ET_ISNEEDSCOPY(E)	(((E)->etype & VM_ET_NEEDSCOPY) != 0)

#ifdef _KERNEL
/*
 * holds all the internal VM data
 */
extern struct vm vm;

#include <machine/vmparam.h>
#endif /* _KERNEL */

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

	int zeropages;			/* number of zero'd pages */
	int reserve_pagedaemon; /* number of pages reserved for pagedaemon */
	int reserve_kernel;		/* number of pages reserved for kernel */
	int anonpages;			/* number of anonpages */
	int vnodepages;			/* XXX # of pages used by vnode page cache */
	int vtextpages;			/* XXX # of pages used by vtext vnodes */

	/* vm_segment constants */
	int segmentsize;	/* size of a segment (SEGMENT_SIZE): must be power of 2 */
	int segmentmask;	/* segment mask */
	int segmentshift;	/* segment shift */

	/* vm_segment counters */
	int nsegments; 		/* number of segments we manage */
	int seg_free; 		/* number of free segments */
	int seg_active; 	/* number of active segments */
	int seg_inactive; 	/* number of inactive segments */

	/* pageout params */
	int freemin;    	/* min number of free pages */
	int freetarg;   	/* target number of free pages */
	int inactarg;   	/* target number of inactive pages */
	int wiredmax;   	/* max number of wired pages */
	int anonmin;		/* min threshold for anon pages */
	int vtextmin;		/* min threshold for vtext pages */
	int vnodemin;		/* min threshold for vnode pages */
	int anonminpct;		/* min percent anon pages */
	int vtextminpct;	/* min percent vtext pages */
	int vnodeminpct;	/* min percent vnode pages */

	/* swap */
	int nswapdev; 		/* number of configured swap devices in system */
	int swpages; 		/* number of PAGE_SIZE'ed swap pages */
	int swpginuse; 		/* number of swap pages in use */
	int swpgonly; 		/* number of swap pages in use, not also in RAM */
	int nswget; 		/* number of times fault calls uvm_swap_get() */
	int nanon; 			/* number total of anon's in system */
	int nanonneeded;	/* number of anons currently needed */
	int nfreeanon; 		/* number of free anon's */

	/* stat counters */

	/* fault subcounters */

	/* daemon counters */
};
extern struct vmexp vmexp;

#endif /* _VMEXP_H_ */
