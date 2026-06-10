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

/* Code is based on 2.11BSD's PDP-11 code */
#include <sys/malloc.h>

#include <vm/include/vm.h>
#include <vm_idspace.h>

/*
 * idspace layout:
 * - An idspace is a vm_segment sub-divided into 16 regions.
 * - Each region contains 64 pages or is 262144 in size.
 * - Each region can contain a register.
 * - A register is similar to pmap (but different purpose),
 * with an address and descriptor.
 */

/* generic registers */
struct vm_segment_register segregs[NOVL];

static void vm_idspace_setup(vm_idspace_t, vm_object_t, vm_offset_t);
static void vm_genmap_get(int, vm_offset_t *, vm_offset_t *);
static void vm_genmap_put(int, vm_offset_t *, vm_offset_t *);
static void vm_savemap_get(int, vm_offset_t *, vm_offset_t *);
static void vm_savemap_put(int, vm_offset_t *, vm_offset_t *);
static void vm_segment_region_saveseg5(vm_segment_region_t, vm_offset_t *, vm_offset_t *);
static void vm_segment_region_saveseg6(vm_segment_region_t, vm_offset_t *, vm_offset_t *);

simple_lock_data_t vm_idspace_lock;

void
vm_idspace_init(idspace, object, offset, mtype)
	vm_idspace_t idspace;
	vm_object_t object;
	vm_offset_t offset;
	int mtype;
{
	idspace = vm_idspace_allocate(object, offset);
	if (idspace != NULL) {
		/* initialize first segment region */
		vm_region_insert(idspace, 0, mtype);
	}
}

vm_idspace_t
vm_idspace_allocate(object, offset, mtype)
	vm_object_t object;
	vm_offset_t offset;
	int mtype;
{
	register vm_idspace_t result;

	MALLOC(result, struct vm_idspace *, sizeof(struct vm_idspace *), mtype, M_WAITOK);
	vm_idspace_setup(result, object, offset);
	return (result);
}

void
vm_idspace_deallocate(idspace, mtype)
	vm_idspace_t idspace;
	int mtype;
{
	if (idspace != NULL) {
		if (!TAILQ_EMPTY(&idspace->header)) {
			return;
		}
		free(idspace, mtype);
	}
}

static void
vm_idspace_setup(idspace, object, offset)
	vm_idspace_t idspace;
	vm_object_t object;
	vm_offset_t offset;
{
	vm_segment_t segment;
	vm_page_t pagemap[NOVL];
	vm_offset_t pgoffset;
	int i;

	TAILQ_INIT(&idspace->header);
	simple_lock_init(&vm_idspace_lock, "vm_idspace_lock");

	/* setup segment */
	segment = vm_segment_alloc(object, offset);
	idspace->segment = segment;

	/* setup pages */
	for (i = 0; i < NOVL; i++) {
		pgoffset = i * NOVL_PAGES * PAGE_SIZE;
		pagemap[i] = vm_page_alloc(segment, pgoffset);
		idspace->pagemap[i] = pagemap[i];
	}
}

void
vm_segment_region_insert(idspace, segnum, mtype)
	vm_idspace_t idspace;
	int segnum, mtype;
{
	vm_segment_region_t region;
	vm_page_t page;

	if (idspace == NULL) {
		return;
	}
	page = idspace->pagemap[segnum];
	if (page == NULL) {
		return;
	}
	region = (vm_segment_region_t)malloc((u_long)sizeof(struct vm_segment_region), mtype, M_WAITOK);
	region->page = page;
	//region->size = ;
	region->segreg = &segregs[segnum];
	region->segnum = segnum;
	region->flags = 0;
	region->protect = VM_PROT_ALL;
	region->is_text = FALSE;
	region->is_extension = FALSE;
	region->is_abs = FALSE;

	simple_lock(&vm_idspace_lock);
	TAILQ_INSERT_TAIL(&idspace->header, region, segm);
	simple_unlock(&vm_idspace_lock);
}

void
vm_segment_region_remove(idspace, segnum)
	vm_idspace_t idspace;
	int segnum;
{
	vm_segment_region_t region;
	vm_page_t pagemap;

	pagemap = idspace->pagemap[segnum];
	simple_lock(&vm_idspace_lock);
	TAILQ_FOREACH(region, &idspace->header, segm) {
		if ((region->page == pagemap) && (region->segnum == segnum)) {
			TAILQ_REMOVE(&idspace->header, region, segm);
			simple_unlock(&vm_idspace_lock);
		}
	}
}

