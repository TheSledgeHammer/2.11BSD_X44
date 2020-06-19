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
 * @(#)htbc_hbchain.c	1.00
 */

/*
 * Hashed Blockchain:
 * - Create a Version/ Snapshot (with Timestamp & Hash) of the current HTBC tree location before committing the write to LFS.
 */

#include <sys/user.h>
#include <devel/htbc/htbc.h>
#include <devel/htbc/htbc_htree.h>

CIRCLEQ_HEAD(hbchain_head, hbchain);
struct hbchain {
	struct hbchain_head			hc_header;
	CIRCLEQ_ENTRY(hbchain)		hc_entries;
	uint32_t					hc_version;
	uint32_t					hc_timestamp;
	uint32_t					hc_hash;
};

void
hbchain_init(hch)
	struct hbchain *hch;
{
	CIRCLEQ_INIT(hch->hc_header);
}

void
hbchain_insert_head(hch)
	struct hbchain *hch;
{
	CIRCLEQ_INSERT_HEAD(hch->hc_header, hch, hc_entries);
}

void
hbchain_insert_tail(hch)
	struct hbchain *hch;
{
	CIRCLEQ_INSERT_TAIL(hch->hc_header, hch, hc_entries);
}

void
hbchain_remove(hch)
	struct hbchain *hch;
{
	CIRCLEQ_REMOVE(hch->hc_header, hch, hc_entries);
}

/* Searches the next entry in the blockchain for matching hash value */
struct hbchain *
hbchain_search_next(hch, hash)
	struct hbchain *hch;
	uint32_t hash;
{
	CIRCLEQ_FOREACH(hch, hch->hc_header, hc_entries) {
		if(CIRCLEQ_NEXT(hch, hc_entries)->hc_hash == hash) {
			return (CIRCLEQ_NEXT(hch, hc_entries));
		}
	}
	return (NULL);
}

/* Searches the previous entry in the blockchain for matching hash value */
struct hbchain *
hbchain_search_prev(hch, hash)
	struct hbchain *hch;
	uint32_t hash;
{
	CIRCLEQ_FOREACH(hch, hch->hc_header, hc_entries) {
		if(CIRCLEQ_PREV(hch, hc_entries)->hc_hash == hash) {
			return (CIRCLEQ_PREV(hch, hc_entries));
		}
	}
	return (NULL);
}

static uint32_t
hbchain_get_hash(struct hbchain *hch)
{
	return hch->hc_hash;
}

static uint32_t
hbchain_get_timestamp(struct hbchain *hch)
{
	return hch->hc_timestamp;
}

static uint32_t
hbchain_get_version(struct hbchain *hch)
{
	return hch->hc_version;
}

static void
hbchain_set_hash(struct hbchain *hch, uint32_t hash)
{
	hch->hc_hash = hash;
}

static void
hbchain_set_timestamp(struct hbchain *hch, uint32_t timestamp)
{
	hch->hc_timestamp = timestamp;
}

static void
hbchain_set_version(struct hbchain *hch, uint32_t version)
{
	hch->hc_version = version;
}
