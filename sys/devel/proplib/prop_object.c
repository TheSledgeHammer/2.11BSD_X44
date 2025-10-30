/*
 * prop_object.c
 *
 *  Created on: 18 Mar 2023
 *      Author: marti
 */
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
