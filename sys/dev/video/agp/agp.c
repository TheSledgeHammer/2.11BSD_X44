/*	$NetBSD: agp.c,v 1.32 2004/02/13 11:36:22 wiz Exp $	*/

/*-
 * Copyright (c) 2000 Doug Rabson
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD: src/sys/pci/agp.c,v 1.12 2001/05/19 01:28:07 alfred Exp $
 */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agp.c,v 1.32 2004/02/13 11:36:22 wiz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/agpio.h>
#include <sys/proc.h>

#include <vm/include/vm_extern.h>

#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <dev/video/agp/agpvar.h>
#include <dev/video/agp/agpreg.h>
#include <dev/core/pci/pcidevs.h>

#include <machine/bus.h>

/* Helper functions for implementing chipset mini drivers. */
/* XXXfvdl get rid of this one. */

extern struct cfdriver agp_cd;

dev_type_open(agpopen);
dev_type_close(agpclose);
dev_type_ioctl(agpioctl);
dev_type_mmap(agpmmap);

const struct cdevsw agp_cdevsw = {
	.d_open = agpopen,
	.d_close = agpclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = agpioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = agpmmap,
	.d_kqfilter = nokqfilter,
	.d_type = D_OTHER
};

int agpmatch(struct device *, struct cfdata *, void *);
void agpattach(struct device *, struct device *, void *);

CFOPS_DECL(agp, agpmatch, agpattach, NULL, NULL);
CFDRIVER_DECL(NULL, agp, DV_DULL);
CFATTACH_DECL(agp, &agp_cd, &agp_cops, sizeof(struct agp_softc));

static int agp_info_user(struct agp_softc *, agp_info *);
static int agp_setup_user(struct agp_softc *, agp_setup *);
static int agp_allocate_user(struct agp_softc *, agp_allocate *);
static int agp_deallocate_user(struct agp_softc *, int);
static int agp_bind_user(struct agp_softc *, agp_bind *);
static int agp_unbind_user(struct agp_softc *, agp_unbind *);
static int agpdev_match(struct pci_attach_args *);

#include "agp_ali.h"
#include "agp_amd.h"
#include "agp_i810.h"
#include "agp_intel.h"
#include "agp_sis.h"
#include "agp_via.h"

const struct agp_product {
	uint32_t	ap_vendor;
	uint32_t	ap_product;
	int			(*ap_match)(const struct pci_attach_args *);
	int			(*ap_attach)(struct device *, struct device *, void *);
} agp_products[] = {
#if NAGP_ALI > 0
	{ PCI_VENDOR_ALI,	-1,
	  NULL,			agp_ali_attach },
#endif

#if NAGP_AMD > 0
	{ PCI_VENDOR_AMD,	-1,
	  agp_amd_match,	agp_amd_attach },
#endif

#if NAGP_I810 > 0
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82810_MCH,
	  NULL,			agp_i810_attach },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82810_DC100_MCH,
	  NULL,			agp_i810_attach },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82810E_MCH,
	  NULL,			agp_i810_attach },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82815_FULL_HUB,
	  NULL,			agp_i810_attach },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82840_HB,
	  NULL,			agp_i810_attach },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82830MP_IO_1,
	  NULL,			agp_i810_attach },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82845G_DRAM,
	  NULL,			agp_i810_attach },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82855GM_MCH,
	  NULL,			agp_i810_attach },
	{ PCI_VENDOR_INTEL,	PCI_PRODUCT_INTEL_82865_HB,
	  NULL,			agp_i810_attach },
#endif

#if NAGP_INTEL > 0
	{ PCI_VENDOR_INTEL,	-1,
	  NULL,			agp_intel_attach },
#endif

#if NAGP_SIS > 0
	{ PCI_VENDOR_SIS,	-1,
	  NULL,			agp_sis_attach },
#endif

#if NAGP_VIA > 0
	{ PCI_VENDOR_VIATECH,	-1,
	  NULL,			agp_via_attach },
#endif

	{ 0,			0,
	  NULL,			NULL },
};

