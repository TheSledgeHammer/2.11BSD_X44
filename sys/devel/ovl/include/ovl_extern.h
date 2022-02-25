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

struct buf;
struct proc;
struct ovlspace;
struct vmtotal;
struct mount;
struct vnode;

#ifdef _KERNEL
void			ovl_mem_init(void);
vm_offset_t		omem_alloc(ovl_map_t, vm_size_t);
void			omem_free(ovl_map_t, vm_offset_t, vm_size_t);
void			omem_init(vm_offset_t , vm_offset_t );
ovl_map_t		omem_suballoc(ovl_map_t, vm_offset_t, vm_offset_t, vm_size_t);
vm_offset_t		omem_malloc(ovl_map_t, vm_size_t, bool_t);
vm_offset_t		omem_alloc_wait(ovl_map_t, vm_size_t);
void			omem_free_wakeup(ovl_map_t, vm_offset_t, vm_size_t);
int 			ovl_allocate(ovl_map_t, vm_offset_t, vm_size_t, bool_t);
int 			ovl_deallocate(ovl_map_t, vm_offset_t, vm_size_t);
int				ovl_allocate_with_pager(ovl_map_t, vm_offset_t *, vm_size_t, bool_t, vm_pager_t, bool_t);
struct ovlspace *ovlspace_alloc(vm_offset_t, vm_offset_t, bool_t);
struct ovlspace *ovlspace_free(struct ovlspace *);
void			ovl_object_enter_vm_object (ovl_object_t, vm_object_t);
vm_object_t		ovl_object_lookup_vm_object (ovl_object_t, vm_object_t);
void			ovl_object_remove_vm_object (ovl_object_t, vm_object_t);
void			ovl_segment_insert_vm_segment(ovl_segment_t, vm_segment_t);
vm_segment_t	ovl_segment_lookup_vm_segment(ovl_segment_t);
void			ovl_segment_remove_vm_segment(vm_segment_t);
void			ovl_page_insert_vm_page(ovl_page_t, vm_page_t);
vm_page_t		ovl_page_lookup_vm_page(ovl_page_t);
void			ovl_page_remove_vm_page(vm_page_t);
#endif
