/*	$NetBSD: agp_i810.c,v 1.22.2.1 2004/11/12 06:04:32 jmc Exp $	*/

/*-
 * Copyright (c) 2000 Doug Rabson
 * Copyright (c) 2000 Ruslan Ermilov
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
 *	$FreeBSD: src/sys/pci/agp_i810.c,v 1.4 2001/07/05 21:28:47 jhb Exp $
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agp_i810.c,v 1.22.2.1 2004/11/12 06:04:32 jmc Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <vm/include/vm_extern.h>

#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcidevs.h>
#include <dev/video/agp/agpvar.h>
#include <dev/video/agp/agpreg.h>

#include <sys/agpio.h>

#include <machine/bus.h>

#include "agp_intel.h"

#define READ1(off)	bus_space_read_1(isc->bst, isc->bsh, off)
#define READ4(off)	bus_space_read_4(isc->bst, isc->bsh, off)
#define WRITE4(off,v)	bus_space_write_4(isc->bst, isc->bsh, off, v)

#define CHIP_I810 0	/* i810/i815 */
#define CHIP_I830 1	/* 830M/845G */
#define CHIP_I855 2	/* 852GM/855GM/865G */

struct agp_i810_softc {
	u_int32_t initial_aperture;	/* aperture size at startup */
	struct agp_gatt *gatt;
	int chiptype;			/* i810-like or i830 */
	u_int32_t dcache_size;		/* i810 only */
	u_int32_t stolen;		/* number of i830/845 gtt entries
					   for stolen memory */
	bus_space_tag_t bst;		/* bus_space tag */
	bus_space_handle_t bsh;		/* bus_space handle */
	struct pci_attach_args vga_pa;
};

static u_int32_t agp_i810_get_aperture(struct agp_softc *);
static int agp_i810_set_aperture(struct agp_softc *, u_int32_t);
static int agp_i810_bind_page(struct agp_softc *, off_t, bus_addr_t);
static int agp_i810_unbind_page(struct agp_softc *, off_t);
static void agp_i810_flush_tlb(struct agp_softc *);
static int agp_i810_enable(struct agp_softc *, u_int32_t mode);
static struct agp_memory *agp_i810_alloc_memory(struct agp_softc *, int,
						vm_size_t);
static int agp_i810_free_memory(struct agp_softc *, struct agp_memory *);
static int agp_i810_bind_memory(struct agp_softc *, struct agp_memory *, off_t);
static int agp_i810_unbind_memory(struct agp_softc *, struct agp_memory *);

struct agp_methods agp_i810_methods = {
	agp_i810_get_aperture,
	agp_i810_set_aperture,
	agp_i810_bind_page,
	agp_i810_unbind_page,
	agp_i810_flush_tlb,
	agp_i810_enable,
	agp_i810_alloc_memory,
	agp_i810_free_memory,
	agp_i810_bind_memory,
	agp_i810_unbind_memory,
};

/* XXXthorpej -- duplicated code (see arch/i386/pci/pchb.c) */
static int
agp_i810_vgamatch(struct pci_attach_args *pa)
{

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY ||
	    PCI_SUBCLASS(pa->pa_class) != PCI_SUBCLASS_DISPLAY_VGA)
		return (0);

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_INTEL_82810_GC:
	case PCI_PRODUCT_INTEL_82810_DC100_GC:
	case PCI_PRODUCT_INTEL_82810E_GC:
	case PCI_PRODUCT_INTEL_82815_FULL_GRAPH:
	case PCI_PRODUCT_INTEL_82830MP_IV:
	case PCI_PRODUCT_INTEL_82845G_IGD:
	case PCI_PRODUCT_INTEL_82855GM_IGD:
	case PCI_PRODUCT_INTEL_82865_IGD:
		return (1);
	}

	return (0);
}

