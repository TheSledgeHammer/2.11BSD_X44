/*
 * prop_object.h
 *
 *  Created on: 18 Mar 2023
 *      Author: marti
 */

#ifndef SYS_DEVEL_PROPLIB_PROP_OBJECT_H_
#define SYS_DEVEL_PROPLIB_PROP_OBJECT_H_

typedef enum {
	_PROP_OBJECT_FREE_DONE,
	_PROP_OBJECT_FREE_RECURSE,
	_PROP_OBJECT_FREE_FAILED
} _prop_object_free_rv_t;

struct prop_object_type {
	/* type indicator */
	uint32_t						pot_type;

	_prop_object_free_rv_t 			(*pot_free)(opaque_t *);
	void							(*pot_emergency_free)(opaque_t);
};

struct prop_object {
//	const struct prop_object_type 	*po_type;
	//propdb_t						po_db;
	opaque_t 						po_obj;
	const char 						*po_name;
	void 							*po_value;
	size_t							po_length;

	uint32_t						po_type;
	uint32_t						po_refcnt;		/* reference count */
};


void prop_object_init(struct prop_object *, propdb_t, opaque_t, void *val, size_t len);
int	 prop_object_type(opaque_t);
void prop_object_retain(opaque_t);
void prop_object_release(opaque_t);

int	prop_object_set(propdb_t, struct prop_object *);

#endif /* SYS_DEVEL_PROPLIB_PROP_OBJECT_H_ */
