/*
 * pool.c
 *
 *  Created on: 4 Apr 2020
 *      Author: marti
 */


#include <pool.h>
#include <malloc2.h>
#include <stdlib.h>

struct kmempool_entry pool[MINBUCKET + 16];

void
kmempool_init()
{
	LIST_INIT(&pool_head);
	&pool = table_zone;
}

void
kmempool_entry_setup(indx)
	long indx;
{
	register struct kmempool_entry *kple = (struct kmempool_entry *) &pool_head;
	kple->kple_pool[indx] = table_zone[indx];
}

struct kmempools *
kmempool_create(kple, indx)
	struct kmempool_entry *kple;
	long indx;
{
	return (kple->kple_pool[indx]);
}

unsigned long
slots_total(zone_size, blk_size)
	unsigned long zone_size, blk_size;
{
	return (zone_size / blk_size);
}

unsigned long
slots_filled()
{

}

unsigned long
slots_free(zone_size, blk_size)
	unsigned long zone_size, blk_size;
{
	unsigned long total = slots_total(zone_size, blk_size);
	unsigned long filled = slots_filled();
	return (total - filled);
}

slots_alloc(size)
{

}
