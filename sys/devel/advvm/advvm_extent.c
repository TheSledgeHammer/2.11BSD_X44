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
 *	@(#)advvm_extent.c 1.0 	20/02/21
 */

/*
 * AdvVM Extent Memory Allocation. Using an extent allocator.
 */

#include <sys/user.h>

#include <devel/sys/malloctypes.h>
#include <devel/advvm/advvm_extent.h>
#include <devel/advvm/advvm_var.h>
#include <devel/advvm/advvm_domain.h>

void
advvm_storage_create(pool, name, start, end, storage, storagesize, flags)
	advvm_storage_t *pool;
	char *name;
	u_long start, end;
	caddr_t storage;
	size_t storagesize;
	int flags;
{
	advvm_malloc((advvm_storage_t *) pool, sizeof(advvm_storage_t *));
	pool->adp_start = start;
	pool->adp_end = end;
	pool->adp_flags = flags;
	pool->adp_storage = storage;
	pool->adp_storagesize = storagesize;

	pool->adp_extent = extent_create(name, pool->adp_start, pool->adp_end, M_ADVVM, pool->adp_storage, pool->adp_storagesize, pool->adp_flags);
}

int
advvm_allocate_region(pool, start, size, flags)
	advvm_storage_t		*pool;
	u_long 				start, size;
	int 				flags;
{
	int error;

	if(advvm_extent_check(pool)) {
		if(start >= pool->adp_start && start <= pool->adp_end) {
			if(size <= (pool->adp_end - pool->adp_start)) {
				error = extent_alloc_region(pool->adp_extent, start, size, flags);
			} else {
				panic("advvm_allocate_region: extent region size too big");
			}
		}
	} else {
		return (ADVVM_NOEXTENT);
	}
	return (error);
}

int
advvm_allocate_subregion(pool, size, alignment, boundary, flags)
	advvm_storage_t 	*pool;
	u_long 				size, alignment, boundary;
	int 				flags;
{
	int error;

	if(advvm_extent_check(pool)) {
		error = extent_alloc(pool->adp_extent, size, alignment, boundary, flags, pool->adp_pool);
	} else {
		return (ADVVM_NOEXTENT);
	}
	return (error);
}

/* free's advvm extent only */
int
advvm_free(pool, start, size, flags)
	advvm_storage_t *pool;
	u_long 			start, size;
	int 			flags;
{
	int error;
	if(advvm_extent_check(pool)) {
		error = extent_free(pool->adp_extent, start, size, flags);
	} else {
		return (ADVVM_NOEXTENT);
	}
	return (error);
}

/* destroys advvm extent and free's advvm storage */
void
advvm_destroy(pool)
	advvm_storage_t *pool;
{
	if(advvm_extent_check(pool)) {
		extent_destroy(pool->adp_extent);
		advvm_free((advvm_storage_t *) pool);
	} else {
		printf("advvm_destroy: no extent to destroy");
	}
}

u_long *
advvm_get_storage_pool(pool)
	advvm_storage_t *pool;
{
	return (pool->adp_pool);
}

/* internal use only */
boolean_t
advvm_extent_check(pool)
	advvm_storage_t *pool;
{
	if(pool->adp_extent) {
		return (TRUE);
	}
	return (FALSE);
}

/* a generic malloc for advvm with kern_malloc */
void
advvm_malloc(addr, size)
	void 	*addr;
	u_long 	size;
{
	addr = malloc(addr, size, M_ADVVM, M_WAITOK);
}

void
advvm_free(addr)
	void 	*addr;
{
	free(addr, M_ADVVM);
}
