/*
 * pool.c
 *
 *  Created on: 4 Apr 2020
 *      Author: marti
 */

#include <stdlib.h>
#include "../kern/memory/malloc2.h"
#include "../kern/memory/pool.h"

struct kmempool_entry pool[MINBUCKET + 16];
int nslots = 0;									/* number of slots taken in pool */

void
kmempool_init()
{

}

void
kmempool_entry_setup(indx)
	long indx;
{
	register struct kmempool_entry *kple = (struct kmempool_entry *) &pool_head;
	kple->kple_pool[indx];
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

int
slots_filled(blk_size)
	unsigned long blk_size;
{
	if(blk_size != 0) {
		nslots++;
	}
	return (nslots);
}

unsigned long
slots_free(zone_size, blk_size)
	unsigned long zone_size, blk_size;
{
	unsigned long total = slots_total(zone_size, blk_size);
	unsigned long filled = slots_filled(blk_size);
	return (total - filled);
}


slots_alloc(size)
{
	return (size);
}
