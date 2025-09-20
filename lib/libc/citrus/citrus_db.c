/*	$NetBSD: citrus_db.c,v 1.3 2004/01/02 21:49:35 itojun Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_db.c,v 1.3 2004/01/02 21:49:35 itojun Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>

#include "citrus_bcs.h"
#include "citrus_region.h"
#include "citrus_memstream.h"
#include "citrus_mmap.h"
#include "citrus_db.h"
#include "citrus_db_file.h"

struct _citrus_db {
	/* private */
	struct _citrus_region db_region;
	u_int32_t (*db_hashfunc)(void *, struct _citrus_region *);
	void *db_hashfunc_closure;
};

int
_citrus_db_open(struct _citrus_db **rdb, struct _citrus_region *r, const char *magic,
		u_int32_t (*hashfunc)(void *, struct _citrus_region *),
		void *hashfunc_closure)
{
	struct _citrus_memstream ms;
	struct _citrus_db *db;
	struct _citrus_db_header_x *dhx;

	_citrus_memstream_bind(&ms, r);

	/* sanity check */
	dhx = _citrus_memstream_getregion(&ms, NULL, sizeof(*dhx));
	if (dhx == NULL)
		return EFTYPE;
	if (strncmp(dhx->dhx_magic, magic, _CITRUS_DB_MAGIC_SIZE) != 0)
		return EFTYPE;
	if (_citrus_memstream_seek(&ms, be32toh(dhx->dhx_entry_offset), SEEK_SET))
		return EFTYPE;

	if (be32toh(dhx->dhx_num_entries)*_CITRUS_DB_ENTRY_SIZE > _citrus_memstream_remainder(&ms))
		return EFTYPE;

	db = malloc(sizeof(*db));
	if (db==NULL)
		return errno;
	db->db_region = *r;
	db->db_hashfunc = hashfunc;
	db->db_hashfunc_closure = hashfunc_closure;
	*rdb = db;

	return 0;
}

void
_citrus_db_close(struct _citrus_db *db)
{
	free(db);
}

int
_citrus_db_lookup(struct _citrus_db *db, struct _citrus_region *key,
		  struct _citrus_region *data, struct _citrus_db_locator *dl)
{
	u_int32_t hashval, num_entries;
	size_t offset;
	struct _citrus_memstream ms;
	struct _citrus_db_header_x *dhx;
	struct _citrus_db_entry_x *dex;
	struct _citrus_region r;

	_citrus_memstream_bind(&ms, &db->db_region);

	dhx = _citrus_memstream_getregion(&ms, NULL, sizeof(*dhx));
	_DIAGASSERT(dhx);
	num_entries = be32toh(dhx->dhx_num_entries);
	if (num_entries == 0)
		return ENOENT;

	if (dl != NULL && dl->dl_offset>0) {
		hashval = dl->dl_hashval;
		offset = dl->dl_offset;
		if (offset >= _citrus_region_size(&db->db_region))
			return ENOENT;
	} else {
		hashval =
		    db->db_hashfunc(db->db_hashfunc_closure, key)%num_entries;
		offset =
		    be32toh(dhx->dhx_entry_offset) +
		    hashval * _CITRUS_DB_ENTRY_SIZE;
		if (dl)
			dl->dl_hashval = hashval;
	}
	do {
		/* seek to the next entry */
		if (_citrus_memory_stream_seek(&ms, offset, SEEK_SET))
			return EFTYPE;
		/* get the entry record */
		dex = _citrus_memstream_getregion(&ms, NULL, _CITRUS_DB_ENTRY_SIZE);
		if (dex == NULL)
			return EFTYPE;

		/* jump to next entry having the same hash value. */
		offset = be32toh(dex->dex_next_offset);

		/* save the current position */
		if (dl) {
			dl->dl_offset = offset;
			if (offset==0)
				dl->dl_offset = _citrus_region_size(&db->db_region);
		}

		/* compare hash value. */
		if (be32toh(dex->dex_hash_value) != hashval)
			/* not found */
			break;
		/* compare key length */
		if (be32toh(dex->dex_key_size) == _citrus_region_size(key)) {
			/* seek to the head of the key. */
			if (_citrus_memstream_seek(&ms, be32toh(dex->dex_key_offset),
					    SEEK_SET))
				return EFTYPE;
			/* get the region of the key */
			if (_citrus_memstream_getregion(&ms, &r,
					_citrus_region_size(key)) == NULL)
				return EFTYPE;
			/* compare key byte stream */
			if (memcmp(_citrus_region_head(&r), _citrus_region_head(key),
					_citrus_region_size(key)) == 0) {
				/* match */
				if (_citrus_memstream_seek(
					&ms, be32toh(dex->dex_data_offset),
					SEEK_SET))
					return EFTYPE;
				if (_citrus_memstream_getregion(
					&ms, data,
					be32toh(dex->dex_data_size)) == NULL)
					return EFTYPE;
				return 0;
			}
		}
	} while (offset != 0);

