/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vmparam.h	7.1.1 (2.11BSD GTE) 1/14/95
 */

/*
 * Machine dependent constants
 */
#ifndef	_VMPARAM_
#define	_VMPARAM_
#include <machine/vmparam.h>

/*
 * CTL_VM identifiers
 */
#define	VM_METER	1		/* struct vmmeter */
#define	VM_LOADAVG	2		/* struct loadavg */
#define	VM_SWAPMAP	3		/* struct mapent _swapmap[] */
#define	VM_COREMAP	4		/* struct mapent _coremap[] */
#define	VM_MAXID	5		/* number of valid vm ids */

#ifndef	KERNEL
#define CTL_VM_NAMES { \
	{ 0, 0 }, \
	{ "vmmeter", CTLTYPE_STRUCT }, \
	{ "loadavg", CTLTYPE_STRUCT }, \
	{ "swapmap", CTLTYPE_STRUCT }, \
	{ "coremap", CTLTYPE_STRUCT }, \
}

#ifndef ASSEMBLER
/*
 *	Convert addresses to pages and vice versa.
 *	No rounding is used.
 */
#ifdef KERNEL
#define	atop(x)		(((unsigned long)(x)) >> PAGE_SHIFT)
#define	ptoa(x)		((vm_offset_t)((x) << PAGE_SHIFT))

/*
 * Round off or truncate to the nearest page.  These will work
 * for either addresses or counts (i.e., 1 byte rounds to 1 page).
 */
#define	round_page(x) \
	((vm_offset_t)((((vm_offset_t)(x)) + PAGE_MASK) & ~PAGE_MASK))
#define	trunc_page(x) \
	((vm_offset_t)(((vm_offset_t)(x)) & ~PAGE_MASK))
#define	num_pages(x) \
	((vm_offset_t)((((vm_offset_t)(x)) + PAGE_MASK) >> PAGE_SHIFT))

extern vm_size_t	mem_size;	/* size of physical memory (bytes) */
extern vm_offset_t	first_addr;	/* first physical page */
extern vm_offset_t	last_addr;	/* last physical page */

#else
/* out-of-kernel versions of round_page and trunc_page */
#define	round_page(x) \
	((((vm_offset_t)(x) + (vm_page_size - 1)) / vm_page_size) * \
	    vm_page_size)
#define	trunc_page(x) \
	((((vm_offset_t)(x)) / vm_page_size) * vm_page_size)

#endif /* KERNEL */
#endif /* ASSEMBLER */
#endif
