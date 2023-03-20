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

propdb_t
proplib_create(opaque_t obj, struct prop_object *po, struct prop_object_type *type)
{
	propdb_t db;
	db = propdb_create(type->pot_name);

	po->po_type = type;
	po->po_obj = obj;
	po->po_refcnt = 1;

	return (db);
}

int
proplib_set(propdb_t db, opaque_t obj, struct prop_object *po, struct prop_object_type *type, void *val, size_t len)
{
	return (propdb_set(db, obj, type->pot_name, val, len, type->pot_type, M_WAITOK));
}

size_t
proplib_get(propdb_t db, opaque_t obj, struct prop_object *po, struct prop_object_type *type, void *val, size_t len)
{
	return (propdb_get(db, obj, type->pot_name, val, len, type->pot_type));
}

size_t
proplib_objs(propdb_t db, struct prop_object *po, size_t len)
{
	return (propdb_objs(db, &po->po_obj, len));
}

size_t
proplib_list(propdb_t db, struct prop_object *po, struct prop_object_type *type, size_t len)
{
	return (propdb_list(db, po->po_obj, type->pot_name, len));
}

int
proplib_delete(propdb_t db, struct prop_object *po, struct prop_object_type *type)
{
	return (propdb_delete(db, po->po_obj, type->pot_name));
}

int
proplib_copy(propdb_t db, struct prop_object *source, struct prop_object *dest)
{
	return (propdb_copy(db, source->po_obj, dest->po_obj, M_WAITOK));
}
