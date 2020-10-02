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

#include "vm/ovl/ovl.h"
#include "vm/ovl/koverlay.h"

#define MINBUCKET		4				/* 4 => min allocation of 16 bytes */
#define MAXALLOCSAVE	(2 * )

/* Kernel Overlay Memory Management */
struct ovlbuckets 		bucket[MINBUCKET + 16];
struct ovlstats 		ovlstats[M_LAST];
struct ovlusage 		*ovlusage, *vovlusage;

char 					*ovlbase, *ovllimit;
char 					*kovlbase, *kovllimit;
char 					*vovlbase, *vovllimit;

koverlay_insert(size)
{
	register struct ovlbuckets 	*obp;
	register struct ovlusage 	*oup;
	register struct asl 		*freep;

	long indx, npg, allocsize;

	indx = BUCKETINDX(size);
	obp = &bucket[indx];


	if(oup->ou_kovlcnt < NKOVL) {
		if (obp->ob_next == NULL) {
			obp->ob_last = NULL;

		}
	} else {
		goto out;
	}
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

void
voverlay_init()
{
	register long indx;
	int npg = (OVL_MEM_SIZE / NBOVL);

	/* vm overlay allocation */
	vovlusage = (struct ovlusage *) ovl_alloc(ovl_map, (vm_size_t)(npg * sizeof(struct ovlusage)), OVL_OBJ_VM);
	vovl_mmap = ovl_suballoc(ovl_map, (vm_offset_t *)&vovlbase, (vm_offset_t *)&vovllimit, (vm_size_t *) npg);
}
