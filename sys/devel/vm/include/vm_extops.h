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

#ifndef SYS_DEVEL_VM_INCLUDE_VM_EXTOPS_H_
#define SYS_DEVEL_VM_INCLUDE_VM_EXTOPS_H_

struct vm_extops {
	struct extent *(* extops_create)(void *);
	int (* extops_mallocok)(void *);
	int (* extops_alloc)(void *);
	int (* extops_suballoc)(void *);
	int (* extops_free)(void *);
	int (* extops_destroy)(void *);
};

struct vm_extops_generic_args {
	struct vm_extops				*a_ops;
};

struct vm_extops_create_args {
	struct vm_extops_generic_args	a_head;
	struct extent					*a_ext;
	char							*a_name;
	vm_offset_t 					a_start;
	vm_offset_t						a_end;
	int 							a_mtype;
	caddr_t							a_storage;
	vm_size_t 						a_storagesize;
	int								a_flags;
};

struct vm_extops_mallocok_args {
	struct vm_extops_generic_args	a_head;
	int								a_mallocok;
};

struct vm_extops_alloc_args {
	struct vm_extops_generic_args	a_head;
	struct extent					*a_ext;
	vm_offset_t 					a_start;
	vm_offset_t             		a_addr;
	vm_size_t 						a_size;
	int 							a_flags;
};

struct vm_extops_suballoc_args {
	struct vm_extops_generic_args	a_head;
	struct extent					*a_ext;
	vm_offset_t 					a_start;
	vm_offset_t						a_end;
	vm_size_t 						a_size;
	int 							a_malloctypes;
	int 							a_mallocflags;
	u_long							a_alignment;
	u_long							a_boundary;
	int 							a_flags;
	u_long 							*a_result;
};

struct vm_extops_free_args {
	struct vm_extops_generic_args	a_head;
	struct extent					*a_ext;
	vm_offset_t 					a_start;
	vm_size_t 						a_size;
	int 							a_malloctypes;
	int								a_flags;
};

struct vm_extops_destroy_args	{
	struct vm_extops_generic_args	a_head;
	struct extent					*a_ext;
};

struct extent *extops_create(struct vm_extops *, struct extent *, char *, vm_offset_t, vm_offset_t, int, caddr_t, vm_size_t, int);
int extops_mallocok(struct vm_extops *, int);
int extops_alloc(struct vm_extops *, struct extent *, vm_offset_t, vm_size_t, int);
int extops_suballoc(struct vm_extops *, struct extent *, vm_offset_t, vm_offset_t, vm_size_t, int, int, int, u_long, u_long, u_long);
int extops_free(struct vm_extops *, struct extent *, vm_offset_t, vm_size_t, int , int);
int extops_destroy(struct vm_extops *, struct extent *);

/* Alternative to Above: Easier to Work with
 * - add operations that interface with extentloader? extops_create(union vm_extentloader *)
 */
union vm_extentloader {
	struct extent					vmel_ext;
	struct vm_extent_create			vmel_create;
	struct vm_extent_mallocok		vmel_mallocok;
	struct vm_extent_alloc			vmel_alloc;
	struct vm_extent_suballoc		vmel_suballoc;
	struct vm_extent_free			vmel_free;
};

struct vm_extent_create {
	char							*vmel_name;
	vm_offset_t 					vmel_start;
	vm_offset_t						vmel_end;
	int 							vmel_mtype;
	caddr_t							vmel_storage;
	vm_size_t 						vmel_storagesize;
	int								vmel_flags;
	caddr_t							vmel_storage;
	vm_size_t 						vmel_storagesize;
};

struct vm_extent_mallocok {
	int								vmel_mallocok;
};

struct vm_extent_alloc {
	vm_offset_t 					a_start;
	vm_offset_t             		a_addr;
	vm_size_t 						a_size;
	int 							a_flags;
};

struct vm_extent_suballoc {
	vm_offset_t 					a_start;
	vm_offset_t						a_end;
	vm_size_t 						a_size;
	int 							a_malloctypes;
	int 							a_mallocflags;
	u_long							a_alignment;
	u_long							a_boundary;
	int 							a_flags;
	u_long 							*a_result;
};

struct vm_extent_free {
	vm_offset_t 					a_start;
	vm_size_t 						a_size;
	int 							a_malloctypes;
	int								a_flags;
};

#endif /* _VM_EXTOPS_H_ */
