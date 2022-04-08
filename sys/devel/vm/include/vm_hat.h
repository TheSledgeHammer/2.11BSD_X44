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

#ifndef _VM_HAT_H_
#define _VM_HAT_H_

/* pmap to vm/ovl address translation */

struct hatspl;
SPLAY_HEAD(hatspl, vm_hat);
struct vm_hat {
	SPLAY_ENTRY(vm_hat)		vh_node;
	char 					*vh_name;
	int 					vh_type;
	void					*vh_item;
	u_long 					vh_size;
	struct lock_object		vh_lock;
};

#define HAT_VM 	0x01
#define HAT_OVL 0x02

#define vm_hat_lock_init(hat) 	(simple_lock_init((hat)->vh_lock, "vm_hat_lock"))
#define vm_hat_lock(hat) 		(simple_lock((hat)->vh_lock))
#define vm_hat_unlock(hat) 		(simple_unlock((hat)->vh_lock))

/* vm_hat */
void 		*vm_halloc(vm_hat_t);
void		vm_hfree(vm_hat_t);
void		vm_hbootinit(vm_hat_t, char *, int, void *, int, u_long);
vm_offset_t	vm_hat_bootstrap_alloc(int, vm_size_t);
void		vm_hat_bootstrap(vm_hat_t);
void 		*vm_hat_lookup(char *, int);
void		vm_hat_add(vm_hat_t, char *, int, void *, u_long);
void		vm_hat_remove(char *, int);
#endif /* _VM_HAT_H_ */
