/*-
 * Copyright (c) 2016 Mindaugas Rasiukevicius <rmind at noxt eu>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Longest Prefix Match (LPM) library supporting IPv4 and IPv6.
 *
 * Algorithm:
 *
 * Each prefix gets its own hash map and all added prefixes are saved
 * in a bitmap.  On a lookup, we perform a linear scan of hash maps,
 * iterating through the added prefixes only.  Usually, there are only
 * a few unique prefixes used and such simple algorithm is very efficient.
 * With many IPv6 prefixes, the linear scan might become a bottleneck.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lpm.c,v 1.6 2019/06/12 14:36:32 christos Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/fnv_hash.h>
#include <sys/systm.h>

#include "lpm.h"

#define	LPM_MAX_PREFIX		(128)
#define	LPM_MAX_WORDS		(LPM_MAX_PREFIX >> 5)
#define	LPM_TO_WORDS(x)		((x) >> 2)
#define	LPM_HASH_STEP		(8)
#define	LPM_LEN_IDX(len)	((len) >> 4)

#ifdef DEBUG
#define	ASSERT			assert
#else
#define	ASSERT(x)
#endif

typedef struct lpm_ent {
	struct lpm_ent 	*next;
	void 			*val;
	unsigned		len;
	uint8_t			key[];
} lpm_ent_t;

typedef struct {
	unsigned	hashsize;
	unsigned	nitems;
	lpm_ent_t 	**bucket;
} lpm_hmap_t;

struct lpm {
	uint32_t	bitmask[LPM_MAX_WORDS];
	int			flags;
	void 		*defvals[2];
	lpm_hmap_t	prefix[LPM_MAX_PREFIX + 1];
};

static const uint32_t zero_address[LPM_MAX_WORDS];

lpm_t *
lpm_create(int flags)
{
	lpm_t *lpm = malloc(sizeof(*lpm), M_DEVBUF, flags);
	lpm->flags = flags;
	return lpm;
}

void
lpm_clear(lpm_t *lpm, lpm_dtor_t dtor, void *arg)
{
	for (unsigned n = 0; n <= LPM_MAX_PREFIX; n++) {
		lpm_hmap_t *hmap = &lpm->prefix[n];

		if (!hmap->hashsize) {
			KASSERT(!hmap->bucket);
			continue;
		}
		for (unsigned i = 0; i < hmap->hashsize; i++) {
			lpm_ent_t *entry = hmap->bucket[i];

			while (entry) {
				lpm_ent_t *next = entry->next;

				if (dtor) {
					dtor(arg, entry->key,
					    entry->len, entry->val);
				}
				free(entry, M_DEVBUF);
				entry = next;
			}
		}
		free(hmap->bucket, M_DEVBUF);
		hmap->bucket = NULL;
		hmap->hashsize = 0;
		hmap->nitems = 0;
	}
	if (dtor) {
		dtor(arg, zero_address, 4, lpm->defvals[0]);
		dtor(arg, zero_address, 16, lpm->defvals[1]);
	}
	memset(lpm->bitmask, 0, sizeof(lpm->bitmask));
	memset(lpm->defvals, 0, sizeof(lpm->defvals));
}

void
lpm_destroy(lpm_t *lpm)
{
	lpm_clear(lpm, NULL, NULL);
	free(lpm, M_DEVBUF);
}

/*
 * fnv1a_hash: Fowler-Noll-Vo hash function (FNV-1a variant).
 */
static uint32_t
fnv1a_hash(const void *buf, size_t len)
{
	uint32_t hash;
	hash = fnva_32_buf(buf, len, FNV1_32_INIT);
	return (hash);
}

