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
 * Cpu_top (MI & MD related) is an interface to the underlying Bitset/hash array.
 * With the main goal of transferring information about the smp topology (topo_node)
 * to the cpu (cpu_info).
 *
 * Backing Store:
 * Bitset/hash array uses a tailq with an integer hash function (triple32) to store
 * information.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/user.h>

#include <devel/sys/smp.h>

struct bitlist;
TAILQ_HEAD(bitlist, bitmap);
struct bitmap {
    TAILQ_ENTRY(bitmap) entry;
    union {
        uint32_t        value;
    };
};
typedef struct bitmap   bitmap_t;
//extern struct bitlist 	bitset[];
//extern bitset_counter;

union cpu_top {
    uint32_t 			ct_mask;
};
typedef union cpu_top 	ctop_t;

#define BITHASH_MASK 256
struct bitlist       bitset[BITHASH_MASK];
int 				 bitset_counter;
ctop_t 				 *ctop;

void
ctop_init()
{
	ctop = (ctop_t *)malloc(sizeof(ctop_t *));
	bitset_init();
}

void
ctop_add(top, val)
	ctop_t *top;
    int val;
{
    top = ctop;
    top->ct_mask = val;
    bitmap_insert(val);
}

uint32_t
ctop_get(top)
	ctop_t *top;
{
    top = ctop;
    return (bitmap_search(top->ct_mask)->value);
}

void
ctop_remove(top)
	ctop_t *top;
{
    top = ctop;
    bitmap_remove(top->ct_mask);
}

/* bitset functions */
static void
bitset_init()
{
    int i;
    bitset_counter = 0;

    for(i = 0; i < BITHASH_MASK; i++) {
        TAILQ_INIT(&bitset[i]);
    }
}

uint32_t
bithash(value)
    uint32_t value;
{
    uint32_t hash1 = triple32(value)%BITHASH_MASK;
    return (hash1);
}

static void
bitmap_insert(value)
    uint32_t value;
{
    register struct bitlist *head;
    register bitmap_t *mask;

    head = &bitset[bithash(value)];
    mask = (bitmap_t *)malloc(sizeof(bitmap_t *));
    mask->value = value;
    TAILQ_INSERT_HEAD(head, mask, entry);
    bitset_counter++;
}

static bitmap_t *
bitmap_search(value)
    uint32_t value;
{
    register struct bitlist *head;
    register bitmap_t *mask;
    head = &bitset[bithash(value)];
    for(mask = TAILQ_FIRST(head); mask != NULL; mask = TAILQ_NEXT(mask, entry)) {
        if(mask->value == value) {
            return (mask);
        }
    }
    return (NULL);
}

static void
bitmap_remove(value)
    uint32_t value;
{
    register struct bitlist *head;
    register bitmap_t       *map;
    head = &bitset[bithash(value)];
    for(map = TAILQ_FIRST(head); map != NULL; map = TAILQ_NEXT(map, entry)) {
        if(map->value == value) {
            TAILQ_REMOVE(head, map, entry);
            bitset_counter--;
        }
    }
}
