/*
 * prop_dictionary.c
 *
 *  Created on: 17 Mar 2023
 *      Author: marti
 */
#include <devel/sys/properties.h>

#include "proplib_compat.h"
#include "prop_dictionary.h"

struct prop_dict_entry {
	prop_object_t			pde_objref;
};

struct prop_dictionary {
	propdb_t 				pd_propdb;
	prop_object_t			pd_obj;
	struct prop_dict_entry 	*pd_array;
	u_int					pd_capacity;
	u_int					pd_count;
	int						pd_flags;

	u_int32_t				pd_version;
};

prop_dictionary_t
prop_dictionary_alloc(const char *name, u_int capacity)
{
	propdb_t db;
	prop_dictionary_t pd;
	struct prop_dict_entry *pde;

	db = propdb_create(name);
	pd = (struct prop_dictionary *)db;
	pd->pd_propdb = db;

	if (capacity > db->kd_size) {
		capacity = db->kd_size;
	}
	if (capacity != 0) {
		pde = pde * capacity;
		if (pde == NULL) {
			return (NULL);
		}
	} else {
		pde = NULL;
	}
	pd->pd_capacity = capacity;
	pd->pd_array = pde;
	pd->pd_count = 0;
	pd->pd_flags = 0;
	pd->pd_version = 0;

	return (pd);
}

prop_dictionary_t
prop_dictionary_create(const char *name)
{
	return (prop_dictionary_alloc(name, 0));
}

prop_dictionary_t
prop_dictionary_create_with_capacity(const char *name, u_int capacity)
{
	return (prop_dictionary_alloc(name, capacity));
}

int
prop_dictionary_set(prop_dictionary_t pd, prop_object_t object, const char *name, int wait)
{
	int ret;

	ret = prop_set(pd->pd_propdb, object, name, pd->pd_array, pd->pd_capacity, PROP_DICTIONARY, wait);
	return (ret);
}

size_t
prop_dictionary_get(prop_dictionary_t pd, prop_object_t object, const char *name)
{
	size_t ret;

	ret = prop_get(pd->pd_propdb, object, name, pd->pd_array, pd->pd_capacity, PROP_DICTIONARY);
	return (ret);
}
