/*	$NetBSD: uvm_amap.c,v 1.53.2.2 2006/01/27 22:21:01 tron Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * uvm_amap.c: amap operations
 */

/*
 * this file contains functions that perform operations on amaps.  see
 * uvm_amap.h for a brief explanation of the role of amaps in uvm.
 */

#include <sys/cdefs.h>
/*__KERNEL_RCSID(0, "$NetBSD: uvm_amap.c,v 1.53.2.2 2006/01/27 22:21:01 tron Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/extent.h>

#define UVM_AMAP_C		/* ensure disabled inlines are in */
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_swap.h>

/*
 * local functions
 */

static struct avm_map *amap_alloc1(int, int, int);

#ifdef UVM_AMAP_PPREF

/*
 * what is ppref?   ppref is an _optional_ amap feature which is used
 * to keep track of reference counts on a per-page basis.  it is enabled
 * when UVM_AMAP_PPREF is defined.
 *
 * when enabled, an array of ints is allocated for the pprefs.  this
 * array is allocated only when a partial reference is added to the
 * map (either by unmapping part of the amap, or gaining a reference
 * to only a part of an amap).  if the malloc of the array fails
 * (M_NOWAIT), then we set the array pointer to PPREF_NONE to indicate
 * that we tried to do ppref's but couldn't alloc the array so just
 * give up (after all, this is an optional feature!).
 *
 * the array is divided into page sized "chunks."   for chunks of length 1,
 * the chunk reference count plus one is stored in that chunk's slot.
 * for chunks of length > 1 the first slot contains (the reference count
 * plus one) * -1.    [the negative value indicates that the length is
 * greater than one.]   the second slot of the chunk contains the length
 * of the chunk.   here is an example:
 *
 * actual REFS:  2  2  2  2  3  1  1  0  0  0  4  4  0  1  1  1
 *       ppref: -3  4  x  x  4 -2  2 -1  3  x -5  2  1 -2  3  x
 *              <----------><-><----><-------><----><-><------->
 * (x = don't care)
 *
 * this allows us to allow one int to contain the ref count for the whole
 * chunk.    note that the "plus one" part is needed because a reference
 * count of zero is neither positive or negative (need a way to tell
 * if we've got one zero or a bunch of them).
 *
 * here are some in-line functions to help us.
 */

static __inline void pp_getreflen(int *, int, int *, int *);
static __inline void pp_setreflen(int *, int, int, int);

struct avmspace *
avmspace_alloc(min, max)
	vm_offset_t min, max;
{
	register struct avmspace *avm;
	MALLOC(avm, struct avmspace *, sizeof(struct avmspace), M_AVMMAP, M_WAITOK);
	VM_EXTENT_CREATE(avm->avm_extent, "avm_map", min, max, M_AVMMAP, NULL, 0, EX_WAITOK | EX_MALLOCOK);
	if(avm->avm_extent) {
		if(vm_extent_alloc_region(avm->avm_extent, min, max, EX_WAITOK | EX_MALLOCOK)) {
			amap_init(&avm->avm_amap, min, max);
			avm->avm_refcnt = 1;
		}
	}
	return (avm);
}

void
avmspace_free(avm)
	register struct avmspace *avm;
{
	if (--avm->avm_refcnt == 0) {
		amap_lock(&avm->avm_amap);
		VM_EXTENT_FREE(avm->avm_extent, avm, sizeof(struct avmspace *), EX_WAITOK);
		FREE(avm, M_AVMMAP);
	}
}

#ifdef UVM_AMAP_PPREF
/*
 * pp_getreflen: get the reference and length for a specific offset
 *
 * => ppref's amap must be locked
 */
static __inline void
pp_getreflen(ppref, offset, refp, lenp)
	int *ppref, offset, *refp, *lenp;
{
	if (ppref[offset] > 0) {		/* chunk size must be 1 */
		*refp = ppref[offset] - 1;	/* don't forget to adjust */
		*lenp = 1;
	} else {
		*refp = (ppref[offset] * -1) - 1;
		*lenp = ppref[offset+1];
	}
}

/*
 * pp_setreflen: set the reference and length for a specific offset
 *
 * => ppref's amap must be locked
 */
static __inline void
pp_setreflen(ppref, offset, ref, len)
	int *ppref, offset, ref, len;
{
	if (len == 0)
		return;
	if (len == 1) {
		ppref[offset] = ref + 1;
	} else {
		ppref[offset] = (ref + 1) * -1;
		ppref[offset+1] = len;
	}
}
#endif

void
amap_init(amap, min, max)
	register struct avm_map *amap;
	vm_offset_t	min, max;
{
	RB_INIT(&amap->am_rb_root);
	amap->am_header->next = amap->am_header->prev = amap->am_header;
	amap->am_ref = 1;
	lockinit(&amap->am_lock, PVM, "thrd_sleep", 0, 0);
	simple_lock_init(&amap->am_ref_lock);
}
