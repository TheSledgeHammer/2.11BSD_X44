/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: vm_mmap.c 1.6 91/10/21$
 *
 *	@(#)vm_mmap.c	8.10 (Berkeley) 2/19/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */

/*
 * Mapped file (mmap) interface to VM
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/resourcevar.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/mount.h>
#include <sys/sysdecl.h>

#include <miscfs/specfs/specdev.h>

#include <vm/include/vm.h>
#include <vm/include/vm_pager.h>
#include <vm/include/vm_prot.h>
#include <vm/include/vm_text.h>

int
mincore_segment_page(vm_map_entry_t entry, vm_amap_t amap, vm_object_t object, vm_size_t size, vm_offset_t offset)
{
	vm_anon_t anon;
	vm_segment_t segment;
	vm_page_t page;
	int sgi, pgi;
	int i, j;

	if (amap != NULL) {
		vm_amap_lock(amap);
	}
	if (object != NULL) {
		vm_object_lock(object);
	}

	for (i = 0; i < round_segment(size); i += SEGMENT_SIZE) {
		sgi = 0;
		if (amap != NULL) {
			/* Check the upper layer first. */
			anon = vm_amap_lookup(&entry->aref, offset + i);
			if ((anon != NULL) && (anon->u.an_segment != NULL)) {
				/*
				 * Anon has the segment for this entry
				 * offset.
				 */
				sgi = 1;
			}
		}
		if ((object != NULL) && (sgi == 0)) {
			segment = vm_segment_lookup(object, offset + i);
			if (segment != NULL) {
				/*
				 * Object has the segment for this entry
				 * offset.
				 */
				sgi = 1;
			}
		}

		if ((segment != NULL) && (sgi != 0)) {
			for (j = 0; j < round_page(size); j += PAGE_SIZE) {
				pgi = 0;
				if (amap != NULL) {
					/* Check the upper layer first. */
					anon = vm_amap_lookup(&entry->aref, offset + j);
					if ((anon != NULL) && (anon->u.an_page != NULL)) {
						/*
						 * Anon has the segment page for this entry
						 * offset.
						 */
						pgi = 1;
					}
				}
				if ((segment != NULL) && (pgi == 0)) {
					page = vm_page_lookup(segment, offset + j);
					if (page != NULL) {
						/*
						 * Segment has the page for this entry
						 * offset.
						 */
						pgi = 1;
					}
				}
			}
		}
	}
	if (amap != NULL) {
		vm_amap_unlock(amap);
	}
	if (object != NULL) {
		vm_object_unlock(object);
	}

	/* check for segment and page inconsistencies */
	if (sgi < pgi) {
		return (ENOMEM);
	}
	return (pgi);
}

int
vm_mincore(vm_map_t map, vm_offset_t addr, vm_size_t len, vm_offset_t offset, char *vec)
{
	vm_map_entry_t entry;
	vm_object_t object;
	vm_amap_t amap;
	vm_offset_t estart, eend, eaddr, eoffset;
	vm_size_t elen;
	int error, npages, vecindex;
	char *evec;

	vm_map_lock_read(map);
	if (vm_map_lookup_entry(map, addr, &entry)) {
		estart = entry->start;
		eend = entry->end;
		eoffset = entry->offset;
		elen = (eend - estart);
		eaddr = ((estart + elen) - elen);

		if (elen != len) {
			error = ENOMEM;
			goto out;
		}
		if (eaddr != addr) {
			error = ENOMEM;
			goto out;
		}
		if (eoffset != offset) {
			vm_map_unlock_read(map);
			error = ENOMEM;
			goto out;
		}
		if (entry->is_sub_map == TRUE) {
			continue;
		}
		if ((entry->object.vm_object == NULL)
				|| (entry->aref.ar_amap == NULL)) {
			error = ENOMEM;
			goto out;
		}
		amap = entry->aref.ar_amap;			/* top layer */
		object = entry->object.vm_object;	/* bottom layer */

		error = mincore_segment_page(entry, amap, object, elen, eoffset);
		switch (error) {
		case 0:
			/* Nothing was found */
			error = ENOMEM;
			goto out;
		case 1:
			*evec = (char)error;
			npages = atop(eaddr - eoffset);
			for (vecindex = 0; vecindex < npages; vecindex++) {
				/* found vec */
				if (evec == vec) {
					error = 0;
					goto out;
				} else {
					error = ENOMEM;
					goto out;
				}
			}
			break;
		case ENOMEM:
			goto out;
		}
	}

out:
	vm_map_unlock_read(map);
	return (error);
}