int
agp_i810_attach(struct device *parent, struct device *self, void *aux)
{
	struct agp_softc *sc = (void *)self;
	struct agp_i810_softc *isc;
	struct agp_gatt *gatt;
	int error;

	isc = malloc(sizeof *isc, M_AGP, M_NOWAIT|M_ZERO);
	if (isc == NULL) {
		printf(": can't allocate chipset-specific softc\n");
		return ENOMEM;
	}
	sc->as_chipc = isc;
	sc->as_methods = &agp_i810_methods;

	if (pci_find_device(&isc->vga_pa, agp_i810_vgamatch) == 0) {
#if NAGP_INTEL > 0
		const struct pci_attach_args *pa = aux;

		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_INTEL_82840_HB:
		case PCI_PRODUCT_INTEL_82865_HB:
		case PCI_PRODUCT_INTEL_82845G_DRAM:
		case PCI_PRODUCT_INTEL_82815_FULL_HUB:
			return agp_intel_attach(parent, self, aux);
		}
#endif
		printf(": can't find internal VGA device config space\n");
		free(isc, M_AGP);
		return ENOENT;
	}

	/* XXXfvdl */
	sc->as_dmat = isc->vga_pa.pa_dmat;

	error = agp_map_aperture(&isc->vga_pa, sc);
	if (error != 0) {
		printf(": can't map aperture\n");
		free(isc, M_AGP);
		return error;
	}

	switch (PCI_PRODUCT(isc->vga_pa.pa_id)) {
	case PCI_PRODUCT_INTEL_82810_GC:
	case PCI_PRODUCT_INTEL_82810_DC100_GC:
	case PCI_PRODUCT_INTEL_82810E_GC:
	case PCI_PRODUCT_INTEL_82815_FULL_GRAPH:
		isc->chiptype = CHIP_I810;
		break;
	case PCI_PRODUCT_INTEL_82830MP_IV:
	case PCI_PRODUCT_INTEL_82845G_IGD:
		isc->chiptype = CHIP_I830;
		break;
	case PCI_PRODUCT_INTEL_82855GM_IGD:
	case PCI_PRODUCT_INTEL_82865_IGD:
		isc->chiptype = CHIP_I855;
		break;
	}

	error = pci_mapreg_map(&isc->vga_pa, AGP_I810_MMADR,
	    PCI_MAPREG_TYPE_MEM, 0, &isc->bst, &isc->bsh, NULL, NULL);
	if (error != 0) {
		printf(": can't map mmadr registers\n");
		return error;
	}

	isc->initial_aperture = AGP_GET_APERTURE(sc);

	gatt = malloc(sizeof(struct agp_gatt), M_AGP, M_NOWAIT);
	if (!gatt) {
 		agp_generic_detach(sc);
 		return ENOMEM;
	}
	isc->gatt = gatt;

	gatt->ag_entries = AGP_GET_APERTURE(sc) >> AGP_PAGE_SHIFT;

	if (isc->chiptype == CHIP_I810) {
		int dummyseg;
		/* Some i810s have on-chip memory called dcache */
		if (READ1(AGP_I810_DRT) & AGP_I810_DRT_POPULATED)
			isc->dcache_size = 4 * 1024 * 1024;
		else
			isc->dcache_size = 0;

		/* According to the specs the gatt on the i810 must be 64k */
		if (agp_alloc_dmamem(sc->as_dmat, 64 * 1024,
		    0, &gatt->ag_dmamap, (caddr_t *)&gatt->ag_virtual,
		    &gatt->ag_physical, &gatt->ag_dmaseg, 1, &dummyseg) != 0) {
			free(gatt, M_AGP);
			agp_generic_detach(sc);
			return ENOMEM;
		}

		gatt->ag_size = gatt->ag_entries * sizeof(u_int32_t);
		memset(gatt->ag_virtual, 0, gatt->ag_size);
		
		agp_flush_cache();
		/* Install the GATT. */
		WRITE4(AGP_I810_PGTBL_CTL, gatt->ag_physical | 1);
	} else if (isc->chiptype == CHIP_I830) {
		/* The i830 automatically initializes the 128k gatt on boot. */
		pcireg_t reg;
		u_int32_t pgtblctl;
		u_int16_t gcc1;

		reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_I830_GCC0);
		gcc1 = (u_int16_t)(reg >> 16);
		switch (gcc1 & AGP_I830_GCC1_GMS) {
		case AGP_I830_GCC1_GMS_STOLEN_512:
			isc->stolen = (512 - 132) * 1024 / 4096;
			break;
		case AGP_I830_GCC1_GMS_STOLEN_1024: 
			isc->stolen = (1024 - 132) * 1024 / 4096;
			break;
		case AGP_I830_GCC1_GMS_STOLEN_8192: 
			isc->stolen = (8192 - 132) * 1024 / 4096;
			break;
		default:
			isc->stolen = 0;
			printf(
			    ": unknown memory configuration, disabling\n");
			agp_generic_detach(sc);
			return EINVAL;
		}
		if (isc->stolen > 0) {
			printf(": detected %dk stolen memory\n%s",
			    isc->stolen * 4, sc->as_dev.dv_xname);
		}

		/* GATT address is already in there, make sure it's enabled */
		pgtblctl = READ4(AGP_I810_PGTBL_CTL);
		pgtblctl |= 1;
		WRITE4(AGP_I810_PGTBL_CTL, pgtblctl);

		gatt->ag_physical = pgtblctl & ~1;
	} else {	/* CHIP_I855 */
		/* The 855GM automatically initializes the 128k gatt on boot. */
		pcireg_t reg;
		u_int32_t pgtblctl;
		u_int16_t gcc1;

		reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_I855_GCC1);
		gcc1 = (u_int16_t)(reg >> 16);
		switch (gcc1 & AGP_I855_GCC1_GMS) {
		case AGP_I855_GCC1_GMS_STOLEN_1M:
			isc->stolen = (1024 - 132) * 1024 / 4096;
			break;
		case AGP_I855_GCC1_GMS_STOLEN_4M:
			isc->stolen = (4096 - 132) * 1024 / 4096;
			break;
		case AGP_I855_GCC1_GMS_STOLEN_8M:
			isc->stolen = (8192 - 132) * 1024 / 4096;
			break;
		case AGP_I855_GCC1_GMS_STOLEN_16M:
			isc->stolen = (16384 - 132) * 1024 / 4096;
			break;
		case AGP_I855_GCC1_GMS_STOLEN_32M:
			isc->stolen = (32768 - 132) * 1024 / 4096;
			break;
		default:
			isc->stolen = 0;
			printf(
			    ": unknown memory configuration, disabling\n");
			agp_generic_detach(sc);
			return EINVAL;
		}
		if (isc->stolen > 0) {
			printf(": detected %dk stolen memory\n%s",
			    isc->stolen * 4, sc->as_dev.dv_xname);
		}

		/* GATT address is already in there, make sure it's enabled */
		pgtblctl = READ4(AGP_I810_PGTBL_CTL);
		pgtblctl |= 1;
		WRITE4(AGP_I810_PGTBL_CTL, pgtblctl);

		gatt->ag_physical = pgtblctl & ~1;
	}

	/*
	 * Make sure the chipset can see everything.
	 */
	agp_flush_cache();

	return 0;
}

