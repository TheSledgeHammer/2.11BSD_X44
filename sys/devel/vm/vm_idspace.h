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

/* Code is based on 2.11BSD's PDP-11 code */

/* Work In Progress! */

/*
 * vm_maps:
 * Additions: address space layout
 * USPACE_MIN: USRSTACK or USRTEXT
 * USPACE_MAX: VM_MAXUSER_ADDRESS
 * KSPACE_MIN: VM_MIN_KERNEL_ADDRESS
 * KSPACE_MAX: VM_MAX_KERNEL_ADDRESS
 *
 * Changes:
 * - perform checks on min and max range in vm_map_init and vm_map_create
 *      - initialize kspace and uspace structures
 *      - kspace vm_maps should be within above kspace min and max if successful
 *      - uspace vm_maps should be within above uspace min and max if successful
 */

/*
 * segment_register:
 * 2.11BSD on the PDP-11: Serves the base kernel and user mapping
 * - saves and restores to 2 segments (5 and 6: i.e KDSA5, KDSD5, KDSD6, KDSA6)
 * 		- with each set at a predefined address.
 * - each segment has access bits and flags
 * - when saving: KDSA6 is checked, stored in variable kdsa6 and the protection bits applied
 *
 * 2.11BSD_X44:
 * Current:
 * Can Perform:
 * - save and restore any number (up to NOVL) of segments to seginfo[NOVL]
 * - can set address and descriptors
 *
 * Cannot Perform:
 * - segment protection bits (will be determined from vmspace)
 * - prevention from a non-empty segment being overwritten
 * - no reserved segments like 5 and 6, all are available...!
 *
 * TODO:
 * - defining an address:
 * 		- statically: setting a base address for each segment from above vm_map??
 * 		- dynamically: let the vm_map define the base address for each segment??
 * - protect non-empty segments
 * - provide reserved segments
 */

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

extern struct vm_segment_register seginfo[NOVL];

/* virtual segmentation map */
struct vm_segmap_head;
LIST_HEAD(vm_segmap_head, vm_segment_map);
struct vm_segment_map {
	LIST_ENTRY(vm_segment_map) segmlist; 			/* register list */
	vm_segment_register_t segment_register[NOVL]; 	/* virtual segment registers */
	int segment_number; 							/* virtual segment register number */
	int flags;										/* virtual segment register flags */
	short attributes;								/* virtual segment register attributes */

	union { /* segment mapping store */
		vm_offset_t kdsa5;	/* virtual SEG5 address */
		vm_offset_t kdsd5;	/* virtual SEG5 descriptor */
		vm_offset_t kdsa6;	/* virtual SEG6 address */
		vm_offset_t kdsd6;	/* virtual SEG6 descriptor */
	} mapstore;
};
extern struct vm_segmap_head segmaplist;

/* segment map attributes */
#define SEGM_RO			0x002	/* Read only */
#define SEGM_RW			0x006	/* Read and write */
#define SEGM_NOACCESS 	0x000	/* Abort all accesses */
#define SEGM_ACCESS		0x007	/* Mask for access field */
#define SEGM_ED			0x010	/* Extension direction */
#define SEGM_TX			0x020	/* Software: text segment */
#define SEGM_ABS		0x040	/* Software: absolute address */

#define SEGM_SAVE		0x60	/* Software: save virtual segment register's to savemap */
#define SEGM_RESTORE	0x80	/* Software: restore virtual segment register's from savemap */

#define SEGM_SEG5		0x100	/* map SEG5 only */
#define SEGM_SEG6		0x120	/* map SEG6 only */
#define SEGM_SEG56		(SEGM_SEG5|SEGM_SEG6) /* map both SEG5 and SEG6 */

/* virtual kernel I & D space */
extern char *kispace_min, *kispace_max; /* kernel i-space vm_map range */
extern char *kdspace_min, *kdspace_max; /* kernel d-space vm_map range */

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
extern char *uispace_min, *uispace_max; /* user i-space vm_map range */
extern char *udspace_min, *udspace_max; /* user d-space vm_map range */

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
