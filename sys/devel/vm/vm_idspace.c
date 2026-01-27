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

#include <vm_idspace.h>

/* vm_segment_map */
#define M_VMSEGMAP 103

struct vm_segmap_head segmaplist;

#define segmap_lock()
#define segmap_unlock()

void
vm_segment_map_init(void)
{
	LIST_INIT(&segmaplist);
	//lock_init
}

void
vm_segment_map_insert(segment_register, segment_number)
	vm_segment_register_t segment_register;
	int segment_number;
{
	vm_segment_map_t segm;

	segm = (vm_segment_map_t)malloc((u_long)sizeof(struct vm_segment_map), M_VMSEGMAP, M_WAITOK);
	segm->segment_register = segment_register;
	segm->segment_number = segment_number;
	//segm->attributes;
	//segm->flags = flags;
	//lock
	LIST_INSERT_HEAD(&segmaplist, segm, segmlist);
	//unlock
}

vm_segment_map_t
vm_segment_map_lookup(segment_register, segment_number)
	vm_segment_register_t segment_register;
	int segment_number;
{
	vm_segment_map_t segm;

	//lock
	LIST_FOREACH(segm, &segmaplist, segmlist) {
		if ((segm->segment_register == segment_register) && (segm->segment_number == segment_number)) {
			//unlock
			return (segm);
		}
	}
	//unlock
	return (NULL);
}

void
vm_segment_map_remove(segment_register, segment_number)
	vm_segment_register_t segment_register;
	int segment_number;
{
	vm_segment_map_t segm;

	segm = vm_segment_map_lookup(segment_register, segment_number);
	if (segm != NULL) {
		LIST_REMOVE(segm, segmlist);
	}
}

/* vm_segment_register */

struct vm_segment_register seginfo[NOVL];

/* vm_segment_register savemaps */
struct vm_segment_register savemap[2];

static void
savemap_put(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if ((segnum < 0) || (segnum > 1)) {
		panic("savemap_put: segnum is outside the valid range. Range is between 0 and 1\n");
		return;
	}
	if (&savemap[segnum] != NULL) {
		*addr = savemap[segnum].addr;
		*desc = savemap[segnum].desc;
	}
}

static void
savemap_get(segnum, addr, desc)
	int segnum;
	vm_offset_t *addr, *desc;
{
	if ((segnum < 0) || (segnum > 1)) {
		panic("savemap_get: segnum is outside the valid range. Range is between 0 and 1\n");
		return;
	}
    if ((addr != NULL) && (desc != NULL)) {
    	savemap[segnum].addr = *addr;
        savemap[segnum].desc = *desc;
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
		savemap_put(0, &map->mapstore.kdsa5, &map->mapstore.kdsd5);
		break;
	case SEGM_SEG6:
		savemap_put(1, &map->mapstore.kdsa6, &map->mapstore.kdsd6);
		break;
	case SEGM_SEG56:
		savemap_put(0, &map->mapstore.kdsa5, &map->mapstore.kdsd5);
		savemap_put(1, &map->mapstore.kdsa6, &map->mapstore.kdsd6);
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
		savemap_get(0, &map->mapstore.kdsa5, &map->mapstore.kdsd5);
		break;
	case SEGM_SEG6:
		savemap_get(1, &map->mapstore.kdsa6, &map->mapstore.kdsd6);
		break;
	case SEGM_SEG56:
		savemap_get(0, &map->mapstore.kdsa5, &map->mapstore.kdsd5);
		savemap_get(1, &map->mapstore.kdsa6, &map->mapstore.kdsd6);
		break;
	}
}

void
vm_segment_register_put(segment_number, addr, desc)
	int segment_number;
	vm_offset_t addr, desc;
{
	seginfo[segment_number].addr = addr;
	seginfo[segment_number].addr = desc;
}

vm_segment_register_t
vm_segment_register_get(segment_number)
	int segment_number;
{
	vm_segment_register_t segm;

	segm = &seginfo[segment_number];
	if (segm != NULL) {
		return (segm);
	}
	return (NULL);
}
