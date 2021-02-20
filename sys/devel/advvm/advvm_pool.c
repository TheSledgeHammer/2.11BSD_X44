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
 *	@(#)advvm_pool.c 1.0 	20/02/21
 */

/*
 * AdvVM Pool Memory Allocation. Using an extent allocator.
 */

#include <sys/extent.h>
#include <sys/tree.h>
#include <sys/user.h>
#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_domain.h>

static u_long *advvm_pool;		/* resulting pool from pool_allocate_subregion (should not be 0) */

struct advvm_pool {
	struct extent 				*adp_extent;
	u_long						adp_start;
	u_long						adp_end;
	caddr_t 					adp_storage;
	size_t						adp_storagesize;
	int 						adp_flags;
	SPLAY_HEAD(,advvm_pool) 	adp_root;
};

struct advvm_region {
	SPLAY_ENTRY(advvm_region) 	adr_entry;
};

#define ADVVM_FIXED 			0x01

boolean_t
advvm_extent_check(pool)
	struct advvm_pool *pool;
{
	if(pool->adp_extent) {
		return (TRUE);
	}
	panic("advvm_allocate_pool: no extent created");
	return (FALSE);
}

struct advvm_pool *
advvm_create(name, start, end, mtype, storage, storagesize, flags)
	char *name;
	u_long start, end;
	int mtype;
	caddr_t storage;
	size_t storagesize;
	int flags;
{
	struct advvm_pool *pool;

	if (pool == NULL) {
		pool = (struct advvm_pool*) malloc(pool, sizeof(struct advvm_pool*), M_ADVVM, M_WAITOK);
		memset(pool, sizeof(struct advvm_pool*));
	} else {
		SPLAY_INIT(pool->adp_root);
		pool->adp_start = start;
		pool->adp_end = end;
		if(flags == ADVVM_FIXED) {
			pool->adp_storage = storage;
			pool->adp_storagesize = storagesize;
			pool->adp_flags = flags | ADVVM_FIXED;
		}
		pool->adp_storage = NULL;
		pool->adp_storagesize = NULL;
		pool->adp_flags = flags;
	}

	pool->adp_extent = extent_create(name, pool->adp_start, pool->adp_end, M_ADVVM, pool->adp_storage, pool->adp_storagesize, flags);

	return (pool);
}

int
advvm_allocate_region(pool, start, size, flags)
	struct advvm_pool 	*pool;
	u_long 				start, size;
	int 				flags;
{
	int error;

	if(advvm_extent_check(pool)) {
		if(start >= pool->adp_start && start <= pool->adp_end) {
			if(size <= (pool->adp_end - pool->adp_start)) {
				error = extent_alloc_region(pool->adp_extent, start, size, flags);
			} else {
				panic("advvm_allocate_pool: extent region size too big");
			}
		} else {
			error = extent_alloc_region(pool->adp_extent, pool->adp_start, size, flags);
		}
	}

	if(error != 0) {
		return (error);
	}
	return (0);
}

int
advvm_allocate_subregion(pool, size, alignment, boundary, flags)
	struct advvm_pool 	*pool;
	u_long 				size, alignment, boundary;
	int 				flags;
{
	int error;

	if(advvm_extent_check(pool)) {
		error = extent_alloc(pool->adp_extent, size, alignment, boundary, flags, &advvm_pool);
	}

	if(error != 0) {
		&advvm_pool = 0;
		return (error);
	}
	return (0);
}
