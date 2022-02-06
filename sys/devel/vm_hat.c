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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/tree.h>

#include <devel/vm_hat.h>

struct hatspl hat_splay = SPLAY_INITIALIZER(hat_splay);

int
vm_hat_compare(hat1, hat2)
	struct vm_hat *hat1, *hat2;
{
	if (hat1->vh_type < hat2->vh_type) {
		return (-1);
	} else if (hat1->vh_type > hat2->vh_type) {
		return (1);
	}
	return (0);
}

SPLAY_PROTOTYPE(hatspl, vm_hat, vh_node, vm_hat_compare);
SPLAY_GENERATE(hatspl, vm_hat, vh_node, vm_hat_compare);

void
vm_hat_bootstrap(hat)
	vm_hat_t hat;
{
	hat = (vm_hat_t)pmap_bootstrap(sizeof(vm_hat_t));
	vm_hat_lock_init(hat);
}

vm_hat_t
vm_hat_alloc(hat, name, type, item)
	vm_hat_t 		hat;
	char 			*name;
	int 			type;
	void 			*item;
{
	if(hat != NULL) {
		vm_hat_add(hat, name, type, item, 0);
		return (hat);
	}
	return (NULL);
}

void
vm_hat_free(hat, name, type)
	vm_hat_t 		hat;
	char 			*name;
	int 			type;
{
	if(hat != NULL) {
		vm_hat_remove(hat, name, type, 0);
	}
}

/* add multiple items to the splay tree */
void
vm_hat_add(hat, name, type, item, nitems)
	vm_hat_t hat;
	char 	*name;
	int 	type;
	void 	*item;
	int 	nitems;
{
	int i;

	hat->vh_name = name;
	hat->vh_type = type;
	hat->vh_item = item;

	vm_hat_lock(hat);
	for(i = 0; i <= nitems; i++) {
		SPLAY_INSERT(hatspl, hat, item);
		hat->vh_nitems++;
		vm_hat_unlock(hat);
	}
}

/* remove multiple items from the splay tree */
void
vm_hat_remove(hat, name, type, nitems)
	vm_hat_t hat;
	char *name;
	int type, nitems;
{
	int i;

	vm_hat_lock(hat);
	for(i = 0; i <= nitems; i++) {
		if(hat->vh_name == name && hat->vh_type == type) {
			hat->vh_item = NULL;
			SPLAY_REMOVE(hatspl, hat, hat->vh_item);
			hat->vh_nitems--;
			vm_hat_unlock(hat);
		}
	}
}

void *
vm_hat_lookup(name, type)
	char *name;
	int type;
{
	vm_hat_t hat;
	vm_hat_lock(hat);
	SPLAY_FOREACH(hat, hatspl, hat_splay) {
		if(hat->vh_name == name && hat->vh_type == type) {
			vm_hat_unlock(hat);
			return (SPLAY_FIND(hatspl, hat, hat->vh_item));
		}
	}
	vm_hat_unlock(hat);
	return (NULL);
}