static const struct agp_product *
agp_lookup(const struct pci_attach_args *pa)
{
	const struct agp_product *ap;

	/* First find the vendor. */
	for (ap = agp_products; ap->ap_attach != NULL; ap++) {
		if (PCI_VENDOR(pa->pa_id) == ap->ap_vendor)
			break;
	}

	if (ap->ap_attach == NULL)
		return (NULL);

	/* Now find the product within the vendor's domain. */
	for (; ap->ap_attach != NULL; ap++) {
		if (PCI_VENDOR(pa->pa_id) != ap->ap_vendor) {
			/* Ran out of this vendor's section of the table. */
			return (NULL);
		}
		if (ap->ap_product == PCI_PRODUCT(pa->pa_id)) {
			/* Exact match. */
			break;
		}
		if (ap->ap_product == (uint32_t) -1) {
			/* Wildcard match. */
			break;
		}
	}

	if (ap->ap_attach == NULL)
		return (NULL);

	/* Now let the product-specific driver filter the match. */
	if (ap->ap_match != NULL && (*ap->ap_match)(pa) == 0)
		return (NULL);

	return (ap);
}

int
agpmatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct agpbus_attach_args *apa = aux;
	struct pci_attach_args *pa = &apa->apa_pci_args;

	if (strcmp(apa->apa_busname, "agp") != 0)
		return (0);

	if (agp_lookup(pa) == NULL)
		return (0);

	return (1);
}

static int agp_max[][2] = {
	{0,	0},
	{32,	4},
	{64,	28},
	{128,	96},
	{256,	204},
	{512,	440},
	{1024,	942},
	{2048,	1920},
	{4096,	3932}
};
#define agp_max_size	(sizeof(agp_max) / sizeof(agp_max[0]))

void
agpattach(struct device *parent, struct device *self, void *aux)
{
	struct agpbus_attach_args *apa = aux;
	struct pci_attach_args *pa = &apa->apa_pci_args;
	struct agp_softc *sc = (void *)self;
	const struct agp_product *ap;
	int memsize, i, ret;

	ap = agp_lookup(pa);
	if (ap == NULL) {
		printf("\n");
		panic("agpattach: impossible");
	}

	printf(": AGP controller\n");

	sc->as_dmat = pa->pa_dmat;
	sc->as_pc = pa->pa_pc;
	sc->as_tag = pa->pa_tag;
	sc->as_id = pa->pa_id;

	/*
	 * Work out an upper bound for agp memory allocation. This
	 * uses a heurisitc table from the Linux driver.
	 */
	memsize = ptoa(physmem) >> 20;
	for (i = 0; i < agp_max_size; i++) {
		if (memsize <= agp_max[i][0])
			break;
	}
	if (i == agp_max_size)
		i = agp_max_size - 1;
	sc->as_maxmem = agp_max[i][1] << 20U;

	/*
	 * The lock is used to prevent re-entry to
	 * agp_generic_bind_memory() since that function can sleep.
	 */
	lockinit(&sc->as_lock, PZERO|PCATCH, "agplk", 0, 0);

	TAILQ_INIT(&sc->as_memory);

	ret = (*ap->ap_attach)(parent, self, pa);
	if (ret == 0)
		printf(": aperture at 0x%lx, size 0x%lx\n",
		    (unsigned long)sc->as_apaddr,
		    (unsigned long)AGP_GET_APERTURE(sc));
	else
		sc->as_chipc = NULL;
}

int
agp_map_aperture(struct pci_attach_args *pa, struct agp_softc *sc)
{
	/*
	 * Find the aperture. Don't map it (yet), this would
	 * eat KVA.
	 */
	if (pci_mapreg_info(pa->pa_pc, pa->pa_tag, AGP_APBASE,
	    PCI_MAPREG_TYPE_MEM, &sc->as_apaddr, &sc->as_apsize,
	    &sc->as_apflags) != 0)
		return ENXIO;

	sc->as_apt = pa->pa_memt;

	return 0;
}

