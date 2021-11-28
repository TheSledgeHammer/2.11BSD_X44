/*
 * driver.c
 *
 *  Created on: 21 Nov 2021
 *      Author: marti
 */

#define DKR_STRATEGY(dk, bp)					(*((dk)->dk_driver->d_strategy))(dk, bp)
#define DKR_MINPHYS(dk, bp)						(*((dk)->dk_driver->d_minphys))(dk, bp)
#define DKR_OPEN(dk, dev, flags, fmt, p)		(*((dk)->dk_driver->d_close))(dk, dev, flags, fmt, p)
#define DKR_CLOSE(dk, dev, flags, fmt, p)		(*((dk)->dk_driver->d_close))(dk, dev, flags, fmt, p)
#define DKR_IOCTL(dk, dev, cmd, data, flag, p)	(*((dk)->dk_driver->d_ioctl))(dk, dev, cmd, data, flag, p)
#define DKR_DUMP(dk, dev)						(*((dk)->dk_driver->d_dump))(dk, dev)
#define DKR_START(dk, bp)						(*((dk)->dk_driver->d_start))(dk, bp)
#define DKR_MKLABEL(dk)							(*((dk)->dk_driver->d_mklabel))(dk)

void
dkr_strategy(disk, bp)
	struct dkdevice *disk;
	struct buf 		*bp;
{
	struct dkdriver dkr;
	void rv;

	dkr = disk_driver(disk, bp->b_dev);
	if(dkr.d_strategy == NULL) {
		return (EOPNOTSUPP);
	}

	rv = dkr.d_strategy(disk, bp);

	return (rv);
}

void
dkr_minphys(disk, bp)
	struct dkdevice *disk;
	struct buf 		*bp;
{
	struct dkdriver dkr;
	void rv;

	dkr = disk_driver(disk, bp->b_dev);
	if(dkr.d_minphys == NULL) {
		return (EOPNOTSUPP);
	}

	rv = dkr.d_minphys(disk, bp);

	return (rv);
}

int
dkr_open(disk, dev, flags, fmt, p)
	struct dkdevice *disk;
	dev_t 			dev;
	int flags, fmt;
	struct proc *p;
{
	struct dkdriver dkr;
	int rv;

	dkr = disk_driver(disk, dev);
	if(dkr.d_open == NULL) {
		return (EOPNOTSUPP);
	}

	rv = dkr.d_open(dev, flags, fmt, p);

	return (rv);
}

int
dkr_close(disk, dev, flags, fmt, p)
	struct dkdevice *disk;
	dev_t 			dev;
	int flags, fmt;
	struct proc *p;
{
	struct dkdriver dkr;
	int rv;

	dkr = disk_driver(disk, dev);
	if(dkr.d_close == NULL) {
		return (EOPNOTSUPP);
	}

	rv = dkr.d_close(dev, flags, fmt, p);

	return (rv);
}

int
dkr_ioctl(disk, dev, cmd, data, flag, p)
	struct dkdevice *disk;
	dev_t dev;
	u_long cmd;
	void *data;
	int flag;
	struct proc *p;
{
	struct dkdriver dkr;
	int rv;

	dkr = disk_driver(disk, dev);
	if(dkr.d_ioctl == NULL) {
		return (EOPNOTSUPP);
	}

	rv = dkr.d_ioctl(dev, cmd, data, flag, p);

	return (rv);
}

void
dkr_start(disk)
	struct dkdevice *disk;
{
	struct dkdriver dkr;
	void rv;

	dkr = disk->dk_driver;//disk_driver(disk);
	if(dkr.d_start == NULL) {
		return (EOPNOTSUPP);
	}

	rv = dkr.d_start(disk);

	return (rv);
}

int
dkr_dump(disk, dev)
	struct dkdevice *disk;
	dev_t dev;
{
	struct dkdriver dkr;
	int rv;

	dkr = disk_driver(disk, dev);
	if(dkr.d_dump == NULL) {
		return (EOPNOTSUPP);
	}

	rv = dkr.d_dump(disk, dev);

	return (rv);
}

int
dkr_mklabel(disk)
	struct dkdevice *disk;
{
	struct dkdriver dkr;
	int rv;

	dkr = disk_driver(disk);
	if(dkr.d_mklabel == NULL) {
		return (EOPNOTSUPP);
	}

	rv = dkr.d_mklabel(disk);

	return (rv);
}

#define MAX_KMAP	10
#define	MAX_KMAPENT	500
static struct vm_map 		kmap_init[MAX_KMAP];
static struct vm_map_entry 	kentry_init[MAX_KMAPENT];

struct extent *kmap_extent;
struct extent *kentry_extent;

void
vm_map_init()
{
	kmap_extent = extent_create("KMAP", sizeof(kmap_init[0]), sizeof(kmap_init[MAX_KMAP]), M_VMMAP, 0, 0, NULL);
	kentry_extent = extent_create("KENTRY", );

	struct vm_map *kmap;
	struct vm_object *obj;
	int size, totsize, nentries;

	totsize = round_page(kmap->size * nentries);
	size = kmem_alloc_pageable(kernel_map, totsize);
	if (obj == NULL) {
		obj = vm_object_allocate(totsize / PAGE_SIZE);
	} else {
		_vm_object_allocate(totsize / PAGE_SIZE, obj);
	}
}

#include <devel/sys/slab.h>

vmem_hash(slab, size)
	struct slab *slab;
	u_long size;
{
	long hash = &slab_list[BUCKETINDX(size)];
	register struct slab_cache 	*cache;
	register struct slab_meta	*meta;

	cache = &slabCache;
	CIRCLEQ_FOREACH(slab, &cache->sc_head, s_list) {
		if(slab != NULL) {
			meta = slab->s_meta;
			if(meta->sm_bindx == BUCKETINDX(size)) {

			}
		}
	}
}

void
vmem_create(slab, size, mtype)
	struct slab *slab;
	u_long size;
	int mtype;
{
	register struct slab_cache 	*cache;
	register struct slab_meta	*meta;

	int totsize;
	cache = &slabCache;
	CIRCLEQ_FOREACH(slab, &cache->sc_head, s_list) {
		if(slab != NULL) {
			meta = slab->s_meta;
			if((meta->sm_max - meta->sm_min) >= size && slab->s_size == size) {
				vbp->vb_min = meta->sm_min;
			    vbp->vb_max = meta->sm_max;
			    vbp->vb_nentries = meta->sm_max - meta->sm_min;
			}
		} else {
			slab_insert(cache, size, mtype);
			meta = slab->s_meta;
			if((meta->sm_max - meta->sm_min) >= size) {

			}
		}
	}
}