static bool_t
hashmap_rehash(lpm_hmap_t *hmap, unsigned size, int flags)
{
	lpm_ent_t **bucket;
	unsigned hashsize;

	for (hashsize = 1; hashsize < size; hashsize <<= 1) {
		continue;
	}
	bucket = calloc(hashsize, sizeof(lpm_ent_t *), M_DEVBUF, flags);
	if (bucket == NULL)
		return false;
	for (unsigned n = 0; n < hmap->hashsize; n++) {
		lpm_ent_t *list = hmap->bucket[n];

		while (list) {
			lpm_ent_t *entry = list;
			uint32_t hash = fnv1a_hash(entry->key, entry->len);
			const unsigned i = hash & (hashsize - 1);

			list = entry->next;
			entry->next = bucket[i];
			bucket[i] = entry;
		}
	}
	if (hmap->bucket)
		free(hmap->bucket, M_DEVBUF);
	hmap->bucket = bucket;
	hmap->hashsize = hashsize;
	return true;
}

static lpm_ent_t *
hashmap_insert(lpm_hmap_t *hmap, const void *key, size_t len, int flags)
{
	const unsigned target = hmap->nitems + LPM_HASH_STEP;
	const size_t entlen = offsetof(lpm_ent_t, key[len]);
	uint32_t hash, i;
	lpm_ent_t *entry;

	if (hmap->hashsize < target && !hashmap_rehash(hmap, target, flags)) {
		return NULL;
	}

	hash = fnv1a_hash(key, len);
	i = hash & (hmap->hashsize - 1);
	entry = hmap->bucket[i];
	while (entry) {
		if (entry->len == len && memcmp(entry->key, key, len) == 0) {
			return entry;
		}
		entry = entry->next;
	}

	if ((entry = malloc(entlen, M_DEVBUF, flags)) != NULL) {
		memcpy(entry->key, key, len);
		entry->next = hmap->bucket[i];
		entry->len = len;

		hmap->bucket[i] = entry;
		hmap->nitems++;
	}
	return entry;
}

static lpm_ent_t *
hashmap_lookup(lpm_hmap_t *hmap, const void *key, size_t len)
{
	const uint32_t hash = fnv1a_hash(key, len);
	const unsigned i = hash & (hmap->hashsize - 1);
	lpm_ent_t *entry;

	if (hmap->hashsize == 0) {
		return NULL;
	}
	entry = hmap->bucket[i];

	while (entry) {
		if (entry->len == len && memcmp(entry->key, key, len) == 0) {
			return entry;
		}
		entry = entry->next;
	}
	return NULL;
}

static int
hashmap_remove(lpm_hmap_t *hmap, const void *key, size_t len)
{
	const uint32_t hash = fnv1a_hash(key, len);
	const unsigned i = hash & (hmap->hashsize - 1);
	lpm_ent_t *prev = NULL, *entry;

	if (hmap->hashsize == 0) {
		return -1;
	}
	entry = hmap->bucket[i];

	while (entry) {
		if (entry->len == len && memcmp(entry->key, key, len) == 0) {
			if (prev) {
				prev->next = entry->next;
			} else {
				hmap->bucket[i] = entry->next;
			}
			free(entry, M_DEVBUF);
			return 0;
		}
		prev = entry;
		entry = entry->next;
	}
	return -1;
}

/*
 * compute_prefix: given the address and prefix length, compute and
 * return the address prefix.
 */
static inline void
compute_prefix(const unsigned nwords, const uint32_t *addr,
    unsigned preflen, uint32_t *prefix)
{
	uint32_t addr2[4];

	if ((uintptr_t)addr & 3) {
		/* Unaligned address: just copy for now. */
		memcpy(addr2, addr, nwords * 4);
		addr = addr2;
	}
	for (unsigned i = 0; i < nwords; i++) {
		if (preflen == 0) {
			prefix[i] = 0;
			continue;
		}
		if (preflen < 32) {
			uint32_t mask = htonl(0xffffffff << (32 - preflen));
			prefix[i] = addr[i] & mask;
			preflen = 0;
		} else {
			prefix[i] = addr[i];
			preflen -= 32;
		}
	}
}

/*
 * lpm_insert: insert the CIDR into the LPM table.
 *
 * => Returns zero on success and -1 on failure.
 */
