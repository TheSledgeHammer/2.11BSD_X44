/*
 * pool.h
 *
 *  Created on: 4 Apr 2020
 *      Author: marti
 */

#ifndef SYS_POOL_H_
#define SYS_POOL_H_

#include "../kern/memory/malloc2.h"

/* Implements the infrastructure for slabs & pool */
/* table = buckets, tert tree */
struct kmem_entry {
	struct kmemtable 		ke_tble;
	struct kmempool_entry 	ke_pool;
};

LIST_HEAD(kple_list, kmempool_entry) pool_head;
struct kmempool_entry {
	LIST_ENTRY(kmempool_entry) 	kple_entry;						/* Pool Entries */
	struct kmempools 			kple_pool;


	int							slotsfree;						/* slots free */
	int							slotsfilled;					/* slots filled */
	int 						nslots; 						/* number of slots: (total size (a zone) / block size) example: the zone 4096 ((4096 / 4) = 1024 slots) */
};

struct kmempools
{
	struct kple_list		kpl_head;

	unsigned long			maxsize;
	unsigned long			cursize;
	unsigned long			curfree;
	unsigned long			curalloc;

	unsigned long			minarena;						/* smallest size of new arena */
	unsigned long			quantum;						/* allocated blocks should be multiple of */
	unsigned long			minblock;						/* smallest newly allocated block */
};

extern struct kmempool_entry  pool[];

#endif /* SYS_POOL_H_ */
