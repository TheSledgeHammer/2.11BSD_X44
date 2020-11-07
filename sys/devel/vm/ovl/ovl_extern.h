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

struct ovlspace;

//#ifdef KERNEL
void			ovl_mem_init(void);
vm_offset_t		ovlmem_alloc(ovl_map_t, vm_size_t);
void			ovlmem_free(ovl_map_t, vm_offset_t, vm_size_t);
void			ovlmem_init(vm_offset_t , vm_offset_t );
ovl_map_t		ovlmem_suballoc(ovl_map_t, vm_offset_t, vm_offset_t, vm_size_t);
vm_offset_t		ovlmem_malloc(ovl_map_t, vm_size_t, boolean_t);
vm_offset_t		ovlmem_alloc_wait(ovl_map_t, vm_size_t);
void			ovlmem_free_wakeup(ovl_map_t, vm_offset_t, vm_size_t);
int 			ovl_allocate(ovl_map_t, vm_offset_t, vm_size_t, boolean_t);
int 			ovl_deallocate(ovl_map_t, vm_offset_t, vm_size_t);
int				ovl_allocate_with_overlayer(ovl_map_t, vm_offset_t *, vm_size_t, boolean_t, ovl_overlay_t, boolean_t);
struct ovlspace *ovlspace_alloc(vm_offset_t, vm_offset_t);
struct ovlspace *ovlspace_free(struct ovlspace *);
//#endif
