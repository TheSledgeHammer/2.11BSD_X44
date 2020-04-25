
/*
 *
 * kern_overlay.c
 *
 *  Created on: 25 Apr 2020
 *      Author: marti
 *  3-Clause BSD License
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include "ovl/koverlay.h"

//static size_t koverlay_ex_storage[EXTENT_FIXED_STORAGE_SIZE(NOVLSR)];
//koverlay_ex_storage = extent_create(kovl->kovl_name, kovl->kovl_start, kovl->kovl_end, (void *)koverlay_ex_storage, sizeof(koverlay_ex_storage), EX_NOCOALESCE | EX_NOWAIT);

/* Kernel Overlay Memory Management */

/* Allocate a region of memory for Kernel Overlay's */
struct koverlay *
koverlay_extent_create(kovl, start, end, addr, size)
	struct koverlay *kovl;
	u_long start, end;
	caddr_t addr;
	u_long size;
{
	if (kovl == NULL) {
		kovl = malloc(sizeof(*kovl), M_KOVL, M_NOWAIT);
		kovl->kovl_flags = KOVL_ALLOCATED;
	} else {
		memset(kovl, 0, sizeof(*kovl));
	}

	koverlay_init(kovl, start, end, addr, size);

	if (size == 0) {
		kovl->kovl_addr = addr;
	} else {
		kovl->kovl_extent = extent_create(kovl->kovl_name, start, end, M_KOVL, addr, size, EX_NOWAIT | EX_MALLOCOK);
		if(kovl->kovl_extent == NULL) {
			panic("%s:: unable to create kernel overlay space for ""0x%08lx-%#lx", __func__, addr, size);
		}
	}

	return (kovl);
}

static void
koverlay_init(kovl, start, end, addr, size)
	register struct koverlay *kovl;
	u_long start, end;
	caddr_t addr;
	u_long size;
{
	kovl->kovl_name = "kern_overlay";
	kovl->kovl_start = start;
	kovl->kovl_end = end;
	kovl->kovl_addr = addr;
	kovl->kovl_size = size;
	kovl->kovl_regcnt = 0;
	kovl->kovl_sregcnt = 0;
}

/* Destroy a kernel overlay region */
void
koverlay_destroy(kovl)
	struct koverlay *kovl;
{
	struct extent *ex = kovl->kovl_extent;

	if(ex != NULL) {
		extent_destroy(ex);
	}

	if(kovl->kovl_flags & KOVL_ALLOCATED) {
		free(kovl, M_KOVL);
	}
}

/* Allocate a region for kernel overlay */
int
koverlay_extent_alloc_region(kovl, size)
	struct koverlay *kovl;
	u_long size;
{
	struct extent *ex = kovl->kovl_extent;
	int error;

	if(ex == NULL) {
		return (0);
	}
	if(NOVL <= 1) {
		NOVL = 1;
	}
	if(kovl->kovl_regcnt < NOVL) {
		kovl->kovl_addr += ex->ex_start;
		error = extent_alloc_region(ex, kovl->kovl_addr, size, EX_NOWAIT | EX_MALLOCOK);
	} else {
		printf("No more overlay regions currently available");
	}

	if(error) {
		return (error);
	}

	kovl->kovl_regcnt++;
	return (0);
}

int
koverlay_extent_alloc_subregion(kovl, name, start, end, size, alignment, boundary, result)
	struct koverlay *kovl;
	char *name;
	u_long start, end;
	u_long size, alignment, boundary;
	u_long *result;
{
	struct extent *ex = kovl->kovl_extent;
	int error;

	if(NOVL == 1) {
		NOVLSR = 1;
	}

	if(kovl->kovl_regcnt < NOVLSR) {
		if(start > kovl->kovl_start && start < kovl->kovl_end) {
			if(end > start) {
				error = extent_alloc_subregion(ex, start, end, size, alignment, boundary, EX_FAST | EX_NOWAIT | EX_MALLOCOK, result);
			} else {
				printf("overlay end address must be greater than the overlay start address");
			}
		} else {
			printf("overlay sub-region outside of kernel overlay allocated space");
		}
	} else {
		printf("No more overlay sub-regions currently available");
	}

	if(error) {
		return (error);
	}

	kovl->kovl_sregcnt++;
	return (0);
}

void
koverlay_free(kovl, start, size)
	struct koverlay *kovl;
	u_long start, size;
{
	struct extent *ex = kovl->kovl_extent;
	if(ex != NULL) {
		koverlay_unmap(kovl, start, size);
	}
	kovl->kovl_regcnt--;
}

static void
koverlay_unmap(kovl, start, size)
	struct koverlay *kovl;
	u_long start, size;
{
	struct extent *ex = kovl->kovl_extent;
	int error;
	if(ex == NULL) {
		return;
	}

	error = extent_free(ex, start, size, EX_NOWAIT);

	if (error) {
		printf("%#lx-%#lx of %s space lost\n", start, start + size, ex->ex_name);
	}
}
