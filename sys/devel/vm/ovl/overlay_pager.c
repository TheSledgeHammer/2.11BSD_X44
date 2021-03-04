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
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 1987, 1990 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Avadis Tevanian, Jr., Michael Wayne Young
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/uio.h>
#include <sys/mount.h>

#include <devel/vm/ovl/ovl.h>
#include <devel/vm/ovl/ovl_page.h>
#include <devel/vm/ovl/overlay_pager.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_page.h>

struct pagerlst 	overlay_pager_list;

static void			overlay_pager_init(void);
static vm_pager_t	overlay_pager_alloc(caddr_t, vm_size_t, vm_prot_t, vm_offset_t);
static boolean_t	overlay_pager_dealloc(vm_pager_t);

struct pagerops overlaypagerops = {
		overlay_pager_init,
		overlay_pager_alloc,
		overlay_pager_dealloc,
		overlay_pager_getpage,
		overlay_pager_putpage,
		overlay_pager_haspage,
		overlay_pager_cluster
};

static void
overlay_pager_init()
{
	TAILQ_INIT(&overlay_pager_list);
}

static vm_pager_t
overlay_pager_alloc(handle, size, prot, foff)
	caddr_t handle;
	vm_size_t size;
	vm_prot_t prot;
	vm_offset_t foff;
{
	register vm_pager_t  	pager;
	register ovl_pager_t 	ovl;
	ovl_object_t			ovl_object;

	if (handle) {
		pager = vm_pager_lookup(&overlay_pager_list, handle);
		if (pager != NULL) {
			if (ovl_object_lookup(pager) == NULL) {
				panic("overlay_pager_alloc: bad overlay object");
			}
			return (pager);
		}
	}

	ovl = (ovl_pager_t)handle;
	ovl_object = ovl->ovl_object;
	pager = ovl_object->ovo_pager;

	vm_object_t fake_vm_object = ovl_object->ovo_vm_object;

	if (ovl_object == NULL) {
		ovl_object = ovl_object_allocate(0);
		fake_vm_object = vm_object_allocate(0);
	} else {

		if (fake_vm_object) { 											/* check ovl_object contains a vm object */
			fake_vm_object = ovl_object_lookup_vm_object(ovl_object); 	/* search for the vm_object */
		}

		TAILQ_INSERT_TAIL(&overlay_pager_list, pager, pg_list);

		/*
		 * Associate ovl_object with pager & add vm_object to
		 * the ovl_object's list of associated vm_objects.
		 */
		ovl_object_enter(ovl_object, pager);
		ovl_object_enter_vm_object(ovl_object, fake_vm_object);
		ovl_object_setpager(ovl_object, pager, (vm_offset_t)0, FALSE);
	}

	pager->pg_handle = handle;
	pager->pg_ops = &overlaypagerops;
	pager->pg_type = PG_OVERLAY;
	pager->pg_data = ovl;

	return (pager);
}

static void
overlay_pager_dealloc(pager)
	vm_pager_t  	pager;
{
	register ovl_pager_t  ovl;
	ovl_object_t		  ovl_object;
	vm_object_t 		  fake_vm_object;

	TAILQ_REMOVE(&overlay_pager_list, pager, pg_list);

	ovl = (ovl_pager_t) pager->pg_data;
	ovl_object = ovl->ovl_object;
	fake_vm_object = ovl_object->ovo_vm_object;

	free((caddr_t)ovl, M_VMPGDATA);
	free((caddr_t)pager, M_VMPAGER);
}

static int
overlay_pager_getpage(pager, mlist, npages, sync)
	vm_pager_t	pager;
	vm_page_t	*mlist;
	int			npages;
	boolean_t	sync;
{
	ovl_pager_t ovl = (ovl_pager_t) pager->pg_data;
	vm_page_t 	vm_page = *mlist;

	return (overlay_pager_io(ovl, vm_page, npages, OVL_PGR_GET));
}

static int
overlay_pager_putpage(pager, mlist, npages, sync)
	vm_pager_t	pager;
	vm_page_t	*mlist;
	int			npages;
	boolean_t	sync;
{
	ovl_pager_t ovl = (ovl_pager_t) pager->pg_data;
	vm_page_t 	vm_page = *mlist;

	return (overlay_pager_io(ovl, vm_page, npages, OVL_PGR_PUT));
}

static boolean_t
overlay_pager_haspage(pager, offset)
	vm_pager_t pager;
	vm_offset_t offset;
{
	if(pager == NULL) {
		return (FALSE);
	}

	return (TRUE);
}

static void
overlay_pager_cluster(pager, offset, loffset, hoffset)
	vm_pager_t	pager;
	vm_offset_t	offset;
	vm_offset_t	*loffset;
	vm_offset_t	*hoffset;
{
	panic("overlay_pager_cluster is yet to be implemented");
}

static int
overlay_pager_io(ovl, vm_page, npages, flags)
	register ovl_pager_t ovl;
	vm_page_t vm_page;
	int npages;
	int flags;
{
	register vm_offset_t 	segoffset, pgoffset;
	ovl_object_t 			ovl_object;
	ovl_segment_t 			ovl_segment;
	ovl_page_t 	  			ovl_page;

	ovl_object = ovl->ovl_object;

	/* if overlay object is null. Assume overlay_pager is also null & return */
	if(ovl_object == NULL) {
		return (VM_PAGER_FAIL);
	}
	/* determine number of pages, to find overlay segment & page offset */
	if(npages == 1) {
		ovl_segment = CIRCLEQ_FIRST(&ovl_object->ovo_ovseglist);
		ovl_page = TAILQ_FIRST(&ovl_segment->ovs_ovpglist);
	} else {
		while(npages--) {
			segoffset = stoa(round_segment(npages) / SEGMENT_SIZE);
			pgoffset = ptoa(round_page(npages) / PAGE_SIZE);
			if(ovl_segment_lookup(ovl_object, segoffset)) {
				ovl_segment = ovl_segment_lookup(ovl_object, segoffset);

				if(ovl_page_lookup(ovl_segment, pgoffset)) {
					ovl_page = ovl_page_lookup(ovl_segment, pgoffset);
					break;
				}
			}
		}
	}

	/* check if overlay page contains a vm_page */
	vm_page = ovl_page->ovp_vm_page;

	if(vm_page) {
		/* find vm page in list of overlaid pages */
		vm_page = ovl_page_lookup_vm_page(ovl_page);
		/* if vm_page is not null return depending on the flags set */
		if(vm_page != NULL) {
			if(flags == OVL_PGR_GET){
				return (VM_PAGER_OK);
			}
			if(flags == OVL_PGR_PUT) {
				return (VM_PAGER_ERR);
			}
		} else {
			if(flags == OVL_PGR_GET){
				return (VM_PAGER_ERR);
			}
			if(flags == OVL_PGR_PUT) {
				return (VM_PAGER_OK);
			}
		}
	}
	/* Shouldn't reach this */
	return (VM_PAGER_FAIL);
}
