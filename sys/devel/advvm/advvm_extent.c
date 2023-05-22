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
#include <sys/malloc.h>
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
	size_t 	storagesize;
	int flags;
{
	pool = (advvm_storage_t *)malloc(sizeof(advvm_storage_t *), M_ADVVM, M_WAITOK);
	pool->ads_start = start;
	pool->ads_end = end;
	pool->ads_flags = flags;
	pool->ads_storage = storage;
	pool->ads_storagesize = storagesize;

	pool->ads_extent = extent_create(name, pool->ads_start, pool->ads_end, M_ADVVM, pool->ads_storage, pool->ads_storagesize, pool->ads_flags);

	//return (pool);
}

int
advvm_allocate_region(pool, start, size, flags)
	advvm_storage_t		*pool;
	u_long 				start, size;
	int 				flags;
{
	int error;

	if (advvm_extent_check(pool)) {
		if ((start >= pool->ads_start) && (start <= pool->ads_end)) {
			if (size <= (pool->ads_end - pool->ads_start)) {
				error = extent_alloc_region(pool->ads_extent, start, size, flags);
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
advvm_allocate_subregion(pool, substart, subend, size, alignment, boundary, flags)
	advvm_storage_t 	*pool;
	u_long 				substart, subend, size, alignment, boundary;
	int 				flags;
{
	int error;
	u_long result;

	if (advvm_extent_check(pool)) {
		if ((substart >= pool->ads_start) && (subend <= pool->ads_end) && (subend > substart)) {
			error = extent_alloc_subregion(pool->ads_extent, substart, subend, size, alignment, boundary, flags, &result);
		}
	} else {
		return (ADVVM_NOEXTENT);
	}
	pool->ads_pool = &result;
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
	if (advvm_extent_check(pool)) {
		error = extent_free(pool->ads_extent, start, size, flags);
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
	if (advvm_extent_check(pool)) {
		extent_destroy(pool->ads_extent);
		advvm_free((advvm_storage_t *) pool);
	} else {
		printf("advvm_destroy: no extent to destroy");
	}
}

u_long *
advvm_get_storage_pool(pool)
	advvm_storage_t *pool;
{
	return (pool->ads_pool);
}

/* internal use only */
bool_t
advvm_extent_check(pool)
	advvm_storage_t *pool;
{
	if (pool->ads_extent) {
		return (TRUE);
	}
	return (FALSE);
}

/* a generic malloc for advvm with kern_malloc */
void
advvm_malloc(addr, size, flags)
	void 	*addr;
	u_long 	size;
	int 	flags;
{
	addr = (void *)malloc(addr, size, M_ADVVM, flags);
}

void
advvm_calloc(num, addr, size, flags)
	int 	num;
	void 	*addr;
	u_long 	size;
	int 	flags;
{
	addr = (void *)calloc(num, addr, size, M_ADVVM, flags);
}

void
advvm_free(addr)
	void 	*addr;
{
	free(addr, M_ADVVM);
}
