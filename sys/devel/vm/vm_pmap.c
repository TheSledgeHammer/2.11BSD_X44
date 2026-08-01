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
 *	@(#)vm_page.c	8.4 (Berkeley) 1/9/95
 *
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

#include <vm/include/vm.h>
#include <vm/include/vm_page.h>

vm_offset_t	kentry_data;

/*
 * vm_pmap_bootstrap:
 *
 * Allocates virtual address space from pmap_bootstrap_alloc.
 */
void
vm_pmap_bootstrap(void)
{
    extern vm_offset_t	kentry_data;
    vm_size_t kentry_data_size, kmap_size, kentry_size;

    kmap_size = (MAX_KMAP * sizeof(struct vm_map));
	kentry_size = (MAX_KMAPENT * sizeof(struct vm_map_entry));
	kentry_data_size = round_page(kmap_size + kentry_size);
	kentry_data = (vm_offset_t)pmap_bootstrap_alloc(kentry_data_size);
}

/*
 * vm_pmap_bootinit:
 *
 * Allocates item from space made available by vm_pmap_bootstrap.
 */
void *
vm_pmap_bootinit(item, size, nitems)
	void 		*item;
	vm_size_t 	size;
	int 		nitems;
{
	extern vm_offset_t	kentry_data;
	vm_size_t free, totsize, result, itemsize;

	free = kentry_data;
	totsize = (size * nitems);
	result = (free - totsize);
	itemsize = (vm_size_t)(sizeof(item) + size);
	if ((free < totsize) || (itemsize > result)) {
		panic("vm_pmap_bootinit: not enough space allocated");
	}
	bzero(item, totsize);
	item = (void *)(vm_size_t)itemsize;
	kentry_data = result;
	return (item);
}


void
vm_pmap_bootstrap(vm_offset_t *data, vm_size_t map_size, unsigned long map_number, vm_size_t entry_size, unsigned long entry_number)
{
	vm_size_t entry_data_size, mapsize, entrysize;

    mapsize = (map_number * map_size);
	entrysize = (entry_number * entry_size);
	entry_data_size = round_page(mapsize + entrysize);
	*data = (vm_offset_t)pmap_bootstrap_alloc(entry_data_size);
}

#include "vm_idspace.h"

static int
estabur_lookup(map, size, addr, desc, val, type, flags)
	vm_map_t map;
	vm_size_t size;
	vm_offset_t *addr, *desc;
	vm_offset_t val;
	int type, flags;
{
	vm_offset_t virt, phys, num, data;
	int error;

	if (map == NULL) {
		return (ENOMEM);
	}

	error = vm_pmap_validate_phys(map, &virt, &phys, &num, size, vm_map_min(map), vm_map_max(map));
	if (error != 0) {
		return (error);
	}

	/* check data size */
	if (data < SEGMENT_SIZE) {
		data = ptoa(num);
	} else {
		data = stoa(num);
	}

#define emask(x, y) (((x) - (y)) << 8)

	while (size >= data) {
		if (type == (PSEG_DATA | PSEG_TEXT)) {
			*desc++ = (emask(data, 1) | flags);
			*addr++ = val;
			val += data;
			data -= data;
		} else {
			val -= data;
			data -= data;
			*--desc = (emask(0, 0) | flags);
			*--addr = val;
		}
	}
	if (size) {
		if (type == (PSEG_DATA | PSEG_TEXT)) {
			*desc++ = (emask(size, 1) | flags);
			*addr++ = val;
			if (type == PSEG_DATA) {
				val += data;
			}
		} else {
			*--desc = (emask(data, size) | flags);
			*--addr = (val - data);
		}
	}
#undef emask
	return (0);
}

/* text */
static int
estabur_text(map, text, tsize, taddr, addr, desc, flags)
	vm_map_t map;
	vm_text_t text;
	vm_size_t tsize;
	caddr_t taddr;
	vm_offset_t *addr, *desc;
	int flags;
{
	vm_offset_t val;
	int error;

	val = 0;
	if (text == NULL) {
		return (ENOMEM);
	}

	error = estabur_lookup(map, tsize, addr, desc, val, PSEG_TEXT, flags);
	if (error != 0) {
		return (error);
	}
	TEXT_SEGMENT(text, tsize, taddr, flags);
	return (0);
}

/* data */
static int
estabur_data(map, data, dsize, daddr, addr, desc, flags)
	vm_map_t map;
	vm_data_t data;
	vm_size_t dsize;
	caddr_t daddr;
	vm_offset_t *addr, *desc;
	int flags;
{
	vm_offset_t val;
	int error;

	val = 0;
	if (data == NULL) {
		return (ENOMEM);
	}

	error = estabur_lookup(map, dsize, addr, desc, val, PSEG_DATA, flags);
	if (error != 0) {
		return (error);
	}
	DATA_SEGMENT(data, dsize, daddr, flags);
	return (0);
}

/* stack */
static int
estabur_stack(map, stack, ssize, saddr, addr, desc, flags)
	vm_map_t map;
	vm_stack_t stack;
	vm_size_t ssize;
	caddr_t saddr;
	vm_offset_t *addr, *desc;
	int flags;
{
	vm_offset_t val;
	int error;

	val = ssize;
	if (stack == NULL) {
		return (ENOMEM);
	}

	error = estabur_lookup(map, ssize, saddr, addr, desc, val, PSEG_STACK, flags);
	if (error != 0) {
		return (error);
	}
	STACK_SEGMENT(stack, ssize, saddr, flags);
	return (0);
}

static int
estabur(map, text, data, stack, tsize, dsize, ssize, sep, flags)
	vm_map_t map;
	vm_text_t text;
	vm_data_t data;
	vm_stack_t stack;
	segsz_t tsize, dsize, ssize;
	int sep, flags;
{
	vm_offset_t *addr, *desc;
	int error;

	addr = &u.u_uisa[0];
	desc = &u.u_uisd[0];

	error = estabur_text(map, text, (vm_size_t)tsize, text->psx_taddr, addr, desc, (flags | SEGM_TX));
	if (error != 0) {
		return (error);
	}

#ifndef NONSEPARATE
	if (sep) {
		while (addr < &u.u_uisa[8]) {
			*addr++ = 0;
			*desc++ = 0;
		}
	}
#endif /* !NONSEPARATE */

	error = estabur_data(map, data, (vm_size_t)dsize, data->psx_daddr, addr, desc, SEG_RW);
	if (error != 0) {
		return (error);
	}

	while (*addr < &u.u_uisa[8]) {
		if (*desc & SEGM_ABS) {
			desc++;
			addr++;
			continue;
		}
		*desc++ = 0;
		*addr++ = 0;
	}

#ifndef NONSEPARATE
	if (sep) {
		while (addr < &u.u_uisa[16]) {
			if (*desc & SEGM_ABS) {
				desc++;
				addr++;
				continue;
			}
			*desc++ = 0;
			*addr++ = 0;
		}
	}
#endif /* !NONSEPARATE */

	error = estabur_stack(map, stack, (vm_size_t)ssize, stack->psx_saddr, addr, desc, (SEG_RW | SEGM_ED));
	if (error != 0) {
		return (error);
	}

#ifndef NONSEPARATE
	if (!sep) {
		addr = &u.u_uisa[0];
		desc = &u.u_uisa[8];
		while (addr < &u.u_uisa[8]) {
			*desc++ = *addr++;
		}
		addr = &u.u_uisd[0];
		desc = &u.u_uisd[8];
		while (addr < &u.u_uisd[8]) {
			*desc++ = *addr++;
		}
	}
#endif /* !NONSEPARATE */

	vm_sureg();
	return (0);
}

int
vm_estabur(p, tsize, dsize, ssize, sep, flags)
	struct proc	*p;
	segsz_t	 tsize, dsize, ssize;
	int sep, flags;
{
	register struct vmspace *vm;
	vm_map_t map;
	vm_pseudo_segment_t	pseg;
	int error;

	vm = p->p_vmspace;
	map = vm->vm_map;
	pseg = &vm->vm_psegment;
	if (pseg == NULL) {
		return (ENOMEM);
	}
	error = estabur(map, pseg->ps_text, pseg->ps_data, pseg->ps_stack, tsize, dsize, ssize, sep, flags);
	if (error != 0) {
		return (error);
	}
	return (0);
}

static int
vm_idspace_map_check_map_entry(idspacemap, addr, size)
	vm_idspace_map_t idspacemap;
	vm_offset_t addr;
	vm_size_t size;
{
	vm_map_t map;
	vm_map_entry_t entry, next;
	vm_object_t object;
	vm_offset_t estart, eend, eoffset, eaddr;
	vm_size_t esize;
	bool_t isentry;
	int error;

	map = idspacemap->map;
	vm_map_lock_read(map);
	isentry = vm_map_lookup_entry(map, addr, &entry);
retry:
	if (isentry) {
		estart = entry->start;
		eend = entry->end;
		eoffset = entry->offset;
		esize = round_novl(eend - estart);
		eaddr = (eoffset + esize);
		object = entry->object.vm_object;

		if (eoffset != offset) {
			error = ENOMEM;
			goto out;
		}
		if (esize != size) {
			error = ENOMEM;
			goto out;
		}
		if (eaddr != addr) {
			error = ENOMEM;
			goto out;
		}
		if (object == NULL) {
			error = ENOMEM;
			goto out;
		}
		kmem_free_wakeup(map, eaddr, esize);
		error = 0;
		goto out;
	} else {
		CIRCLEQ_FOREACH(next, &map->cl_header, cl_entry) {
			if (next != entry) {
				estart = next->start;
				eend = next->end;
				eoffset = next->offset;
				esize = round_novl(eend - estart);
				eaddr = (eoffset + esize);

				if (eaddr <= addr) {
					isentry = vm_map_lookup_entry(map, addr, &entry);
					goto retry;
				} else {
					error = ENOMEM;
					goto out;
				}
			}  else {
				error = ENOMEM;
				goto out;
			}
		}
	}

out:
	vm_map_unlock_read(map);
	return (error);
}