/* ARGSUSED */
int
mincore2()
{
	register struct mincore_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(char *) vec;
	} *uap = (struct mincore_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_map_t map;
	vm_offset_t addr, offset;
	vm_size_t size;
	char *vec;

	p = u.u_procp;
	map = &p->p_vmspace->vm_map;
	addr = (vm_offset_t)SCARG(uap, addr);
	size = (vm_size_t)SCARG(uap, len);
	vec = SCARG(uap, vec);
	offset = (addr + vm_map_min(map));
	if ((addr & PAGE_MASK) || ((addr + size) < addr)) {
		return (ENOMEM);
	}

	return (mincore_entry(map, addr, size, offset, vec));
}


/* ARGSUSED */
int
mincore1()
{
	register struct mincore_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(char *) vec;
	} *uap = (struct mincore_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_map_t map;
	vm_map_entry_t current, entry;
	vm_object_t object;
	vm_object_t page;
	vm_amap_t amap;
	vm_anon_t anon;
	vm_offset_t addr, start, end, cend;
	vm_size_t size;
	char *vec;
	char pgi;
	int error;

	p = u.u_procp;
	map = &p->p_vmspace->vm_map;
	addr = (vm_offset_t)SCARG(uap, addr);
	size = round_page((vm_size_t)SCARG(uap, len));
	vec = SCARG(uap, vec);
	if ((addr & PAGE_MASK) || (SCARG(uap, addr) + SCARG(uap, len)) < SCARG(uap, addr)) {
		return (ENOMEM);
	}

	vm_map_lock_read(map);
	/* validate map entry */
	if (vm_map_lookup_entry(map, addr, &entry)) {
		start = entry->start;
		end = entry->end;
		if ((start != addr) || (end != (start + size))) {
			vm_map_unlock_read(map);
			return (ENOMEM);
		}
	} else {
		start = addr;
		end = (start + size);
	}

	if (start <= end) {
		vm_map_unlock_read(map);
		return (ENOMEM);
	}
	entry = CIRCLEQ_NEXT(entry, cl_entry);

	CIRCLEQ_FOREACH(current, &map->cl_header, cl_entry) {
		if (current != entry) {
			vm_map_unlock_read(map);
			return (EINVAL);
		}
		if ((current->start < end) && (entry->start > current->end)) {
			vm_map_unlock_read(map);
			return (ENOMEM);
		}
		if (current->is_sub_map || current->object.vm_object == NULL) {
			continue;
		}
		if (addr < current->start) {
			addr = current->start;
		}
		cend = current->end;
		if (cend > end) {
			cend = end;
		}
		amap = entry->aref.ar_amap;	/* top layer */
		object = entry->object.vm_object;	/* bottom layer */
		if (amap != NULL) {
			vm_amap_lock(amap);
		}
		if (object != NULL) {
			vm_object_lock(object);
		}

		for (; addr < cend; addr += PAGE_SIZE, vec++) {
			pgi = 0;
			if (amap != NULL) {
				anon = vm_amap_lookup(&current->aref, start - entry->start);
				if (anon != NULL && anon->u.an_page != NULL) {
					pgi = 1;
				}
			}
		}
		if (object != NULL && pgi == 0) {
			page = vm_page_lookup(segment, entry->offset + (start - entry->start));
		}
	}


	vm_map_unlock_read(map);
	return (error);
}
