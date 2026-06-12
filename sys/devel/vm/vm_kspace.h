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

#ifndef _VM_KSPACE_H_
#define _VM_KSPACE_H_

struct vm_kspace;
typedef struct vm_kspace *vm_kspace_t;

/* virtual kernel I & D space */
extern char *kispace_min, *kispace_max; /* kernel i-space vm_map range */
extern char *kdspace_min, *kdspace_max; /* kernel d-space vm_map range */

struct vm_kspace {
	/* vm */
	vm_map_t desc_map;	/* descriptor map (i.e kdsd_map and kisd_map) */
	vm_map_t addr_map; 	/* address map (i.e kdsa_map and kisa_map) */
	vm_offset_t i_start; /* i-space start address */
	vm_offset_t i_end;	/* i-space end address */
	vm_offset_t d_start; /* d-space start address */
	vm_offset_t d_end;	/* d-space end address */
	vm_object_t object; /* object */
	vm_offset_t offset; /* object offset */

	vm_idspace_t idspace; /* idspace for kspace */
	vm_offset_t addr;	/* segment register address */
	vm_offset_t desc;	/* segment register descriptor */
};

extern vm_object_t kspace_object; /* single kspace object */

/* Should be placed in vm_kern.h and be external */
extern vm_map_t    kisd_map; /* kernel I-Space descriptor map */
extern vm_map_t    kdsd_map; /* kernel D-Space descriptor map */
extern vm_map_t    kisa_map; /* kernel I-Space address map */
extern vm_map_t    kdsa_map; /* kernel D-Space address map */

#endif /* _VM_KSPACE_H_ */