struct agp_gatt *
agp_alloc_gatt(struct agp_softc *sc)
{
	u_int32_t apsize = AGP_GET_APERTURE(sc);
	u_int32_t entries = apsize >> AGP_PAGE_SHIFT;
	struct agp_gatt *gatt;
	int dummyseg;

	gatt = malloc(sizeof(struct agp_gatt), M_AGP, M_NOWAIT);
	if (!gatt)
		return NULL;
	gatt->ag_entries = entries;

	if (agp_alloc_dmamem(sc->as_dmat, entries * sizeof(u_int32_t),
	    0, &gatt->ag_dmamap, (caddr_t *)&gatt->ag_virtual,
	    &gatt->ag_physical, &gatt->ag_dmaseg, 1, &dummyseg) != 0)
		return NULL;

	gatt->ag_size = entries * sizeof(u_int32_t);
	memset(gatt->ag_virtual, 0, gatt->ag_size);
	agp_flush_cache();

	return gatt;
}

void
agp_free_gatt(struct agp_softc *sc, struct agp_gatt *gatt)
{
	agp_free_dmamem(sc->as_dmat, gatt->ag_size, gatt->ag_dmamap,
	    (caddr_t)gatt->ag_virtual, &gatt->ag_dmaseg, 1);
	free(gatt, M_AGP);
}


int
agp_generic_detach(struct agp_softc *sc)
{
	lockmgr(&sc->as_lock, LK_DRAIN, NULL, 0);
	agp_flush_cache();
	return 0;
}

static int
agpdev_match(struct pci_attach_args *pa)
{
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_DISPLAY_VGA)
		if (pci_get_capability(pa->pa_pc, pa->pa_tag, PCI_CAP_AGP,
		    NULL, NULL))
		return 1;

	return 0;
}

int
agp_generic_enable(struct agp_softc *sc, u_int32_t mode)
{
	struct pci_attach_args pa;
	pcireg_t tstatus, mstatus;
	pcireg_t command;
	int rq, sba, fw, rate, capoff;

	if (pci_find_device(&pa, agpdev_match) == 0 ||
	    pci_get_capability(pa.pa_pc, pa.pa_tag, PCI_CAP_AGP,
	     &capoff, NULL) == 0) {
		printf("%s: can't find display\n", sc->as_dev.dv_xname);
		return ENXIO;
	}

	tstatus = pci_conf_read(sc->as_pc, sc->as_tag,
	    sc->as_capoff + AGP_STATUS);
	mstatus = pci_conf_read(pa.pa_pc, pa.pa_tag,
	    capoff + AGP_STATUS);

	/* Set RQ to the min of mode, tstatus and mstatus */
	rq = AGP_MODE_GET_RQ(mode);
	if (AGP_MODE_GET_RQ(tstatus) < rq)
		rq = AGP_MODE_GET_RQ(tstatus);
	if (AGP_MODE_GET_RQ(mstatus) < rq)
		rq = AGP_MODE_GET_RQ(mstatus);

	/* Set SBA if all three can deal with SBA */
	sba = (AGP_MODE_GET_SBA(tstatus)
	       & AGP_MODE_GET_SBA(mstatus)
	       & AGP_MODE_GET_SBA(mode));

	/* Similar for FW */
	fw = (AGP_MODE_GET_FW(tstatus)
	       & AGP_MODE_GET_FW(mstatus)
	       & AGP_MODE_GET_FW(mode));

	/* Figure out the max rate */
	rate = (AGP_MODE_GET_RATE(tstatus)
		& AGP_MODE_GET_RATE(mstatus)
		& AGP_MODE_GET_RATE(mode));
	if (rate & AGP_MODE_RATE_4x)
		rate = AGP_MODE_RATE_4x;
	else if (rate & AGP_MODE_RATE_2x)
		rate = AGP_MODE_RATE_2x;
	else
		rate = AGP_MODE_RATE_1x;

	/* Construct the new mode word and tell the hardware */
	command = AGP_MODE_SET_RQ(0, rq);
	command = AGP_MODE_SET_SBA(command, sba);
	command = AGP_MODE_SET_FW(command, fw);
	command = AGP_MODE_SET_RATE(command, rate);
	command = AGP_MODE_SET_AGP(command, 1);
	pci_conf_write(sc->as_pc, sc->as_tag,
	    sc->as_capoff + AGP_COMMAND, command);
	pci_conf_write(pa.pa_pc, pa.pa_tag, capoff + AGP_COMMAND, command);

	return 0;
}

