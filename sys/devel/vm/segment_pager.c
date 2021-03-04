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
 * segment pager or multi-page pager
 * read/write to multiple pages within a vm segment.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/map.h>

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_pager.h>
#include <vm/include/segment_pager.h>

struct pagerlst segment_pager_list;	/* list of managed segments */

struct pagerops segmentpagerops = {
		segment_pager_init,
		segment_pager_alloc,
		segment_pager_dealloc,
		segment_pager_getpage,
		segment_pager_putpage,
		segment_pager_haspage,
		segment_pager_cluster
};

static void
segment_pager_init()
{
	TAILQ_INIT(&segment_pager_list);
}

/*
 * Allocate a pager structure and associated resources.
 * Note that if we are called from the pageout daemon (handle == NULL)
 * we should not wait for memory as it could resulting in deadlock.
 */
static vm_pager_t
segment_pager_alloc(handle, size, prot, foff)
	caddr_t handle;
	register vm_size_t size;
	vm_prot_t prot;
	vm_offset_t foff;
{
	register vm_pager_t pager;
	register seg_pager_t seg;
	vm_object_t 		object;
	vm_segment_t 		sg;

	if (handle == NULL)
		return(NULL);

	sg = (vm_segment_t) handle;
	pager = vm_pager_lookup(&segment_pager_list, handle);
	if (pager == NULL) {
		/*
		 * Allocate and initialize pager structs
		 */
		pager = (vm_pager_t)malloc(sizeof *pager, M_VMPAGER, M_WAITOK);
		if (pager == NULL)
			return (NULL);
		sg = (seg_pager_t)malloc(sizeof *seg, M_VMPGDATA, M_WAITOK);
		if (seg == NULL) {
			free((caddr_t)pager, M_VMPAGER);
			return (NULL);
		}
	}
	return (pager);
}

static void
segment_pager_dealloc(pager)
	vm_pager_t pager;
{

}

static int
segment_pager_getpage(pager, mlist, npages, sync)
	vm_pager_t pager;
	vm_page_t *mlist;
	int 	  npages;
	boolean_t sync;
{
	struct vm_object *object;

}

static int
segment_pager_putpage(pager, mlist, npages, sync)
	vm_pager_t pager;
	vm_page_t *mlist;
	int npages;
	boolean_t sync;
{

}

static boolean_t
segment_pager_haspage(pager, offset)
	vm_pager_t pager;
	vm_offset_t offset;
{

}

static void
segment_pager_cluster(pager, offset, loffset, hoffset)
	vm_pager_t	pager;
	vm_offset_t	offset;
	vm_offset_t	*loffset;
	vm_offset_t	*hoffset;
{

}

segment_pager_cluster_read()
{

}


segment_pager_cluster_write()
{

}
