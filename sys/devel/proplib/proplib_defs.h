/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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

#ifndef _PROPLIB_DEFS_H_
#define _PROPLIB_DEFS_H_

#include <sys/stddef.h>
#include <sys/malloc.h>

#include <devel/sys/properties.h>

static __inline void
propdb_add(propdb_t db, opaque_t obj, const char *name, void *val, size_t len, int type)
{
	int ret;

	ret = (int)propdb_set(db, obj, name, val, len, &type, M_NOWAIT);
    if (ret) {
        printf("propdb_add: successful \n");
        return;
    }
    printf("propdb_add: unsucessful \n");
}

static __inline void
propdb_remove(propdb_t db, opaque_t obj, const char *name)
{
    int ret;

    ret = propdb_delete(db, obj, name);
    if (ret != 0) {
        printf("propdb_delete: successful \n");
        return;
    }
    printf("propdb_delete: unsucessful \n");
}

static __inline void
propdb_lookup(propdb_t db, opaque_t obj, const char *name, void *val, size_t len, int type)
{
	size_t ret;

	ret = propdb_get(db, obj, name, val, len, type);
	if ((ret == len) && (propdb_type(val) == type)) {
		printf("propdb_lookup: successful \n");
		return;
	}
	printf("propdb_lookup: unsucessful \n");
}

/* API's */
/* array */
#define propdb_set_array(db, obj, name, val)			\
	propdb_add(db, obj, name, val, sizeof(val), PROP_ARRAY)

#define propdb_get_array(db, obj, name, val)			\
	propdb_lookup(db, obj, name, val, sizeof(val), PROP_ARRAY)

#define propdb_delete_array(db, obj, name)				\
	propdb_remove(db, obj, name)

/* number */
#define propdb_set_number(db, obj, name, val)			\
	propdb_add(db, obj, name, val, sizeof(val), PROP_NUMBER)

#define propdb_get_number(db, obj, name, val)			\
	propdb_lookup(db, obj, name, val, sizeof(val), PROP_NUMBER)

#define propdb_delete_number(db, obj, name)				\
	propdb_remove(db, obj, name)

/* bool */
#define propdb_set_bool(db, obj, name, val)				\
	propdb_add(db, obj, name, val, sizeof(val), PROP_BOOL)

#define propdb_get_bool(db, obj, name, val)				\
	propdb_lookup(db, obj, name, val, sizeof(val), PROP_BOOL)

#define propdb_delete_bool(db, obj, name)				\
	propdb_remove(db, obj, name)

/* string */
#define propdb_set_string(db, obj, name, val)			\
	propdb_add(db, obj, name, val, sizeof(val), PROP_STRING)

#define propdb_get_string(db, obj, name, val)			\
	propdb_lookup(db, obj, name, val, sizeof(val), PROP_STRING)

#define propdb_delete_string(db, obj, name)				\
	propdb_remove(db, obj, name)

/* data */
#define propdb_set_data(db, obj, name, val)				\
	propdb_add(db, obj, name, val, sizeof(val), PROP_DATA)

#define propdb_get_data(db, obj, name, val)				\
	propdb_lookup(db, obj, name, val, sizeof(val), PROP_DATA)

#define propdb_delete_data(db, obj, name)				\
	propdb_remove(db, obj, name)

/* dictionary */
#define propdb_set_dictionary(db, obj, name, val)		\
	propdb_add(db, obj, name, val, sizeof(val), PROP_DICTIONARY)

#define propdb_get_dictionary(db, obj, name, val)		\
	propdb_lookup(db, obj, name, val, sizeof(val), PROP_DICTIONARY)

#define propdb_delete_dictionary(db, obj, name)			\
	propdb_remove(db, obj, name)

/* dictionary keysym */
#define propdb_set_dict_keysym(db, obj, name, val)		\
	propdb_add(db, obj, name, val, sizeof(val), PROP_DICT_KEYSYM)

#define propdb_get_dict_keysym(db, obj, name, val)		\
	propdb_lookup(db, obj, name, val, sizeof(val), PROP_DICT_KEYSYM)

#define propdb_delete_dict_keysym(db, obj, name)		\
	propdb_remove(db, obj, name)

/* aggregate */
#define propdb_set_aggregate(db, obj, name, val, type)	\
	propdb_add(db, obj, name, val, sizeof(val), PROP_TYPE(type))

#define propdb_get_aggregate(db, obj, name, val, type)	\
	propdb_lookup(db, obj, name, val, sizeof(val), PROP_TYPE(type))

#define propdb_delete_aggregate(db, obj, name)			\
	propdb_remove(db, obj, name)

#endif /* _PROPLIB_DEFS_H_ */
