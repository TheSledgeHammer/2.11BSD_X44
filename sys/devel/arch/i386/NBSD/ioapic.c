/*	$OpenBSD: ioapic.c,v 1.41 2018/08/25 16:09:29 kettenis Exp $	*/
/* 	$NetBSD: ioapic.c,v 1.7 2003/07/14 22:32:40 lukem Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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


/*
 * Copyright (c) 1999 Stefan Grefen
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/user.h>

#include <vm/include/vm_extern.h>

#include <arch/i386/isa/isa_machdep.h> 			/* XXX intrhand */

#include <devel/arch/i386/NBSD/pic.h>
#include <devel/arch/i386/NBSD/apicvar.h>
#include <devel/arch/i386/NBSD/i82093reg.h>
#include <devel/arch/i386/NBSD/i82093var.h>

#include <arch/i386/include/pio.h>
#include <arch/i386/include/pmap.h>
#include <arch/i386/include/intr.h>

/*
 * XXX locking
 */

int     	ioapic_match(struct device *, struct cfdata *, void *);
void    	ioapic_attach(struct device *, struct device *, void *);

extern int 	i386_mem_add_mapping(bus_addr_t, bus_size_t, int, bus_space_handle_t *); /* XXX XXX */

void 		ioapic_hwmask(struct pic *, int);
void 		ioapic_hwunmask(struct pic *, int);
boolean_t 	ioapic_trymask(struct pic *, int);
static void ioapic_addroute(struct pic *, struct cpu_info *, int, int, int);
static void ioapic_delroute(struct pic *, struct cpu_info *, int, int, int);

struct ioapic_softc *ioapics;	 /* head of linked list */
int nioapics = 0;	   	 		/* number attached */
static int ioapic_vecbase;

static inline u_long
ioapic_lock(struct ioapic_softc *sc)
{
	u_long flags;

	flags = x86_read_psl();
	disable_intr();
	simple_lock(&sc->sc_pic.pic_lock);
	return flags;
}

static inline void
ioapic_unlock(struct ioapic_softc *sc, u_long flags)
{
	simple_unlock(&sc->sc_pic.pic_lock);
	//i386_write_psl(flags);
}

#ifndef _IOAPIC_CUSTOM_RW
/*
 * Register read/write routines.
 */
static inline  uint32_t
ioapic_read_ul(struct ioapic_softc *sc, int regid)
{
	uint32_t val;

	*(sc->sc_reg) = regid;
	val = *sc->sc_data;

	return val;
}

static inline  void
ioapic_write_ul(struct ioapic_softc *sc, int regid, uint32_t val)
{
	*(sc->sc_reg) = regid;
	*(sc->sc_data) = val;
}
#endif /* !_IOAPIC_CUSTOM_RW */

static inline uint32_t
ioapic_read(struct ioapic_softc *sc, int regid)
{
	uint32_t val;
	u_long flags;

	flags = ioapic_lock(sc);
	val = ioapic_read_ul(sc, regid);
	ioapic_unlock(sc, flags);
	return val;
}

static inline  void
ioapic_write(struct ioapic_softc *sc, int regid, int val)
{
	u_long flags;

	flags = ioapic_lock(sc);
	ioapic_write_ul(sc, regid, val);
	ioapic_unlock(sc, flags);
}

struct ioapic_softc *
ioapic_find(int apicid)
{
	struct ioapic_softc *sc;

	if (apicid == MPS_ALL_APICS) {	/* XXX mpbios-specific */
		/*
		 * XXX kludge for all-ioapics interrupt support
		 * on single ioapic systems
		 */
		if (nioapics <= 1)
			return ioapics;
		panic("unsupported: all-ioapics interrupt with >1 ioapic");
	}

	for (sc = ioapics; sc != NULL; sc = sc->sc_next)
		if (sc->sc_pic.pic_apicid == apicid)
			return sc;

	return NULL;
}

/*
 * For the case the I/O APICs were configured using ACPI, there must
 * be an option to match global ACPI interrupts with APICs.
 */
struct ioapic_softc *
ioapic_find_bybase(int vec)
{
	struct ioapic_softc *sc;

	for (sc = ioapics; sc != NULL; sc = sc->sc_next) {
		if (vec >= sc->sc_pic.pic_vecbase &&
		    vec < (sc->sc_pic.pic_vecbase + sc->sc_apic_sz))
			return sc;
	}

	return NULL;
}

static inline void
ioapic_add(struct ioapic_softc *sc)
{
	struct ioapic_softc **scp;

	sc->sc_next = NULL;

	for (scp = &ioapics; *scp != NULL; scp = &(*scp)->sc_next)
		;
	*scp = sc;
	nioapics++;
}