struct agp_memory *
agp_generic_alloc_memory(struct agp_softc *sc, int type, vm_size_t size)
{
	struct agp_memory *mem;

	if ((size & (AGP_PAGE_SIZE - 1)) != 0)
		return 0;

	if (sc->as_allocated + size > sc->as_maxmem)
		return 0;

	if (type != 0) {
		printf("agp_generic_alloc_memory: unsupported type %d\n",
		       type);
		return 0;
	}

	mem = malloc(sizeof *mem, M_AGP, M_WAITOK);
	if (mem == NULL)
		return NULL;

	if (bus_dmamap_create(sc->as_dmat, size, size / PAGE_SIZE + 1,
			      size, 0, BUS_DMA_NOWAIT, &mem->am_dmamap) != 0) {
		free(mem, M_AGP);
		return NULL;
	}

	mem->am_id = sc->as_nextid++;
	mem->am_size = size;
	mem->am_type = 0;
	mem->am_physical = 0;
	mem->am_offset = 0;
	mem->am_is_bound = 0;
	TAILQ_INSERT_TAIL(&sc->as_memory, mem, am_link);
	sc->as_allocated += size;

	return mem;
}

int
agp_generic_free_memory(struct agp_softc *sc, struct agp_memory *mem)
{
	if (mem->am_is_bound)
		return EBUSY;

	sc->as_allocated -= mem->am_size;
	TAILQ_REMOVE(&sc->as_memory, mem, am_link);
	bus_dmamap_destroy(sc->as_dmat, mem->am_dmamap);
	free(mem, M_AGP);
	return 0;
}

