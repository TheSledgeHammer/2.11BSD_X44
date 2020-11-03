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
#include <sys/malloc.h>
#include <sys/user.h>
#include <devel/vm/lvm/vm_lvm.h>


/*
 * Use Case Ideas:
 * PV's & LV's: (linked to object via page & segment)
 * search next, prev: for a page/segment
 * get: a page/segment
 *
 * VG: (linked to pager & ovl):
 * create: lv's & pv's
 * remove: lv's & pv's
 * allocate: lv's & pv's
 * free: lv's & pv's
 */
/*
 * volume group creation & management
 * link volume group to logical volume
 * mapping logical volumes & physical volumes
 */

struct vm_vg volumegroup;

/* loads in vm page startup */
void
vm_physical_volume_startup(pv, start, end, list, page)
    struct vm_physical_volume *pv;
    vm_offset_t start, end;
    struct pglist *list;
    vm_page_t page;
{
	pv->pv_start = start;
	pv->pv_end = end;

	pv->pv_npages = (end - start + sizeof(struct vm_page))
			/ (PAGE_SIZE + sizeof(struct vm_page));

	pv->pv_first_page = start;
	pv->pv_first_page += pv->pv_npages * sizeof(struct vm_page);
	pv->pv_first_page = atop(round_page(first_page));
	pv->pv_last_page = pv->pv_first_page + pv->pv_npages - 1;

	pv->pv_first_paddr = ptoa(first_page);
	pv->pv_last_paddr = ptoa(last_page) + PAGE_MASK;
	pv->pv_pglist = list;
	pv->pv_page_array = page;

    //freelist: tracking physical volume freespace
}

/* loads in vm segment startup */
void
vm_logical_volume_startup(lv, start, end, list, segment)
    struct vm_logical_volume *lv;
    vm_offset_t start, end;
    struct seglist *list;
    vm_segment_t segment;
{
	lv->lv_start = start;
	lv->lv_end = end;
	lv->lv_nsegments = (end - start + sizeof(struct vm_segment)) / (SEGMENT_SIZE + sizeof(struct vm_segment));

	lv->lv_first_segment = start;
	lv->lv_first_segment += lv->lv_nsegments * sizeof(struct vm_segment);
	lv->lv_first_segment = atos(round_segment(first_segment));
	lv->lv_last_segment = lv->lv_first_segment + lv->lv_nsegments - 1;

	lv->lv_first_laddr = stoa(first_segment);
	lv->lv_last_laddr = stoa(last_segment) + SEGMENT_MASK;
	lv->lv_seglist = list;
	lv->lv_segment_array = segment;

	//freelist: tracking physical volume freespace
}

/* start in vm_mem_init? */
void
vm_volume_group_init()
{
	struct vm_volume_group *vg;
	volume_group_alloc(vg);
	TAILQ_INIT(&volumegroup);
	TAILQ_INIT(&vg->vg_pvs);
	TAILQ_INIT(&vg->vg_lvs);
}

void
vm_volume_group_alloc(vg)
	struct vm_volume_group *vg;
{
	MALLOC(vg, struct vm_volume_group, sizeof(struct vm_volume_group), M_VMVOLGRP, M_WAITOK);
}

void
vm_volume_group_free(vg)
{
	FREE(vg, M_VMVOLGRP);
}

