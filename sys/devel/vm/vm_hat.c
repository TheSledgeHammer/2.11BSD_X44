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
 * Issues:
 * - pmap_bootstrap_alloc can't be used post vm_page_startup/ vm_segment_startup
 * - this means kmap & kentry allocation in vm_map_startup is not possible.
 *
 * - One solution is to initialize them in vm_page_startup.
 * 	 - The concern is would this cause problems elsewhere? (where kmap & kentry are used)
 *
 * - Second solution is to have them in two stages, similar to the original mach vm for 4.4BSD
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/null.h>

#include <devel/vm/include/vm_hat.h>

struct hatspl hat_splay = SPLAY_INITIALIZER(hat_splay);

void *
vm_halloc(h)
	vm_hat_t h;
{
	void *item;

	if (h == NULL) {
		panic("hat: invalid");
	}
	item = vm_hat_lookup(h->vh_name, h->vh_type);
	h->vh_item = item;

	return (item);
}

void
vm_hfree(h)
	vm_hat_t h;
{
	void *item;
	item = vm_hat_lookup(h->vh_name, h->vh_type);
	if(item != NULL) {
		vm_hat_remove(h->vh_name, h->vh_type);
	}
}

vm_offset_t
vm_hat_bootstrap_alloc(type, nentries, size)
	int type, nentries;
	u_long size;
{
	vm_offset_t		data;
	vm_size_t		data_size, totsize;

	data_size = (nentries * size);
	totsize = round_page(data_size);

	switch(type) {
	case HAT_VM:
		data = (vm_offset_t)pmap_bootstrap_alloc(totsize);
		break;
	case HAT_OVL:
		data = (vm_offset_t)pmap_bootstrap_overlay_alloc(totsize);
		break;
	}
	return (data);
}

vm_offset_t
vm_hat_bootstrap(type, nentries, size)
	int type, nentries;
	u_long size;
{
	vm_offset_t	data;

	data = vm_hat_bootstrap_alloc(type, nentries, size);

	return (data);
}

void
vm_hinit(name, type, item, nentries, size)
	char *name;
	int type, nentries;
	void *item;
	u_long size;
{
	vm_hat_t h;

	h = (vm_hat_t)malloc(sizeof(h), M_VMHAT, M_WAITOK);

	vm_hat_lock_init(h);
	item = (vm_offset_t)vm_hat_bootstrap(type, nentries, size);
	if (item != NULL) {
		vm_hat_add(h, name, type, item, size);
	}
}

/*
 *	Pre-allocate maps and map entries that cannot be dynamically
 *	allocated via malloc().  The maps include the kernel_map and
 *	kmem_map which must be initialized before malloc() will
 *	work (obviously).  Also could include pager maps which would
 *	be allocated before kmeminit.
 *
 *	Allow some kernel map entries... this should be plenty
 *	since people shouldn't be cluttering up the kernel
 *	map (they should use their own maps).
 */
void
vm_hbootinit(h, name, type, item, nentries, size)
	vm_hat_t h;
	char *name;
	int type, nentries;
	void *item;
	u_long size;
{
	item = (vm_offset_t)vm_hat_bootstrap(type, nentries, size);

	vm_hat_lock_init(h);
	if (item != NULL) {
		vm_hat_add(h, name, type, item, size);
	}
}

int
vm_hat_compare(hat1, hat2)
	struct vm_hat *hat1, *hat2;
{
	if ((hat1->vh_type < hat2->vh_type) && (hat1->vh_size < hat2->vh_size)) {
		return (-1);
	} else if ((hat1->vh_type > hat2->vh_type) && (hat1->vh_size > hat2->vh_size)) {
		return (1);
	}
	return (0);
}

SPLAY_PROTOTYPE(hatspl, vm_hat, vh_node, vm_hat_compare);
SPLAY_GENERATE(hatspl, vm_hat, vh_node, vm_hat_compare);

/* add items to the splay tree */
void
vm_hat_add(hat, name, type, item, size)
	vm_hat_t hat;
	char 	*name;
	int 	type;
	void 	*item;
	u_long 	size;
{
	hat->vh_name = name;
	hat->vh_type = type;
	hat->vh_item = item;
	hat->vh_size = size;

	vm_hat_lock(hat);
	SPLAY_INSERT(hatspl, &hat_splay, item);
	vm_hat_unlock(hat);
}

/* remove items from the splay tree */
void
vm_hat_remove(name, type)
	char *name;
	int type;
{
	vm_hat_t hat;
	void 	*item;

	vm_hat_lock(hat);
	SPLAY_FOREACH(hat, hatspl, hat_splay) {
		item = hat->vh_item;
		if(hat->vh_name == name && hat->vh_type == type) {
			SPLAY_REMOVE(hatspl, &hat_splay, item);
			vm_hat_unlock(hat);
		}
	}
}

void *
vm_hat_lookup(name, type)
	char *name;
	int type;
{
	vm_hat_t hat;
	void 	*item;

	vm_hat_lock(hat);
	SPLAY_FOREACH(hat, hatspl, hat_splay) {
		item = hat->vh_item;
		if (hat->vh_name == name && hat->vh_type == type) {
			vm_hat_unlock(hat);
			return (SPLAY_FIND(hatspl, hat, item));
		}
	}
	vm_hat_unlock(hat);
	return (NULL);
}