int
agp_generic_bind_memory(struct agp_softc *sc, struct agp_memory *mem,
			off_t offset)
{
	off_t i, k;
	bus_size_t done, j;
	int error;
	bus_dma_segment_t *segs, *seg;
	bus_addr_t pa;
	int contigpages, nseg;

	lockmgr(&sc->as_lock, LK_EXCLUSIVE, NULL, 0);

	if (mem->am_is_bound) {
		printf("%s: memory already bound\n", sc->as_dev.dv_xname);
		lockmgr(&sc->as_lock, LK_RELEASE, NULL, 0);
		return EINVAL;
	}
	
	if (offset < 0
	    || (offset & (AGP_PAGE_SIZE - 1)) != 0
	    || offset + mem->am_size > AGP_GET_APERTURE(sc)) {
		printf("%s: binding memory at bad offset %#lx\n",
			      sc->as_dev.dv_xname, (unsigned long) offset);
		lockmgr(&sc->as_lock, LK_RELEASE, NULL, 0);
		return EINVAL;
	}

	/*
	 * XXXfvdl
	 * The memory here needs to be directly accessable from the
	 * AGP video card, so it should be allocated using bus_dma.
	 * However, it need not be contiguous, since individual pages
	 * are translated using the GATT.
	 *
	 * Using a large chunk of contiguous memory may get in the way
	 * of other subsystems that may need one, so we try to be friendly
	 * and ask for allocation in chunks of a minimum of 8 pages
	 * of contiguous memory on average, falling back to 4, 2 and 1
	 * if really needed. Larger chunks are preferred, since allocating
	 * a bus_dma_segment per page would be overkill.
	 */

	for (contigpages = 8; contigpages > 0; contigpages >>= 1) {
		nseg = (mem->am_size / (contigpages * PAGE_SIZE)) + 1;
		segs = malloc(nseg * sizeof *segs, M_AGP, M_WAITOK);
		if (segs == NULL) {
			lockmgr(&sc->as_lock, LK_RELEASE, NULL, 0);
			return ENOMEM;
		}
		if (bus_dmamem_alloc(sc->as_dmat, mem->am_size, PAGE_SIZE, 0,
				     segs, nseg, &mem->am_nseg,
				     contigpages > 1 ?
				     BUS_DMA_NOWAIT : BUS_DMA_WAITOK) != 0) {
			free(segs, M_AGP);
			continue;
		}
		if (bus_dmamem_map(sc->as_dmat, segs, mem->am_nseg,
		    mem->am_size, &mem->am_virtual, BUS_DMA_WAITOK) != 0) {
			bus_dmamem_free(sc->as_dmat, segs, mem->am_nseg);
			free(segs, M_AGP);
			continue;
		}
		if (bus_dmamap_load(sc->as_dmat, mem->am_dmamap,
		    mem->am_virtual, mem->am_size, NULL, BUS_DMA_WAITOK) != 0) {
			bus_dmamem_unmap(sc->as_dmat, mem->am_virtual,	
			    mem->am_size);
			bus_dmamem_free(sc->as_dmat, segs, mem->am_nseg);
			free(segs, M_AGP);
			continue;
		}
		mem->am_dmaseg = segs;
		break;
	}

	if (contigpages == 0) {
		lockmgr(&sc->as_lock, LK_RELEASE, NULL, 0);
		return ENOMEM;
	}


	/*
	 * Bind the individual pages and flush the chipset's
	 * TLB.
	 */
	done = 0;
	for (i = 0; i < mem->am_dmamap->dm_nsegs; i++) {
		seg = &mem->am_dmamap->dm_segs[i];
		/*
		 * Install entries in the GATT, making sure that if
		 * AGP_PAGE_SIZE < PAGE_SIZE and mem->am_size is not
		 * aligned to PAGE_SIZE, we don't modify too many GATT 
		 * entries.
		 */
		for (j = 0; j < seg->ds_len && (done + j) < mem->am_size;
		     j += AGP_PAGE_SIZE) {
			pa = seg->ds_addr + j;
			AGP_DPF("binding offset %#lx to pa %#lx\n",
				(unsigned long)(offset + done + j),
				(unsigned long)pa);
			error = AGP_BIND_PAGE(sc, offset + done + j, pa);
			if (error) {
				/*
				 * Bail out. Reverse all the mappings
				 * and unwire the pages.
				 */
				for (k = 0; k < done + j; k += AGP_PAGE_SIZE)
					AGP_UNBIND_PAGE(sc, offset + k);

				bus_dmamap_unload(sc->as_dmat, mem->am_dmamap);
				bus_dmamem_unmap(sc->as_dmat, mem->am_virtual,
						 mem->am_size);
				bus_dmamem_free(sc->as_dmat, mem->am_dmaseg,
						mem->am_nseg);
				free(mem->am_dmaseg, M_AGP);
				lockmgr(&sc->as_lock, LK_RELEASE, NULL, 0);
				return error;
			}
		}
		done += seg->ds_len;
	}

	/*
	 * Flush the CPU cache since we are providing a new mapping
	 * for these pages.
	 */
	agp_flush_cache();

	/*
	 * Make sure the chipset gets the new mappings.
	 */
	AGP_FLUSH_TLB(sc);

	mem->am_offset = offset;
	mem->am_is_bound = 1;

	lockmgr(&sc->as_lock, LK_RELEASE, NULL, 0);

	return 0;
}