#if 0
static int
agp_i810_detach(struct agp_softc *sc)
{
	int error;
	struct agp_i810_softc *isc = sc->as_chipc;

	error = agp_generic_detach(sc);
	if (error)
		return error;

	/* Clear the GATT base. */
	if (sc->chiptype == CHIP_I810) {
		WRITE4(AGP_I810_PGTBL_CTL, 0);
	} else {
		unsigned int pgtblctl;
		pgtblctl = READ4(AGP_I810_PGTBL_CTL);
		pgtblctl &= ~1;
		WRITE4(AGP_I810_PGTBL_CTL, pgtblctl);
	}

	/* Put the aperture back the way it started. */
	AGP_SET_APERTURE(sc, isc->initial_aperture);

	if (sc->chiptype == CHIP_I810) {
		agp_free_dmamem(sc->as_dmat, gatt->ag_size, gatt->ag_dmamap,
		    (caddr_t)gatt->ag_virtual, &gatt->ag_dmaseg, 1);
	}
	free(sc->gatt, M_AGP);

	return 0;
}
#endif

static u_int32_t
agp_i810_get_aperture(struct agp_softc *sc)
{
	struct agp_i810_softc *isc = sc->as_chipc;
	pcireg_t reg;

	if (isc->chiptype == CHIP_I810) {
		u_int16_t miscc;

		reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_I810_SMRAM);
		miscc = (u_int16_t)(reg >> 16);
		if ((miscc & AGP_I810_MISCC_WINSIZE) ==
		    AGP_I810_MISCC_WINSIZE_32)
			return 32 * 1024 * 1024;
		else
			return 64 * 1024 * 1024;
	} else if (isc->chiptype == CHIP_I830) {
		u_int16_t gcc1;

		reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_I830_GCC0);
		gcc1 = (u_int16_t)(reg >> 16);
		if ((gcc1 & AGP_I830_GCC1_GMASIZE) == AGP_I830_GCC1_GMASIZE_64)
			return 64 * 1024 * 1024;
		else
			return 128 * 1024 * 1024;
	} else {	/* CHIP_I855 */
		return 128 * 1024 * 1024;
	}
}

