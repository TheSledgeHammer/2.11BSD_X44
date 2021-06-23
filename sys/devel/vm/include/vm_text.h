/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)text.h	1.2 (2.11BSD GTE) 1/19/95
 */

#ifndef _VM_TEXT_H_
#define _VM_TEXT_H_

#include <sys/queue.h>

/*
 * Text structure
 * One allocated per pure
 * procedure on swap device.
 * Manipulated by text.c
 */
#define	NXDAD				12		/* param.h:MAXTSIZ / vmparam.h:DMTEXT */

struct txtlist;
CIRCLEQ_HEAD(txtlist, vm_text);
struct vm_text {
    /* text information */
	segsz_t 				sp_tsize;				/* text size */
	caddr_t					sp_taddr;				/* text addr */
    int 					sp_tflag;				/* text flags */
    u_long					sp_tresult;				/* text extent */

    CIRCLEQ_ENTRY(vm_text)  x_list;					/* text freelist */
    caddr_t	                x_daddr;				/* segment's disk address */
    caddr_t                 x_caddr;				/* core address, if loaded */
    size_t	                x_size;					/* size (clicks) */
    struct vnode            *x_vptr;    			/* vnode pointer */
    u_char	                x_count;				/* reference count */
    u_char	                x_ccount;				/* number of loaded references */
    u_char	                x_flag;					/* traced, written flags */
    char	                dummy;					/* room for one more */

    /* 4.3BSD-Reno */
    swblk_t					x_psdaddr[NXDAD];		/* disk addresses of dmtext-page segments */
    swblk_t					x_ptdaddr;				/* disk address of page table */
};

#define	XTRC				0x01					/* Text may be written, exclusive use */
#define	XWRIT				0x02					/* Text written into, must swap out */
#define	XLOAD				0x04					/* Currently being read from file */
#define	XLOCK				0x08					/* Being swapped in or out */
#define	XWANT				0x10					/* Wanted for swapping */
#define	XPAGV				0x20					/* Page in on demand from vnode */
#define	XUNUSED				0x40					/* unused since swapped out for cache */
#define	XCACHED				0x80					/* Cached but not sticky */

/* arguments to xswap: */
#define	X_OLDSIZE			(-1)					/* the old size is the same as current */
#define	X_DONTFREE			0	    				/* save core image (for parent in newproc) */
#define	X_FREECORE			1	    				/* free core space after swap */

/*
 * Text table statistics
 */
struct vm_xstats {
    u_long	                xs_alloc;			    /* calls to xalloc */
    u_long	                xs_alloc_inuse;		    /* found in use/sticky */
    u_long	                xs_alloc_cachehit;		/* found in cache */
    u_long	                xs_alloc_cacheflush;	/* flushed cached text */
    u_long	                xs_alloc_unused;		/* flushed unused cached text */
    u_long	                xs_free;			    /* calls to xfree */
    u_long	                xs_free_inuse;		    /* still in use/sticky */
    u_long	                xs_free_cache;		    /* placed in cache */
    u_long	                xs_free_cacheswap;		/* swapped out to place in cache */
    u_long					xs_purge;				/* calls to xpurge */
};

extern
struct txtlist              vm_text_list;

extern
simple_lock_data_t			vm_text_list_lock;

#define xlock(lock)			simple_lock(lock);
#define xunlock(lock)		simple_unlock(lock);

void	vm_text_init(vm_psegment_t, u_long, int);
void	vm_xalloc(struct vnode *);
void	vm_xfree();
void	vm_xexpand(struct proc *, vm_text_t);
void	vm_xccdec(vm_text_t);
void	vm_xuntext(vm_text_t);
void	vm_xuncore(size_t);
int		vm_xpurge();
void	vm_xrele(struct vnode *);

#endif /* _VM_TEXT_H_ */
