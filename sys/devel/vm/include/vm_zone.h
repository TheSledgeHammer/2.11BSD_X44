/*
 * vm_zone.h
 *
 *  Created on: 15 Nov 2021
 *      Author: marti
 */

#ifndef _VM_ZONE_H_
#define _VM_ZONE_H_

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>

#include <devel/vm/include/vm.h>

#define ZONE_INTERRUPT 0x0001	/* If you need to allocate at int time */
#define ZONE_PANICFAIL 0x0002	/* panic if the zalloc fails */
#define ZONE_BOOT      0x0010	/* Internal flag used by zbootinit */

#define ZONE_VM      	0x0100
#define ZONE_OVL    	0x0200

struct zlist;
LIST_HEAD(zlist, vm_zone);
struct vm_zone {
	simple_lock_data_t	zlock;		/* lock for data structure */
	void				*zitems;	/* linked list of items */
	long				zfreecnt;	/* free entries */
	long				zfreemin;	/* minimum number of free entries */
	long				znalloc;	/* allocations from global */

	vm_offset_t			zkva;		/* Base kva of zone */
	vm_offset_t			zova;		/* Base ova of zone */

	vm_object_t			zvobj;		/* vm object */
	ovl_object_t		zoobj;		/* overlay object */

	long				zpagecount;	/* Total # of allocated pages */
	long				zpagemax;	/* Max address space */
	long				zmax;		/* Max number of entries allocated */
	long				zmax_pcpu;	/* Max pcpu cache */
	long				ztotal;		/* Total entries allocated now */
	long				zsize;		/* size of each entry */
	long				zalloc;		/* hint for # of pages to alloc */
	long				zflags;		/* flags for zone */
	uint32_t			zallocflag;	/* flag for allocation */
	char				*zname;		/* name for diags */
	LIST_ENTRY(vm_zone) zlink;		/* link in zlist */
};
typedef struct vm_zone *vm_zone_t;

extern struct zlist zlist;

#endif /* _VM_ZONE_H_ */
