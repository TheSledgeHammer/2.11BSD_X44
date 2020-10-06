/*
 * koverlay.h
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 *	3-Clause BSD License
 */

/*	koverlay:
 * 	-
 * 	- Should be changed ovlspace allocation
 * 		- as to access and allocate a portion of physical memory
 * 		- ovlspace can be split
 */
/* Current Kernel Overlays:
 * - Akin to a more advanced & versatile overlay from original 2.11BSD
 * - Allocates memory from kernelspace (thus vm allocation)
 * - While useful (in some situations), for extending memory management
 * - This version would not provide any benefit to vm paging, if the overlay is a page in itself.
 * - would likely be slower than using just paging.
 */

#ifndef SYS_KOVERLAY_H_
#define SYS_KOVERLAY_H_


#include <sys/queue.h>

struct ovlstats {
	long			os_inuse;			/* # of packets of this type currently in use */
	long			os_calls;			/* total packets of this type ever allocated */
	long 			os_memuse;			/* total memory held in bytes */
	u_short			os_limblocks;		/* number of times blocked for hitting limit */
	u_short			os_mapblocks;		/* number of times blocked for kernel map */
	long			os_maxused;			/* maximum number ever used */
	long			os_limit;			/* most that are allowed to exist */
	long			os_size;			/* sizes of this thing that are allocated */
	long			os_spare;

	int 			slotsfilled;
	int 			slotsfree;
	int 			nslots;
};

struct ovlusage {
	short 			ou_indx;			/* bucket index */
	u_short 		ou_kovlcnt;			/* kernel overlay count */
	u_short 		ou_vovlcnt;			/* vm overlay count */
	u_short			ou_bucketcnt;		/* buckets */
};

struct ovlbuckets {
	caddr_t 		ob_next;			/* list of free blocks */
	caddr_t 		ob_last;			/* last free block */

	long			ob_calls;			/* total calls to allocate this size */
	long			ob_total;			/* total number of blocks allocated */
	long			ob_totalfree;		/* # of free elements in this bucket */
	long			ob_elmpercl;		/* # of elements in this sized allocation */
	long			ob_highwat;			/* high water mark */
	long			ob_couldfree;		/* over high water mark and could free */
};

struct overlay {
	caddr_t 		ot_next;			/* list of free blocks */
	caddr_t 		ot_last;			/* last free block */

	struct tbtree	*ot_tbtree;			/* tertiary buddy tree allocation */
};

#define ovlmemxtob(base, alloc)	((base) + (alloc) * NBPG)
#define btoovlmemx(addr, base)	(((char)(addr) - (base)) / NBPG)
#define btooup(addr, base)		(&ovlusage[((char)(addr) - (base)) >> CLSHIFT])

/* Overlay Flags */
#define OVL_ALLOCATED  (1 < 0) 							/* kernel overlay region allocated */

extern struct ovlstats 	ovlstats[];
extern struct ovlusage 	*ovlusage;
extern char 			*kovlbase;
extern struct overlay 	bucket[];

#endif /* SYS_KOVERLAY_H_ */
