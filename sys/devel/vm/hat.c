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

typedef void *hat_object_t;
typedef void *hat_map_t;

struct hat_list;
SIMPLEQ_HEAD(hat_list, hat);
struct hat {
	SIMPLEQ_ENTRY(hat)  h_next;
    pmap_t              h_pmap;
    pv_entry_t			h_table;
    hat_map_t			h_map;
    hat_object_t		h_object;

    struct hat_ops		*h_ops;
};
typedef struct hat      *hat_t;

struct hat_ops {
    void        (*hops_bootstrap)(vm_offset_t);
    void        *(*hops_bootstrap_alloc)(u_long);
    void        (*hops_init)(vm_offset_t, vm_offset_t);
    vm_offset_t (*hops_map)(vm_offset_t, vm_offset_t, vm_offset_t, int);
    void	   	(*hops_enter)(pmap_t, vm_offset_t, vm_offset_t, vm_prot_t, bool_t);
    void        (*hops_remove)(pmap_t, vm_offset_t, vm_offset_t);
};

struct hat_list hat_header = SIMPLEQ_HEAD_INITIALIZER(hat_header);

void
hat_attach(hat_t hat, hat_map_t map, hat_object_t object)
{
    hat->h_map = map;
    hat->h_object = object;
    SIMPLEQ_INSERT_HEAD(&hat_header, hat, h_next);
}

void
hat_detach(hat_map_t map, hat_object_t object)
{
	hat_t hat;
    SIMPLEQ_FOREACH(hat, &hat_header, h_next) {
        if (map == hat->h_map && object == hat->h_object) {
            SIMPLEQ_REMOVE(&hat_header, hat, hat, h_next);
        }
    }
}

hat_t
hat_find(hat_map_t map, hat_object_t object)
{
	hat_t hat;
    SIMPLEQ_FOREACH(hat, &hat_header, h_next) {
        if (map == hat->h_map && object == hat->h_object) {
            return (hat);
        }
    }
    return (NULL);
}

pv_entry_t
hat_to_pv(hat_map_t map, hat_object_t object)
{
    hat_t hat;

    hat = hat_find(map, object);
    return (hat->h_table);
}

void
hat_bootstrap(hat, firstaddr)
    hat_t hat;
	vm_offset_t firstaddr;
{
    struct hat_ops *hops;

    hops = hat->h_ops;
    return ((*hops->hops_bootstrap)(firstaddr));
}

void *
hat_bootstrap_alloc(hat, size)
	hat_t hat;
	u_long size;
{
	struct hat_ops *hops;

	hops = hat->h_ops;
	return ((*hops->hops_bootstrap_alloc)(size));
}

void
hat_init(hat, phys_start, phys_end)
	hat_t hat;
	vm_offset_t	phys_start, phys_end;
{
   struct hat_ops *hops;

   hops = hat->h_ops;
   return ((*hops->hops_init)(phys_start, phys_end));
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

	hops = hat->h_ops;
	return ((*hops->hops_map)(virt, start, end, prot));
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

	hops = hat->h_ops;
	return ((*hops->hops_enter)(pmap, va, pa, prot, wired));
}

void
hat_remove(hat, pmap, sva, eva)
	hat_t hat;
	register pmap_t pmap;
	vm_offset_t sva, eva;
{
	struct hat_ops *hops;

	hops = hat->h_ops;
	return ((*hops->hops_remove)(pmap, sva, eva));
}

/* static hat initializer */
hat_t vm_hat;
hat_t ovl_hat;

void
vm_hat_init(hat, offset, addr, length, find_space, phys_start, phys_end)
	hat_t hat;
	vm_offset_t	offset;
	vm_offset_t	*addr;
	vm_size_t	length;
	bool_t		find_space;
	vm_offset_t phys_start, phys_end;
{
	vm_size_t size, npg;

    vm_object_reference(kernel_object);
    vm_map_find(kernel_map, kernel_object, offset, addr, length, find_space);

	npg = atop(phys_end - phys_start);
	size = (vm_size_t)(sizeof(struct pv_entry) * npg + npg);

	hat->h_table = (struct pv_entry *)kmem_alloc(kernel_map, size);
	hat_attach(hat, kernel_map, kernel_object);
}

void
ovl_hat_init(hat, offset, addr, length, find_space, phys_start, phys_end)
	hat_t hat;
	vm_offset_t	offset;
	vm_offset_t	*addr;
	vm_size_t	length;
	bool_t		find_space;
	vm_offset_t phys_start, phys_end;
{
	vm_size_t size, npg;

	ovl_object_reference(overlay_object);
	ovl_map_find(overlay_map, overlay_object, offset, addr, length, find_space);

	npg = atop(phys_end - phys_start);
	size = (vm_size_t) (sizeof(struct pv_entry) * npg + npg);

	hat->h_table = (struct pv_entry *)omem_alloc(overlay_map, size);
	hat_attach(hat, overlay_map, overlay_object);
}

void
pmap_init(vm_offset_t phys_start, vm_offset_t phys_end)
{
    vm_offset_t addr;

    vm_hat_init(&vm_hat, addr, &addr, 2 * NBPG, FALSE, phys_start, phys_end);
    ovl_hat_init(&ovl_hat, addr, &addr, 2 * NBPG, FALSE, phys_start, phys_end);
}
