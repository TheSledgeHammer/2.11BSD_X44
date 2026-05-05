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
#include <vm_idspace.h>

/* vm_segment_map */
#define M_VMSEGMAP 103

struct vm_segmap_head segmaplist;

/* seginfo */
struct vm_segment_register seginfo[NOVL];
static void vm_seginfo_get(int, vm_offset_t *, vm_offset_t *);
static void vm_seginfo_put(int, vm_offset_t *, vm_offset_t *);

/* savemap */
struct vm_segment_register savemap[2];
static void vm_savemap_get(int, vm_offset_t *, vm_offset_t *);
static void vm_savemap_put(int, vm_offset_t *, vm_offset_t *);

#define segmap_lock()
#define segmap_unlock()

void
vm_segment_map_init(void)
{
	LIST_INIT(&segmaplist);
	//lock_init
}

void
vm_segment_map_insert(segreg, segnum, flags, attr)
	vm_segment_register_t segreg;
	int segnum, flags;
	short attr;
{
	vm_segment_map_t segmap;

	segmap = (vm_segment_map_t)malloc((u_long)sizeof(struct vm_segment_map), M_VMSEGMAP, M_WAITOK);
	segmap->segreg = segreg;
	segmap->segnum = segnum;
	segmap->flags = flags;
	segmap->attr = attr;
	//lock
	LIST_INSERT_HEAD(&segmaplist, segmap, segmlist);
	//unlock
}

vm_segment_map_t
vm_segment_map_lookup(segreg, segnum)
	vm_segment_register_t segreg;
	int segnum;
{
	vm_segment_map_t segmap;

	//lock
	LIST_FOREACH(segmap, &segmaplist, segmlist) {
		if ((segmap->segreg == segreg) && (segmap->segnum == segnum)) {
			//unlock
			return (segmap);
		}
	}
	//unlock
	return (NULL);
}

void
vm_segment_map_remove(segreg, segnum)
	vm_segment_register_t segreg;
	int segnum;
{
	vm_segment_map_t segmap;

	//lock
	LIST_FOREACH(segmap, &segmaplist, segmlist) {
		if ((segmap->segreg == segreg) && (segmap->segnum == segnum)) {
			LIST_REMOVE(segmap, segmlist);
			//unlock
		}
	}
	//unlock
}

/* vm_segment_register: seginfo */

static void
vm_seginfo_get(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (seginfo[segnum] != NULL) {
		if ((segnum >= 0) && (segnum <= NOVL)) {
			*addr = seginfo[segnum].addr;
			*desc = seginfo[segnum].desc;
		}
	}
}

static void
vm_seginfo_put(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if ((addr != NULL) && (desc != NULL)) {
		if ((segnum >= 0) && (segnum <= NOVL)) {
			seginfo[segnum].addr = *addr;
			seginfo[segnum].desc = *desc;
		}
	}
}

/* vm_segment_register savemap */
static void
vm_savemap_get(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if (&savemap[segnum] != NULL) {
		if ((segnum >= 0) && (segnum <= 1)) {
			*addr = savemap[segnum].addr;
			*desc = savemap[segnum].desc;
		}
	}
}

static void
vm_savemap_put(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if ((addr != NULL) && (desc != NULL)) {
		if ((segnum >= 0) && (segnum <= 1)) {
			savemap[segnum].addr = *addr;
			savemap[segnum].desc = *desc;
		}
	}
}

/* save registers to SEG5 and SEG6 */
void
vm_segment_map_save(map, flags)
	vm_segment_map_t map;
	int flags;
{
	switch (flags) {
	case SEGM_SEG5:
		vm_savemap_put(0, &map->mapstore.kdsa5, &map->mapstore.kdsd5);
		break;
	case SEGM_SEG6:
		vm_savemap_put(1, &map->mapstore.kdsa6, &map->mapstore.kdsd6);
		break;
	case SEGM_SEG56:
		vm_savemap_put(0, &map->mapstore.kdsa5, &map->mapstore.kdsd5);
		vm_savemap_put(1, &map->mapstore.kdsa6, &map->mapstore.kdsd6);
		break;
	}
}

/* restore registers from SEG5 and SEG6 */
void
vm_segment_map_restore(map, flags)
	vm_segment_map_t map;
	int flags;
{
	switch (flags) {
	case SEGM_SEG5:
		vm_savemap_get(0, &map->mapstore.kdsa5, &map->mapstore.kdsd5);
		break;
	case SEGM_SEG6:
		vm_savemap_get(1, &map->mapstore.kdsa6, &map->mapstore.kdsd6);
		break;
	case SEGM_SEG56:
		vm_savemap_get(0, &map->mapstore.kdsa5, &map->mapstore.kdsd5);
		vm_savemap_get(1, &map->mapstore.kdsa6, &map->mapstore.kdsd6);
		break;
	}
}
