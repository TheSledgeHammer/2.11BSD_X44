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

#ifndef _VM_USPACE_H_
#define _VM_USPACE_H_

struct vm_uspace;
typedef struct vm_uspace *vm_uspace_t;

/* virtual user I & D space */
extern char *uispace_min, *uispace_max; /* user i-space vm_map range */
extern char *udspace_min, *udspace_max; /* user d-space vm_map range */

struct vm_uspace {
	/* vm */
	vm_map_t desc_map;	/* descriptor vm_map (i.e udsd_map and uisd_map) */
	vm_map_t addr_map; 	/* address vm_map (i.e udsa_map and uisa_map) */
	vm_object_t object;	/* object */
	vm_offset_t offset; /* object offset */

	vm_idspace_t idspace;	/* idspace for uspace */
};

extern vm_object_t uspace_object; /* single uspace object */

/* Should be placed in vm_kern.h and be external */
extern vm_map_t    uisd_map; /* user I-Space descriptor map */
extern vm_map_t    udsd_map; /* user D-Space descriptor map */
extern vm_map_t    uisa_map; /* user I-Space address map */
extern vm_map_t    udsa_map; /* user D-Space address map */

#endif /* _VM_USPACE_H_ */
