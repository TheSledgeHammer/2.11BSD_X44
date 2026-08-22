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

void vm_kspace_init(void);
int vm_kspace_saveseg5(vm_offset_t, vm_size_t, int);
int vm_kspace_restoreseg5(vm_offset_t, vm_size_t, int);
int vm_kspace_saveseg6(vm_offset_t, vm_size_t, int);
int vm_kspace_restoreseg6(vm_offset_t, vm_size_t, int);
int vm_kspace_saveseg56(vm_offset_t, vm_size_t, int);
int vm_kspace_restoreseg56(vm_offset_t, vm_size_t, int);
vm_offset_t *vm_kspace_offset(vm_offset_t, int);
vm_offset_t *vm_kspace_min(int);
vm_offset_t *vm_kspace_max(int);

/* Kspace macro's */
#ifdef NONSEPARATE

/* KISA */
#define KISA_SAVESEG5(addr, size) \
	vm_kspace_save(addr, size, KISA, SEGM_SEG5)

#define KISA_SAVESEG6(addr, size) \
	vm_kspace_save(addr, size, KISA, SEGM_SEG6)

#define KISA_SAVESEG56(addr, size) \
	vm_kspace_save(addr, size, KISA, SEGM_SEG56)

#define KISA_RESTORESEG5(addr, size) \
	vm_kspace_restore(addr, size, KISA, SEGM_SEG5)

#define KISA_RESTORESEG6(addr, size) \
	vm_kspace_restore(addr, size, KISA, SEGM_SEG6)

#define KISA_RESTORESEG56(addr, size) \
	vm_kspace_restore(addr, size, KISA, SEGM_SEG56)

#define KISA_OFFSET(addr) \
	vm_kspace_offset(addr, KISA)

#define KISA_MIN \
	vm_kspace_min(KISA)

#define KISA_MAX \
	vm_kspace_max(KISA)

/* KISD */
#define KISD_SAVESEG5(addr, size) \
	vm_kspace_save(addr, size, KISD, SEGM_SEG5)

#define KISD_SAVESEG6(addr, size) \
	vm_kspace_save(addr, size, KISD, SEGM_SEG6)

#define KISD_SAVESEG56(addr, size) \
	vm_kspace_save(addr, size, KISD, SEGM_SEG56)

#define KISD_RESTORESEG5(addr, size) \
	vm_kspace_restore(addr, size, KISD, SEGM_SEG5)

#define KISD_RESTORESEG6(addr, size) \
	vm_kspace_restore(addr, size, KISD, SEGM_SEG6)

#define KISD_RESTORESEG56(addr, size) \
	vm_kspace_restore(addr, size, KISD, SEGM_SEG56)

#define KISD_OFFSET(addr) \
	vm_kspace_offset(addr, KISD)

#define KISD_MIN \
	vm_kspace_min(KISD)

#define KISD_MAX \
	vm_kspace_max(KISD)

#else /* !NONSEPARATE */

/* KDSA */
#define KDSA_SAVESEG5(addr, size) \
	vm_kspace_save(addr, size, KDSA, SEGM_SEG5)

#define KDSA_SAVESEG6(addr, size) \
	vm_kspace_save(addr, size, KDSA, SEGM_SEG6)

#define KDSA_SAVESEG56(addr, size) \
	vm_kspace_save(addr, size, KDSA, SEGM_SEG56)

#define KDSA_RESTORESEG5(addr, size) \
	vm_kspace_restore(addr, size, KDSA, SEGM_SEG5)

#define KDSA_RESTORESEG6(addr, size) \
	vm_kspace_restore(addr, size, KDSA, SEGM_SEG6)

#define KDSA_RESTORESEG56(addr, size) \
	vm_kspace_restore(addr, size, KDSA, SEGM_SEG56)

#define KDSA_OFFSET(addr) \
	vm_kspace_offset(addr, KDSA)

#define KDSA_MIN \
	vm_kspace_min(KDSA)

#define KDSA_MAX \
	vm_kspace_max(KDSA)

/* KDSD */
#define KDSD_SAVESEG5(addr, size) \
	vm_kspace_save(addr, size, KDSD, SEGM_SEG5)

#define KDSD_SAVESEG6(addr, size) \
	vm_kspace_save(addr, size, KDSD, SEGM_SEG6)

#define KDSD_SAVESEG56(addr, size) \
	vm_kspace_save(addr, size, KDSD, SEGM_SEG56)

#define KDSD_RESTORESEG5(addr, size) \
	vm_kspace_restore(addr, size, KDSD, SEGM_SEG5)

#define KDSD_RESTORESEG6(addr, size) \
	vm_kspace_restore(addr, size, KDSD, SEGM_SEG6)

#define KDSD_RESTORESEG56(addr, size) \
	vm_kspace_restore(addr, size, KDSD, SEGM_SEG56)

#define KDSD_OFFSET(addr) \
	vm_kspace_offset(addr, KDSD)

#define KDSD_MIN \
	vm_kspace_min(KDSD)

#define KDSD_MAX \
	vm_kspace_max(KDSD)

#endif /* !NONSEPARATE */
#endif /* _VM_KSPACE_H_ */