static int
agp_i810_set_aperture(struct agp_softc *sc, u_int32_t aperture)
{
	struct agp_i810_softc *isc = sc->as_chipc;
	pcireg_t reg;

	if (isc->chiptype == CHIP_I810) {
		u_int16_t miscc;

		/*
		 * Double check for sanity.
		 */
		if (aperture != (32 * 1024 * 1024) &&
		    aperture != (64 * 1024 * 1024)) {
			printf("%s: bad aperture size %d\n",
			    sc->as_dev.dv_xname, aperture);
			return EINVAL;
		}

		reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_I810_SMRAM);
		miscc = (u_int16_t)(reg >> 16);
		miscc &= ~AGP_I810_MISCC_WINSIZE;
		if (aperture == 32 * 1024 * 1024)
			miscc |= AGP_I810_MISCC_WINSIZE_32;
		else
			miscc |= AGP_I810_MISCC_WINSIZE_64;

		reg &= 0x0000ffff;
		reg |= ((pcireg_t)miscc) << 16;
		pci_conf_write(sc->as_pc, sc->as_tag, AGP_I810_SMRAM, reg);
	} if (isc->chiptype == CHIP_I830) {
		u_int16_t gcc1;

		if (aperture != (64 * 1024 * 1024) &&
		    aperture != (128 * 1024 * 1024)) {
			printf("%s: bad aperture size %d\n",
			    sc->as_dev.dv_xname, aperture);
			return EINVAL;
		}
		reg = pci_conf_read(sc->as_pc, sc->as_tag, AGP_I830_GCC0);
		gcc1 = (u_int16_t)(reg >> 16);
		gcc1 &= ~AGP_I830_GCC1_GMASIZE;
		if (aperture == 64 * 1024 * 1024)
			gcc1 |= AGP_I830_GCC1_GMASIZE_64;
		else
			gcc1 |= AGP_I830_GCC1_GMASIZE_128;

		reg &= 0x0000ffff;
		reg |= ((pcireg_t)gcc1) << 16;
		pci_conf_write(sc->as_pc, sc->as_tag, AGP_I830_GCC0, reg);
	} else {	/* CHIP_I855 */
		if (aperture != 128 * 1024 * 1024) {
			printf("%s: bad aperture size %d\n",
			    sc->as_dev.dv_xname, aperture);
			return EINVAL;
		}
	}

	return 0;
}

static int
agp_i810_bind_page(struct agp_softc *sc, off_t offset, bus_addr_t physical)
{
	struct agp_i810_softc *isc = sc->as_chipc;

	if (offset < 0 || offset >= (isc->gatt->ag_entries << AGP_PAGE_SHIFT)) {
#ifdef DEBUG
		printf("%s: failed: offset 0x%08x, shift %d, entries %d\n",
		    sc->as_dev.dv_xname, (int)offset, AGP_PAGE_SHIFT,
		    isc->gatt->ag_entries);
#endif
		return EINVAL;
	}

	if (isc->chiptype != CHIP_I830) {
		if ((offset >> AGP_PAGE_SHIFT) < isc->stolen) {
#ifdef DEBUG
			printf("%s: trying to bind into stolen memory",
			    sc->as_dev.dv_xname);
#endif
			return EINVAL;
		}
	}

	WRITE4(AGP_I810_GTT + (u_int32_t)(offset >> AGP_PAGE_SHIFT) * 4,
	    physical | 1);
	return 0;
}

static int
agp_i810_unbind_page(struct agp_softc *sc, off_t offset)
{
	struct agp_i810_softc *isc = sc->as_chipc;

	if (offset < 0 || offset >= (isc->gatt->ag_entries << AGP_PAGE_SHIFT))
		return EINVAL;

	if (isc->chiptype != CHIP_I810 ) {
		if ((offset >> AGP_PAGE_SHIFT) < isc->stolen) {
#ifdef DEBUG
			printf("%s: trying to unbind from stolen memory",
			    sc->as_dev.dv_xname);
#endif
			return EINVAL;
		}
	}

	WRITE4(AGP_I810_GTT + (u_int32_t)(offset >> AGP_PAGE_SHIFT) * 4, 0);
	return 0;
}

/*
 * Writing via memory mapped registers already flushes all TLBs.
 */
static void
agp_i810_flush_tlb(struct agp_softc *sc)
{
}

static int
agp_i810_enable(struct agp_softc *sc, u_int32_t mode)
{

	return 0;
}

