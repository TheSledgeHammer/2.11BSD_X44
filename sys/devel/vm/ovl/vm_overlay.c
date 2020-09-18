/*
 * vm_overlay.c
 *
 *  Created on: 18 Sep 2020
 *      Author: marti
 */
#include <ovl.h>

struct kern_overlay {
	ovl_map_t 		ovl_map;
	ovl_map_entry_t ovl_entry;
	ovl_object_t 	ovl_object;

	vm_offset_t		phys_addr;
};

struct vm_overlay {
	ovl_map_t 		ovl_map;
	ovl_map_entry_t ovl_entry;
	ovl_object_t 	ovl_object;

	vm_offset_t		phys_addr;
};

ovl_insert(map, entry, object)
	ovl_map_t map;
	ovl_map_entry_t entry;
	ovl_object_t object;
{
	int error;
	if(map->ovl_is_kern_overlay && map->ovl_nkovl < NKOVLE) {

	}

	/* create new map entry */
	entry = ovl_map_entry_create(map);

	object = entry->ovle_object;

	/* check/ find space to insert  */
	error = ovl_map_find(map, object, offset, addr, length, find_space);

	ovl_map_reference(map); /* increase ref count */
}


//ovl_object -> vm_object/page or avm_object
