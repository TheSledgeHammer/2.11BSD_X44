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

/*
 * Cpu_top (MI & MD related) is an interface to the underlying Bitmap/hash array.
 * With the main goal of transferring information about the smp topology (topo_node)
 * to the cpu (cpu_info).
 *
 * Backing Store:
 * Bitmap/hash array uses a list with an integer hash function (triple32) to store
 * information.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <devel/sys/bitmap.h>
#include <devel/sys/malloctypes.h>

#define BITHASH_MASK 256
struct bitlist       bitset[BITHASH_MASK];
int 				 bitmap_counter;
struct lock_object   *bitmap_lock;

#define bitmap_lock(lock) 	(simple_lock(lock))
#define bitmap_unlock(lock) (simple_unlock(lock))

void
bitmap_init()
{
    int i;
    bitmap_counter = 0;

    simple_lock_init(&bitmap_lock, "bitmap_lock");

    for(i = 0; i < BITHASH_MASK; i++) {
    	LIST_INIT(&bitset[i]);
    }
}

uint32_t
bithash(value)
	uint64_t value;
{
    uint32_t hash1 = triple32(value)%BITHASH_MASK;

    return (hash1);
}

void
bitmap_insert(value)
	uint64_t value;
{
    register struct bitlist *head;
    register bitmap_t *mask;

    head = &bitset[bithash(value)];
    mask = (bitmap_t *)malloc(sizeof(bitmap_t *), M_BITMAP, M_WAITOK);
    mask->value = value;
    bitmap_lock(&bitmap_lock);
    LIST_INSERT_HEAD(head, mask, entry);
    bitmap_unlock(&bitmap_lock);
    bitmap_counter++;
}

bitmap_t *
bitmap_search(value)
	uint64_t value;
{
    register struct bitlist *head;
    register bitmap_t *mask;
    head = &bitset[bithash(value)];
    bitmap_lock(&bitmap_lock);
    for(mask = LIST_FIRST(head); mask != NULL; mask = LIST_NEXT(mask, entry)) {
        if(mask->value == value) {
        	 bitmap_unlock(&bitmap_lock);
            return (mask);
        }
    }
    bitmap_unlock(&bitmap_lock);
    return (NULL);
}

void
bitmap_remove(value)
	uint64_t value;
{
    register struct bitlist *head;
    register bitmap_t       *map;
    head = &bitset[bithash(value)];
    bitmap_lock(&bitmap_lock);
    for(map = LIST_FIRST(head); map != NULL; map = LIST_NEXT(map, entry)) {
        if(map->value == value) {
        	LIST_REMOVE(map, entry);
            bitmap_unlock(&bitmap_lock);
            bitmap_counter--;
        }
    }
}
