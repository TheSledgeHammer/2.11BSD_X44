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
 *	@(#)advvm_vdisk.h 1.0 24/07/21
 */

#ifndef _ADVVM_VDISK_H_
#define _ADVVM_VDISK_H_

#include <sys/queue.h>

/* Virtual Disks */

/*
 * stgDescT - Describes a contiguous set of available (free) vd blocks.
 * These structures are used to maintain a list of free disk space.  There
 * is a free list in each vd structure.  The list is ordered by virtual
 * disk block (it could also be ordered by the size of each contiguous
 * set of blocks in the future).
 */

struct advvm_freelist;
LIST_HEAD(advvm_freelist, advvm_vdfree);
struct advvm_vdfree {
	uint32_t						avd_startcluster;	/* vd cluster number of first free cluster */
	uint32_t						avd_numcluster;		/* number of free clusters */
	LIST_ENTRY(advvm_vdfree)		avd_entry;
};
typedef struct advvm_vdfree			advvm_vdfree_t;

/*
 * vd - this structure describes a virtual disk, including accessed
 * bitfile references, its size, i/o queues, name, id, and an
 * access handle for the metadata.
 */
struct advvm_vdisk {
	struct advvm_domain 			*avd_domain;		/* domain pointer for ds */
	struct vnode					*avd_vp;			/* device access (temp vnode *) */
	u_int 							avd_magic;			/* magic number: structure validation */
	uint32_t						avd_vdindex;		/* 1-based virtual disk index */

	uint32_t						avd_vdsize;			/* count of vdSectorSize blocks in vd */
	int 							avd_vdsector; 		/* Sector size, in bytes, normally 512 */
	uint32_t						avd_vdcluster;		/* num clusters in vd */

	uint32_t						avd_numcluster;		/* num blks each stg bitmap bit */


	struct advvm_freelist			*avd_freelist;		/* ptr to list of free storage descriptors */
};
typedef struct advvm_vdisk			advvm_vdisk_t;

void			advvm_vdisk_init(advvm_vdisk_t *);
void			advvm_vdisk_set(advvm_vdisk_t *, uint32_t, uint32_t, u_int, uint32_t, uint32_t);

void			advvm_vdfree_add(advvm_vdisk_t *, advvm_vdfree_t *, uint32_t, uint32_t);
advvm_vdfree_t 	*advvm_vdfree_lookup(advvm_vdisk_t *, uint32_t, uint32_t, uint32_t);
void			advvm_vdfree_remove(advvm_vdisk_t *, uint32_t, uint32_t, uint32_t);

#endif /* _ADVVM_VDISK_H_ */
