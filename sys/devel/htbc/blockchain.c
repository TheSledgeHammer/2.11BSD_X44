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
 */

/* A structure revision of the blockchain */
#include <sys/time.h>
#include <sys/user.h>
#include "blockchain.h"
#include "htbc_htree.h"
#include "sys/malloctypes.h"

struct hashchain 				*blockchain;

#define HASH_VERSION			HTREE_HALF_MD4				/* make configurable */
#define HASH_SEED(hash_seed) 	(random_hash_seed(hash_seed));
#define HASH_MAJOR 				(prospector32(random()))
#define HASH_MINOR 				(prospector32(random()))

void
random_hash_seed(hash_seed)
	uint32_t* hash_seed;
{
	u_long rand;

	for(int i = 0; i < 4; i++) {
		rand = prospector32(random);
		hash_seed[i] = rand;
	}
}

int
htbc_chain_hash(const char *name, int len)
{
	register uint32_t *hash_seed;

	HASH_SEED(hash_seed);
	hash = htbc_htree_hash(name, len, hash_seed, HASH_VERSION, HASH_MAJOR, HASH_MINOR);
	return (hash);
}

static void
get_hash_seed(struct htbc_inode *hi, uint32_t *seed)
{
	for (int i = 0; i < 4; i++) {
		seed[i] = hi->hi_hash_seed[i];
	}
}

struct htree_root *
htbc_chain_htree_root(chain)
	struct ht_hchain *chain;
{
	register struct htree_root *hroot;

	hroot = htbc_chain_lookup(chain->hc_name, chain->hc_len)->hc_hroot;
	return (hroot);
}

struct ht_htransaction *
htbc_lookup_transaction(struct ht_hchain *chain)
{
	register struct ht_htransaction *trans;

	trans = htbc_chain_lookup(chain->hc_name, chain->hc_len)->hc_trans;
	return (trans);
}

/*
 * Knob to control the precision of file timestamps:
 *
 *   0 = seconds only; nanoseconds zeroed.
 *   1 = seconds and nanoseconds, accurate within 1/HZ.
 *   2 = seconds and nanoseconds, truncated to microseconds.
 * >=3 = seconds and nanoseconds, maximum precision.
 */
enum { TSP_SEC, TSP_HZ, TSP_USEC, TSP_NSEC };
static int timestamp_precision = TSP_USEC;

void
htbc_timestamp(struct timespec *tsp)
{
	struct timeval tv;
	switch (timestamp_precision) {
	case TSP_SEC:
		tsp->tv_sec = time_second;
		tsp->tv_nsec = 0;
		break;
	case TSP_HZ:
	case TSP_USEC:
		microtime(&tv);
		TIMEVAL_TO_TIMESPEC(&tv, tsp);
		break;
	case TSP_NSEC:
	default:
		break;
	}
}

void
htbc_add_transaction(struct ht_hchain *chain)
{
	register struct ht_htransaction *trans;
	struct timespec tsp;

	trans = chain->hc_trans;
	tsp = trans->ht_timespec;

	htbc_timestamp(&tsp);
	if(trans->ht_flag & IN_ACCESS) {
		trans->ht_atime = tsp.tv_sec;
		trans->ht_atimesec = tsp.tv_nsec;
	}
	if (trans->ht_flag & IN_UPDATE) {
		trans->ht_mtime = tsp.tv_sec;
		trans->ht_mtimesec = tsp.tv_nsec;
		//trans->ht_modrev++;
	}
	if (trans->ht_flag & IN_CHANGE) {
		trans->ht_ctime = tsp.tv_sec;
		trans->ht_ctimesec = tsp.tv_nsec;
	}
	trans->ht_flag &= ~(IN_ACCESS | IN_CHANGE | IN_UPDATE);
	trans->ht_reclen = (sizeof(*chain) + sizeof(*trans));
}

void
htbc_remove_transaction(struct ht_hchain *chain)
{
	register struct ht_htransaction *trans;

	trans = htbc_lookup_transaction(chain);
	if(trans != NULL) {
		trans = NULL;
	} else {
		printf("this entry does not hold any transaction information");
	}
}

void
htbc_chain_insert(chain, name, len)
	struct ht_hchain 	*chain;
	const char 			*name;
	int 				len;
{
	register struct hashchain *bucket;

	chain->hc_name = name;
	chain->hc_len = len;

	bucket = &blockchain[htbc_chain_hash(name, len)];
	hashchain_lock(chain);
	CIRCLEQ_INSERT_HEAD(bucket, chain, hc_entry);
	htbc_add_transaction(chain);
	chain->hc_hroot = (struct htree_root *)malloc(sizeof(struct htree_root), M_HTREE, NULL);
	hashchain_unlock(chain);
	chain->hc_refcnt++;
}

struct ht_hchain *
htbc_chain_lookup(name, len)
	const char 			*name;
	int 				len;
{
	register struct hashchain 	*bucket;
	struct ht_hchain 			*chain;

	bucket = &blockchain[htbc_chain_hash(name, len)];
	hashchain_lock(chain);
	for(chain = CIRCLEQ_FIRST(bucket); chain != NULL; chain = CIRCLEQ_NEXT(chain, hc_entry)) {
		if((chain->hc_name == name) && (chain->hc_len == len)) {
			hashchain_unlock(chain);
			return (chain);
		}
	}
	hashchain_unlock(chain);
	return (NULL);
}

void
htbc_chain_remove(chain)
	struct ht_hchain *chain;
{
	register struct hashchain 	*bucket;

	bucket = &blockchain[htbc_chain_hash(chain->hc_name, chain->hc_len)];
	hashchain_lock(chain);
	CIRCLEQ_REMOVE(bucket, chain, hc_entry);
	htbc_remove_transaction(chain);
	hashchain_unlock(chain);
	chain->hc_refcnt--;
}