static struct agp_memory *
agp_i810_alloc_memory(struct agp_softc *sc, int type, vm_size_t size)
{
	struct agp_i810_softc *isc = sc->as_chipc;
	struct agp_memory *mem;

	if ((size & (AGP_PAGE_SIZE - 1)) != 0)
		return 0;

	if (sc->as_allocated + size > sc->as_maxmem)
		return 0;

	if (type == 1) {
		/*
		 * Mapping local DRAM into GATT.
		 */
		if (isc->chiptype != CHIP_I810 )
			return 0;
		if (size != isc->dcache_size)
			return 0;
	} else if (type == 2) {
		/*
		 * Bogus mapping of a single page for the hardware cursor.
		 */
		if (size != AGP_PAGE_SIZE)
			return 0;
	}

	mem = malloc(sizeof *mem, M_AGP, M_WAITOK|M_ZERO);
	if (mem == NULL)
		return NULL;
	mem->am_id = sc->as_nextid++;
	mem->am_size = size;
	mem->am_type = type;

	if (type == 2) {
		/*
		 * Allocate and wire down the page now so that we can
		 * get its physical address.
		 */
		mem->am_dmaseg = malloc(sizeof *mem->am_dmaseg, M_AGP,
		    M_WAITOK);
		if (mem->am_dmaseg == NULL) {
			free(mem, M_AGP);
			return NULL;
		}
		if (agp_alloc_dmamem(sc->as_dmat, size, 0,
		    &mem->am_dmamap, &mem->am_virtual, &mem->am_physical,
		    mem->am_dmaseg, 1, &mem->am_nseg) != 0) {
			free(mem->am_dmaseg, M_AGP);
			free(mem, M_AGP);
			return NULL;
		}
	} else if (type != 1) {
		if (bus_dmamap_create(sc->as_dmat, size, size / PAGE_SIZE + 1,
				      size, 0, BUS_DMA_NOWAIT,
				      &mem->am_dmamap) != 0) {
			free(mem, M_AGP);
			return NULL;
		}
	}

	TAILQ_INSERT_TAIL(&sc->as_memory, mem, am_link);
	sc->as_allocated += size;

	return mem;
}

static int
agp_i810_free_memory(struct agp_softc *sc, struct agp_memory *mem)
{
	if (mem->am_is_bound)
		return EBUSY;

	if (mem->am_type == 2) {
		agp_free_dmamem(sc->as_dmat, mem->am_size, mem->am_dmamap,
		    mem->am_virtual, mem->am_dmaseg, mem->am_nseg);
		free(mem->am_dmaseg, M_AGP);
	}

	sc->as_allocated -= mem->am_size;
	TAILQ_REMOVE(&sc->as_memory, mem, am_link);
	free(mem, M_AGP);
	return 0;
}

static int
agp_i810_bind_memory(struct agp_softc *sc, struct agp_memory *mem,
		     off_t offset)
{
	struct agp_i810_softc *isc = sc->as_chipc;
	u_int32_t regval, i;

	/*
	 * XXX evil hack: the PGTBL_CTL appearently gets overwritten by the
	 * X server for mysterious reasons which leads to crashes if we write
	 * to the GTT through the MMIO window.
	 * Until the issue is solved, simply restore it.
	 */
	regval = bus_space_read_4(isc->bst, isc->bsh, AGP_I810_PGTBL_CTL);
	if (regval != (isc->gatt->ag_physical | 1)) {
		printf("agp_i810_bind_memory: PGTBL_CTL is 0x%x - fixing\n",
		       regval);
		bus_space_write_4(isc->bst, isc->bsh, AGP_I810_PGTBL_CTL,
				  isc->gatt->ag_physical | 1);
	}

	if (mem->am_type == 2) {
		WRITE4(AGP_I810_GTT + (u_int32_t)(offset >> AGP_PAGE_SHIFT) * 4,
		       mem->am_physical | 1);
		mem->am_offset = offset;
		mem->am_is_bound = 1;
		return 0;
	}

	if (mem->am_type != 1)
		return agp_generic_bind_memory(sc, mem, offset);

	if (isc->chiptype != CHIP_I810)
		return EINVAL;

	for (i = 0; i < mem->am_size; i += AGP_PAGE_SIZE) {
		WRITE4(AGP_I810_GTT + (u_int32_t)(offset >> AGP_PAGE_SHIFT) * 4,
		       i | 3);
	}
	mem->am_is_bound = 1;
	return 0;
}

static int
agp_i810_unbind_memory(struct agp_softc *sc, struct agp_memory *mem)
{
	struct agp_i810_softc *isc = sc->as_chipc;
	u_int32_t i;

	if (mem->am_type == 2) {
		WRITE4(AGP_I810_GTT +
		       (u_int32_t)(mem->am_offset >> AGP_PAGE_SHIFT) * 4,
		       0);
		mem->am_offset = 0;
		mem->am_is_bound = 0;
		return 0;
	}

	if (mem->am_type != 1)
		return agp_generic_unbind_memory(sc, mem);

	if (isc->chiptype != CHIP_I810)
		return EINVAL;

	for (i = 0; i < mem->am_size; i += AGP_PAGE_SIZE)
		WRITE4(AGP_I810_GTT + (i >> AGP_PAGE_SHIFT) * 4, 0);
	mem->am_is_bound = 0;
	return 0;
}