void
ioapic_print_redir(struct ioapic_softc *sc, const char *why, int pin)
{
	uint32_t redirlo = ioapic_read(sc, IOAPIC_REDLO(pin));
	uint32_t redirhi = ioapic_read(sc, IOAPIC_REDHI(pin));

	apic_format_redir(device_xname(sc->sc_dev), why, pin, APIC_VECTYPE_IOAPIC, redirhi, redirlo);
}

/* Reprogram the APIC ID, and check that it actually got set. */
void
ioapic_set_id(struct ioapic_softc *sc)
{
	u_int8_t apic_id;

	ioapic_write(sc, IOAPIC_ID, (ioapic_read(sc, IOAPIC_ID) & ~IOAPIC_ID_MASK) | (sc->sc_apicid << IOAPIC_ID_SHIFT));

	apic_id = (ioapic_read(sc, IOAPIC_ID) & IOAPIC_ID_MASK) >> IOAPIC_ID_SHIFT;

	if (apic_id != sc->sc_apicid)
		printf(", can't remap");
	else
		printf(", remapped");
}

CFDRIVER_DECL(NULL, ioapic, &ioapic_cops, DV_DULL, sizeof(struct ioapic_softc));
CFOPS_DECL(ioapic, ioapic_match, ioapic_attach, NULL, NULL);

int
ioapic_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct apic_attach_args *aaa = (struct apic_attach_args *)aux;

	if (strcmp(aaa->aaa_name, match->cf_driver->cd_name) == 0) {
		return (1);
	}
	return (0);
}

/*
 * can't use bus_space_xxx as we don't have a bus handle ...
 */
void
ioapic_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioapic_softc *sc = device_private(self);
	struct apic_attach_args *aaa = (struct apic_attach_args *)aux;
	int apic_id;
	bus_space_handle_t bh;
	uint32_t ver_sz;
	int i;

	sc->sc_dev = self;
	sc->sc_flags = aaa->flags;
	sc->sc_pic.pic_apicid = aaa->apic_id;
	sc->sc_pic.pic_name = device_xname(self);
	sc->sc_pic.pic_ioapic = sc;

	aprint_naive("\n");

	if (ioapic_find(aaa->apic_id) != NULL) {
		aprint_error(": duplicate apic id (ignored)\n");
		return;
	}
	ioapic_add(sc);

	aprint_verbose(": pa 0x%jx", (uintmax_t) aaa->apic_address);


	if (i386_mem_add_mapping(aaa->apic_address, PAGE_SIZE, 0, &bh) != 0) {
		aprint_error(": map failed\n");
		return;
	}
	sc->sc_reg = (volatile uint32_t*) (bh + IOAPIC_REG);
	sc->sc_data = (volatile uint32_t*) (bh + IOAPIC_DATA);

	sc->sc_pa = aaa->apic_address;

	sc->sc_pic.pic_type = PIC_IOAPIC;
	simple_lock_init(&sc->sc_pic.pic_lock, "pic_lock");
	sc->sc_pic.pic_hwmask = ioapic_hwmask;
	sc->sc_pic.pic_hwunmask = ioapic_hwunmask;
	sc->sc_pic.pic_addroute = ioapic_addroute;
	sc->sc_pic.pic_delroute = ioapic_delroute;
	sc->sc_pic.pic_trymask = ioapic_trymask;

	sc->sc_pic.pic_edge_stubs = ioapic_edge_stubs;
	sc->sc_pic.pic_level_stubs = ioapic_level_stubs;
	/*
	sc->sc_pic.pic_intr_get_devname = x86_intr_get_devname;
	sc->sc_pic.pic_intr_get_assigned = x86_intr_get_assigned;
	sc->sc_pic.pic_intr_get_count = x86_intr_get_count;
	*/

	apic_id = (ioapic_read(sc, IOAPIC_ID) & IOAPIC_ID_MASK) >> IOAPIC_ID_SHIFT;
	ver_sz = ioapic_read(sc, IOAPIC_VER);

	ioapic_add(sc);

	sc->sc_apic_vers = (ver_sz & IOAPIC_VER_MASK) >> IOAPIC_VER_SHIFT;
	sc->sc_apic_sz = (ver_sz & IOAPIC_MAX_MASK) >> IOAPIC_MAX_SHIFT;
	sc->sc_apic_sz++;

	if (aaa->apic_vecbase != -1) {
		sc->sc_pic.pic_vecbase = aaa->apic_vecbase;
	} else {
		/*
		 * XXX this assumes ordering of ioapics in the table.
		 * Only needed for broken BIOS workaround (see mpbios.c)
		 */
		sc->sc_pic.pic_vecbase = ioapic_vecbase;
		ioapic_vecbase += sc->sc_apic_sz;
	}

	if (mp_verbose) {
		printf(", %s mode", aaa->flags & IOAPIC_PICMODE ? "PIC" : "virtual wire");
	}

	aprint_verbose(", version 0x%x, %d pins", sc->sc_apic_vers, sc->sc_apic_sz);
	aprint_normal("\n");

	sc->sc_pins = malloc(sizeof(struct ioapic_pin) * sc->sc_apic_sz, M_DEVBUF, M_WAITOK);

	for (i = 0; i < sc->sc_apic_sz; i++) {
		uint32_t redlo, redhi;

		sc->sc_pins[i].ip_next = NULL;
		sc->sc_pins[i].ip_map = NULL;
		sc->sc_pins[i].ip_vector = 0;
		sc->sc_pins[i].ip_type = IST_NONE;
		sc->sc_pins[i].ip_minlevel = 0xff;	 	/* XXX magic */
		sc->sc_pins[i].ip_maxlevel = 0; 		/* XXX magic */
	}

	/*
	 * In case the APIC is not initialized to the correct ID
	 * do it now.
	 * Maybe we should record the original ID for interrupt
	 * mapping later ...
	 */
	if (apic_id != sc->sc_pic.pic_apicid) {
		aprint_debug_dev(sc->sc_dev, "misconfigured as apic %d\n", apic_id);

		ioapic_set_id(sc);
	}

