/*
 * prop_array.c
 *
 *  Created on: 18 Mar 2023
 *      Author: marti
 */

#include <sys/malloc.h>

#include <devel/sys/properties.h>
#include "proplib_compat.h"

struct prop_array {
	propdb_t				pa_db;
	struct prop_object		pa_obj;

	opaque_t 				*pa_array;
	unsigned int			pa_capacity;
	unsigned int			pa_count;
	int						pa_flags;
	uint32_t				pa_version;

	size_t					pa_len;
};

#define PA_F_IMMUTABLE		0x01	/* array is immutable */

static _prop_object_free_rv_t prop_array_free(prop_stack_t, opaque_t *);
static void	prop_array_emergency_free(opaque_t);

const struct prop_object_type prop_object_type_array  = {
		.pot_name 				= 	PROP_NAME_ARRAY,
		.pot_type 				= 	PROP_TYPE_ARRAY,
		.pot_db_set				= 	propdb_array_set,
		.pot_db_get				=	propdb_array_get,
		.pot_free				=	prop_array_free,
		.pot_emergency_free		=	prop_array_emergency_free,
};

#define prop_object_is_array(x)		\
	((x) != NULL && (x)->pa_obj.po_type == &prop_object_type_array)

#define prop_array_is_immutable(x) (((x)->pa_flags & PA_F_IMMUTABLE) != 0)

static _prop_object_free_rv_t
prop_array_free(propdb_t db, opaque_t *obj)
{
	prop_array_t pa = *obj;
	opaque_t	 po;

	_PROP_ASSERT(pa->pa_count <= pa->pa_capacity);
	_PROP_ASSERT((pa->pa_capacity == 0 && pa->pa_array == NULL) ||
		     (pa->pa_capacity != 0 && pa->pa_array != NULL));

	if (pa->pa_count == 0) {
		if (pa->pa_array != NULL) {
			propdb_delete(db, pa->pa_array, &prop_object_type_array.pot_name);
			_PROP_FREE(pa->pa_array, M_PROP_ARRAY);
		}

		_PROP_FREE(pa, M_PROP_ARRAY);
		return (_PROP_OBJECT_FREE_DONE);
	}
	po = pa->pa_array[pa->pa_count - 1];
	_PROP_ASSERT(po != NULL);

	if (db == NULL) {
		*obj = po;
		return (_PROP_OBJECT_FREE_FAILED);
	}

	--pa->pa_count;
	*obj = po;
	return (_PROP_OBJECT_FREE_RECURSE);
}

static void
prop_array_emergency_free(opaque_t obj)
{
	prop_array_t pa = obj;

	_PROP_ASSERT(pa->pa_count != 0);
	--pa->pa_count;
}

prop_array_t
prop_array_alloc(u_int capacity)
{
	propdb_t		db;
	prop_array_t 	pa;
	opaque_t 		*array;

	if (capacity != 0) {
		array = _PROP_CALLOC(capacity, sizeof(opaque_t), M_PROP_ARRAY);
		if (array == NULL) {
			return (NULL);
		}
	} else {
		array = NULL;
	}

	pa = _PROP_MALLOC(sizeof(prop_array_t), M_PROP_ARRAY);
	if (pa != NULL) {
		pa->pa_db = proplib_create(pa, &pa->pa_obj, &prop_object_type_array);
		pa->pa_array = array;
		pa->pa_capacity = capacity;
		pa->pa_count = 0;
		pa->pa_flags = 0;
		pa->pa_version = 0;
	} else if (array != NULL) {
		_PROP_FREE(array, M_PROP_ARRAY);
	}
	return (pa);
}

static bool_t
_prop_array_expand(prop_array_t pa, unsigned int capacity)
{
	opaque_t *array, *oarray;

	/*
	 * Array must be WRITE-LOCKED.
	 */

	oarray = pa->pa_array;

	array = _PROP_CALLOC(capacity, sizeof(*array), M_PROP_ARRAY);
	if (array == NULL) {
		return (false);
	}
	if (oarray != NULL) {
		bcopy(oarray, array, pa->pa_capacity * sizeof(*array));
	}
	pa->pa_array = array;
	pa->pa_capacity = capacity;

	if (oarray != NULL) {
		_PROP_FREE(oarray, M_PROP_ARRAY);
	}

	return (true);
}

prop_array_t
prop_array_create(void)
{
	return (prop_array_alloc(0));
}

prop_array_t
prop_array_create_with_capacity(unsigned int capacity)
{
	return (prop_array_alloc(capacity));
}

