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

#include <arch/i386/include/param.h>
#include <arch/i386/include/vmparam.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_mac.h>

/*
 * Data structure
 */
struct vm_data {
    segsz_t 				psx_dsize;				/* data size */
	caddr_t					psx_daddr;				/* data addr */
	int 					psx_dflag;				/* data flags */
	u_long					*psx_dresult;			/* data extent */
};

/*
 * Stack structure
 */
struct vm_stack {
	segsz_t 				psx_ssize;				/* stack size */
	caddr_t					psx_saddr;				/* stack addr */
    int 					psx_sflag;				/* stack flags */
    u_long					*psx_sresult;			/* stack extent */
};

/*
 * Text structure
 * One allocated per pure
 * procedure on swap device.
 * Manipulated by text.c
 */
#define	NXDAD				12						/* param.h:MAXTSIZ / vmparam.h:DMTEXT */

struct txtlist;
TAILQ_HEAD(txtlist, vm_text);
struct vm_text {
    /* vm_text extents */
	segsz_t 				psx_tsize;				/* text size */
	caddr_t					psx_taddr;				/* text addr */
    int 					psx_tflag;				/* text flags */
    u_long					psx_tresult;			/* text extent */

    /* vm_text generic */
    TAILQ_ENTRY(vm_text)  	psx_list;				/* text freelist */
    swblk_t					psx_ptdaddr;			/* disk address of page table */
    caddr_t	                psx_daddr;				/* segment's disk address */
    caddr_t                 psx_caddr;				/* core address, if loaded */
    size_t	                psx_size;				/* size (clicks) */
    struct vnode            *psx_vptr;    			/* vnode pointer */
    u_char	                psx_count;				/* reference count */
    u_char	                psx_ccount;				/* number of loaded references */
    u_char	                psx_flag;				/* traced, written flags */
    simple_lock_data_t		psx_lock;				/* text lock */
    char	                dummy;					/* room for one more */
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
    u_long	                psxs_alloc;			    /* calls to xalloc */
    u_long	                psxs_alloc_inuse;		/* found in use/sticky */
    u_long	                psxs_alloc_cachehit;	/* found in cache */
    u_long	                psxs_alloc_cacheflush;	/* flushed cached text */
    u_long	                psxs_alloc_unused;		/* flushed unused cached text */
    u_long	                psxs_free;			    /* calls to xfree */
    u_long	                psxs_free_inuse;		/* still in use/sticky */
    u_long	                psxs_free_cache;		/* placed in cache */
    u_long	                psxs_free_cacheswap;	/* swapped out to place in cache */
    u_long					psxs_purge;				/* calls to xpurge */
};

/* pseudo segment registers */
union vm_pseudo_segment {
    struct vm_data      	ps_data;
    struct vm_stack     	ps_stack;
    struct vm_text      	ps_text;

    struct extent 			*ps_extent;				/* segments extent allocator */
    vm_offset_t         	*ps_start;				/* start of space */
    vm_offset_t         	*ps_end;				/* end of space */
    size_t					ps_size;				/* total size (data + stack + text) */
    int 					ps_flags;				/* flags */
};

extern
struct txtlist              vm_text_list;

/*
extern
simple_lock_data_t			vm_text_lock;

#define xlock(lock)			simple_lock(lock);
#define xunlock(lock)		simple_unlock(lock);
*/

/* pseudo-segment types */
#define PSEG_DATA			1						/* data segment */
#define PSEG_STACK			2						/* stack segment */
#define PSEG_TEXT			3						/* text segment */

/* pseudo-segment flags */
#define PSEG_SEP			(PSEG_DATA | PSEG_STACK)				/* I&D seperation */
#define PSEG_NOSEP			(PSEG_DATA | PSEG_STACK | PSEG_TEXT)	/* No I&D seperation */

/* pseudo-segment macros */
#define DATA_SEGMENT(data, dsize, daddr, dflag) {		\
	(data)->psx_dsize = (dsize);						\
	(data)->psx_daddr = (daddr);						\
	(data)->psx_dflag = (dflag);						\
};

#define DATA_EXPAND(data, dsize, daddr) {				\
	(data)->psx_dsize += (dsize);						\
	(data)->psx_daddr += (daddr);						\
};

#define DATA_SHRINK(data, dsize, daddr) {				\
	(data)->psx_dsize -= (dsize);						\
	(data)->psx_daddr -= (daddr);						\
};

#define STACK_SEGMENT(stack, ssize, saddr, sflag) {		\
	(stack)->psx_ssize = (ssize);						\
	(stack)->psx_saddr = (saddr);						\
	(stack)->psx_sflag = (sflag);						\
};

#define STACK_EXPAND(stack, ssize, saddr) {				\
	(stack)->psx_ssize += (ssize);						\
	(stack)->psx_saddr += (saddr);						\
};

#define STACK_SHRINK(stack, ssize, saddr) {				\
	(stack)->psx_ssize -= (ssize);						\
	(stack)->psx_saddr -= (saddr);						\
};

#define TEXT_SEGMENT(text, tsize, taddr, tflag) {		\
	(text)->psx_tsize = (tsize);						\
	(text)->psx_taddr = (taddr);						\
	(text)->psx_tflag = (tflag);						\
};

#define TEXT_EXPAND(text, tsize, taddr) {				\
	(text)->psx_tsize += (tsize);						\
	(text)->psx_taddr += (taddr);						\
};

#define TEXT_SHRINK(text, tsize, taddr) {				\
	(text)->psx_tsize -= (tsize);						\
	(text)->psx_taddr -= (taddr);						\
};

/* vm text sysctl */
extern int sysctl_text(char *, size_t *);

/* vm_psegment */
void	vm_psegment_init(vm_segment_t, vm_offset_t *, vm_offset_t *);
void	vm_psegment_expand(vm_psegment_t *, int, segsz_t, caddr_t);
void	vm_psegment_shrink(vm_psegment_t *, int, segsz_t, caddr_t);
void	vm_psegment_extent_create(vm_psegment_t *, char *, u_long, u_long, int, caddr_t, size_t, int);
void	vm_psegment_extent_alloc(vm_psegment_t *, u_long, u_long, int, int);
void	vm_psegment_extent_suballoc(vm_psegment_t *, u_long, u_long, int, u_long *);
void	vm_psegment_extent_free(vm_psegment_t *, caddr_t, u_long, int, int);
void	vm_psegment_extent_destroy(vm_psegment_t *);

/* vm_text */
void	xlock(vm_text_t);
void	xunlock(vm_text_t);
void	vm_text_init(vm_text_t);
void	vm_xalloc(struct vnode *, u_long, off_t);
void	vm_xfree(void);
void	vm_xexpand(struct proc *, vm_text_t);
void	vm_xccdec(vm_text_t);
void	vm_xumount(dev_t);
void	vm_xuntext(vm_text_t);
void	vm_xuncore(size_t);
int		vm_xpurge(void);
void	vm_xrele(struct vnode *);
void	vm_xswapout(struct proc *, int, u_int, u_int);
#endif /* _VM_TEXT_H_ */