int
agp_generic_unbind_memory(struct agp_softc *sc, struct agp_memory *mem)
{
	int i;

	lockmgr(&sc->as_lock, LK_EXCLUSIVE, NULL, 0);

	if (!mem->am_is_bound) {
		printf("%s: memory is not bound\n", sc->as_dev.dv_xname);
		lockmgr(&sc->as_lock, LK_RELEASE, NULL, 0);
		return EINVAL;
	}


	/*
	 * Unbind the individual pages and flush the chipset's
	 * TLB. Unwire the pages so they can be swapped.
	 */
	for (i = 0; i < mem->am_size; i += AGP_PAGE_SIZE)
		AGP_UNBIND_PAGE(sc, mem->am_offset + i);
		
	agp_flush_cache();
	AGP_FLUSH_TLB(sc);

	bus_dmamap_unload(sc->as_dmat, mem->am_dmamap);
	bus_dmamem_unmap(sc->as_dmat, mem->am_virtual, mem->am_size);
	bus_dmamem_free(sc->as_dmat, mem->am_dmaseg, mem->am_nseg);

	free(mem->am_dmaseg, M_AGP);

	mem->am_offset = 0;
	mem->am_is_bound = 0;

	lockmgr(&sc->as_lock, LK_RELEASE, NULL, 0);

	return 0;
}

/* Helper functions for implementing user/kernel api */

static int
agp_acquire_helper(struct agp_softc *sc, enum agp_acquire_state state)
{
	if (sc->as_state != AGP_ACQUIRE_FREE)
		return EBUSY;
	sc->as_state = state;

	return 0;
}

static int
agp_release_helper(struct agp_softc *sc, enum agp_acquire_state state)
{
	struct agp_memory *mem;

	if (sc->as_state == AGP_ACQUIRE_FREE)
		return 0;

	if (sc->as_state != state)
		return EBUSY;

	/*
	 * Clear out outstanding aperture mappings.
	 * (should not be necessary, done by caller)
	 */
	TAILQ_FOREACH(mem, &sc->as_memory, am_link) {
		if (mem->am_is_bound) {
			printf("agp_release_helper: mem %d is bound\n",
			       mem->am_id);
			AGP_UNBIND_MEMORY(sc, mem);
		}
	}

	sc->as_state = AGP_ACQUIRE_FREE;
	return 0;
}

static struct agp_memory *
agp_find_memory(struct agp_softc *sc, int id)
{
	struct agp_memory *mem;

	AGP_DPF("searching for memory block %d\n", id);
	TAILQ_FOREACH(mem, &sc->as_memory, am_link) {
		AGP_DPF("considering memory block %d\n", mem->am_id);
		if (mem->am_id == id)
			return mem;
	}
	return 0;
}

/* Implementation of the userland ioctl api */

static int
agp_info_user(struct agp_softc *sc, agp_info *info)
{
	memset(info, 0, sizeof *info);
	info->bridge_id = sc->as_id;
	if (sc->as_capoff != 0)
		info->agp_mode = pci_conf_read(sc->as_pc, sc->as_tag,
					       sc->as_capoff + AGP_STATUS);
	else
		info->agp_mode = 0; /* i810 doesn't have real AGP */
	info->aper_base = sc->as_apaddr;
	info->aper_size = AGP_GET_APERTURE(sc) >> 20;
	info->pg_total = info->pg_system = sc->as_maxmem >> AGP_PAGE_SHIFT;
	info->pg_used = sc->as_allocated >> AGP_PAGE_SHIFT;

	return 0;
}

static int
agp_setup_user(struct agp_softc *sc, agp_setup *setup)
{
	return AGP_ENABLE(sc, setup->agp_mode);
}

static int
agp_allocate_user(struct agp_softc *sc, agp_allocate *alloc)
{
	struct agp_memory *mem;

	mem = AGP_ALLOC_MEMORY(sc,
			       alloc->type,
			       alloc->pg_count << AGP_PAGE_SHIFT);
	if (mem) {
		alloc->key = mem->am_id;
		alloc->physical = mem->am_physical;
		return 0;
	} else {
		return ENOMEM;
	}
}

static int
agp_deallocate_user(struct agp_softc *sc, int id)
{
	struct agp_memory *mem = agp_find_memory(sc, id);

	if (mem) {
		AGP_FREE_MEMORY(sc, mem);
		return 0;
	} else {
		return ENOENT;
	}
}

static int
agp_bind_user(struct agp_softc *sc, agp_bind *bind)
{
	struct agp_memory *mem = agp_find_memory(sc, bind->key);

	if (!mem)
		return ENOENT;

	return AGP_BIND_MEMORY(sc, mem, bind->pg_start << AGP_PAGE_SHIFT);
}

