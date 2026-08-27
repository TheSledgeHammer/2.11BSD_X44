/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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
 *	Virtual Memory Overlays. OVL to VM management interface.
 */

/*
 * TODO:
 * Handling of VM Overlayed Maps, Segments and Pages.
 * - VM is to cleanup after itself, when using the OVL.
 * 		- VM Resident Pages etc should be forbidden and actively excluded.
 * 			As they can potentially clog up ovlspace indefinitely.
 * - All VM components (i.e. maps etc) should be marked as being in the OVL.
 * - If the VM misses or fails to clean up overlayed components. The OVL
 * can remove them as needed (more critical if space is required).
 */

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>
#include <vm/include/vm_segment.h>
#include <vm/include/vm_object.h>

#include <ovl/include/ovl.h>
#include <ovl/include/ovl_page.h>
#include <ovl/include/ovl_segment.h>
#include <ovl/include/ovl_object.h>

/*
 * overlay_find_vm_object:
 * look for an existing vm_object within the ovl_object's
 * list of vm_object's. If no vm_object exists allocate
 * a new vm object.
 * returns vm object on success or null if unsuccessful.
 */
vm_object_t
overlay_find_vm_object(oobject, pager, vobject, size)
	ovl_object_t oobject;
	vm_pager_t pager;
	vm_object_t *vobject;
	vm_size_t size;
{
	if (oobject == NULL) {
		return (NULL);
	}

	if (*vobject == NULL) {
		*vobject = ovl_object_lookup_vm_object(oobject);
		if (*vobject != NULL) {
			return (*vobject);
		}
		if (pager != NULL) {
			*vobject = vm_object_lookup(pager);
		} else {
			*vobject = ovl_object_allocate_vm_object(oobject, size);
		}
		if (*vobject != NULL) {
			return (*vobject);
		}
		return (NULL);
	}
	return (*vobject);
}

/*
 * overlay_vm_object:
 * allocate vm_object and insert into
 * ovl_object's associated vm_object list.
 */
void
overlay_vm_object(oobject, vobject)
	ovl_object_t oobject;
	vm_object_t vobject;
{
	vm_object_t tmp;

	tmp = overlay_find_vm_object(oobject, oobject->pager, &vobject, sizeof(vobject));
	if (tmp == NULL || tmp != vobject) {
		ovl_object_deallocate_vm_object(oobject);
	}
}

/*
 * overlay_find_vm_segment:
 * look for an existing vm_segment within the ovl_segment's
 * list of vm_segment's. If no vm_segment exists allocate
 * a new vm_segment.
 * returns vm_segment on success or null if unsuccessful.
 */
vm_segment_t
overlay_find_vm_segment(osegment, vsegment, voffset)
	ovl_segment_t osegment;
	vm_segment_t *vsegment;
	vm_offset_t voffset;
{
	vm_object_t vobject;

	if (osegment == NULL) {
		return (NULL);
	}

	if (*vsegment == NULL) {
		*vsegment = ovl_segment_lookup_vm_segment(osegment);
		if (*vsegment != NULL) {
			return (*vsegment);
		}
		vobject = ovl_object_lookup_vm_object(osegment->object);
		if (vobject != NULL) {
			*vsegment = ovl_segment_allocate_vm_segment(osegment, vobject, voffset);
			if (*vsegment != NULL) {
				return (*vsegment);
			}
		}
		return (NULL);
	}

	return (*vsegment);
}

/*
 * overlay_vm_segment:
 * allocate vm_segment and insert into
 * ovl_segment's associated vm_segment list.
 */
void
overlay_vm_segment(osegment, vsegment)
	ovl_segment_t osegment;
	vm_segment_t vsegment;
{
	vm_segment_t tmp;

	tmp = overlay_find_vm_segment(osegment, &vsegment, vsegment->offset);
	if (tmp == NULL || tmp != vsegment) {
		ovl_segment_deallocate_vm_segment(osegment);
	}
}

/*
 * overlay_find_vm_page:
 * look for an existing vm_page within the ovl_page's
 * list of vm_page's. If no vm_page exists allocate
 * a new vm_page.
 * returns vm_page on success or null if unsuccessful.
 */
vm_page_t
overlay_find_vm_page(opage, vpage, voffset)
	ovl_page_t opage;
	vm_page_t *vpage;
	vm_offset_t voffset;
{
	vm_segment_t vsegment;

	if (opage == NULL) {
		return (NULL);
	}
	if (*vpage == NULL) {
		*vpage = ovl_page_lookup_vm_page(opage);
		if (*vpage != NULL) {
			return (*vpage);
		}
		vsegment = ovl_segment_lookup_vm_segment(opage->segment);
		if (vsegment != NULL) {
			*vpage = ovl_page_allocate_vm_page(opage, vsegment, voffset);
			if (*vpage != NULL) {
				return (*vpage);
			}
		}
		return (NULL);
	}

	return (*vpage);
}

/*
 * overlay_vm_page:
 * allocate vm_page and insert into
 * ovl_page's associated vm_page list.
 */
void
overlay_vm_page(opage, vpage)
	ovl_page_t opage;
	vm_page_t vpage;
{
	vm_page_t tmp;

	tmp = overlay_find_vm_page(opage, &vpage, vpage->offset);
	if (tmp == NULL || tmp != vpage) {
		ovl_page_deallocate_vm_page(opage);
	}
}
