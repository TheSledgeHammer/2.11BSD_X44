/*
 * koverlay.h
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 *	3-Clause BSD License
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

/* TODO: Improve algorithm for number of overlays */
#define	NOVL	(32)			/* number of kernel overlays */
#define NOVLSR 	(NOVL/2)		/* maximum mumber of kernel overlay sub-regions */
#define NOVLPSR	NOVLSR			/* XXX: number of overlays per sub-region */

struct koverlay {
	char 								*kovl_name;			/* overlay name */
	struct extent 						*kovl_extent;		/* overlay extent allocation */
	u_long 								kovl_start;			/* start of region */
	u_long 								kovl_end;			/* end of region */
	caddr_t								kovl_addr;			/* kernel overlay region Address */
	size_t								kovl_size;			/* size of overlay region */
	int									kovl_flags;			/* overlay flags */
	int									kovl_regcnt;		/* number of regions allocated */
	int									kovl_sregcnt;		/* number of sub-regions allocated */
};

//	RB_HEAD(kovlrb, koverlay_region)	kovl_rbtree;
/*
struct koverlay_region {
	char								*kovlr_name;
	u_long 								kovlr_start;
	u_long								kovlr_end;
	RB_ENTRY(koverlay_region)			kovl_region;
	caddr_t								kovlr_addr;
	size_t								kovlr_size;
	int									kovlr_flags;
};
*/

/* Overlay Flags */
#define KOVL_ALLOCATED  (1 < 0) 							/* kernel overlay region allocated */

extern struct koverlay 	*koverlay_extent_create(kovl, name, start, end, addr, size);
extern void				koverlay_destroy(kovl);
extern int				koverlay_extent_alloc_region(kovl, size);
extern int				koverlay_extent_alloc_subregion(kovl, name, start, end, size, alignment, boundary, result);
extern void				koverlay_free(kovl, start, size);

#endif /* SYS_KOVERLAY_H_ */
