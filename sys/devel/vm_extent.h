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

#ifndef _VM_EXTENT_H_
#define _VM_EXTENT_H_

struct vm_extent {
	LIST_ENTRY(vm_extent) 	ve_exnode;		/* extent entry */
	struct extent 			*ve_extent;
	u_long 					ve_size;		/* extent region */
	u_long 					ve_subsize;		/* extent subregion */
	u_long 					*ve_result;
	struct lock_object		*ve_lock;
};
typedef struct vm_extent	*vm_extent_t;

#define vm_extent_lock_init(ex) (simple_lock_init((ex)->ve_lock, "vm_extent_lock"))
#define vm_extent_lock(ex) 		(simple_lock((ex)->ve_lock))
#define vm_extent_unlock(ex) 	(simple_unlock((ex)->ve_lock))

void		vm_exbootinit(vm_extent_t, char *, u_long, u_long, int, caddr_t, size_t, int);
vm_extent_t	vm_exinit(char *, u_long, u_long, int, caddr_t, size_t, int);
void		vm_exalloc_region(vm_extent_t, u_long, u_long, flags);
void		vm_exalloc_subregion(vm_extent_t, u_long, u_long, u_long, int, u_long);
void		vm_exfree(vm_extent_t, u_long, u_long, int);
void		vm_exdestroy(vm_extent_t);

#endif /* _VM_EXTENT_H_ */
