/* 	$NetBSD: ioapic.c,v 1.8 2004/02/13 11:36:20 wiz Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <vm/include/vm_extern.h>

#include <machine/isa/isa_machdep.h> 			/* XXX intrhand */

#include <machine/bus.h>
#include <machine/cpufunc.h>
#include <machine/pio.h>
#include <machine/pmap.h>
#include <machine/intr.h>
#include <machine/cpu.h>
#include <machine/pic.h>
#include <machine/mpbiosvar.h>

#include <machine/apic/apic.h>
#include <machine/apic/ioapicreg.h>
#include <machine/apic/ioapicvar.h>

#define ioapic_lock_init(lock) 	simple_lock_init(lock, "ioapic_lock")
#define ioapic_lock(lock) 		simple_lock(lock)
#define ioapic_unlock(lock) 	simple_unlock(lock)

int     	ioapic_match(struct device *, struct cfdata *, void *);
void    	ioapic_attach(struct device *, struct device *, void *);

extern int 	i386_mem_add_mapping(bus_addr_t, bus_size_t, int, bus_space_handle_t *);

static void	ioapic_init_intpins(struct ioapic_softc *);
static void ioapic_hwmask(struct softpic *, int);
static void ioapic_hwunmask(struct softpic *, int);
static void ioapic_addroute(struct softpic *, struct cpu_info *, int, int, int);
static void ioapic_delroute(struct softpic *, struct cpu_info *, int, int, int);
static void	ioapic_register_pic(struct pic *, struct apic *);

int ioapic_bsp_id = 0;
int ioapic_cold = 1;

struct lock_object icu_lock;
struct ioapic_head ioapics = SIMPLEQ_HEAD_INITIALIZER(ioapics);
int nioapics = 0;	   	 	/* number attached */
static int ioapic_vecbase;

struct pic ioapic_template = {
		.pic_type = PIC_IOAPIC,
		.pic_hwmask = ioapic_hwmask,
		.pic_hwunmask = ioapic_hwunmask,
		.pic_addroute = ioapic_addroute,
		.pic_delroute = ioapic_delroute,
		.pic_register = ioapic_register_pic
};

struct apic ioapic_intrmap = {
		.apic_pic_type = PIC_IOAPIC,
		.apic_edge = apic_edge_stubs,
		.apic_level = apic_level_stubs
};

/*
 * Register read/write routines.
 */
static __inline  u_int32_t
ioapic_read_ul(struct ioapic_softc *sc,int regid)
{
	u_int32_t val;

	*(sc->sc_reg) = regid;
	val = *sc->sc_data;

	return (val);
}

static __inline  void
ioapic_write_ul(struct ioapic_softc *sc,int regid, u_int32_t val)
{
	*(sc->sc_reg) = regid;
	*(sc->sc_data) = val;
}

static __inline u_int32_t
ioapic_read(struct ioapic_softc *sc, int regid)
{
	u_int32_t val;
	u_long flags;

	ioapic_lock(&icu_lock);
	val = ioapic_read_ul(sc, regid);
	ioapic_unlock(&icu_lock);

	return (val);
}

static __inline  void
ioapic_write(struct ioapic_softc *sc,int regid, int val)
{
	u_long flags;

	ioapic_lock(&icu_lock);
	ioapic_write_ul(sc, regid, val);
	ioapic_unlock(&icu_lock);
}

struct ioapic_softc *
ioapic_find(int apicid)
{
	struct ioapic_softc *sc;
	SIMPLEQ_FOREACH(sc, &ioapics, sc_next) {
		if (sc->sc_apicid == apicid) {
			return (sc);
		}
	}
	return (NULL);
}

/*
 * For the case the I/O APICs were configured using ACPI, there must
 * be an option to match global ACPI interrupts with APICs.
 */
struct ioapic_softc *
ioapic_find_bybase(int vec)
{
	struct ioapic_softc *sc;

	SIMPLEQ_FOREACH(sc, &ioapics, sc_next) {
		if (vec >= sc->sc_apic_vecbase && vec < (sc->sc_apic_vecbase + sc->sc_apic_sz)) {
			return (sc);
		}
	}
	return (NULL);
}