static int
agp_unbind_user(struct agp_softc *sc, agp_unbind *unbind)
{
	struct agp_memory *mem = agp_find_memory(sc, unbind->key);

	if (!mem)
		return ENOENT;

	return AGP_UNBIND_MEMORY(sc, mem);
}

int
agpopen(dev_t dev, int oflags, int devtype, struct proc *p)
{
	struct agp_softc *sc = agp_cd.cd_devs[AGPUNIT(dev)];

	if (sc == NULL)
		return ENXIO;

	if (sc->as_chipc == NULL)
		return ENXIO;

	if (!sc->as_isopen)
		sc->as_isopen = 1;
	else
		return EBUSY;

	return 0;
}

int
agpclose(dev_t dev, int fflag, int devtype, struct proc *p)
{
	struct agp_softc *sc = agp_cd.cd_devs[AGPUNIT(dev)];
	struct agp_memory *mem;

	/*
	 * Clear the GATT and force release on last close
	 */
	if (sc->as_state == AGP_ACQUIRE_USER) {
		while ((mem = TAILQ_FIRST(&sc->as_memory))) {
			if (mem->am_is_bound) {
				printf("agpclose: mem %d is bound\n",
				       mem->am_id);
				AGP_UNBIND_MEMORY(sc, mem);
			}
			/*
			 * XXX it is not documented, but if the protocol allows
			 * allocate->acquire->bind, it would be possible that
			 * memory ranges are allocated by the kernel here,
			 * which we shouldn't free. We'd have to keep track of
			 * the memory range's owner.
			 * The kernel API is unsed yet, so we get away with
			 * freeing all.
			 */
			AGP_FREE_MEMORY(sc, mem);
		}
		agp_release_helper(sc, AGP_ACQUIRE_USER);
	}
	sc->as_isopen = 0;

	return 0;
}

int
agpioctl(dev_t dev, u_long cmd, caddr_t data, int fflag, struct proc *p)
{
	struct agp_softc *sc = agp_cd.cd_devs[AGPUNIT(dev)];

	if (sc == NULL)
		return ENODEV;

	if ((fflag & FWRITE) == 0 && cmd != AGPIOC_INFO)
		return EPERM;

	switch (cmd) {
	case AGPIOC_INFO:
		return agp_info_user(sc, (agp_info *) data);

	case AGPIOC_ACQUIRE:
		return agp_acquire_helper(sc, AGP_ACQUIRE_USER);

	case AGPIOC_RELEASE:
		return agp_release_helper(sc, AGP_ACQUIRE_USER);

	case AGPIOC_SETUP:
		return agp_setup_user(sc, (agp_setup *)data);

	case AGPIOC_ALLOCATE:
		return agp_allocate_user(sc, (agp_allocate *)data);

	case AGPIOC_DEALLOCATE:
		return agp_deallocate_user(sc, *(int *) data);

	case AGPIOC_BIND:
		return agp_bind_user(sc, (agp_bind *)data);

	case AGPIOC_UNBIND:
		return agp_unbind_user(sc, (agp_unbind *)data);

	}

	return EINVAL;
}

caddr_t
agpmmap(dev_t dev, off_t offset, int prot)
{
	struct agp_softc *sc = agp_cd.cd_devs[AGPUNIT(dev)];

	if (offset > AGP_GET_APERTURE(sc))
		return ((caddr_t)-1);

	return ((caddr_t)bus_space_mmap(sc->as_apt, sc->as_apaddr, offset, prot, BUS_SPACE_MAP_LINEAR));
}

/* Implementation of the kernel api */

void *
agp_find_device(int unit)
{
	return agp_cd.cd_devs[unit];
}

enum agp_acquire_state
agp_state(void *devcookie)
{
	struct agp_softc *sc = devcookie;
	return sc->as_state;
}