	return ENOENT;
}

int
_citrus_db_lookup_by_string(struct _citrus_db *db, const char *key,
			    struct _citrus_region *data,
			    struct _citrus_db_locator *dl)
{
	struct _region r;

	/* LINTED: discard const */
	_citrus_region_init(&r, (char *)key, strlen(key));

	return _citrus_db_lookup(db, &r, data, dl);
}

int
_citrus_db_lookup8_by_string(struct _citrus_db *db, const char *key,
			     u_int8_t *rval, struct _citrus_db_locator *dl)
{
	int ret;
	struct _region r;

	ret = _citrus_db_lookup_by_string(db, key, &r, dl);
	if (ret)
		return ret;

	if (_citrus_region_size(&r) != 1)
		return EFTYPE;

	if (rval)
		memcpy(rval, _citrus_region_head(&r), 1);

	return 0;
}

int
_citrus_db_lookup16_by_string(struct _citrus_db *db, const char *key,
			      u_int16_t *rval, struct _citrus_db_locator *dl)
{
	int ret;
	struct _region r;
	u_int16_t val;

	ret = _citrus_db_lookup_by_string(db, key, &r, dl);
	if (ret)
		return ret;

	if (_citrus_region_size(&r) != 2)
		return EFTYPE;

	if (rval) {
		memcpy(&val, _citrus_region_head(&r), 2);
		*rval = be16toh(val);
	}

	return 0;
}

int
_citrus_db_lookup32_by_string(struct _citrus_db *db, const char *key,
			      u_int32_t *rval, struct _citrus_db_locator *dl)
{
	int ret;
	struct _region r;
	u_int32_t val;

	ret = _citrus_db_lookup_by_string(db, key, &r, dl);
	if (ret)
		return ret;

	if (_citrus_region_size(&r) != 4)
		return EFTYPE;

	if (rval) {
		memcpy(&val, _citrus_region_head(&r), 4);
		*rval = be32toh(val);
	}

	return 0;
}

int
_citrus_db_lookup_string_by_string(struct _citrus_db *db, const char *key,
				   const char **rdata,
				   struct _citrus_db_locator *dl)
{
	int ret;
	struct _region r;

	ret = _citrus_db_lookup_by_string(db, key, &r, dl);
	if (ret)
		return ret;

	/* check whether the string is null terminated */
	if (_citrus_region_size(&r) == 0)
		return EFTYPE;
	if (*((const char*)_citrus_region_head(&r)+_citrus_region_size(&r)-1) != '\0')
		return EFTYPE;

	if (rdata)
		*rdata = _citrus_region_head(&r);

	return 0;
}

int
_citrus_db_get_number_of_entries(struct _citrus_db *db)
{
	struct _citrus_memstream ms;
	struct _citrus_db_header_x *dhx;

	_citrus_memstream_bind(&ms, &db->db_region);

	dhx = _citrus_memstream_getregion(&ms, NULL, sizeof(*dhx));
	_DIAGASSERT(dhx);
	return (int)be32toh(dhx->dhx_num_entries);
}

int
_citrus_db_get_entry(struct _citrus_db *db, int idx,
		     struct _region *key, struct _region *data)
{
	u_int32_t num_entries;
	size_t offset;
	struct _citrus_memstream ms;
	struct _citrus_db_header_x *dhx;
	struct _citrus_db_entry_x *dex;

	_citrus_memstream_bind(&ms, &db->db_region);

	dhx = _citrus_memstream_getregion(&ms, NULL, sizeof(*dhx));
	_DIAGASSERT(dhx);
	num_entries = be32toh(dhx->dhx_num_entries);
	if (idx < 0 || (u_int32_t)idx >= num_entries)
		return EINVAL;

	/* seek to the next entry */
	offset = be32toh(dhx->dhx_entry_offset) + idx * _CITRUS_DB_ENTRY_SIZE;
	if (_citrus_memory_stream_seek(&ms, offset, SEEK_SET))
		return EFTYPE;
	/* get the entry record */
	dex = _citrus_memstream_getregion(&ms, NULL, _CITRUS_DB_ENTRY_SIZE);
	if (dex == NULL)
		return EFTYPE;
	/* seek to the head of the key. */
	if (_citrus_memstream_seek(&ms, be32toh(dex->dex_key_offset), SEEK_SET))
		return EFTYPE;
	/* get the region of the key. */
	if (_citrus_memstream_getregion(&ms, key, be32toh(dex->dex_key_size))==NULL)
		return EFTYPE;
	/* seek to the head of the data. */
	if (_citrus_memstream_seek(&ms, be32toh(dex->dex_data_offset), SEEK_SET))
		return EFTYPE;
	/* get the region of the data. */
	if (_citrus_memstream_getregion(&ms, data, be32toh(dex->dex_data_size))==NULL)
		return EFTYPE;

	return 0;
}