static __inline void
ioapic_add(struct ioapic_softc *sc)
{
	SIMPLEQ_INSERT_TAIL(&ioapics, sc, sc_next);
	nioapics++;
}

void
ioapic_print_redir(struct ioapic_softc *sc, const char *why, int pin)
{
	uint32_t redirlo = ioapic_read(sc, IOAPIC_REDLO(pin));
	uint32_t redirhi = ioapic_read(sc, IOAPIC_REDHI(pin));

	apic_format_redir(sc->sc_dev->dv_xname, why, pin, APIC_VECTYPE_IOAPIC, redirhi, redirlo);
}

CFOPS_DECL(ioapic, ioapic_match, ioapic_attach, NULL, NULL);
CFDRIVER_DECL(NULL, ioapic, &ioapic_cops, DV_DULL, sizeof(struct ioapic_softc));

int
ioapic_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct apic_attach_args *aaa = (struct apic_attach_args *) aux;

	if (strcmp(aaa->aaa_name, match->cf_driver->cd_name) == 0) {
		return (1);
	}
	return (0);
}

void
ioapic_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioapic_softc *sc = (struct ioapic_softc *)self;
	struct apic_attach_args  *aaa = (struct apic_attach_args  *) aux;
	int apic_id;
	bus_space_handle_t bh;
	u_int32_t ver_sz;
	int i;

	sc->sc_dev = self;
	sc->sc_flags = aaa->flags;
	sc->sc_apicid = aaa->apic_id;
	sc->sc_softpic->sp_ioapic = sc;
	
	printf(" apid %d (I/O APIC)\n", aaa->apic_id);

	if (ioapic_find(aaa->apic_id) != NULL) {
		printf("%s: duplicate apic id (ignored)\n", sc->sc_dev->dv_xname);
		return;
	}
	
	ioapic_add(sc);

	printf("%s: pa 0x%lx", sc->sc_dev->dv_xname, aaa->apic_address);

	if (i386_mem_add_mapping(aaa->apic_address, PAGE_SIZE, 0, &bh) != 0) {
		printf(": map failed\n");
		return;
	}
	sc->sc_reg = (volatile u_int32_t *)(bh + IOAPIC_REG);
	sc->sc_data = (volatile u_int32_t *)(bh + IOAPIC_DATA);

	sc->sc_pic = &ioapic_template;
	sc->sc_apic = &ioapic_intrmap;
	ioapic_lock_init(&icu_lock);

	/* add others here */

	apic_id = (ioapic_read(sc, IOAPIC_ID)&IOAPIC_ID_MASK)>>IOAPIC_ID_SHIFT;
	ver_sz = ioapic_read(sc, IOAPIC_VER);

	sc->sc_apic_vers = (ver_sz & IOAPIC_VER_MASK) >> IOAPIC_VER_SHIFT;
	sc->sc_apic_sz = (ver_sz & IOAPIC_MAX_MASK) >> IOAPIC_MAX_SHIFT;
	sc->sc_apic_sz++;

	if (aaa->apic_vecbase != -1) {
		sc->sc_apic_vecbase = aaa->apic_vecbase;
	} else {
		/*
		 * XXX this assumes ordering of ioapics in the table.
		 * Only needed for broken BIOS workaround (see mpbios.c)
		 */
		sc->sc_apic_vecbase = ioapic_vecbase;
		ioapic_vecbase += sc->sc_apic_sz;
	}

#ifdef SMP
	if (mp_verbose) {
		printf(", %s mode",	aaa->flags & IOAPIC_PICMODE ? "PIC" : "virtual wire");
	}