#if 0
	/* output of this was boring. */
	if (mp_verbose)
		for (i = 0; i < sc->sc_apic_sz; i++)
			ioapic_print_redir(sc, "boot", i);
#endif
}

struct intrhand *apic_intrhand[256];
int	apic_maxlevel[256];

static void
apic_set_redir(struct ioapic_softc *sc, int pin)
{
	uint32_t redlo;
	uint32_t redhi;
	int delmode;
	struct ioapic_pin *pp;
	struct mp_intr_map *map;

	pp = &sc->sc_pins[pin];
	map = pp->ip_map;
	redlo = map == NULL ? IOAPIC_REDLO_MASK : map->redir;
	redhi = 0;
	delmode = (redlo & IOAPIC_REDLO_DEL_MASK) >> IOAPIC_REDLO_DEL_SHIFT;

	/* XXX magic numbers */
	if ((delmode != 0) && (delmode != 1))
		;
	else if (pp->ip_handler == NULL) {
		redlo |= IOAPIC_REDLO_MASK;
	} else {
		redlo |= (pp->ip_vector & 0xff);
		redlo &= ~IOAPIC_REDLO_DEL_MASK;
		redlo |= (IOAPIC_REDLO_DEL_FIXED << IOAPIC_REDLO_DEL_SHIFT);
		redlo &= ~IOAPIC_REDLO_DSTMOD;

		/*
		 * Destination: BSP CPU
		 *
		 * XXX will want to distribute interrupts across cpu's
		 * eventually.  most likely, we'll want to vector each
		 * interrupt to a specific CPU and load-balance across
		 * cpu's.  but there's no point in doing that until after
		 * most interrupts run without the kernel lock.
		 */
		redhi |= (ioapic_bsp_id << IOAPIC_REDHI_DEST_SHIFT);

		/* XXX derive this bit from BIOS info */
		if (pp->ip_type == IST_LEVEL)
			redlo |= IOAPIC_REDLO_LEVEL;
		else
			redlo &= ~IOAPIC_REDLO_LEVEL;
		if (map != NULL && ((map->flags & 3) == MPS_INTPO_DEF)) {
			if (pp->ip_type == IST_LEVEL)
				redlo |= IOAPIC_REDLO_ACTLO;
			else
				redlo &= ~IOAPIC_REDLO_ACTLO;
		}
	}
	/* Do atomic write */
	ioapic_write(sc, IOAPIC_REDLO(pin), IOAPIC_REDLO_MASK);
	ioapic_write(sc, IOAPIC_REDHI(pin), redhi);
	ioapic_write(sc, IOAPIC_REDLO(pin), redlo);
	if (mp_verbose)
		ioapic_print_redir(sc, "int", pin);
}

/*
 * Throw the switch and enable interrupts..
 */

