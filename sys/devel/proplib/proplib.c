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

void
propdb_opaque_alloc(opaque_t obj, size_t size, u_int capacity, int type)
{
	if (capacity != 0 && capacity != -1) {
		obj = prop_calloc(capacity, size, type);
		if (obj == NULL) {
			return;
		}
	} else if (capacity == -1) {
		obj = prop_malloc(size, type);
		if (obj == NULL) {
			return;
		}
	} else {
		obj = NULL;
	}
}

void
propdb_opaque_free(opaque_t obj, int type)
{
	if (obj != NULL) {
		prop_free(obj, type);
	}
}

/* prop object */
prop_object_t
prop_object_create(const char *name, void *val, size_t len, int type)
{
	propdb_t db;
	prop_object_t po;

	db = propdb_create(name);
	propdb_opaque_alloc(po, sizeof(prop_object_t), -1, type);
	if (po == NULL) {
		return (NULL);
	}
	po->po_db = db;
	po->po_name = name;
	po->po_val = val;
	po->po_len = len;
	po->po_type = type;

	return (po);
}

int
prop_object_set(prop_object_t po, int wait)
{
	int ret;

	ret = prop_set(po->po_db, po, po->po_name, po->po_val, po->po_len, po->po_type, wait);
	return (ret);
}

int
prop_object_get(prop_object_t po)
{
	size_t ret;

	ret = prop_get(po->po_db, po, po->po_name, po->po_val, po->po_len, po->po_type);
	return (ret);
}

int
prop_object_type(prop_object_t obj)
{
	prop_object_t po = obj;

	if (obj == NULL) {
		return (PROP_TYPE_UNKNOWN);
	}
	return (po->po_type);
}

/* prop array */
prop_array_t
prop_array_create(const char *name, u_int capacity)
{
	propdb_t 		db;
	prop_array_t 	pa;
	prop_object_t 	*po;

	db = propdb_create(name);
	propdb_opaque_alloc(po, sizeof(prop_object_t), capacity, M_PROP_ARRAY);
	propdb_opaque_alloc(pa, sizeof(prop_array_t), -1, M_PROP_ARRAY);
	if (po == NULL || pa == NULL) {
		return (NULL);
	}
	if (pa != NULL) {
		pa->pa_db = db;
		pa->pa_array = po;
		pa->pa_capacity = capacity;
		pa->pa_count = 0;
		pa->pa_flags = 0;
		pa->pa_version = 0;
	} else if (po != NULL) {
		propdb_opaque_free(po, M_PROP_ARRAY);
	}
	return (pa);
}

/* prop bool */
static struct prop_bool _prop_bool_true;
static struct prop_bool _prop_bool_false;

prop_bool_t
prop_bool_create(const char *name, bool_t val)
{
	propdb_t 	db;
	prop_bool_t pb;

	db = propdb_create(name);
	pb = val ? &_prop_bool_true : &_prop_bool_false;
	pb->pb_db = db;
	pb->pb_value = val;

	return (pb);
}
