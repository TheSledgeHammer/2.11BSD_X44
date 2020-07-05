/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)kern_overlay.c	1.00
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include "vm/ovl/koverlay.h"

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
