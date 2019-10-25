/*
	kern_slaballoc.c
	Copyright (c) 2019, Valentin Debon
	This file is part of the slab_alloc repository
	subject the BSD 3-Clause License, see LICENSE.txt
*/

#include <stdint.h> /* uint8_t uintptr_t */
//#include <stdbool.h> /* bool */
//#include <stdlib.h> /* NULL */
//#include <unistd.h> /* sysconf */
#include <sys/mman.h> /* mmap, munmap */
#include <slaballoc.h>

/* We will safely assume the long -> size_t cast in this code */
#define GET_PAGESIZE() sysconf(_SC_PAGESIZE)

static void
slab_cache_push_front(struct slab_cache *cache,	struct slab *slab) {
	slab->prev = NULL;
	slab->next = cache->front;
	cache->front = slab;

	if(slab->next != NULL) {
		slab->next->prev = slab;
	} else {
		cache->back = slab;
	}
}

static void
slab_cache_push_back(struct slab_cache *cache, struct slab *slab) {
	slab->prev = cache->back;
	slab->next = NULL;
	cache->back = slab;

	if(slab->prev != NULL) {
		slab->prev->next = slab;
	} else {
		cache->front = slab;
	}
}

static void
slab_cache_remove(struct slab_cache *cache,	struct slab *slab) {

	if(cache->front == slab) {
		cache->front = slab->next;

		if(cache->back == slab) {
			cache->back = NULL;
		} else {
			cache->front->prev = NULL;
		}
	} else if(cache->back == slab) {
		cache->back = slab->prev;
		cache->back->next = NULL;
	} else {
		slab->prev->next = slab->next;
		slab->next->prev = slab->prev;
	}
}

int
slab_cache_init(struct slab_cache *cache, size_t size, size_t align) {
	size_t const pagesize = GET_PAGESIZE();
	int retval = -1;

	if(size <= pagesize / 8 && size != 0 /* Respect limits */
		&& (align & (align - 1)) == 0 /* Align power of 2 ? */
		&& (size >= sizeof(void *) /* Can we fit the freelist ? */
			|| align >= sizeof(void *))) {

		cache->size = size;
		cache->stride = align * ((size - 1) / align + 1);

		cache->slabmax = (pagesize - sizeof(struct slab)) / cache->stride;

		cache->front = NULL;
		cache->back = NULL;

		retval = 0;
	}

	return retval;
}

void
slab_cache_deinit(struct slab_cache *cache) {

	if(cache->front != NULL) {
		size_t const pagesize = GET_PAGESIZE();
		struct slab *current = cache->front;

		while(current->next != NULL) {
			struct slab *next = current->next;

			munmap((uint8_t *)(current + 1) - pagesize, pagesize);
			current = next;
		}
	}
}


void *
slab_cache_alloc(struct slab_cache *cache) {
	void *ptr = NULL;
	struct slab *slab = cache->front;

	if(slab == NULL
		|| slab->count == cache->slabmax) {
		/* slab_cache_grow */
		size_t const pagesize = GET_PAGESIZE();

		uint8_t * const page = mmap(NULL, pagesize,
			PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

		if(page != MAP_FAILED) {
			uint8_t *buffer = page + cache->stride;
			slab = (struct slab *)(page + pagesize - sizeof(struct slab));

			slab->freelist = buffer;
			while(buffer < (uint8_t *)slab) {
				uint8_t * const next = buffer + cache->stride;
				*((void **)buffer) = next;
				buffer = next;
			}
			slab->count = 1;

			slab_cache_push_front(cache, slab);

			ptr = page;
		}
	} else {
		ptr = slab->freelist;
		slab->freelist = *((void **)ptr);

		if(slab->count++ == cache->slabmax) {
			slab_cache_remove(cache, slab);
			slab_cache_push_back(cache, slab);
		}
	}

	return ptr;
}

void
slab_cache_dealloc(struct slab_cache *cache, void *ptr) {
	size_t const pagesize = GET_PAGESIZE();
	uint8_t * const page = (uint8_t *)(((uintptr_t)ptr / pagesize) * pagesize);
	struct slab *slab = (struct slab *)(page + pagesize - sizeof(struct slab));

	*((void **)ptr) = slab->freelist;
	slab->freelist = ptr;
	slab->count--;

	if(slab->count == 0) {
		/* slab_cache_reap */
		slab_cache_remove(cache, slab);
		munmap(page, pagesize);
	} else if(slab->count == cache->slabmax - 1) {
		slab_cache_remove(cache, slab);
		slab_cache_push_front(cache, slab);
	}
}

