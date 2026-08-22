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

struct vm_uspace {
	/* idspace */
	vm_idspace_t idspace_i; /* instruction idspace for uspace */
	vm_idspace_t idspace_d; /* data idspace for uspace */
#define uisa_space idspace_i->aspace /* uisa space */
#define uisd_space idspace_i->dspace /* uisd space */
#define udsa_space idspace_d->aspace /* udsa space */
#define udsd_space idspace_d->dspace /* udsd space */
};

/* virtual user I & D space */
extern char *uispace_min, *uispace_max; /* user i-space vm_map range */
extern char *udspace_min, *udspace_max; /* user d-space vm_map range */

extern vm_uspace_t kernel_uspace;	/* kernel uspace */
extern vm_object_t uspace_object; 	/* single uspace object */

/* Should be placed in vm_kern.h and be external */
extern vm_map_t    uisd_map; /* user I-Space descriptor map */
extern vm_map_t    uisa_map; /* user I-Space address map */
extern vm_map_t    udsd_map; /* user D-Space descriptor map */
extern vm_map_t    udsa_map; /* user D-Space address map */

void vm_uspace_init(void);
int vm_uspace_read(vm_size_t, int, int, bool_t, bool_t);
int vm_uspace_write(vm_size_t, int, int, bool_t, bool_t);
vm_offset_t *vm_uspace_offset(vm_offset_t, int);
vm_offset_t *vm_uspace_min(int);
vm_offset_t *vm_uspace_max(int);

/* Uspace macro's */
#ifdef NONSEPARATE

/* UISA */
#define UISA_READ(size, segno, is_txt, is_ext) \
	vm_uspace_read(size, segno, UISA, is_txt, is_ext)

#define UISA_WRITE(size, segno, is_txt, is_ext) \
	vm_uspace_write(size, segno, UISA, is_txt, is_ext)

#define UISA_OFFSET(addr) \
	vm_uspace_offset(addr, UISA)

#define UISA_MIN \
	vm_uspace_min(UISA)

#define UISA_MAX \
	vm_uspace_max(UISA)

/* UISD */
#define UISD_READ(size, segno, is_txt, is_ext) \
	vm_uspace_read(size, segno, UISD, is_txt, is_ext)

#define UISD_WRITE(size, segno, is_txt, is_ext)	\
	vm_uspace_write(size, segno, UISD, is_txt, is_ext)

#define UISD_OFFSET(addr) \
	vm_uspace_offset(addr, UISD)

#define UISD_MIN \
	vm_uspace_min(UISD)

#define UISD_MAX \
	vm_uspace_max(UISD)

#else /* !NONSEPARATE */

/* UDSA */
#define UDSA_READ(size, segno, is_txt, is_ext) \
	vm_uspace_read(size, segno, UDSA, is_txt, is_ext)

#define UDSA_WRITE(size, segno, is_txt, is_ext)	\
	vm_uspace_write(size, segno, UDSA, is_txt, is_ext)

#define UDSA_OFFSET(addr) \
	vm_uspace_offset(addr, UDSA)

#define UDSA_MIN \
	vm_uspace_min(UDSA)

#define UDSA_MAX \
	vm_uspace_max(UDSA)

/* UDSD */
#define UDSD_READ(size, segno, is_txt, is_ext) \
	vm_uspace_read(size, segno, UDSD, is_txt, is_ext)

#define UDSD_WRITE(size, segno, is_txt, is_ext)	\
	vm_uspace_write(size, segno, UDSD, is_txt, is_ext)

#define UDSD_OFFSET(addr) \
	vm_uspace_offset(addr, UDSD)

#define UDSD_MIN \
	vm_uspace_min(UDSD)

#define UDSD_MAX \
	vm_uspace_max(UDSD)

#endif /* !NONSEPARATE */
#endif /* _VM_USPACE_H_ */
