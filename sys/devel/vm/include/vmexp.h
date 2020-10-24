/*	$OpenBSD: uvm.h,v 1.67 2019/12/06 08:33:25 mpi Exp $	*/

#ifndef _VMEXP_H_
#define _VMEXP_H_

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
