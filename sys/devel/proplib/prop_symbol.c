/*
 * prop_symbol.c
 *
 *  Created on: Oct 29, 2025
 *      Author: marti
 */

#include <sys/malloc.h>

#include <devel/sys/properties.h>
#include "proplib_compat.h"

typedef struct prop_symbol       *prop_symbol_t;
typedef struct prop_symbol_table *prop_symbol_table_t;
typedef struct prop_symbol_entry prop_symbol_entry_t;

struct prop_symbol_table {
    propdb_t					pst_db;
    struct prop_object			pst_obj;
    RB_ENTRY() pst_link;
    size_t	            		pst_size;
    int                 		pst_type;
};

struct prop_symbol_entry {
    prop_symbol_table_t 		pse_key;
    opaque_t            		pse_objref;
};

struct prop_symbol {
    propdb_t			      	ps_db;
    struct prop_object	      	ps_obj;
    struct prop_symbol_entry  	*ps_array;

	unsigned int			ps_capacity;
	unsigned int			ps_count;
	int						ps_flags;
	uint32_t				ps_version;
};

static const struct prop_object_type prop_object_type_symbol = {
        .pot_type		=  PROP_TYPE_SYMBOL,
        .pot_name       =  PROP_NAME_SYMBOL,
        .pot_db_set     =  prop_db_symbol_set,
        .pot_db_get     =  prop_db_symbol_get,
        .pot_db_delete  =  prop_db_symbol_delete,
};



prop_symbol_t
prop_symbol_alloc(u_int capacity)
{
	prop_symbol_t ps;
	opaque_t *symbol;

	if (capacity != 0) {
		symbol = _PROP_CALLOC(capacity, sizeof(opaque_t), M_PROP_ARRAY);
		if (symbol == NULL) {
			return (NULL);
		}
	} else {
		symbol = NULL;
	}

	ps = _PROP_MALLOC(sizeof(prop_array_t), M_PROP_ARRAY);
	if (ps != NULL) {
		ps->ps_db = proplib_create(ps, &ps->ps_obj, &prop_object_type_symbol);
		ps->ps_array = symbol;
		ps->ps_capacity = capacity;
		ps->ps_count = 0;
		ps->ps_flags = 0;
		ps->ps_version = 0;
	} else if (symbol != NULL) {
		_PROP_FREE(symbol, M_PROP_ARRAY);
	}
	return (ps);
}

static _prop_object_free_rv_t
prop_symbol_free(propdb_t db, opaque_t *obj)
{
	prop_symbol_t ps = *obj;
	opaque_t	 po;

	_PROP_ASSERT(ps->ps_count <= ps->ps_capacity);
	_PROP_ASSERT((ps->ps_capacity == 0 && ps->ps_array == NULL) ||
		     (ps->ps_capacity != 0 && ps->ps_array != NULL));

	if (ps->ps_count == 0) {
		if (ps->ps_array != NULL) {
			propdb_delete(db, ps->ps_array, &prop_object_type_symbol.pot_name);
			_PROP_FREE(ps->ps_array, M_PROP_ARRAY);
		}

		_PROP_FREE(ps, M_PROP_ARRAY);
		return (_PROP_OBJECT_FREE_DONE);
	}
	po = ps->ps_array[ps->ps_count - 1];
	_PROP_ASSERT(po != NULL);

	if (db == NULL) {
		*obj = po;
		return (_PROP_OBJECT_FREE_FAILED);
	}

	--ps->ps_count;
	*obj = po;
	return (_PROP_OBJECT_FREE_RECURSE);
}

prop_symbol_lookup(prop_symbol_t ps, )
{
	struct prop_symbol_entry *pse;
	unsigned int base, idx, distance;

	for (idx = 0, base = 0, distance = ps->ps_count; distance != 0; distance >>= 1) {
		idx = base + (distance >> 1);
		pse = &ps->ps_array[idx];
	}
}


/* proplib / propdb */
int
prop_db_symbol_set(opaque_t obj)
{
   prop_symbol_t ps;

   ps = (prop_symbol_t)obj;
   return (proplib_set(ps->ps_db, ps, &ps->ps_obj, &prop_object_type_symbol, ps->ps_array, sizeof(ps->ps_array)));
}

int
prop_db_symbol_get(opaque_t obj)
{
   prop_symbol_t ps;

   ps = (prop_symbol_t)obj;
   return (proplib_get(ps->ps_db, ps, &ps->ps_obj, &prop_object_type_symbol, ps->ps_array, sizeof(ps->ps_array)));
}

int
prop_db_symbol_delete(opaque_t obj)
{
    prop_symbol_t ps;

    ps = (prop_symbol_t)obj;
    return (proplib_delete(ps->ps_db, &ps->ps_obj, &prop_object_type_symbol));
}
