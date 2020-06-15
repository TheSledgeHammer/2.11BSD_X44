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
 *
 * @(#)ovl.h	1.00
 */
/* overlay address space (overlay memory) */

/* TODO:
 * Size of Overlay Space is a smaller portion of VM Space.
 * Taken from VM_MAX_ADDRESS and VM_MIN_ADDRESS.
 *
 * OVA_MAX_ADDRESS = VM_MAX_ADDRESS - x amount
 * OVA_MIN_ADDRESS = VM_MIN_ADDRESS - x amount
 */
/*	koverlay:
 * 	-
 * 	- Should be changed ovlspace allocation
 * 		- as to access and allocate a portion of physical memory
 * 		- ovlspace can be split
 */
/* Current Kernel Overlays:
 * - Akin to a more advanced & versatile overlay from original 2.11BSD
 * - Allocates memory from kernelspace (thus vm allocation)
 * - While useful (in some situations), for extending memory management
 * - This version would not provide any benefit to vm paging, if the overlay is a page in itself.
 * - would likely be slower than using just paging.
 */

#ifndef _OVL_H_
#define _OVL_H_

/* TODO: Improve algorithm for number of overlays */
#define	NOVL	(32)			/* number of kernel overlays */
#define NOVLSR 	(NOVL/2)		/* maximum mumber of kernel overlay sub-regions */
#define NOVLPSR	NOVLSR			/* XXX: number of overlays per sub-region */

#define OVL_MIN_KERNEL_ADDRESS
#define OVL_MAX_KERNEL_ADDRESS
#define OVL_MIN_VIRTUAL_ADDRESS
#define OVL_MAX_VIRTUAL_ADDRESS

#include <sys/extent.h>
#include <sys/queue.h>

#include "../vm/ovl/ovl_map.h"
#include "../vm/ovl/ovl_object.h"

union ovl_map_object;
typedef union ovl_map_object 	ovl_map_object_t;

struct ovl_map;
typedef struct ovl_map 			*ovl_map_t;

struct ovl_map_entry;
typedef struct ovl_map_entry	*ovl_map_entry_t;

struct ovl_object;
typedef struct ovl_object 		*ovl_object_t; 		/* akin to a vm_object but in ovlspace */

/* overlay space extent allocator */
struct ovlspace_extent {
	struct extent 		*ovls_extent;		/* overlay extent allocation ptr */
	char 				*ovls_name;			/* overlay name */
	u_long 				ovls_start;			/* start of region */
	u_long 				ovls_end;			/* end of region */
	caddr_t				ovls_addr;			/* kernel overlay region Address */
	size_t				ovls_size;			/* size of overlay region */
	int					ovls_flags;			/* overlay flags */
	int					ovls_regcnt;		/* number of regions allocated */
	int					ovls_sregcnt;		/* number of sub-regions allocated */
};

/*
 * shareable overlay address space.
 */
struct ovlspace {
	struct ovl_map 	    ovls_map;	    	/* Overlay address */
	struct pmap    		ovls_pmap;	    	/* private physical map */

	int		        	ovls_refcnt;	   	/* number of references */
	segsz_t 			ovls_tsize;			/* text size */
	segsz_t 			ovls_dsize;			/* data size */
	segsz_t 			ovls_ssize;			/* stack size */
	caddr_t	        	ovls_taddr;			/* user overlay address of text */
	caddr_t	        	ovls_daddr;			/* user overlay address of data */
	caddr_t         	ovls_minsaddr;		/* user OVA at min stack growth */
	caddr_t         	ovls_maxsaddr;		/* user OVA at max stack growth */
};

/* Overlay Flags */
#define OVL_ALLOCATED  (1 < 0) 							/* kernel overlay region allocated */

extern struct koverlay 	*koverlay_extent_create(kovl, name, start, end, addr, size);
extern void				koverlay_destroy(kovl);
extern int				koverlay_extent_alloc_region(kovl, size);
extern int				koverlay_extent_alloc_subregion(kovl, name, start, end, size, alignment, boundary, result);
extern void				koverlay_free(kovl, start, size);

#endif /* _OVL_H_ */
