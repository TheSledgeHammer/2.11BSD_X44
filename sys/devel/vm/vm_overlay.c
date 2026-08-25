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
	vm_object_t tmp;

	/* check ovl_object for existing vm_object */
	tmp = ovl_object_lookup_vm_object(oobject);
	if (tmp == NULL) {
		/* lookup vm_object or allocate */
		if (pager != NULL) {
			tmp = vm_object_lookup(pager);
		} else {
			tmp = vm_object_allocate(size);
		}
		if (tmp != NULL) {
			if (*vobject == NULL) {
				*vobject = tmp;
			}
			return (tmp);
		}
	}
	return (NULL);
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

	if (oobject == NULL) {
		return;
	}

	tmp = overlay_find_vm_object(oobject, oobject->pager, &vobject, sizeof(vobject));
	if (tmp == NULL) {
		return;
	}
	if (tmp != vobject) {
		return;
	}
	ovl_object_enter_vm_object(oobject, vobject);
}

/*
 * overlay_find_vm_segment:
 * look for an existing vm_segment within the ovl_segment's
 * list of vm_segment's. If no vm_segment exists allocate
 * a new vm_segment.
 * returns vm_segment on success or null if unsuccessful.
 */
vm_segment_t
overlay_find_vm_segment(osegment, vsegment, vmsgoffset)
	ovl_segment_t osegment;
	vm_segment_t *vsegment;
	vm_offset_t vmsgoffset;
{
	vm_object_t vobject;
	vm_segment_t tmp;

	/* check ovl_segment for existing vm_segment */
	tmp = ovl_segment_lookup_vm_segment(osegment);
	if (tmp == NULL) {
		vobject = ovl_object_lookup_vm_object(osegment->object);
		if (vobject != NULL) {
			tmp = vm_segment_lookup(vobject, vmsgoffset);
			if (tmp == NULL) {
				tmp = vm_segment_allocate(vobject, vmsgoffset);
			}
			if (tmp != NULL) {
				if (*vsegment == NULL) {
					*vsegment = tmp;
				}
				return (tmp);
			}
		}
	}
	return (NULL);
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

	if (osegment == NULL) {
		return;
	}

	tmp = overlay_find_vm_segment(osegment, &vsegment, vsegment->offset);
	if (tmp == NULL) {
		return;
	}
	if (tmp != vsegment) {
		return;
	}
	ovl_segment_insert_vm_segment(osegment, vsegment);
}

/*
 * overlay_find_vm_page:
 * look for an existing vm_page within the ovl_page's
 * list of vm_page's. If no vm_page exists allocate
 * a new vm_page.
 * returns vm_page on success or null if unsuccessful.
 */
vm_page_t
overlay_find_vm_page(opage, vpage, vmpgoffset)
	ovl_page_t opage;
	vm_page_t *vpage;
	vm_offset_t vmpgoffset;
{
	vm_segment_t vsegment;
	vm_page_t tmp;

	/* check ovl_page for existing vm_page */
	tmp = ovl_page_lookup_vm_page(opage);
	if (tmp == NULL) {
		vsegment = ovl_segment_lookup_vm_segment(opage->segment);
		if (vsegment != NULL) {
			tmp = vm_page_lookup(vsegment, vmpgoffset);
			if (tmp == NULL) {
				tmp = vm_page_allocate(vsegment, vmpgoffset);
			}
			if (tmp != NULL) {
				if (*vpage == NULL) {
					*vpage = tmp;
				}
				return (tmp);
			}
			return (tmp);
		}
	}
	return (NULL);
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

	if (opage == NULL) {
		return;
	}

	tmp = overlay_find_vm_page(opage, &vpage, vpage->offset);
	if (tmp == NULL) {
		return;
	}
	if (tmp != vpage) {
		return;
	}
	ovl_page_insert_vm_page(opage, vpage);
}
