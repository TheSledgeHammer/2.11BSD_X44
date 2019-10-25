/*
	slaballoc.h
	Copyright (c) 2019, Valentin Debon
	This file is part of the slab_alloc repository
	subject the BSD 3-Clause License, see LICENSE.txt
*/

#ifndef SLABALLOC_H
#define SLABALLOC_H

#include <sys/types.h>

struct slab {
	void *freelist;     /* slab's freelist */
	size_t count;       /* slab's allocated blocks */

	struct slab *prev; /* previous slab in linked list, NULL if first */
	struct slab *next; /* next slab in linked list, NULL if last */
};

struct slab_cache {
	size_t size;        /* size of allocations */
	size_t stride;      /* size respecting contiguous alignment */

	size_t slabmax;     /* maximum numbers of blocks in a slab */

	struct slab *front; /* first slab in linked list, NULL if empty */
	struct slab *back; /* last slab in linked list, NULL if empty */
};

/**
 * Initializes a cache according to required objects' size
 * and alignment
 * @param cache Cache to initialize
 * @param size Objects' size in memory (usually sizeof it)
 * @param align Required objects' alignment
 * @return 0 if not error, -1 else
 */
int
slab_cache_init(struct slab_cache *cache, size_t size, size_t align);

/**
 * Deinitialize a cache
 * @param cache A previously slab_cache_init()'d valid cache
 */
void
slab_cache_deinit(struct slab_cache *cache);

/**
 * Tries to allocate memory from the cache according to
 * its' configuration
 * @param cache A previously slab_cache_init()'d valid cache
 * @param NULL if couldn't allocate, a pointer to a valid
 * memory space the size of cache's size configurated size.
 */
void *
slab_cache_alloc(struct slab_cache *cache);

/**
 * Return a previously slab_cache_alloc()'d pointer to the cache
 * @param cache A previously slab_cache_init()'d valid cache
 * @param ptr A previously slab_cache_alloc()'d valid pointer
 */
void
slab_cache_dealloc(struct slab_cache *cache, void *ptr);

/* SLAB_ALLOC_H */
#endif
