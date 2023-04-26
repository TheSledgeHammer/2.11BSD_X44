/*
 * The 3-Clause BSD License:
 * Copyright (c) 2023 Martin Kelly
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

#ifndef _PMAP_VM_HAT_H_
#define _PMAP_VM_HAT_H_

#include <sys/queue.h>

typedef void *pmap_hat_map_t;
typedef void *pmap_hat_object_t;

struct pmap_hat_list;
LIST_HEAD(pmap_hat_list, pmap_hat);
struct pmap_hat {
	LIST_ENTRY(pmap_hat)		ph_next;
    pv_entry_t					ph_pvtable;
    pmap_hat_map_t				ph_map;
    pmap_hat_object_t			ph_object;
    int 						ph_flags;
};
typedef struct pmap_hat     	*pmap_hat_t;
typedef struct pmap_hat_list	*pmap_hat_list_t;

/* hat flags */
#define PMAP_HAT_VM				0x01	/* vmspace */
#define PMAP_HAT_OVL			0x02	/* ovlspace */

#ifdef _KERNEL
extern struct pmap_hat_list		ovlhat_list;
extern struct pmap_hat_list 	vmhat_list;

__BEGIN_DECLS
void 		pmap_hat_attach(pmap_hat_list_t, pmap_hat_t, pmap_hat_map_t, pmap_hat_object_t, int);
void		pmap_hat_detach(pmap_hat_list_t, pmap_hat_map_t, pmap_hat_object_t, int);
pmap_hat_t	pmap_hat_find(pmap_hat_list_t, pmap_hat_map_t, pmap_hat_object_t, int);
vm_offset_t	pmap_hat_pa_index(vm_offset_t, vm_offset_t);
pv_entry_t 	pmap_hat_to_pvh(pmap_hat_list_t, pmap_hat_map_t, pmap_hat_object_t, vm_offset_t, vm_offset_t, int);

/* VM HAT */
void		vm_hat_init(pmap_hat_list_t, vm_offset_t, vm_offset_t);
vm_offset_t vm_hat_pa_index(vm_offset_t);
pv_entry_t	vm_hat_to_pvh(pmap_hat_list_t, vm_offset_t);

/* OVL HAT */
void		ovl_hat_init(pmap_hat_list_t, vm_offset_t, vm_offset_t);
vm_offset_t ovl_hat_pa_index(vm_offset_t);
pv_entry_t	ovl_hat_to_pvh(pmap_hat_list_t, vm_offset_t);
__END_DECLS
#endif
#endif /* _PMAP_VM_HAT_H_ */
