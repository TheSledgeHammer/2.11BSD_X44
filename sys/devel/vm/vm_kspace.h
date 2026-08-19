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

struct vm_kspace {
	/* idspace */
	vm_idspace_t idspace_i; /* instruction idspace for kspace */
	vm_idspace_t idspace_d; /* data idspace for kspace */
#define kisa_space idspace_i->aspace /* kisa space */
#define kisd_space idspace_i->dspace /* kisd space */
#define kdsa_space idspace_d->aspace /* kdsa space */
#define kdsd_space idspace_d->dspace /* kdsd space */

};

/* virtual kernel I & D space */
extern char *kispace_min, *kispace_max; /* kernel i-space vm_map range */
extern char *kdspace_min, *kdspace_max; /* kernel d-space vm_map range */

extern vm_kspace_t kernel_kspace;	/* kernel kspace */
extern vm_object_t kspace_object; 	/* single kspace object */

/* Should be placed in vm_kern.h and be external */
extern vm_map_t    kisd_map; /* kernel I-Space descriptor map */
extern vm_map_t    kisa_map; /* kernel I-Space address map */
extern vm_map_t    kdsd_map; /* kernel D-Space descriptor map */
extern vm_map_t    kdsa_map; /* kernel D-Space address map */

/* map min offset */
#define KISA_MIN	vm_map_min(kisa_map)
#define KISD_MIN	vm_map_min(kisd_map)
#define KDSA_MIN	vm_map_min(kdsa_map)
#define KDSD_MIN	vm_map_min(kdsd_map)

/* map max offset */
#define KISA_MAX	vm_map_max(kisa_map)
#define KISD_MAX	vm_map_max(kisd_map)
#define KDSA_MAX	vm_map_max(kdsa_map)
#define KDSD_MAX	vm_map_max(kdsd_map)

void vm_kspace_init(void);
vm_kspace_t vm_kspace_allocate(vm_size_t);
void vm_kspace_deallocate(vm_kspace_t);
int vm_kspace_map_alloc(vm_kspace_t, int, int);
int vm_kspace_map_free(vm_kspace_t, int, int);
int vm_kspace_save(vm_kspace_t, vm_offset_t, vm_size_t, int, int);
int vm_kspace_restore(vm_kspace_t, vm_offset_t, vm_size_t, int, int);

vm_offset_t *vm_kspace_offset(vm_kspace_t, vm_offset_t, int);
vm_offset_t *vm_kspace_min(vm_kspace_t, int);
vm_offset_t *vm_kspace_max(vm_kspace_t, int);

/* macros for 2.11BSD like save and restore maps */
#ifdef NONSEPERATE
#define savemap(kspace, flags) \
	(void)vm_kspace_save((kspace), KISA, (flags)); \
	(void)vm_kspace_save((kspace), KISD, (flags));

#define restoremap(kspace, flags) \
	(void)vm_kspace_restore((kspace), KISA, (flags)); \
	(void)vm_kspace_restore((kspace), KISD, (flags));

#else

#define savemap(kspace, flags) \
	(void)vm_kspace_save((kspace), KDSA, (flags)); \
	(void)vm_kspace_save((kspace), KDSD, (flags));

#define restoremap(kspace, flags) \
	(void)vm_kspace_restore((kspace), KDSA, (flags)); \
	(void)vm_kspace_restore((kspace), KDSD, (flags));
#endif

/* Kspace macro's */
#define KSPACE_KISA(kspace, addr)				vm_kspace_offset(kspace, addr, KISA)
#define KSPACE_KISA_MIN(kspace)					vm_kspace_min(kspace, KISA)
#define KSPACE_KISA_MAX(kspace)					vm_kspace_max(kspace, KISA)

#define KSPACE_KISD(kspace, addr)				vm_kspace_offset(kspace, addr, KISD)
#define KSPACE_KISD_MIN(kspace)					vm_kspace_min(kspace, KISD)
#define KSPACE_KISD_MAX(kspace)					vm_kspace_max(kspace, KISD)

#define KSPACE_KDSA(kspace, addr)				vm_kspace_offset(kspace, addr, KDSA)
#define KSPACE_KDSA_MIN(kspace)					vm_kspace_min(kspace, KDSA)
#define KSPACE_KDSA_MAX(kspace)					vm_kspace_max(kspace, KDSA)

#define KSPACE_KDSD(kspace, addr)				vm_kspace_offset(kspace, addr, KDSD)
#define KSPACE_KDSD_MIN(kspace)					vm_kspace_min(kspace, KDSD)
#define KSPACE_KDSD_MAX(kspace)					vm_kspace_max(kspace, KDSD)

#endif /* _VM_KSPACE_H_ */
