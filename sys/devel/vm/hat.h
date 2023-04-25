/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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

#ifndef _HAT_H_
#define _HAT_H_

typedef void *hat_object_t;
typedef void *hat_map_t;

struct hat_list;
LIST_HEAD(hat_list, hat);
struct hat {
	LIST_ENTRY(hat)		h_next;
    pmap_t              h_pmap;
    pv_entry_t			h_table;
    hat_map_t			h_map;
    hat_object_t		h_object;

    struct hat_ops		*h_op;
};
typedef struct hat      *hat_t;
typedef struct hat_list	*hat_list_t;

struct hat_ops {
    void        		(*hop_bootstrap)(vm_offset_t);
    void        		*(*hop_bootstrap_alloc)(u_long);
    void        		(*hop_init)(vm_offset_t, vm_offset_t);
    vm_offset_t 		(*hop_map)(vm_offset_t, vm_offset_t, vm_offset_t, int);
    void	   			(*hop_enter)(pmap_t, vm_offset_t, vm_offset_t, vm_prot_t, bool_t);
    void        		(*hop_remove)(pmap_t, vm_offset_t, vm_offset_t);
};

extern struct hat_list	ovlhat_list;
extern struct hat_list 	vmhat_list;

void 		hat_attach(hat_list_t, hat_t, hat_map_t, hat_object_t);
void		hat_detach(hat_list_t, hat_map_t, hat_object_t);
hat_t		hat_find(hat_list_t, hat_map_t, hat_object_t);

/* hat pv entries */
vm_offset_t	hat_pa_index(vm_offset_t, vm_offset_t);
pv_entry_t 	hat_to_pvh(hat_list_t, hat_map_t, hat_object_t, vm_offset_t, vm_offset_t);
void		hat_pv_enter(pmap_t, hat_list_t, hat_map_t, hat_object_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_offset_t);
void		hat_pv_remove(pmap_t, hat_list_t, hat_map_t, hat_object_t, vm_offset_t, vm_offset_t, vm_offset_t, vm_offset_t);

#endif /* _VM_HAT_H_ */
