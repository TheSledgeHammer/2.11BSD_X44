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

#include <sys/null.h>
#include <sys/queue.h>
#include <sys/tree.h>

#include <vm/include/pmap.h>
#include <devel/vm/hat.h>

vm_offset_t
hat_pa_index(pa, first_phys)
	vm_offset_t pa;
	vm_offset_t first_phys;
{
	vm_offset_t index;
	index = atop(pa - first_phys);
	return (index);
}

pv_entry_t
hat_to_pvh(hatlist, map, object, pa, first_phys)
	hat_list_t hatlist;
	hat_map_t map;
	hat_object_t object;
	vm_offset_t pa;
	vm_offset_t first_phys;
{
    hat_t hat;

    hat = hat_find(hatlist, map, object);
    return (&hat->h_table[hat_pa_index(pa, first_phys)]);
}

void
hat_pv_remove(hatlist, pmap, map, object, sva, eva, first_phys, last_phys)
	hat_list_t hatlist;
	pmap_t pmap;
	hat_map_t map;
	hat_object_t object;
	vm_offset_t sva, eva, first_phys, last_phys;
{
	register pv_entry_t pv, npv;
	register vm_offset_t pa, va;
	int s;

	for (va = sva; va < eva; va += PAGE_SIZE) {
		if (pa < first_phys || pa >= last_phys) {
			continue;
		}

		pv = hat_to_pvh(hatlist, map, object, pa, first_phys);
		s = splimp();
		if (pmap == pv->pv_pmap && va == pv->pv_va) {
			npv = pv->pv_next;
			if (npv) {
				*pv = *npv;
				free((caddr_t) npv, M_VMPVENT);
			} else {
				pv->pv_pmap = NULL;
			}
		} else {
			for (npv = pv->pv_next; npv; npv = npv->pv_next) {
				if (pmap == npv->pv_pmap && va == npv->pv_va) {
					break;
				}
				pv = npv;
			}
			pv->pv_next = npv->pv_next;
			free((caddr_t) npv, M_VMPVENT);
			pv = hat_to_pvh(hatlist, map, object, pa, first_phys);
		}
		splx(s);
	}
}

void
hat_pv_enter(hatlist, pmap, map, object, va, pa, first_phys, last_phys)
	hat_list_t hatlist;
	pmap_t pmap;
	hat_map_t map;
	hat_object_t object;
	vm_offset_t va, first_phys, last_phys;
	vm_offset_t pa;
{
	if (pa >= first_phys && pa < last_phys) {
		register pv_entry_t pv, npv;
		int s;

		pv = hat_to_pvh(hatlist, map, object, pa, first_phys);
		s = splimp();
		if (pv->pv_pmap == NULL) {
			pv->pv_va = va;
			pv->pv_pmap = pmap;
			pv->pv_next = NULL;
			pv->pv_flags = 0;
		} else {
			npv = (pv_entry_t)malloc(sizeof *npv, M_VMPVENT, M_NOWAIT);
		    npv->pv_pmap = pmap;
		    npv->pv_va = va;
		    npv->pv_next = pv->pv_next;
		    pv->pv_next = npv;
		}
	}
}

void
hat_attach(hatlist, hat, map, object)
	hat_list_t hatlist;
	hat_t hat;
	hat_map_t map;
	hat_object_t object;
{
    hat->h_map = map;
    hat->h_object = object;
    LIST_INSERT_HEAD(hatlist, hat, h_next);
}

void
hat_detach(hatlist, map, object)
	hat_list_t hatlist;
	hat_map_t map;
	hat_object_t object;
{
	hat_t hat;
	LIST_FOREACH(hat, hatlist, h_next) {
        if (map == hat->h_map && object == hat->h_object) {
        	LIST_REMOVE(hat, h_next);
        }
    }
}

hat_t
hat_find(hatlist, map, object)
	hat_list_t hatlist;
	hat_map_t map;
	hat_object_t object;
{
	hat_t hat;
	LIST_FOREACH(hat, hatlist, h_next) {
        if (map == hat->h_map && object == hat->h_object) {
            return (hat);
        }
    }
    return (NULL);
}

void
hat_bootstrap(hat, firstaddr)
    hat_t hat;
	vm_offset_t firstaddr;
{
    struct hat_ops *hops;

    hops = hat->h_op;
    return ((*hops->hop_bootstrap)(firstaddr));
}

void *
hat_bootstrap_alloc(hat, size)
	hat_t hat;
	u_long size;
{
	struct hat_ops *hops;

	hops = hat->h_op;
	return ((*hops->hop_bootstrap_alloc)(size));
}

void
hat_init(hat, phys_start, phys_end)
	hat_t hat;
	vm_offset_t	phys_start, phys_end;
{
   struct hat_ops *hops;

   hops = hat->h_op;
   return ((*hops->hop_init)(phys_start, phys_end));
}

vm_offset_t
hat_map(hat, virt, start, end, prot)
	hat_t 		hat;
	vm_offset_t	virt;
	vm_offset_t	start;
	vm_offset_t	end;
	int			prot;
{
	struct hat_ops *hops;

	hops = hat->h_op;
	return ((*hops->hop_map)(virt, start, end, prot));
}

void
hat_enter(hat, pmap, va, pa, prot, wired)
	hat_t hat;
	register pmap_t pmap;
	vm_offset_t va;
	register vm_offset_t pa;
	vm_prot_t prot;
	bool_t wired;
{
	struct hat_ops *hops;

	hops = hat->h_op;
	return ((*hops->hop_enter)(pmap, va, pa, prot, wired));
}

void
hat_remove(hat, pmap, sva, eva)
	hat_t hat;
	register pmap_t pmap;
	vm_offset_t sva, eva;
{
	struct hat_ops *hops;

	hops = hat->h_op;

	return ((*hops->hop_remove)(pmap, sva, eva));
}
