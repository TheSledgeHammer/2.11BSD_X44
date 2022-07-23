/*	$NetBSD: machdep.c,v 1.552.2.3 2004/08/16 17:46:05 jmc Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/rtc.h>

#include <arch/i386/include/bios.h>

extern int biosbasemem, biosextmem;

#define	VM_PHYSSEG_MAX	32

/*
 * Description of a memory segment. To make this suitable for sharing
 * between all architectures, u_quad_t seems to be the necessary type...
 */
typedef struct {
	u_quad_t	start;		/* Physical start address	*/
	u_quad_t	size;		/* Size in bytes		*/
} phys_ram_seg_t;

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int	mem_cluster_cnt;


#define	KBTOB(x)	((size_t)(x) * 1024UL)

int
add_mem_cluster(u_int64_t seg_start, u_int64_t seg_end, u_int32_t type)
{
	extern struct extent *iomem_ex;
	int error, i;

	if (seg_end > 0x100000000ULL) {
		printf("WARNING: skipping large "
				"memory map entry: "
				"0x%qx/0x%qx/0x%x\n", seg_start, (seg_end - seg_start), type);
		return (0);
	}

	if (seg_end == 0x100000000ULL) {
		seg_end -= PAGE_SIZE;
	}

	if (seg_end <= seg_start) {
		return (0);
	}

	for (i = 0; i < mem_cluster_cnt; i++) {
		if ((mem_clusters[i].start == round_page(seg_start)) && (mem_clusters[i].size == trunc_page(seg_end) - mem_clusters[i].start)) {
			return (0);
		}
	}

	/*
	 * Allocate the physical addresses used by RAM
	 * from the iomem extent map.  This is done before
	 * the addresses are page rounded just to make
	 * sure we get them all.
	 */
	error = extent_alloc_region(iomem_ex, seg_start, seg_end - seg_start, EX_NOWAIT);
	if (error) {
		printf("WARNING: CAN'T ALLOCATE "
				"MEMORY SEGMENT "
				"(0x%qx/0x%qx/0x%x) FROM "
				"IOMEM EXTENT MAP!\n", seg_start, seg_end - seg_start, type);
		return (0);
	}

	if (mem_cluster_cnt >= VM_PHYSSEG_MAX) {
		panic("init386: too many memory segments");
	}

	seg_start = round_page(seg_start);
	seg_end = trunc_page(seg_end);

	if (seg_start == seg_end) {
		return (0);
	}
	mem_clusters[mem_cluster_cnt].start = seg_start;
	mem_clusters[mem_cluster_cnt].size = seg_end - seg_start;

	physmem += atop(mem_clusters[mem_cluster_cnt].size);
	mem_cluster_cnt++;

	return (1);
}

void
alloc_ap_trampoline(u_int64_t seg_start, u_int64_t seg_end)
{
	unsigned int i;
	u_int64_t seg_size;
	bool allocated;

	seg_size = seg_end - seg_start;
	allocated = TRUE;
	if((seg_size >= MiB(1)) || (trunc_page(seg_end) - round_page(seg_start) < round_page(bootMP_size))) {
		allocated = TRUE;

		if(seg_end < MiB(1)) {
			boot_address = trunc_page(seg_end);
			if ((seg_end - boot_address) < bootMP_size) {
				boot_address -= round_page(bootMP_size);
			}
			seg_end = boot_address;
		} else {
			boot_address = round_page(seg_start);
			seg_start = boot_address + round_page(bootMP_size);
		}
	}
	if (!allocated) {
		boot_address = biosbasemem * 1024 - bootMP_size;
		if (bootverbose) {
			printf("Cannot find enough space for the boot trampoline, placing it at %#x", boot_address);
		}
	}
}

static int
add_smap_entry(struct bios_smap *smap)
{
	if (boothowto & RB_VERBOSE) {
		printf("SMAP type=%02x base=%016llx len=%016llx\n", smap->type,
				smap->base, smap->length);
	}

	if (smap->type != SMAP_TYPE_MEMORY) {
		return (1);
	}

	return (add_mem_cluster(smap->base, smap->length, smap->type));
}

static void
add_smap_entries(struct bios_smap *smapbase)
{
	struct bios_smap *smap, *smapend;
	u_int32_t smapsize;
	/*
	 * Memory map from INT 15:E820.
	 *
	 * subr_module.c says:
	 * "Consumer may safely assume that size value precedes data."
	 * ie: an int32_t immediately precedes SMAP.
	 */
	smapsize = *((u_int32_t *)smapbase - 1);
	smapend = (struct bios_smap *)((uintptr_t)smapbase + smapsize);

	for (smap = smapbase; smap < smapend; smap++) {
		if (!add_smap_entry(smap)) {
			break;
		}
	}
}

int
has_smapbase(smapbase)
	struct bios_smap *smapbase;
{
	int has_smap;

	has_smap = 0;
	if (smapbase != NULL) {
		add_smap_entries(smapbase);
		has_smap = 1;
		return (has_smap);
	}
	return (has_smap);
}