prop_array_t
prop_array_copy(prop_array_t opa)
{
	prop_array_t pa;
	opaque_t po;
	int ret;
	unsigned int idx;

	if (! prop_object_is_array(opa))
		return (NULL);

	_PROP_RWLOCK_RDLOCK(opa->pa_rwlock);

	pa = _prop_array_alloc(opa->pa_count);
	if (pa != NULL) {
		for (idx = 0; idx < opa->pa_count; idx++) {
			po = opa->pa_array[idx];
			prop_object_retain(po);
			pa->pa_array[idx] = po;
		}
		pa->pa_count = opa->pa_count;
		pa->pa_flags = opa->pa_flags;
		ret = propdb_copy(pa->pa_db, opa, pa, M_WAITOK);
		if (ret != 0) {
			return (NULL);
		}
	}
	_PROP_RWLOCK_UNLOCK(opa->pa_rwlock);
	return (pa);
}

prop_array_t
prop_array_copy_mutable(prop_array_t opa)
{
	prop_array_t pa;

	pa = prop_array_copy(opa);
	if (pa != NULL)
		pa->pa_flags &= ~PA_F_IMMUTABLE;

	return (pa);
}

opaque_t
prop_array_get(prop_array_t pa, unsigned int idx)
{
	opaque_t po = NULL;

	if (! prop_object_is_array(pa))
		return (NULL);

	_PROP_RWLOCK_RDLOCK(pa->pa_rwlock);
	if (idx >= pa->pa_count)
		goto out;
	po = pa->pa_array[idx];
	_PROP_ASSERT(po != NULL);
 out:
	_PROP_RWLOCK_UNLOCK(pa->pa_rwlock);
	return (po);
}


static bool_t
_prop_array_add(prop_array_t pa, opaque_t po)
{
	/*
	 * Array must be WRITE-LOCKED.
	 */

	_PROP_ASSERT(pa->pa_count <= pa->pa_capacity);

	if (prop_array_is_immutable(pa) ||
	    (pa->pa_count == pa->pa_capacity &&
	    _prop_array_expand(pa, pa->pa_capacity + EXPAND_STEP) == false)) {
		return (false);
	}

	prop_object_retain(po);
	pa->pa_array[pa->pa_count++] = po;
	pa->pa_version++;

	return (true);
}

bool_t
prop_array_set(prop_array_t pa, unsigned int idx, opaque_t po)
{
	opaque_t opo;
	bool_t rv = false;

	if (! prop_object_is_array(pa))
		return (false);

	_PROP_RWLOCK_WRLOCK(pa->pa_rwlock);

	if (prop_array_is_immutable(pa)) {
		goto out;
	}

	if (idx == pa->pa_count) {
		rv = _prop_array_add(pa, po);
		goto out;
	}

	_PROP_ASSERT(idx < pa->pa_count);

	opo = pa->pa_array[idx];
	_PROP_ASSERT(opo != NULL);

	prop_object_retain(po);
	pa->pa_array[idx] = po;
	pa->pa_version++;

	prop_object_release(opo);

	rv = true;

 out:
	_PROP_RWLOCK_UNLOCK(pa->pa_rwlock);
	return (rv);
}

bool_t
prop_array_add(prop_array_t pa, opaque_t po)
{
	bool_t rv;

	if (! prop_object_is_array(pa)) {
		return (false);
	}

	_PROP_RWLOCK_WRLOCK(pa->pa_rwlock);
	rv = _prop_array_add(pa, po);
	_PROP_RWLOCK_UNLOCK(pa->pa_rwlock);

	return (rv);
}

void
prop_array_remove(prop_array_t pa, unsigned int idx)
{
	opaque_t po;

	if (! prop_object_is_array(pa))
		return;

	_PROP_RWLOCK_WRLOCK(pa->pa_rwlock);

	_PROP_ASSERT(idx < pa->pa_count);

	/* XXX Should this be a _PROP_ASSERT()? */
	if (prop_array_is_immutable(pa)) {
		_PROP_RWLOCK_UNLOCK(pa->pa_rwlock);
		return;
	}

	po = pa->pa_array[idx];
	_PROP_ASSERT(po != NULL);

	for (++idx; idx < pa->pa_count; idx++)
		pa->pa_array[idx - 1] = pa->pa_array[idx];
	pa->pa_count--;
	pa->pa_version++;

	_PROP_RWLOCK_UNLOCK(pa->pa_rwlock);

	prop_object_release(po);
}

int
propdb_array_set(opaque_t obj)
{
	prop_array_t pa;

	pa = obj;
	return (proplib_set(pa->pa_db, pa, &pa->pa_obj, &prop_object_type_array, pa->pa_array, sizeof(pa->pa_array)));
}

size_t
propdb_array_get(opaque_t obj)
{
	prop_array_t pa;

	pa = obj;
	return (proplib_get(pa->pa_db, pa, &pa->pa_obj, &prop_object_type_array, pa->pa_array, sizeof(pa->pa_array)));
}