void
agp_get_info(void *devcookie, struct agp_info *info)
{
	struct agp_softc *sc = devcookie;

	info->ai_mode = pci_conf_read(sc->as_pc, sc->as_tag,
	    sc->as_capoff + AGP_STATUS);
	info->ai_aperture_base = sc->as_apaddr;
	info->ai_aperture_size = sc->as_apsize;	/* XXXfvdl inconsistent */
	info->ai_memory_allowed = sc->as_maxmem;
	info->ai_memory_used = sc->as_allocated;
}

int
agp_acquire(void *dev)
{
	return agp_acquire_helper(dev, AGP_ACQUIRE_KERNEL);
}

int
agp_release(void *dev)
{
	return agp_release_helper(dev, AGP_ACQUIRE_KERNEL);
}

int
agp_enable(void *dev, u_int32_t mode)
{
	struct agp_softc *sc = dev;

	return AGP_ENABLE(sc, mode);
}

void *
agp_alloc_memory(void *dev, int type, vm_size_t bytes)
{
	struct agp_softc *sc = dev;

	return (void *)AGP_ALLOC_MEMORY(sc, type, bytes);
}

void 
agp_free_memory(void *dev, void *handle)
{
	struct agp_softc *sc = dev;
	struct agp_memory *mem = (struct agp_memory *) handle;
	AGP_FREE_MEMORY(sc, mem);
}

int 
agp_bind_memory(void *dev, void *handle, off_t offset)
{
	struct agp_softc *sc = dev;
	struct agp_memory *mem = (struct agp_memory *) handle;

	return AGP_BIND_MEMORY(sc, mem, offset);
}

int 
agp_unbind_memory(void *dev, void *handle)
{
	struct agp_softc *sc = dev;
	struct agp_memory *mem = (struct agp_memory *) handle;

	return AGP_UNBIND_MEMORY(sc, mem);
}

void 
agp_memory_info(void *dev, void *handle, struct agp_memory_info *mi)
{
	struct agp_memory *mem = (struct agp_memory *) handle;

	mi->ami_size = mem->am_size;
	mi->ami_physical = mem->am_physical;
	mi->ami_offset = mem->am_offset;
	mi->ami_is_bound = mem->am_is_bound;
}

int
agp_alloc_dmamem(bus_dma_tag_t tag, size_t size, int flags, bus_dmamap_t *mapp, caddr_t *vaddr, bus_addr_t *baddr, bus_dma_segment_t *seg, int nseg, int *rseg)

{
	int error, level = 0;

	if ((error = bus_dmamem_alloc(tag, size, PAGE_SIZE, 0,
			seg, nseg, rseg, BUS_DMA_NOWAIT)) != 0)
		goto out;
	level++;

	if ((error = bus_dmamem_map(tag, seg, *rseg, size, vaddr,
			BUS_DMA_NOWAIT | flags)) != 0)
		goto out;
	level++;

	if ((error = bus_dmamap_create(tag, size, *rseg, size, 0,
			BUS_DMA_NOWAIT, mapp)) != 0)
		goto out;
	level++;

	if ((error = bus_dmamap_load(tag, *mapp, *vaddr, size, NULL,
			BUS_DMA_NOWAIT)) != 0)
		goto out;

	*baddr = (*mapp)->dm_segs[0].ds_addr;

	return 0;
out:
	switch (level) {
	case 3:
		bus_dmamap_destroy(tag, *mapp);
		/* FALLTHROUGH */
	case 2:
		bus_dmamem_unmap(tag, *vaddr, size);
		/* FALLTHROUGH */
	case 1:
		bus_dmamem_free(tag, seg, *rseg);
		break;
	default:
		break;
	}

	return error;
}

void
agp_free_dmamem(bus_dma_tag_t tag, size_t size, bus_dmamap_t map, caddr_t vaddr, bus_dma_segment_t *seg, int nseg)
{
	bus_dmamap_unload(tag, map);
	bus_dmamap_destroy(tag, map);
	bus_dmamem_unmap(tag, vaddr, size);
	bus_dmamem_free(tag, seg, nseg);
}