vm_segment_region_t
vm_segment_region_lookup(idspace, segnum)
	vm_idspace_t idspace;
	int segnum;
{
	vm_segment_region_t region;
	vm_page_t pagemap;

	pagemap = idspace->pagemap[segnum];
	simple_lock(&vm_idspace_lock);
	TAILQ_FOREACH(region, &idspace->header, segm) {
		if ((region->page == pagemap) && (region->segnum == segnum)) {
			simple_unlock(&vm_idspace_lock);
			return (NULL);
		}
	}
	simple_unlock(&vm_idspace_lock);
	return (NULL);
}

/*
 * Write to a segment register.
 * segreg will not be null if successful.
 */
int
vm_segment_register_write(region, segnum, addr, desc)
	vm_segment_region_t region;
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (region->protect & VM_PROT_WRITE) {
		if (region->flags & SEGM_SAVE) {
			switch (region->flags) {
			case SEGM_SEG5:
				vm_segment_region_saveseg5(region, addr, desc);
				vm_segmap_put((NOVL - 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				break;
			case SEGM_SEG6:
				vm_segment_region_saveseg6(region, addr, desc);
				vm_segmap_put(NOVL, &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			case SEGM_SEG56:
				vm_segment_region_saveseg5(region, addr, desc);
				vm_savemap_put((NOVL - 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				vm_segment_region_saveseg6(region, addr, desc);
				vm_savemap_put(NOVL, &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			default:
				panic("vm_segment_register_write: no valid save register specified");
				break;
			}
			goto out;
		}
		vm_genmap_put(segnum, addr, desc);
	}

out:
	if (region->segreg == &segregs[segnum]) {
		if ((region->segreg->addr == addr) && (region->segreg->desc == desc)) {
			return (0);
		}
	}
	return (1);
}

/*
 * Reads from a segment register.
 * segreg will not be null if successful.
 */
int
vm_segment_register_read(region, segnum, addr, desc)
	vm_segment_region_t region;
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (region->protect & VM_PROT_READ) {
		if (region->flags & SEGM_RESTORE) {
			switch (region->flags) {
			case SEGM_SEG5:
				vm_savemap_get((NOVL - 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				break;
			case SEGM_SEG6:
				vm_savemap_get(NOVL, &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			case SEGM_SEG56:
				vm_savemap_get((NOVL - 1), &region->mapstore.kdsa5, &region->mapstore.kdsd5);
				vm_savemap_get(NOVL, &region->mapstore.kdsa6, &region->mapstore.kdsd6);
				break;
			default:
				panic("vm_segment_register_read: no valid save register specified");
				break;
			}
			goto out;
		}
		vm_genmap_get(segnum, addr, desc);
	}

out:
	if (region->segreg == &segregs[segnum]) {
		if ((region->segreg->addr == addr) && (region->segreg->desc == desc)) {
			return (0);
		}
	}
	return (1);
}

/* vm_segment_register: infomap */
static void
vm_genmap_get(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (&segregs[segnum] != NULL) {
		if ((segnum >= 0) && (segnum <= (NOVL - 2))) {
			*addr = segregs[segnum].addr;
			*desc = segregs[segnum].desc;
		}
	}
}

static void
vm_genmap_put(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if ((addr != NULL) && (desc != NULL)) {
		if ((segnum >= 0) && (segnum <= (NOVL - 2))) {
			segregs[segnum].addr = *addr;
			segregs[segnum].desc = *desc;
		}
	}
}

/* vm_segment_register savemap */
static void
vm_savemap_get(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (&segregs[segnum] != NULL) {
		if ((segnum >= (NOVL - 1)) && (segnum <= NOVL)) {
			*addr = segregs[segnum].addr;
			*desc = segregs[segnum].desc;
		}
	}
}

static void
vm_savemap_put(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if ((addr != NULL) && (desc != NULL)) {
		if ((segnum >= (NOVL - 1)) && (segnum <= NOVL)) {
			segregs[segnum].addr = *addr;
			segregs[segnum].desc = *desc;
		}
	}
}

/*
 * vm_segment_region_saveseg5:
 * copy contents of the address and the descriptor to
 * seg5 mapstore.
 */
static void
vm_segment_region_saveseg5(region, addr, desc)
	vm_segment_region_t region;
	vm_offset_t *addr, *desc;
{
	bcopy(addr, &region->mapstore.kdsa5, sizeof(*addr));
	bcopy(desc, &region->mapstore.kdsd5, sizeof(*desc));
}

/*
 * vm_segment_region_saveseg6:
 * copy contents of the address and the descriptor to
 * seg6 mapstore.
 */
static void
vm_segment_region_saveseg6(region, addr, desc)
	vm_segment_region_t region;
	vm_offset_t *addr, *desc;
{
	bcopy(addr, &region->mapstore.kdsa6, sizeof(*addr));
	bcopy(desc, &region->mapstore.kdsd6, sizeof(*desc));
}
