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

/* Virtual I & D Space */

/* Work In Progress! */

#ifndef _VM_IDSPACE_H_
#define _VM_IDSPACE_H_

#include <vm/include/vm.h>

struct vm_segment_register;
typedef struct vm_segment_register *vm_segment_register_t;

struct vm_segment_map;
typedef struct vm_segment_map *vm_segment_map_t;

struct vm_kspace;
typedef struct vm_kspace *vm_kspace_t;

struct vm_uspace;
typedef struct vm_uspace *vm_uspace_t;

/* virtual segment registers */
struct vm_segment_register {
	vm_offset_t addr;	/* register address */
	vm_offset_t desc;	/* register descriptor */
};

#define NOVL 32

/* virtual segmentation map */
struct vm_segmap_head;
LIST_HEAD(vm_segmap_head, vm_segment_map);
struct vm_segment_map {
	LIST_ENTRY(vm_segment_map) segmlist; 	/* register list */
	vm_segment_register_t segm[NOVL]; 		/* segment registers */
	int segnum; 							/* segment register number */
	int flags;
};
extern struct vm_segmap_head segmapinfo;

/* virtual kernel I & D space */
char *kispace_min, *kispace_max; /* kernel i-space vm_map range */
char *kdspace_min, *kdspace_max; /* kernel d-space vm_map range */

struct vm_kspace {
	/* vm */
	vm_map_t desc_map;	/* descriptor map (i.e kdsd_map and kisd_map) */
	vm_map_t addr_map; 	/* address map (i.e kdsa_map and kisa_map) */
	vm_object_t object; /* object */
	vm_offset_t offset; /* object offset */

	/* segmentation */
	vm_segment_map_t segmap; /* segment map */
};

extern vm_object_t kspace_object; /* single kspace object */

/* virtual user I & D space */
char *uispace_min, *uispace_max; /* user i-space vm_map range */
char *udspace_min, *udspace_max; /* user d-space vm_map range */

struct vm_uspace {
	/* vm */
	vm_map_t desc_map;	/* descriptor vm_map (i.e udsd_map and uisd_map) */
	vm_map_t addr_map; 	/* address vm_map (i.e udsa_map and uisa_map) */
	vm_object_t object;	/* object */
	vm_offset_t offset; /* object offset */

	/* segmentation */
	vm_segment_map_t segmap; /* segment map */
};

extern vm_object_t uspace_object; /* single uspace object */

/* Should be placed in vm_kern.h and be external */
extern vm_map_t    kisd_map; /* kernel I-Space descriptor map */
extern vm_map_t    kdsd_map; /* kernel D-Space descriptor map */
extern vm_map_t    kisa_map; /* kernel I-Space address map */
extern vm_map_t    kdsa_map; /* kernel D-Space address map */

extern vm_map_t    uisd_map; /* user I-Space descriptor map */
extern vm_map_t    udsd_map; /* user D-Space descriptor map */
extern vm_map_t    uisa_map; /* user I-Space address map */
extern vm_map_t    udsa_map; /* user D-Space address map */

#endif /* _VM_IDSPACE_H_ */
