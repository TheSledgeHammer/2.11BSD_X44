/*
 * prop_dictionary.h
 *
 *  Created on: 17 Mar 2023
 *      Author: marti
 */

#ifndef _PROPLIB_PROP_DICTIONARY_H_
#define _PROPLIB_PROP_DICTIONARY_H_

#include <sys/cdefs.h>

typedef struct prop_dictionary *prop_dictionary_t;

__BEGIN_DECLS
prop_dictionary_t 	prop_dictionary_create(const char *);
prop_dictionary_t 	prop_dictionary_create_with_capacity(const char *, u_int);
int 				prop_dictionary_set(prop_dictionary_t, prop_object_t, const char *, int);
size_t				prop_dictionary_get(prop_dictionary_t, prop_object_t, const char *name);
__END_DECLS

#endif /* _PROPLIB_PROP_DICTIONARY_H_ */
