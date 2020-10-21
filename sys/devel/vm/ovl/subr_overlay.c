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

/* overlay subroutines & management */

#include <sys/map.h>
#include <sys/queue.h>
#include <devel/vm/ovl/overlay.h>

struct overlay_ops ovlops = {
		.koverlay_allocate = 	koverlay_allocate,
		.koverlay_free = 		koverlay_free,
		.voverlay_allocate = 	voverlay_allocate,
		.voverlay_free = 		voverlay_free
};

struct overlay_head *ovlist;

#ifndef MAXLKMS
#define	MAXLKMS		20
#endif

void
ovlops_init(ovlops)
	struct overlay_ops *ovlops;
{
	RMALLOC(ovlops, struct overlay_ops *, sizeof(struct overlay_ops *));
}

void
overlay_init()
{
	koverlay_init();
	voverlay_init();
	ovlops_init(&ovlops);


	CIRCLEQ_INIT(ovlist);
}

static struct overlay_table	otoverlays[MAXLKMS];	/* table of loaded overlays */
static struct overlay_table	*curp;					/* global for in-progress ops */

overlay_startup(p)
	struct proc *p;
{
	struct overlay_load		*ovl_load;		/* load into overlay list */
	struct overlay_unload	*ovl_unload;	/* unload from overlay list */
	int i;

	for (i = 0; i < MAXLKMS; i++) {
		if (!otoverlays[i].o_used) {
			break;
		}
	}
	if (i == MAXLKMS) {
		error = ENOMEM;		/* no slots available */
		break;
	}
	curp = &otoverlays[i];
	curp->o_id = i;			/* self reference slot offset */

	curp->o_area = rmalloc(ovl_map, curp->o_size);

}

void
overlay_add(ovltp)
	struct overlay_table *ovltp;
{
	struct overlay_head *ovlist;

	struct overlay_entries *ove = ovltp->o_private.ovl_list;

	ovlist = &ovlist;
/* TODO: default: set flags to inactive */
	switch (ovltp->o_objtype) {
	case OVL_KERN:
		if(ove->o_nkovl <= NKOVLE) {
			CIRCLEQ_INSERT_HEAD(ovlist, ove, o_entries);
			ove->o_nkovl++;
		};
		break;
	case OVL_VM:
		if(ove->o_nvovl <= NVOVLE) {
			CIRCLEQ_INSERT_TAIL(ovlist, ove, o_entries);
			ove->o_nvovl++;
		}
		break;
	}
}

void
overlay_remove(ovltp)
	struct overlay_table *ovltp;
{
	struct overlay_head *ovlist;

	struct overlay_entries *ove = ovltp->o_private.ovl_list;

	ovlist = &ovlist;
/* TODO: check flags: if active, set inactive */
	switch (ovltp->o_objtype) {
	case OVL_KERN:
		CIRCLEQ_REMOVE(ovlist, ove, o_entries);
		ove->o_nkovl--;
		break;
	case OVL_VM:
		CIRCLEQ_REMOVE(ovlist, ove, o_entries);
		ove->o_nvovl--;
		break;
	}
}

overlay_find()
{

}

overlay_exec(ovltp)
	struct overlay_table *ovltp;
{
	struct overlay_exec *args = ovltp->o_private.ovl_exec;

	/* set flags to exec */
}

overlay_load(ovltp)
	struct overlay_table *ovltp;
{
	struct overlay_load *ovl = ovltp->o_private.ovl_load;

	/* pseudo-code:
	 * if(... == NULL)
	 * 	set flags to loaded
	 * 	overlay_allocate(ovltp, size, type, flags);
	 *
	 * 	overlay_add(ovltp, ..);
	 */

}

overlay_unload(ovltp)
	struct overlay_table *ovltp;
{
	struct overlay_unload *ovul = ovltp->o_private.ovl_unload;

	/* pseudo-code:
	 * if(... != NULL)
	 * set flags to unloaded if loaded
	 * 	 overlay_free(ovltp, addr, type);
	 *
	 * 	 overlay_remove(ovltp, ..);
	 */

}

void
overlay_allocate(ovltp, size, type, flags)
	struct overlay_table *ovltp;
	unsigned long size;
	int type, flags;
{
	struct overlay_ops *ops = ovltp->o_private.ovl_ops;

	int error = 0;

	switch (ovltp->o_objtype) {
	case OVL_KERN:
		return (ops->koverlay_allocate(size, type, flags));
	case OVL_VM:
		return (ops->voverlay_allocate(size, type, flags));
	default:
		error = EOPNOTSUPP;
		break;
	}
}

void
overlay_free(ovltp, addr, type)
	struct overlay_table *ovltp;
	void *addr;
	int type;
{
	struct overlay_ops *ops = ovltp->o_private.ovl_ops;

	int error = 0;

	switch (ovltp->o_objtype) {
	case OVL_KERN:
		return (ops->koverlay_free(addr, type));
	case OVL_VM:
		return (ops->voverlay_free(addr, type));
	default:
		error = EOPNOTSUPP;
		break;
	}
}
