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
	/* name & type indicator */
	const char 						*pot_name;
	uint32_t						pot_type;

	/* propdb */
	int 							(*pot_db_set)(opaque_t);
	int 							(*pot_db_get)(opaque_t);
	int 							(*pot_db_delete)(opaque_t);

	_prop_object_free_rv_t 			(*pot_free)(propdb_t, opaque_t *);
	void							(*pot_emergency_free)(opaque_t);
};

struct prop_object {
	const struct prop_object_type 	*po_type;
	opaque_t 						po_obj;
	uint32_t						po_refcnt;		/* reference count */
};

struct prop_object_iterator {
	prop_object_t	(*pi_next_object)(void *);
	void			(*pi_reset)(void *);
	prop_object_t	pi_obj;
	uint32_t		pi_version;
};

typedef struct prop_object_iterator *prop_object_iterator_t;

int	 	 prop_object_type(opaque_t);
void 	 prop_object_retain(opaque_t);
void 	 prop_object_release(opaque_t);

prop_object_t prop_object_iterator_next(prop_object_iterator_t);
void prop_object_iterator_reset(prop_object_iterator_t);
void prop_object_iterator_release(prop_object_iterator_t);

#endif /* SYS_DEVEL_PROPLIB_PROP_OBJECT_H_ */
