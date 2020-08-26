/*
 * ovl_object.h
 *
 *  Created on: 8 Jun 2020
 *      Author: marti
 */

#ifndef _OVL_OBJECT_H_
#define _OVL_OBJECT_H_

#include <sys/queue.h>

struct ovl_object {
	TAILQ_ENTRY(ovl_object)				object_list;	/* list of all objects */
	u_short								flags;			/* see below */

	simple_lock_data_t					lock;			/* Synchronization */
	int									ref_count;		/* How many refs?? */
	vm_size_t							size;			/* Object size */
};

TAILQ_HEAD(ovl_object_hash_head, ovl_object_hash_entry);
struct ovl_object_hash_entry {
	TAILQ_ENTRY(ovl_object_hash_entry)  hash_links;		/* hash chain links */
	ovl_object_t						object;
};

struct object_q		vm_object_cached_list;	/* list of objects persisting */
int					vm_object_cached;		/* size of cached list */
simple_lock_data_t	vm_cache_lock;			/* lock for object cache */

struct object_q		vm_object_list;			/* list of allocated objects */
long				vm_object_count;		/* count of all objects */
simple_lock_data_t	vm_object_list_lock;
											/* lock for object list and count */

ovl_object_t		kern_ovl_object;			/* the single kernel overlay object */
ovl_object_t		vm_ovl_object;

#endif /* _OVL_OBJECT_H_ */
