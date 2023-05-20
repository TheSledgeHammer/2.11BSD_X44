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
 * Backing Store:
 * bitlist/hash array uses a list with an integer hash function (triple32) to store
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
#include <sys/bitlist.h>

#define BITHASH_MASK 			256
struct bitlist_header   		bitset[BITHASH_MASK];
int 				 			bitlist_counter;
struct lock_object   			bitlist_lock;

#define bitlist_lock_init(lock) (simple_lock_init(lock, "bitlist_slock"))
#define bitlist_lock(lock) 		(simple_lock(lock))
#define bitlist_unlock(lock) 	(simple_unlock(lock))

void
bitlist_init(void)
{
    int i;
    bitlist_counter = 0;

    bitlist_lock_init(&bitlist_lock);

    for (i = 0; i < BITHASH_MASK; i++) {
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
bitlist_insert(value)
	uint64_t value;
{
    register struct bitlist_header *head;
    register bitlist_t *mask;

    head = &bitset[bithash(value)];
    mask = (bitlist_t *)malloc(sizeof(bitlist_t *), M_BITLIST, M_WAITOK);
    mask->value = value;
    bitlist_lock(&bitlist_lock);
    LIST_INSERT_HEAD(head, mask, entry);
    bitlist_unlock(&bitlist_lock);
    bitlist_counter++;
}

bitlist_t *
bitlist_search(value)
	uint64_t value;
{
    register struct bitlist_header *head;
    register bitlist_t *mask;
    head = &bitset[bithash(value)];
    bitlist_lock(&bitlist_lock);
    for (mask = LIST_FIRST(head); mask != NULL; mask = LIST_NEXT(mask, entry)) {
        if (mask->value == value) {
        	 bitlist_unlock(&bitlist_lock);
            return (mask);
        }
    }
    bitlist_unlock(&bitlist_lock);
    return (NULL);
}

void
bitlist_remove(value)
	uint64_t value;
{
    register struct bitlist_header *head;
    register bitlist_t       *map;
    head = &bitset[bithash(value)];
    bitlist_lock(&bitlist_lock);
    for (map = LIST_FIRST(head); map != NULL; map = LIST_NEXT(map, entry)) {
        if (map->value == value) {
        	LIST_REMOVE(map, entry);
            bitlist_unlock(&bitlist_lock);
            bitlist_counter--;
        }
    }
}
