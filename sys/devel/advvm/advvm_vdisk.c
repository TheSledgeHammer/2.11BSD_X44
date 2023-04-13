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
 *	@(#)advvm_vdisk.c 1.0 	27/11/21
 */

/*
 * AdvVM vdisk's are setup within an AdvVM Domain (associated). (akin to zfs's zvol).
 * - Create fileset's for file and directory access
 */
/*
 * TODO:
 * - setup vdindex: related to the vdisk number (e.g. 1 vdisk (index = 0), 2 vdisk (index = 1), etc...)
 * - vdisk parameters: start & end instead of size?
 * - freelist: needs improvement. each vdisk has it own associated freelist.
 * 	  - current freelist: tracks freespace independently of the vdisk. using the
 * 	  starting cluster and the cluster number.
 * 	  - change: freelist space is directly connected to the vdisk.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/null.h>

#include <devel/advvm/advvm_extent.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_domain.h>
#include <devel/advvm/advvm_vdisk.h>

void
advvm_vdisk_init(advdsk)
	advvm_vdisk_t	*advdsk;
{
	advdsk->avd_domain = NULL;
	advdsk->avd_vp = NULL;
	advdsk->avd_magic = 0;
	advdsk->avd_vdindex = 0;
	advdsk->avd_vdsize = 0;
	advdsk->avd_vdsector = 0;
	advdsk->avd_vdcluster = 0;
	advdsk->avd_numcluster = 0;

	LIST_INIT(advdsk->avd_freelist);
}

void
advvm_vdisk_set_domain(advdsk, adom)
	advvm_vdisk_t	*advdsk;
	advvm_domain_t 	*adom;
{
	advdsk->avd_domain = adom;
}

void
advvm_vdisk_set(advdsk, vindex, vsize, vsector, vcluster, vnumcluster)
	advvm_vdisk_t	*advdsk;
	u_int 			vsector;
	uint32_t vindex, vsize, vcluster, vnumcluster;
{
	advvm_vdfree_t 	*advfree;
	advdsk->avd_vdindex = vindex;
	advdsk->avd_vdsize = vsize;
	advdsk->avd_vdsector = vsector;
	advdsk->avd_vdcluster = vcluster;
	advdsk->avd_numcluster = vnumcluster;

	advvm_malloc((advvm_vdfree_t *)advfree, sizeof(advvm_vdfree_t *), M_WAITOK);
	/* TODO: Calculate free space before adding to vdfree */
	advvm_vdfree_add(advdsk, advfree, vsector, vcluster, vnumcluster);
}

void
advvm_vdisk_insert(advdsk, adom)
	advvm_vdisk_t	*advdsk;
	advvm_domain_t 	*adom;
{

	if(advdsk == NULL) {
		advvm_malloc((advvm_vdisk_t *)advdsk, sizeof(advvm_vdisk_t *), M_WAITOK);
	}
	advvm_vdisk_set_domain(advdsk, adom);
}

void
advvm_vdfree_add(advdsk, advfree, cluster, number)
	advvm_vdisk_t 	*advdsk;
	advvm_vdfree_t 	*advfree;
	uint32_t cluster, number;
{
	struct advvm_freelist *freelist;
	freelist = advdsk->avd_freelist;

	advfree->avd_startcluster = cluster;
	advfree->avd_numcluster = number;

	LIST_INSERT_HEAD(freelist, advfree, avd_entry);
}

advvm_vdfree_t *
advvm_vdfree_lookup(advdsk, vindex, cluster, number)
	advvm_vdisk_t *advdsk;
	uint32_t vindex, cluster, number;
{
	struct advvm_freelist *freelist;
	advvm_vdfree_t *advfree;

	freelist = advdsk->avd_freelist;
	LIST_FOREACH(advfree, freelist, avd_entry) {
		if (advdsk->avd_vdindex == vindex) {
			if(advfree->avd_startcluster == cluster && advfree->avd_numcluster == number) {
				return (advfree);
			}
		}
	}
	return (NULL);
}

void
advvm_vdfree_remove(advdsk, vindex, cluster, number)
	advvm_vdisk_t *advdsk;
	uint32_t vindex, cluster, number;
{
	advvm_vdfree_t *advfree;

	advfree = advvm_vdfree_lookup(advdsk, vindex, cluster, number);
	if(advfree != NULL) {
		LIST_REMOVE(advfree, avd_entry);
	}
}
