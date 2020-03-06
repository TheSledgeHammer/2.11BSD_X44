
#ifndef _UVM_UVM_ANON_H_
#define _UVM_UVM_ANON_H_
/*
 * uvm_anon.h
 */
/*
 * anonymous memory management
 *
 * anonymous virtual memory is short term virtual memory that goes away
 * when the processes referencing it go away.    an anonymous page of
 * virtual memory is described by the following data structure:
 */

struct vm_anon {
	struct simplelock   an_lock;	/* lock for an_ref */
	union {
		int             au_ref;		/* reference count [an_lock] */
		struct vm_anon	*au_link;	/* Link for deferred free */
	} an_u;
#define	an_ref	an_u.au_ref
#define	an_link	an_u.au_link
    struct vm_page      *an_page;   /* if in RAM [an_lock] */
#if defined(VMSWAP) || 1 /* XXX libkvm */
	/*
	 * Drum swap slot # (if != 0) [an_lock.  also, it is ok to read
	 * an_swslot if we hold an_page PG_BUSY].
	 */
    int					an_swslot;
#endif
};

/*
 * for active vm_anon's the data can be in one of the following state:
 * [1] in a vm_page with no backing store allocated yet, [2] in a vm_page
 * with backing store allocated, or [3] paged out to backing store
 * (no vm_page).
 *
 * for pageout in case [2]: if the page has been modified then we must
 * flush it out to backing store, otherwise we can just dump the
 * vm_page.
 */

/*
 * anons are grouped together in anonymous memory maps, or amaps.
 * amaps are defined in uvm_amap.h.
 */

/*
 * processes reference anonymous virtual memory maps with an anonymous
 * reference structure:
 */

struct vm_aref {
	int             ar_pageoff;		/* page offset into amap we start */
	struct vm_amap  *ar_amap;	    /* pointer to amap */
};

/*
 * the offset field indicates which part of the amap we are referencing.
 * locked by vm_map lock.
 */

#ifdef _KERNEL

/*
 * prototypes
 */

struct  	vm_anon *uvm_analloc(void);
void    	uvm_anfree(struct vm_anon *);
void    	uvm_anon_init(void);
struct  	vm_page *uvm_anon_lockloanpg(struct vm_anon *);
#if defined(VMSWAP)
void    	uvm_anon_dropswap(struct vm_anon *);
#else /* defined(VMSWAP) */
#define		uvm_anon_dropswap(a)	/* nothing */
#endif /* defined(VMSWAP) */
void        uvm_anon_release(struct vm_anon *);
boolean_t   uvm_anon_pagein(struct vm_anon *);
#endif /* _KERNEL */

#endif /* _UVM_UVM_ANON_H_ */
