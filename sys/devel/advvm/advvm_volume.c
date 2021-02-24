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
 *
 *	@(#)advvm_volume.c 1.0 	20/02/21
 */

#include <sys/extent.h>
#include <sys/tree.h>
#include <lib/libkern/libkern.h>

#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_volume.h>

struct advdomain_list 	domain_list[MAXDOMAIN];

void
advvm_volume_init(advol)
	advvm_volume_t *advol;
{
	advol->vol_name = NULL;
	advol->vol_id = 0;
	advol->vol_block = NULL;
	advol->vol_label = NULL;
	advol->vol_domain = NULL;
	advol->vol_domain_allocated = 0;
	advol->vol_domain_used = 0;
}

void
advvm_volume_set_label(label, sysname, name, creation, update, dsize)
	struct advvm_label 	*label;
	char 			*sysname, *name;
	struct timeval 		creation, update;
	off_t 			dsize;
{
	if (label == NULL) {
		advvm_malloc((struct advvm_label*) label, sizeof(struct advvm_label*));
	}
	label->alb_sysname = sysname;
	label->alb_name = name;
	label->alb_creation_date = creation;
	label->alb_last_update = update;
	label->alb_drive_size = dsize;
}

void
advvm_volume_set_block(block, start, end, size, addr, flags)
	struct advvm_block 	*block;
	uint64_t 		start, end;
	uint32_t 		size;
	caddr_t 		addr;
	int 			flags;
{
	if (block == NULL) {
		advvm_malloc((struct advvm_block*) block, sizeof(struct advvm_block*));
	}
	block->ablk_start = start;
	block->ablk_end = end;
	block->ablk_size = size;
	block->ablk_addr = addr;
	block->ablk_flags = flags;
}

void
advvm_volume_set_domain(advol, adom)
	advvm_volume_t *advol;
	advvm_domain_t *adom;
{
	advol->vol_domain = adom;
	advol->vol_domain_name = adom->dom_name;
	advol->vol_domain_id = adom->dom_id;
}

void
advvm_volume_create(advol, block, name, id, flags)
	advvm_volume_t 		*advol;
	struct advvm_block 	*block;
	char 			*name;
	uint32_t 		id;
	int 			flags;
{
	advvm_malloc((advvm_volume_t *)advol, sizeof(advvm_volume_t *));
	advol->vol_name = name;
	advol->vol_id = id;				/* generate a random uuid */
	advol->vol_flags = flags;

	advvm_storage_create(advol->vol_storage, block->ablk_start, block->ablk_end, block->ablk_size, block->ablk_addr, block->ablk_flags);
}

advvm_volume_t *
advvm_volume_find(adom, name, id)
	advvm_domain_t 	*adom;
	char 		*name;
	uint32_t 	id;
{
	struct advdomain_list 	*bucket;
	advvm_volume_t 		*advol;
	
	bucket = &domain_list[advvm_hash(adom)];
	TAILQ_FOREACH(advol, bucket, vol_entries) {
		if (advol->vol_domain == adom) {
			if (advol->vol_name == name && advol->vol_id == id) {
				return (advol);
			}
		}
	}
	return (NULL);
}

void
advvm_volume_insert(adom, advol, name, id, flags)
	advvm_domain_t *adom;
	advvm_volume_t 	*advol;
	char 		*name;
	uint32_t 	id;
	int 		flags;
{
	struct advdomain_list 	*bucket;
	
	if(adom == NULL) {
		return;
	}
	if(advol == NULL) {
		return;
	}

	if(advol->vol_block == NULL) {
		panic("advvm_volume_insert: no volume block set");
		return;
	} else if(advol->vol_label == NULL) {
		panic("advvm_volume_insert: no volume label set");
		return;
	} else {
		advvm_volume_create(advol, advol->vol_block, name, id, flags);
	}
	advvm_volume_set_domain(advol, adfst);
	
	bucket = &domain_list[advvm_hash(adom)];

	TAILQ_INSERT_HEAD(bucket, advol, vol_entries);
	advol->vol_domain_used++;
}

void
advvm_volume_remove(adom, name, id)
	advvm_domain_t *adom;
	char 		*name;
	uint32_t 	id;
{
	struct advdomain_list 	*bucket;
	advvm_volume_t 		*advol;
	
	bucket = &domain_list[advvm_hash(adom)];
	TAILQ_FOREACH(advol, bucket, vol_entries) {
		if (advol->vol_domain == adom) {
			if (advol->vol_name == name && advol->vol_id == id) {
				TAILQ_REMOVE(bucket, advol, dom_volumes);
				advol->vol_domain_used--;
			}
		}
	}
}

/* destroy and free an AdvVM domain */
void
advvm_volume_destroy(advol)
	advvm_volume_t *advol;
{
	if(advol->vol_storage) {
		advvm_storage_delete(advol->vol_storage);
	}
	advvm_free((advvm_volume_t *)advol);
}
