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

#define NOVL 			16								/* Number of Overlays */
#define NOVL_SIZE		(SEGMENT_SIZE / NOVL) 			/* size of novl */
#define NOVL_MASK		(NOVL_SIZE - 1)					/* size of novl - 1 */
#define NOVL_SHIFT		18
#define NOVL_PAGES 		((SEGMENT_SIZE/PAGE_SIZE)/NOVL) /* Number of Pages per NOVL (64) */
#define NOVL_SEGMENTS 	1								/* Number of Segments per NOVL */

#define	trunc_novl(x)    ((vm_offset_t)(((vm_offset_t)(x)) & ~NOVL_MASK))
#define	round_novl(x)    ((vm_offset_t)((((vm_offset_t)(x)) + NOVL_MASK) & ~NOVL_MASK))
#define num_novl(x)      ((vm_offset_t)((((vm_offset_t)(x)) + NOVL_MASK) >> NOVL_SHIFT))

/*
 * returns offset from segnum
 */
static inline vm_offset_t
segnum_to_offset(int x)
{
	 vm_offset_t offset;

	 if (x == 0) {
		 offset = (vm_offset_t)(x + NOVL_SIZE);
	 } else {
		 offset = (vm_offset_t)(x * NOVL_SIZE);
	 }
	 return (offset);
}

/*
 * returns a single segment offset from segnum
 */
static inline vm_offset_t
segnum_to_segment_offset(int x)
{
	vm_offset_t offset;
	int i;

	for (i = 0; i < NOVL + 1; i++) {
		if (i == x) {
			offset = segnum_to_offset(i);
			offset = (offset / NOVL_SEGMENTS);
			return (offset);
		}
	}
	return (0);
}

/*
 * returns a single page offset from segnum
 */
static inline vm_offset_t
segnum_to_page_offset(int x)
{
	vm_offset_t offset;

	offset = segnum_to_offset(x);
	offset = (offset / NOVL_PAGES);
	return (offset);
}

struct vm_segment_register;
typedef struct vm_segment_register *vm_segment_register_t;

struct vm_segment_region;
typedef struct vm_segment_region *vm_segment_region_t;

struct vm_idspace_map;
typedef struct vm_idspace_map *vm_idspace_map_t;

struct vm_idspace;
typedef struct vm_idspace *vm_idspace_t;

/* segment registers */
struct vm_segment_register {
	vm_offset_t addr;	/* register address */
	vm_offset_t desc;	/* register descriptor */
};

/* segment region */
struct vm_segment_region {
	TAILQ_ENTRY(vm_segment_region) segm; /* region entry */
	vm_segment_t segment;				/* segment in idspace */
	vm_page_t page;						/* page in idspace */
	vm_segment_register_t segreg; 		/* virtual segment register */
	int segnum; 						/* virtual segment register number */
	int flags;							/* virtual segment register flags */
	vm_prot_t protect; 					/* protection codes */
	bool_t is_text; 					/* text segment */
	bool_t is_extension; 				/* extension direction */
	bool_t is_abs;						/* absolute address */
	struct { 							/* segment mapping store */
		vm_offset_t kdsa5;				/* virtual SEG5 address */
		vm_offset_t kdsd5;				/* virtual SEG5 descriptor */
		vm_offset_t kdsa6;				/* virtual SEG6 address */
		vm_offset_t kdsd6;				/* virtual SEG6 descriptor */
	} mapstore;
};

/* idspace map */
struct vm_idspace_map {
	vm_map_t map;		/* map */
	vm_offset_t start;	/* start address */
	vm_offset_t end;	/* end address */
	vm_size_t size;		/* size */
};

/* idspace */
struct vm_segregion_queue;
TAILQ_HEAD(vm_segregion_queue, vm_segment_region);
struct vm_idspace {
	struct vm_segregion_queue header;	/* list of regions */
	vm_segment_region_t region;			/* region back-pointer */
	struct vm_idspace_map aspace;		/* address space (i.e kdsa_map, kisa_map, udsa_map, uisa_map) */
	struct vm_idspace_map dspace;		/* descriptor space (i.e kdsd_map, kisd_map, udsd_map, uisd_map) */
	vm_object_t object;					/* idspace object */
	vm_segment_t segment;				/* idspace segment */
	vm_page_t page;						/* idspace page */
	int mtype;							/* idspace malloctype */
};

/* segment region flags */
#define SEGM_RO			0x002	/* Read only: same as VM_PROT_READ */
#define SEGM_RW			0x006	/* Read and write: same as (VM_PROT_READ|VM_PROT_WRITE) */
#define SEGM_NOACCESS 	0x000	/* Abort all accesses (No/Cancel Execute permissions??) */
#define SEGM_ACCESS		0x007	/* Mask for access field (Execute permissions??) */
#define SEGM_ED			0x010	/* Extension direction (usage ??) */
#define SEGM_TX			0x020	/* Software: text segment */
#define SEGM_ABS		0x040	/* Software: absolute address (usage ??) */
/*
 * absolute address: (unsure of context: is this physical, processor, etc)
 * if address means physical address, then this
 * is not relevant from the vm/ovl
 * context, as both physical and virtual addresses
 * can already be obtained and managed by pmap.
 */
#define SEGM_SEG5		0x60	/* map SEG5 only */
#define SEGM_SEG6		0x80	/* map SEG6 only */
#define SEGM_SEG56		0x100 	/* map both SEG5 and SEG6 */
#define SEGM_SAVE		(0x120 & (SEGM_SEG5|SEGM_SEG6|SEGM_SEG56))	/* Software: save virtual segment register's to savemap */
#define SEGM_RESTORE	(0x140 & (SEGM_SEG5|SEGM_SEG6|SEGM_SEG56))	/* Software: restore virtual segment register's from savemap */

/* vm_idspace */
extern struct vm_segment_register segregs[NOVL];

int vm_idspace_init(vm_idspace_t, vm_idspace_map_t, int, vm_map_t, vm_offset_t *,
		vm_offset_t *, vm_size_t, bool_t);
vm_idspace_t vm_idspace_allocate(vm_object_t, vm_offset_t, int);
void vm_idspace_deallocate(vm_idspace_t, int);

#ifdef deprecated
void vm_idspace_init(vm_idspace_t, vm_object_t, vm_offset_t, int);
vm_map_t vm_idspace_map_allocate(vm_object_t, vm_offset_t, vm_offset_t *, vm_offset_t *, vm_size_t, bool_t);
vm_page_t vm_idspace_pagemap_allocate(vm_segment_t, int);
#endif

/* vm_segment_region */
vm_segment_region_t vm_segment_region_alloc(int);
void vm_segment_region_free(vm_segment_region_t, int);
void vm_segment_region_insert(vm_idspace_t, vm_segment_region_t, int);
void vm_segment_region_remove(vm_idspace_t, int);
vm_segment_region_t vm_segment_region_lookup(vm_idspace_t, int);

/* vm_segment_register */
int vm_segment_register_write(vm_segment_region_t, int, vm_offset_t *, vm_offset_t *);
int vm_segment_register_read(vm_segment_region_t, int, vm_offset_t *, vm_offset_t *);

#endif /* _VM_IDSPACE_H_ */
