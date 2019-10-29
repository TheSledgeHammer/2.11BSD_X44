/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm.h	7.1 (Berkeley) 6/4/86
 */

/*
 *	#include "../h/vm.h"
 * or	#include <vm.h>		 in a user program
 * is a quick way to include all the vm header files.
 */
#ifndef VM_H
#define VM_H

#include <vm/vmparam.h>
#include <vm/vmmac.h>
#include <vm/vmmeter.h>
#include <vm/vmsystm.h>

/*
 * Shareable process virtual address space.
 * May eventually be merged with vm_map.
 * Several fields are temporary (text, data stuff).
 */
struct vmspace {
	struct	vm_map vm_map;	/* VM address map */
	struct	pmap vm_pmap;	/* private physical map */
	int		vm_refcnt;		/* number of references */
	caddr_t	vm_shm;			/* SYS5 shared memory private data XXX */
/* we copy from vm_startcopy to the end of the structure on fork */
#define vm_startcopy vm_rssize
	segsz_t vm_rssize; 		/* current resident set size in pages */
	segsz_t vm_swrss;		/* resident set size before last swap */
	segsz_t vm_tsize;		/* text size (pages) XXX */
	segsz_t vm_dsize;		/* data size (pages) XXX */
	segsz_t vm_ssize;		/* stack size (pages) */
	caddr_t	vm_taddr;		/* user virtual address of text XXX */
	caddr_t	vm_daddr;		/* user virtual address of data XXX */
	caddr_t vm_maxsaddr;	/* user VA at max stack growth */
	caddr_t vm_minsaddr;	/* user VA at max stack growth */
};

#endif /* VM_H */