void
ioapic_enable(void)
{
	if (ioapics == NULL)
		return;

	i8259_setmask(0xffff);

	if (ioapics->sc_flags & IOAPIC_PICMODE) {
		aprint_debug_dev(ioapics->sc_dev, "writing to IMCR to disable pics\n");
		outb(IMCR_ADDR, IMCR_REGISTER);
		outb(IMCR_DATA, IMCR_APIC);
	}
}

void
ioapic_reenable(void)
{
	int p, apic_id;
	struct ioapic_softc *sc;

	if (ioapics == NULL)
		return;

	aprint_normal("%s reenabling\n", device_xname(ioapics->sc_dev));

	for (sc = ioapics; sc != NULL; sc = sc->sc_next) {
		apic_id = (ioapic_read(sc, IOAPIC_ID) & IOAPIC_ID_MASK)
		    >> IOAPIC_ID_SHIFT;
		if (apic_id != sc->sc_pic.pic_apicid) {
			ioapic_write(sc, IOAPIC_ID,
			    (ioapic_read(sc, IOAPIC_ID) & ~IOAPIC_ID_MASK)
			    | (sc->sc_pic.pic_apicid << IOAPIC_ID_SHIFT));
		}

		for (p = 0; p < sc->sc_apic_sz; p++)
			apic_set_redir(sc, p, sc->sc_pins[p].ip_vector, sc->sc_pins[p]);
	}

	ioapic_enable();
}

void
ioapic_hwmask(struct pic *pic, int pin)
{
	uint32_t redlo;
	struct ioapic_softc *sc = pic->pic_ioapic;
	u_long flags;

	flags = ioapic_lock(sc);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo |= IOAPIC_REDLO_MASK;
	redlo &= ~IOAPIC_REDLO_RIRR;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_unlock(sc, flags);
}

boolean_t
ioapic_trymask(struct pic *pic, int pin)
{
	uint32_t redlo;
	struct ioapic_softc *sc = pic->pic_ioapic;
	u_long flags;
	bool rv;

	/* Mask it. */
	flags = ioapic_lock(sc);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo |= IOAPIC_REDLO_MASK;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);

	/* If pending, unmask and abort. */
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	if ((redlo & (IOAPIC_REDLO_RIRR | IOAPIC_REDLO_DELSTS)) != 0) {
		redlo &= ~IOAPIC_REDLO_MASK;
		ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
		rv = FALSE;
	} else {
		rv = TRUE;
	}
	ioapic_unlock(sc, flags);
	return rv;
}

void
ioapic_hwunmask(struct pic *pic, int pin)
{
	uint32_t redlo;
	struct ioapic_softc *sc = pic->pic_ioapic;
	u_long flags;

	flags = ioapic_lock(sc);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo &= ~(IOAPIC_REDLO_MASK | IOAPIC_REDLO_RIRR);
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_unlock(sc, flags);
}

static void
ioapic_addroute(struct pic *pic, struct cpu_info *ci, int pin, int idtvec, int type)
{
	struct ioapic_softc *sc = pic->pic_ioapic;
	struct ioapic_pin *pp;

	pp = &sc->sc_pins[pin];
	pp->ip_type = type;
	pp->ip_vector = idtvec;
	pp->ip_cpu = ci;
	apic_set_redir(sc, pin, idtvec, ci);
}

static void
ioapic_delroute(struct pic *pic, struct cpu_info *ci, int pin,
    int idtvec, int type)
{
	ioapic_hwmask(pic, pin);
}

#ifdef DDB
void ioapic_dump(void);
void ioapic_dump_raw(void);

void
ioapic_dump(void)
{
	struct ioapic_softc *sc;
	struct ioapic_pin *ip;
	int p;

	for (sc = ioapics; sc != NULL; sc = sc->sc_next) {
		for (p = 0; p < sc->sc_apic_sz; p++) {
			ip = &sc->sc_pins[p];
			if (ip->ip_type != IST_NONE)
				ioapic_print_redir(sc, "dump", p);
		}
	}
}

void
ioapic_dump_raw(void)
{
	struct ioapic_softc *sc;
	int i;
	uint32_t reg;

	for (sc = ioapics; sc != NULL; sc = sc->sc_next) {
		printf("Register dump of %s\n", device_xname(sc->sc_dev));
		i = 0;
		do {
			if (i % 0x08 == 0)
				printf("%02x", i);
			reg = ioapic_read(sc, i);
			printf(" %08x", (u_int)reg);
			if (++i % 0x08 == 0)
				printf("\n");
		} while (i < IOAPIC_REDTBL + (sc->sc_apic_sz * 2));
	}
}
#endif
