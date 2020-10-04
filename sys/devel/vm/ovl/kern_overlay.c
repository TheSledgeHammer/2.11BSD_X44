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
 * @(#)kern_overlay.c	1.00
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include "kern/memory/tbtree.h"
#include "vm/ovl/ovl.h"
#include "vm/ovl/koverlay.h"

#define MINBUCKET		4				/* 4 => min allocation of 16 bytes */
#define MAXALLOCSAVE	(2 * )

/* Kernel Overlay Memory Management */
struct overlay 				bucket[MINBUCKET + 16];
struct ovlstats 			ovlstats[M_LAST];
struct ovlusage 			*ovlusage;
char 						*kovlbase, *kovllimit;


/* TODO: fix asl and freep next.
 * freep->next reference freelist in buckets.
 */
void *
koverlay_insert(size, type, flags)
	unsigned long size;
	int type, flags;
{
	register struct overlay 	*ovp;
    register struct ovlusage 	*oup;
    register struct asl         *freep;
	long indx, npg, allocsize;
	caddr_t  va, cp, savedlist;

	indx = BUCKETINDX(size);
	ovp = &bucket[indx];
	ovp->ot_tbtree = tbtree_allocate(&bucket[indx]);

	if (oup->ou_kovlcnt < NKOVL) {
		if (ovp->ot_next == NULL) {
			ovp->ot_last = NULL;

			if (size > MAXALLOCSAVE) {
				allocsize = roundup(size, CLBYTES);
			} else {
				allocsize = 1 << indx;
			}
			npg = clrnd(btoc(allocsize));

			va = (caddr_t) tbtree_malloc(ovp->ot_tbtree, (vm_size_t) ctob(npg), OVL_OBJ_KERNEL, !(flags & M_NOWAIT));

			if(va != NULL) {
				oup->ou_kovlcnt++;
			}

			if (va == NULL) {
				return ((void*) NULL);
			}

			oup = btooup(va, kovlbase);
			oup->ou_indx = indx;
			if (allocsize > MAXALLOCSAVE) {
				if (npg > 65535)
					panic("malloc: allocation too large");
				oup->ou_bucketcnt = npg;
				goto out;
			}
			savedlist = ovp->ot_next;
			ovp->ot_next = cp = (caddr_t) va + (npg * NBPG) - allocsize;
			for(;;) {
				freep = (struct asl*) cp;

				cp -= allocsize;
				freep->asl_next->asl_addr = cp;
			}

			asl_set_addr(freep->asl_next, savedlist);
			if(ovp->ot_last == NULL) {
				ovp->ot_last = (caddr_t)freep;
			}
		}
		va = ovp->ot_next;
		ovp->ot_next = asl_get_addr(((struct asl *)va)->asl_next);
	}

out:
	//splx(s);
	return ((void *) va);
}

void
koverlay_free(addr, type)
	void *addr;
	int type;
{
	register struct overlay *ovp;
	register struct ovlusage *oup;
	register struct asl *freep;
	long size;

	oup = btokup(addr);
	size = 1 << oup->ou_indx;
	ovp = &bucket[oup->ou_indx];

	if (size > MAXALLOCSAVE) {
		ovl_free(ovl_map, (vm_offset_t)addr, oup->ou_bucketcnt);
		tbtree_free(ovp->ot_tbtree, size);
		return;
	}
	freep = (struct asl *)addr;
	if (ovp->ot_next == NULL)
		ovp->ot_next = addr;
	else
		((struct asl *)ovp->ot_last)->asl_next->asl_addr = addr;
	freep->asl_next->asl_addr = NULL;
	ovp->ot_last = addr;
}

void
koverlay_init()
{
	register long indx;
	int npg = (OVL_MEM_SIZE / NBOVL);

	/* kernel overlay allocation */
	ovlusage = (struct ovlusage *) ovl_alloc(ovl_map, (vm_size_t)(npg * sizeof(struct ovlusage)), OVL_OBJ_KERNEL);
	kovl_mmap = ovl_suballoc(ovl_map, (vm_offset_t *)&kovlbase, (vm_offset_t *)&kovllimit, (vm_size_t *) npg);
}
