/*
 * prop_array.c
 *
 *  Created on: 18 Mar 2023
 *      Author: marti
 */

#include <sys/malloc.h>
#include <devel/sys/properties.h>

#include "proplib_compat.h"

prop_array_t
prop_array_create(u_int capacity)
{
	prop_array_t 	pa;
	opaque_t 		*array;

	if (capacity != 0) {
		array = propdb_calloc(capacity, sizeof(opaque_t), M_PROP_ARRAY);
		if (array == NULL) {
			return (NULL);
		}
	} else {
		array = NULL;
	}

	pa = propdb_malloc(sizeof(prop_array_t), M_PROP_ARRAY);
	if (pa != NULL) {
		pa->pa_db = propdb_create(PROP_NAME_ARRAY);
		pa->pa_array = array;
		pa->pa_capacity = capacity;
		pa->pa_count = 0;
		pa->pa_flags = 0;
		pa->pa_version = 0;
	} else if (array != NULL) {
		propdb_free(array, M_PROP_ARRAY);
	}
	return (pa);
}

void
prop_array_set(prop_array_t pa)
{
	(void)propdb_set(pa->pa_db, pa, PROP_NAME_ARRAY, pa->pa_array, sizeof(pa->pa_array), PROP_TYPE_ARRAY);
}

void
prop_array_get(prop_array_t pa)
{
	propdb_get(pa->pa_db, pa, PROP_NAME_ARRAY, pa->pa_array, sizeof(pa->pa_array), PROP_TYPE_ARRAY);
}