#endif

	printf(", version %x, %d pins\n", sc->sc_apic_vers, sc->sc_apic_sz);

	ioapic_init_intpins(sc);

	/*
	 * In case the APIC is not initialized to the correct ID
	 * do it now.
	 * Maybe we should record the original ID for interrupt
	 * mapping later ...
	 */
	if (apic_id != sc->sc_apicid) {
		printf("%s: misconfigured as apic %d\n", sc->sc_dev->dv_xname, apic_id);

		ioapic_write(sc, IOAPIC_ID, (ioapic_read(sc, IOAPIC_ID) & ~IOAPIC_ID_MASK) | (sc->sc_apicid << IOAPIC_ID_SHIFT));

		apic_id = (ioapic_read(sc, IOAPIC_ID) & IOAPIC_ID_MASK)	>> IOAPIC_ID_SHIFT;

		if (apic_id != sc->sc_apicid) {
			printf("%s: can't remap to apid %d\n", sc->sc_dev->dv_xname, sc->sc_apicid);
		} else {
			printf("%s: remapped to apic %d\n", sc->sc_dev->dv_xname, sc->sc_apicid);
		}
	}
}

/*
 * Initialize pins.  Start off with interrupts disabled.  Default
 * to active-hi and edge-triggered for ISA interrupts and active-lo
 * and level-triggered for all others.
 */
static void
ioapic_init_intpins(struct ioapic_softc *sc)
{
	struct softpic *spic;
	struct softpic_pin *spin;
	int i;
	
	spin = calloc(sc->sc_apic_sz, sizeof(struct softpic_pin), M_DEVBUF, M_WAITOK);
	for (i = 0, spic = sc->sc_softpic; i < sc->sc_apic_sz; i++, spic++) {
		spin[i].sp_next = NULL;
		spin[i].sp_map = NULL;
		spin[i].sp_vector = 0;
		spin[i].sp_type = IST_NONE;
		spic->sp_pins[i] = spin[i];
		spic->sp_ioapic = sc;
	}
	sc = spic->sp_ioapic;
}

