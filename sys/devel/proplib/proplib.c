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

#include <sys/malloc.h>
#include <devel/sys/properties.h>

#include "proplib_compat.h"

opaque_t
propdb_opaque_malloc(size_t size, int mtype)
{
	opaque_t obj;

	obj = malloc(size, mtype, M_WAITOK);
	if (obj == NULL) {
		return (NULL);
	}
	return (obj);
}

opaque_t
propdb_opaque_calloc(u_int capacity, size_t size, int mtype)
{
	opaque_t obj;

	obj = calloc(capacity, size, mtype, M_WAITOK);
	if (obj == NULL) {
		return (NULL);
	}
	return (obj);
}

opaque_t
propdb_opaque_realloc(opaque_t obj, size_t size, int mtype)
{
	opaque_t newobj;

	newobj = realloc(obj, size, mtype, M_WAITOK);
	if (newobj == NULL) {
		return (NULL);
	}
	return (newobj);
}

void
propdb_opaque_free(opaque_t obj, int mtype)
{
	if (obj != NULL) {
		free(obj, mtype);
	}
}

int
propdb_opaque_set(propdb_t db, opaque_t obj, const char *name, void *val, size_t len, int type)
{
	int ret;

	if (prop_object_type(obj) == type) {
		ret = prop_set(db, obj, name, val, len, type, M_WAITOK);
	}
	if (ret != 0) {
		return (ret);
	}
	return (0);
}

int
propdb_opaque_get(propdb_t db, opaque_t obj, const char *name, void *val, size_t len, int type)
{
	size_t ret;

	if (prop_object_type(obj) == type) {
		ret = prop_get(db, obj, name, val, len, type);
	}
	if (ret != 0) {
		return (ret);
	}
	return (0);
}
