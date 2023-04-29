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

#include <sys/queue.h>
#include <sys/tree.h>
#include <vm/include/vm_param.h>
#include <sys/lock.h>
#include <vm/include/vm_prot.h>
#include <vm/include/vm_inherit.h>
#include <ovl/include/ovl_map.h>
#include <ovl/include/ovl_object.h>
#include <ovl/include/pmap.h>
#include <ovl/include/ovl_extern.h>

/*
 * Shareable overlay address space.
 */
struct ovlspace {
	struct ovl_map 	    ovl_map;	    	/* overlay address */
	struct pmap    		ovl_pmap;	    	/* private physical map */

	int		        	ovl_refcnt;	   		/* number of references */
	segsz_t				ovl_startcopy;
};

#endif /* _OVL_H_ */