int
lpm_insert(lpm_t *lpm, const void *addr,
    size_t len, unsigned preflen, void *val)
{
	const unsigned nwords = LPM_TO_WORDS(len);
	uint32_t prefix[LPM_MAX_WORDS];
	lpm_ent_t *entry;
	KASSERT(len == 4 || len == 16);

	if (preflen == 0) {
		/* 0-length prefix is a special case. */
		lpm->defvals[LPM_LEN_IDX(len)] = val;
		return 0;
	}
	compute_prefix(nwords, addr, preflen, prefix);
	entry = hashmap_insert(&lpm->prefix[preflen], prefix, len, lpm->flags);
	if (entry) {
		const unsigned n = --preflen >> 5;
		lpm->bitmask[n] |= 0x80000000U >> (preflen & 31);
		entry->val = val;
		return 0;
	}
	return -1;
}

/*
 * lpm_remove: remove the specified prefix.
 */
int
lpm_remove(lpm_t *lpm, const void *addr, size_t len, unsigned preflen)
{
	const unsigned nwords = LPM_TO_WORDS(len);
	uint32_t prefix[LPM_MAX_WORDS];
	KASSERT(len == 4 || len == 16);

	if (preflen == 0) {
		lpm->defvals[LPM_LEN_IDX(len)] = NULL;
		return 0;
	}
	compute_prefix(nwords, addr, preflen, prefix);
	return hashmap_remove(&lpm->prefix[preflen], prefix, len);
}

/*
 * lpm_lookup: find the longest matching prefix given the IP address.
 *
 * => Returns the associated value on success or NULL on failure.
 */
void *
lpm_lookup(lpm_t *lpm, const void *addr, size_t len)
{
	const unsigned nwords = LPM_TO_WORDS(len);
	unsigned i, n = nwords;
	uint32_t prefix[LPM_MAX_WORDS];

	while (n--) {
		uint32_t bitmask = lpm->bitmask[n];

		while ((i = ffs(bitmask)) != 0) {
			const unsigned preflen = (32 * n) + (32 - --i);
			lpm_hmap_t *hmap = &lpm->prefix[preflen];
			lpm_ent_t *entry;

			compute_prefix(nwords, addr, preflen, prefix);
			entry = hashmap_lookup(hmap, prefix, len);
			if (entry) {
				return entry->val;
			}
			bitmask &= ~(1U << i);
		}
	}
	return lpm->defvals[LPM_LEN_IDX(len)];
}

/*
 * lpm_lookup_prefix: return the value associated with a prefix
 *
 * => Returns the associated value on success or NULL on failure.
 */
void *
lpm_lookup_prefix(lpm_t *lpm, const void *addr, size_t len, unsigned preflen)
{
	const unsigned nwords = LPM_TO_WORDS(len);
	uint32_t prefix[LPM_MAX_WORDS];
	lpm_ent_t *entry;
	KASSERT(len == 4 || len == 16);

	if (preflen == 0) {
		return lpm->defvals[LPM_LEN_IDX(len)];
	}
	compute_prefix(nwords, addr, preflen, prefix);
	entry = hashmap_lookup(&lpm->prefix[preflen], prefix, len);
	if (entry) {
		return entry->val;
	}
	return NULL;
}

#if !defined(_KERNEL)
/*
 * lpm_strtobin: convert CIDR string to the binary IP address and mask.
 *
 * => The address will be in the network byte order.
 * => Returns 0 on success or -1 on failure.
 */
int
lpm_strtobin(const char *cidr, void *addr, size_t *len, unsigned *preflen)
{
	char *p, buf[INET6_ADDRSTRLEN];

	strncpy(buf, cidr, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';

	if ((p = strchr(buf, '/')) != NULL) {
		const ptrdiff_t off = p - buf;
		*preflen = atoi(&buf[off + 1]);
		buf[off] = '\0';
	} else {
		*preflen = LPM_MAX_PREFIX;
	}

	if (inet_pton(AF_INET6, buf, addr) == 1) {
		*len = 16;
		return 0;
	}
	if (inet_pton(AF_INET, buf, addr) == 1) {
		if (*preflen == LPM_MAX_PREFIX) {
			*preflen = 32;
		}
		*len = 4;
		return 0;
	}
	return -1;
}
#endif
