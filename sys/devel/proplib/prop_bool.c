/*
 * prop_bool.c
 *
 *  Created on: 18 Mar 2023
 *      Author: marti
 */

#include <devel/sys/properties.h>

#include "proplib_compat.h"
#include "prop_object.h"

struct prop_bool {
	propdb_t				pb_db;
	struct prop_object		pb_obj;
	bool_t					pb_value;
	size_t					pb_len;
};

static struct prop_bool _prop_bool_true;
static struct prop_bool _prop_bool_false;

struct prop_object_type _prop_object_type_bool = {

};

bool_t
prop_object_is_bool(prop_bool_t pb)
{
	if (pb != NULL && pb->pb_obj.po_type == &_prop_object_type_bool){
		return (TRUE);
	}
	return (FALSE);
}

/* ARGSUSED */
static _prop_object_free_rv_t
prop_bool_free(opaque_t *obj)
{
	/*
	 * This should never happen as we "leak" our initial reference
	 * count.
	 */

	/* XXX forced assertion failure? */
	return (_PROP_OBJECT_FREE_DONE);
}

static int
prop_bool_init(void)
{
	prop_object_init(&_prop_bool_true.pb_obj, &_prop_object_type_bool);
	_prop_bool_true.pb_value = TRUE;

	_prop_object_init(&_prop_bool_false.pb_obj, &_prop_object_type_bool);
	_prop_bool_false.pb_value = FALSE;

	return (0);
}

prop_bool_t
prop_bool_alloc(bool_t val)
{
	prop_bool_t pb;

	pb = val ? &_prop_bool_true : &_prop_bool_false;
	prop_object_retain(pb);

	return (pb);
}

/*
 * prop_bool_copy --
 *	Copy a prop_bool_t.
 */
prop_bool_t
prop_bool_copy(prop_bool_t opb)
{

	if (! prop_object_is_bool(opb)) {
		return (NULL);
	}

	/*
	 * Because we only ever allocate one true and one false, this
	 * can be reduced to a simple retain operation.
	 */
	prop_object_retain(opb);
	return (opb);
}

/*
 * prop_bool_true --
 *	Get the value of a prop_bool_t.
 */
bool_t
prop_bool_true(prop_bool_t pb)
{
	if (!prop_object_is_bool(pb)) {
		return (false);
	}

	return (pb->pb_value);
}

bool_t
prop_bool_equals(prop_bool_t b1, prop_bool_t b2)
{
	if (!prop_object_is_bool(b1) || !prop_object_is_bool(b2)) {
		return (false);
	}

	return (prop_object_equals(b1, b2));
}