static void
apic_set_redir(struct ioapic_softc *sc, int pin, int idt_vec, struct cpu_info *ci)
{
	u_int32_t redlo;
	u_int32_t redhi = 0;
	int delmode;

	struct softpic *sp;
	struct softpic_pin	*pp;
	struct mp_intr_map *map;

	sp = sc->sc_softpic;
	pp = &sp->sp_pins[pin];
	map = pp->sp_map;
	redlo = map == NULL ? IOAPIC_REDLO_MASK : map->redir;
	delmode = (redlo & IOAPIC_REDLO_DEL_MASK) >> IOAPIC_REDLO_DEL_SHIFT;

	/* XXX magic numbers */
	if ((delmode != 0) && (delmode != 1)) {
		;
	} else if (pp->sp_type == IST_NONE) {
		redlo |= IOAPIC_REDLO_MASK;
	} else {
		redlo |= (idt_vec & 0xff);
		redlo |= (IOAPIC_REDLO_DEL_FIXED << IOAPIC_REDLO_DEL_SHIFT);
		redlo &= ~IOAPIC_REDLO_DSTMOD;

		/*
		 * Destination: BSP CPU
		 *
		 * XXX will want to distribute interrupts across CPUs
		 * eventually.  most likely, we'll want to vector each
		 * interrupt to a specific CPU and load-balance across
		 * CPUs.  but there's no point in doing that until after
		 * most interrupts run without the kernel lock.
		 */
		redhi |= (ioapic_bsp_id << IOAPIC_REDHI_DEST_SHIFT);

		/* XXX derive this bit from BIOS info */
		if (pp->sp_type == IST_LEVEL) {
			redlo |= IOAPIC_REDLO_LEVEL;
		} else {
			redlo &= ~IOAPIC_REDLO_LEVEL;
		}
		if (map != NULL && ((map->flags & 3) == MPS_INTPO_DEF)) {
			if (pp->sp_type == IST_LEVEL) {
				redlo |= IOAPIC_REDLO_ACTLO;
			} else {
				redlo &= ~IOAPIC_REDLO_ACTLO;
			}
		}
	}
	ioapic_write(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_write(sc, IOAPIC_REDHI(pin), redhi);

#ifdef SMP
	if (mp_verbose) {
		ioapic_print_redir(sc, "int", pin);
	}
#endif
}

/*
 * Throw the switch and enable interrupts..
 */
void
ioapic_enable(void)
{
	int p;
	struct ioapic_softc *sc;
	struct softpic 		*spic;
	struct softpic_pin	*spin;

	ioapic_cold = 0;

	if (SIMPLEQ_EMPTY(&ioapics) == NULL) {
		return;
	}

	if (sc->sc_flags & IOAPIC_PICMODE) {
		printf("%s: writing to IMCR to disable pics\n", sc->sc_dev->dv_xname);
		outb(IMCR_ADDR, IMCR_REGISTER);
		outb(IMCR_DATA, IMCR_APIC);
	}

	SIMPLEQ_FOREACH(sc, &ioapics, sc_next) {
		printf("%s: enabling\n", sc->sc_dev->dv_xname);

		for (p = 0; p < sc->sc_apic_sz; p++) {
			spic = sc->sc_softpic;
			spin = &spic->sp_pins[p];
			if (spin->sp_type != IST_NONE) {
				apic_set_redir(sc, p, spin->sp_vector, spin->sp_cpu);
			}
		}
	}
}

void
ioapic_hwmask(spic, pin)
	struct softpic *spic;
	int pin;
{
	u_int32_t redlo;
	struct ioapic_softc *sc;

	sc = spic->sp_ioapic;
	softpic_pic_hwmask(spic, pin, TRUE, PIC_IOAPIC);

	if (ioapic_cold) {
		return;
	}
	ioapic_lock(&icu_lock);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo |= IOAPIC_REDLO_MASK;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_unlock(&icu_lock);
}

void
ioapic_hwunmask(spic, pin)
	struct softpic *spic;
	int pin;
{
	u_int32_t redlo;
	struct ioapic_softc *sc;

	sc = spic->sp_ioapic;
	softpic_pic_hwunmask(spic, pin, TRUE, PIC_IOAPIC);

	if (ioapic_cold) {
		return;
	}
	ioapic_lock(&icu_lock);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo &= ~IOAPIC_REDLO_MASK;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_unlock(&icu_lock);
}

static void
ioapic_addroute(spic, ci, pin, idtvec, type)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type;
{
	struct ioapic_softc *sc;
	struct softpic_pin *spin;

	sc = spic->sp_ioapic;
	softpic_pic_addroute(spic, ci, pin, idtvec, type, TRUE, PIC_IOAPIC);

	if (ioapic_cold) {
		spin = &spic->sp_pins[pin];
		spin->sp_type = type;
		spin->sp_vector = idtvec;
		spin->sp_cpu = ci;
		return;
	}
	apic_set_redir(sc, pin, idtvec, ci);
}

static void
ioapic_delroute(spic, ci, pin, idtvec, type)
	struct softpic *spic;
	struct cpu_info *ci;
	int pin, idtvec, type;
{
	struct ioapic_softc *sc;
	struct softpic_pin *spin;

	sc = spic->sp_ioapic;
	softpic_pic_delroute(spic, ci, pin, idtvec, type, TRUE, PIC_IOAPIC);

	if (ioapic_cold) {
		spin = &spic->sp_pins[pin];
		spin->sp_type = IST_NONE;
		return;
	}
	ioapic_hwmask(spic, pin);
}

/*
 * Register I/O APIC interrupt pins.
 */
static void
ioapic_register_pic(pic, apic)
	struct pic *pic;
	struct apic *apic;
{
	pic = &ioapic_template;
	apic = &ioapic_intrmap;
	softpic_register(pic, apic);
}

#ifdef DDB
void ioapic_dump(void);
void
ioapic_dump(void)
{
	struct ioapic_softc *sc;
	struct softpic 		*spic;
	struct softpic_pin 	*spin;
	int p;

	SIMPLEQ_FOREACH(sc, &ioapics, sc_next) {
		for (p = 0; p < sc->sc_apic_sz; p++) {
			spin = &spic->sp_pins[p];
			if (spin->sp_type != IST_NONE) {
				ioapic_print_redir(sc, "dump", p);
			}
		}
	}
}

#endif
