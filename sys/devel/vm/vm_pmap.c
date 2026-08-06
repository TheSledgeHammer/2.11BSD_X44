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
#include <vm/include/vm_map.h>
#include <vm/include/vm_page.h>

#include "vm_idspace.h"

/*
 * vm_map_range_valid:
 * Check's address is within the map's min and max offset.
 * returns true is it is or false if not.
 */
bool_t
vm_map_range_valid(map, addr)
	vm_map_t map;
	vm_offset_t addr;
{
	vm_offset_t start, end;
	vm_offset_t base, i;

	start = map->min_offset;
	end = map->max_offset;
	if (((end - start) < addr) || (addr < start) || (addr > end)) {
		return (FALSE);
	}
	if ((addr == vm_map_min(map)) || (addr == vm_map_max(map))) {
		return (TRUE);
	}
	for (i = trunc_page(start); i < round_page(end); i += PAGE_SIZE) {
		base = (addr + i);
		if (base == (addr + i)) {
			addr = base;
			return (TRUE);
		}
	}
	return (FALSE);
}

#	define UISA	((u_short *) 0177640)	/* first user I-space address */

choverlay_lookup(map, ovbase)
{
	for (i = trunc_page(map->min_offset); i < round_page(map->max_offset); i += PAGE_SIZE) {

	}

}

vm_offset_t
vm_map_addr(map, addr, use_min, use_max)
	vm_map_t map;
	vm_offset_t addr;
	bool_t use_min, use_max;
{
	vm_offset_t base;
	bool_t valid;

	valid = vm_map_range_valid(map, addr);
	if (valid != TRUE) {
		return (0);
	}
	if (addr == 0) {
		if ((use_min == TRUE) && (use_max != TRUE)) {
			return (vm_map_min(map));
		}
		if ((use_min != TRUE) && (use_max == TRUE)) {
			return (vm_map_max(map));
		}
		return (0);
	}
	return (addr);
}

vm_offset_t
vm_idspace_map_addr(idspace, entry, addr, use_min, use_max)
	vm_idspace_t idspace;
	vm_idspace_entry_t entry;
	vm_offset_t addr;
	bool_t use_min, use_max;
{
	if (idspace != NULL) {
		return (vm_map_addr(entry->map, addr, use_min, use_max));
	}
	return (0);
}

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
		data = ptoa(atop(stoa(num)));
	} else {
		data = stoa(num);
	}

