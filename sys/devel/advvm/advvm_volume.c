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
#include <sys/null.h>
#include <sys/tree.h>
#include <lib/libkern/libkern.h>

#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_domain.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_extent.h>
#include <devel/advvm/advvm_volume.h>

static long fixed_volume_extent[EXTENT_FIXED_STORAGE_SIZE(128)/sizeof(long)];

struct advdomain_list 	domain_list[MAXDOMAIN];

void
advvm_volume_init(advol, name, id)
	advvm_volume_t *advol;
	char *name;
	uint32_t id;
{
	advol->vol_name = advvm_strcat(name, "_volume");
	advol->vol_id = id;
	advol->vol_domain = NULL;
	advol->vol_flags = 0;
	advol->vol_domain_allocated = 0;
	advol->vol_domain_used = 0;
}

void
advvm_volume_set_domain(adom, advol)
	advvm_domain_t *adom;
	advvm_volume_t *advol;
{
	advol->vol_domain = adom;
	advol->vol_domain_name = adom->dom_name;
	advol->vol_domain_id = adom->dom_id;
}

#ifdef notyet
void
advvm_volume_set_label(label, sysname, name, creation, update, dsize)
	struct advvm_label 	*label;
	char 				*sysname, *name;
	struct timeval 		creation, update;
	off_t 				dsize;
{
	if (label == NULL) {
		advvm_malloc((struct advvm_label*) label, sizeof(struct advvm_label*), M_WAITOK);
	}
	label->alb_sysname = sysname;
	label->alb_name = name;
	label->alb_creation_date = creation;
	label->alb_last_update = update;
	label->alb_drive_size = dsize;
}

struct advvm_block *
advvm_volume_create_block(storage, start, end, size, addr, flags)
	advvm_storage_t *storage;
	uint64_t 			start, end;
	uint32_t 			size;
	caddr_t 			addr;
	int 				flags;
{
	struct advvm_block 	*block;

	advvm_malloc((struct advvm_block*) block, sizeof(struct advvm_block*), M_WAITOK);
	block->ablk_start = start;
	block->ablk_end = end;
	block->ablk_size = size;
	block->ablk_addr = addr;
	block->ablk_flags = flags;
	advvm_storage_create(storage, start, end, size, addr, flags);
	return (block);
}
#endif

void
advvm_volume_create(adom, advol, start, end)
	advvm_domain_t *adom;
	advvm_volume_t 	*advol;
	uint64_t 		start, end;
{
	advvm_volume_set_domain(adom, advol);
	/* Fixed Regions Currently Disabled */
	advvm_volume_init_storage(advol, start, end, NULL, 0, EX_WAITOK | EX_MALLOCOK);
}

advvm_volume_t *
advvm_volume_find(adom, name, id)
	advvm_domain_t 	*adom;
	char 			*name;
	uint32_t 		id;
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
advvm_volume_insert(adom, advol)
	advvm_domain_t *adom;
	advvm_volume_t 	*advol;
{
	struct advdomain_list 	*bucket;
	
	if (adom == NULL || advol == NULL) {
		return;
	}
	
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
	advvm_volume_t 			*advol;
	
	bucket = &domain_list[advvm_hash(adom)];
	TAILQ_FOREACH(advol, bucket, vol_entries) {
		if (advol->vol_domain == adom) {
			if (advol->vol_name == name && advol->vol_id == id) {
				TAILQ_REMOVE(bucket, advol, vol_entries);
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
		advvm_extent_destroy(advol->vol_storage);
	}
	advvm_free((advvm_volume_t *)advol);
}

/*
 * Volume Storage Allocator
 */
void
advvm_volume_init_storage(advol, start, end, storage, storagesize, flags)
	advvm_volume_t 		*advol;
	uint64_t 			start, end, storagesize;
	caddr_t 			storage;
	int 				flags;
{
	advvm_storage_t *volstore;

	volstore = advvm_extent_create(advol->vol_name, start, end, storage, storagesize, flags);
	if (volstore != NULL) {
		advol->vol_extent = volstore;
		advol->vol_start = start;
		advol->vol_end = end;
		advol->vol_size = (end - start);
		advol->vol_storage = storage;
		advol->vol_storagesize = storagesize;
	} else {
		advvm_extent_destroy(volstore);
	}
}

int
advvm_volume_allocate_region(advol, start, size, flags)
	advvm_volume_t 		*advol;
	uint64_t 			start, size;
	int 				flags;
{
	int error;
	if (size <= advol->vol_size) {
		error = advvm_extent_allocate_region(advol->vol_extent, start, size, flags);
	} else {
		size = advol->vol_size;
		error = advvm_extent_allocate_region(advol->vol_extent, start, size, flags);
	}
	if (error != 0) {
		error = advvm_extent_free(advol->vol_extent, start, size, flags);
		return (error);
	}
	advvm_volume_insert(advol->vol_domain, advol);
	return (0);
}

int
advvm_volume_allocate_subregion(advol, substart, subend, size, alignment, boundary, flags)
	advvm_volume_t 		*advol;
	uint64_t 			substart, subend, size, alignment, boundary;
	int 				flags;
{
	int error;

	if ((substart == advol->vol_start) && (subend == advol->vol_end)) {
		if (size <= advol->vol_size) {
			error = advvm_extent_allocate_subregion(advol->vol_extent, substart, subend, size, alignment, boundary, flags);
		} else {
			size = advol->vol_size;
			error = advvm_extent_allocate_subregion(advol->vol_extent, substart, subend, size, alignment, boundary, flags);
		}
	} else {
		error = advvm_extent_allocate_subregion(advol->vol_extent, substart, subend, size, alignment, boundary, flags);
	}
	if (error != 0) {
		error = advvm_extent_free(advol->vol_extent, start, size, flags);
		return (error);
	}

	//advol->vol_alignment = alignment;
	//advol->vol_boundary = boundary;
	return (0);
}
