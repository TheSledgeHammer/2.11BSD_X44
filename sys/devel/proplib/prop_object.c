/*
 * prop_object.c
 *
 *  Created on: 18 Mar 2023
 *      Author: marti
 */
#include <sys/cdefs.h>
#include <sys/stddef.h>
#include <sys/atomic.h>
#include <sys/malloc.h>

#include <devel/sys/properties.h>

#include "proplib_compat.h"
#include "prop_object.h"

#define atomic_inc_32_nv(x)		(atomic_add_int_nv(x, 1)+1)
#define atomic_dec_32_nv(x)		(atomic_sub_int_nv(x, -1)-1)
#define atomic_inc_32(x)		atomic_add_int(x, 1)
#define atomic_dec_32(x)		atomic_add_int(x, 1)

void
prop_object_init(struct prop_object *po, const struct prop_object_type *pot)
{
	po->po_type = pot;
	po->po_refcnt = 1;
}

void
prop_object_retain(opaque_t obj)
{
	struct prop_object *po;
	uint32_t ncnt;

	po = obj;
	ncnt = atomic_inc_32_nv(&po->po_refcnt);
	KASSERT(ncnt != 0);
}

static void
prop_object_release_emergency(opaque_t obj)
{
	struct prop_object *po;
	opaque_t parent = NULL;
	uint32_t ocnt;

	for (;;) {
		po = obj;
		KASSERT(obj);

		ocnt = atomic_dec_32_nv(&po->po_refcnt);
		ocnt++;
		KASSERT(ocnt != 0);

		if (ocnt != 1) {
			break;
		}
		KASSERT(po->po_type);
		if ((po->po_type->pot_free)(&obj) == _PROP_OBJECT_FREE_DONE) {
			break;
		}
		parent = po;
		atomic_inc_32(&po->po_refcnt);
	}
	KASSERT(parent);
	/* One object was just freed. */
	po = parent;
	(*po->po_type->pot_emergency_free)(parent);
}

void
prop_object_release(opaque_t obj)
{
	struct prop_object *po;
	int ret;
	uint32_t ocnt;

	do {
		po = obj;
		KASSERT(obj);

		ocnt = atomic_dec_32_nv(&po->po_refcnt);
		ocnt++;
		KASSERT(ocnt != 0);

		if (ocnt != 1) {
			ret = 0;
			break;
		}

		ret = (po->po_type->pot_free)(&obj);
		if (ret == _PROP_OBJECT_FREE_DONE) {
			break;
		}
		atomic_inc_32(&po->po_refcnt);
	} while (ret == _PROP_OBJECT_FREE_RECURSE);

	if (ret == _PROP_OBJECT_FREE_FAILED) {
		prop_object_release_emergency(obj);
	}
}

int
prop_object_type(opaque_t obj)
{
	struct prop_object *po;

	po = (struct prop_object *)obj;
	if (obj == NULL || propdb_type(obj)) {
		return (PROP_TYPE_UNKNOWN);
	}
	return (po->po_type->pot_type);
}

opaque_t
prop_object_iterator_next(prop_object_iterator_t pi)
{
	return ((*pi->pi_next_object)(pi));
}

void
prop_object_iterator_reset(prop_object_iterator_t pi)
{
	(*pi->pi_reset)(pi);
}

void
prop_object_iterator_release(prop_object_iterator_t pi)
{
	prop_object_release(pi->pi_obj);
	_PROP_FREE(pi, M_TEMP);
}

/*
 * propdb object
 */

struct propdb_object_type {
	const char 	*pot_name;
	uint32_t	pot_type;
	int			(*pot_db_set)(opaque_t);
	int			(*pot_db_get)(opaque_t);
	int			(*pot_db_delete)(opaque_t);
};

struct propdb_object {
	const struct propdb_object_type *po_type;
	opaque_t 						po_obj;
	uint32_t						po_refcnt;		/* reference count */
};

typedef struct propdb_object_type 	propdb_object_type_t;
typedef struct propdb_object 		propdb_object_t;

/* propdb */
propdb_t propdb_object_create(opaque_t, struct prop_object *, const struct prop_object_type *);
int		propdb_object_set(propdb_t, opaque_t, struct prop_object *, const struct prop_object_type *, void *, size_t);
int		propdb_object_delete(propdb_t, opaque_t, struct prop_object *, struct prop_object_type *);
size_t	propdb_object_get(propdb_t, opaque_t, struct prop_object *, struct prop_object_type *, void *, size_t);
size_t 	propdb_object_objs(propdb_t, struct prop_object *, size_t);
size_t 	propdb_object_list(propdb_t, struct prop_object *, struct prop_object_type *, size_t);
int		propdb_object_copy(propdb_t, struct prop_object *, struct prop_object *);

propdb_t
propdb_object_create(opaque_t obj, struct prop_object *po, const struct prop_object_type *type)
{
	propdb_t db;

	db = propdb_create(type->pot_name);
	po->po_type = type;
	po->po_obj = obj;
	po->po_refcnt = 1;
	return (db);
}

int
propdb_object_set(propdb_t db, opaque_t obj, struct prop_object *po, const struct prop_object_type *type, void *val, size_t len)
{
	if ((obj != NULL) && (po != NULL)) {
		return (propdb_set(db, obj, type->pot_name, val, len, type->pot_type, M_WAITOK));
	}
	return (-1);
}

int
propdb_object_delete(propdb_t db, opaque_t obj, struct prop_object *po, struct prop_object_type *type)
{
	if ((obj != NULL) && (po != NULL)) {
		return (propdb_delete(db, obj, type->pot_name));
	}
	return (-1);
}

size_t
propdb_object_get(propdb_t db, opaque_t obj, struct prop_object *po, struct prop_object_type *type, void *val, size_t len)
{
	if ((obj != NULL) && (po != NULL)) {
		return (propdb_get(db, obj, type->pot_name, val, len, type->pot_type));
	}
	return (-1);
}

size_t
propdb_object_objs(propdb_t db, struct prop_object *po, size_t len)
{
	return (propdb_objs(db, &po->po_obj, len));
}

size_t
propdb_object_list(propdb_t db, struct prop_object *po, struct prop_object_type *type, size_t len)
{
	return (propdb_list(db, po->po_obj, type->pot_name, len));
}

int
propdb_object_copy(propdb_t db, struct prop_object *source, struct prop_object *dest)
{
	return (propdb_copy(db, source->po_obj, dest->po_obj, M_WAITOK));
}