#define emask(x, y) (((x) - (y)) << 8)

	while (size >= data) {
		if (type == (PSEG_DATA | PSEG_TEXT)) {
			*desc++ = (emask(data, 1) | flags);
			*addr++ = val;
			val += data;
			size -= data;
		} else {
			val -= data;
			size -= data;
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
estabur_text(map, text, tsize, taddr, addr, desc, val, flags)
	vm_map_t map;
	vm_text_t text;
	vm_size_t tsize;
	caddr_t taddr;
	vm_offset_t *addr, *desc, val;
	int flags;
{
	int error;

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
estabur_data(map, data, dsize, daddr, addr, desc, val, flags)
	vm_map_t map;
	vm_data_t data;
	vm_size_t dsize;
	caddr_t daddr;
	vm_offset_t *addr, *desc, val;
	int flags;
{
	int error;

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
estabur_stack(map, stack, ssize, saddr, addr, desc, val, flags)
	vm_map_t map;
	vm_stack_t stack;
	vm_size_t ssize;
	caddr_t saddr;
	vm_offset_t *addr, *desc, val;
	int flags;
{
	int error;

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
	vm_offset_t *addr, *desc, val, ts;
	int error;

	if (u.u_ovdata.uo_ovbase && tsize) {
		ts = u.u_ovdata.uo_dbase;
	} else {
		ts = tsize;
	}
	if (sep) {
#ifndef NONSEPARATE
		if (!sep_id) {
			goto nomem;
		}
		if (ctos(ts) > 8 || (ctos(dsize) + ctos(ssize)) > 8) {
#endif /* !NONSEPARATE */
			goto nomem;
		}
	} else {
		if ((ctos(ts) + ctos(dsize) + ctos(ssize)) > 8) {
			goto nomem;
		}
	}
	if (u.u_ovdata.uo_ovbase && tsize) {
		ts = u.u_ovdata.uo_ov_offst[NOVL];
	}
	if ((ts + dsize + ssize + USIZE) > maxmem) {
nomem:
		u.u_error = ENOMEM;
		return (-1);
	}

	val = 0;
	addr = &u.u_uisa[0];
	desc = &u.u_uisd[0];
	error = estabur_text(map, text, (vm_size_t)tsize, text->psx_taddr, addr, desc, val, (flags | SEGM_TX));
	if (error != 0) {
		u.u_error = error;
		return (-1);
	}
#ifdef NONSEPARATE
	if (u.u_ovdata.uo_ovbase && ts) {
#else /* !NONSEPARATE */
	if ((u.u_ovdata.uo_ovbase && ts) && !sep) {
#endif /* !NONSEPARATE */
		/*
		 * overlay process, adjust accordingly.
		 * The overlay segment's registers will be set by
		 * choverlay() from sureg().
		 */
		for (val = 0; val < u.u_ovdata.uo_nseg; val++) {
			*addr++ = 0;
			*desc++ = 0;
		}
	}
#ifndef NONSEPARATE
	if (sep) {
		while (addr < &u.u_uisa[8]) {
			*addr++ = 0;
			*desc++ = 0;
		}
	}
#endif /* !NONSEPARATE */
	val = 0;
	error = estabur_data(map, data, (vm_size_t)dsize, data->psx_daddr, addr, desc, val, SEG_RW);
	if (error != 0) {
		u.u_error = error;
		return (-1);
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
	val = ssize;
	error = estabur_stack(map, stack, (vm_size_t)ssize, stack->psx_saddr, addr, desc, val, (SEG_RW | SEGM_ED));
	if (error != 0) {
		u.u_error = error;
		return (-1);
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

#include "vm_uspace.h"

void
vm_sureg(void)
{
	struct proc *p;
	vm_text_t tp;
	vm_offset_t *limudp, *uap, *udp, *rap, *rdp;
	caddr_t taddr, daddr, saddr;

	p = u.u_procp;
	taddr = daddr = p->p_daddr;
	saddr = p->p_saddr;
	tp = p->p_textp;
	if (tp != NULL) {
		taddr = tp->psx_caddr;
	}
#ifndef NONSEPARATE
	limudp = &u.u_uisd[16];
	if (!sep_id) {
		limudp = &u.u_uisd[8];
	}
#else /* !NONSEPARATE */
	limudp = &u.u_uisd[8];
#endif /* !NONSEPARATE */
	rap = UISA_MIN;//vm_map_min(uisa_map);
	rdp = UISD_MIN;//vm_map_min(uisd_map);
	uap = &u.u_uisa[0];
	for (udp = &u.u_uisd[0]; udp < limudp;) {
		*rap++ = *uap++ + (*udp & SEGM_TX ? taddr :
				(*udp & SEGM_ED ? saddr: (*udp & SEGM_ABS ? 0 : daddr)));
		*rdp++ = *udp++;
	}

	if (u.u_ovdata.uo_ovbase && (u.u_uisd[0] & SEGM_TX)) {
		choverlay(u.u_uisd[0] & SEGM_ACCESS);
	}
}


vm_sureg(void)
{
	vm_offset_t *rap, *rdp;
#ifdef NONSEPARATE
	rap = vm_map_addr(uisa_map, 0, TRUE, FALSE);
	rdp = vm_map_addr(uisd_map, 0, TRUE, FALSE);
#else /* !NONSEPARATE */
	rap = vm_map_addr(udsa_map, 0, TRUE, FALSE);
	rdp = vm_map_addr(udsd_map, 0, TRUE, FALSE);
#endif /* !NONSEPARATE */
}

choverlay(ovbase)
{
	vm_offset_t *rap, *rdp;
#ifdef NONSEPARATE
	rap = vm_map_addr(uisa_map, ovbase, FALSE, FALSE);
	rdp = vm_map_addr(uisd_map, ovbase, FALSE, FALSE);
#else /* !NONSEPARATE */
	rap = vm_map_addr(udsa_map, ovbase, FALSE, FALSE);
	rdp = vm_map_addr(udsd_map, ovbase, FALSE, FALSE);
#endif /* !NONSEPARATE */
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
