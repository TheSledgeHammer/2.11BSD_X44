/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly All rights reserved.
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

/* overlay space is designed to compliment vm & kernel
 * It does this in 2 ways:
 * 1) mimic'ing vmspace's mapping of objects.
 * 2) Direct access to physical memory, separate from virtual memory without paging.
 * 3) Split into 2 spaces, VM & Kernel
 */

/* TODO:
 * Size of Overlay Space is a smaller portion of VM Space.
 * Taken from VM_MAX_ADDRESS and VM_MIN_ADDRESS.
 *
 * OVA_MAX_ADDRESS = VM_MAX_ADDRESS - x amount
 * OVA_MIN_ADDRESS = VM_MIN_ADDRESS - x amount
 */


#ifndef _OVL_H_
#define _OVL_H_

union ovl_map_object;
typedef union ovl_map_object 	ovl_map_object_t;

struct ovl_map;
typedef struct ovl_map 			*ovl_map_t;

struct ovl_map_entry;
typedef struct ovl_map_entry	*ovl_map_entry_t;

struct ovl_object;
typedef struct ovl_object 		*ovl_object_t;

struct ovl_segment;
typedef struct ovl_segment		*ovl_segment_t;

struct ovl_page;
typedef struct ovl_page			*ovl_page_t;

#include <sys/tree.h>
#include <sys/queue.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/ovl/ovl_map.h>
#include <devel/vm/ovl/ovl_object.h>
#include <devel/vm/ovl/ovl_overlay.h>
#include <devel/vm/ovl/ovl_segment.h>

/*
 * shareable overlay address space.
 */
struct ovlspace {
	struct ovl_map 	    ovl_map;	    	/* overlay address */
	struct pmap    		ovl_pmap;	    	/* private physical map */
	struct extent 		*ovl_extent;		/* overlay extent allocation */

	int		        	ovl_refcnt;	   		/* number of references */
	segsz_t 			ovl_tsize;			/* text size */
	segsz_t 			ovl_dsize;			/* data size */
	segsz_t 			ovl_ssize;			/* stack size */
	caddr_t	        	ovl_taddr;			/* user overlay address of text */
	caddr_t	        	ovl_daddr;			/* user overlay address of data */
	caddr_t         	ovl_minsaddr;		/* user OVA at min stack growth */
	caddr_t         	ovl_maxsaddr;		/* user OVA at max stack growth */
};

#endif /* _OVL_H_ */
