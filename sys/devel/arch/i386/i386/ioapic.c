/*
 * ioapic.c
 *
 *  Created on: 26 Mar 2021
 *      Author: marti
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <vm/include/vm_extern.h>

#include <arch/i386/isa/isa_machdep.h> 			/* XXX intrhand */

#include <devel/sys/malloctypes.h>
#include <devel/arch/i386/include/cpu.h>

#include <devel/arch/i386/FBSD/apicreg.h>
#include <devel/arch/i386/FBSD/apicvar.h>

#define ioapic_lock_init(lock) 	simple_lock_init(lock, "ioapic_lock")
#define ioapic_lock(lock) 		simple_lock(lock)
#define ioapic_unlock(lock) 	simple_unlock(lock)

struct ioapic_intsrc {
	struct intsrc 			io_intsrc;
	int 					io_irq;
	u_int 					io_intpin:8;
	u_int 					io_vector:8;
	u_int 					io_cpu;
	u_int 					io_activehi:1;
	u_int 					io_edgetrigger:1;
	u_int 					io_masked:1;
	int 					io_bus:4;
	uint32_t 				io_lowreg;
	u_int 					io_remap_cookie;
};

struct ioapic_softc {
	struct pic 				sc_pic;
	struct device			sc_dev;
	SIMPLEQ_ENTRY(ioapic) 	sc_next;
	int						sc_apicid;
	int						sc_apic_vers;
	int						sc_apic_vecbase; 	/* global int base if ACPI */
	int						sc_apic_sz;			/* apic size*/
	int						sc_flags;
	caddr_t					sc_pa;				/* PA of ioapic */
	volatile u_int32_t		*sc_reg;			/* KVA of ioapic addr */
	volatile u_int32_t		*sc_data;			/* KVA of ioapic data */

	struct ioapic_intsrc 	sc_pins;
};
static SIMPLEQ_HEAD(,ioapic_softc) ioapics = SIMPLEQ_HEAD_INITIALIZER(ioapics);

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

	flags = ioapic_lock(sc);
	val = ioapic_read_ul(sc, regid);
	ioapic_unlock(sc);

	return (val);
}

static __inline  void
ioapic_write(struct ioapic_softc *sc,int regid, int val)
{
	u_long flags;

	flags = ioapic_lock(sc);
	ioapic_write_ul(sc, regid, val);
	ioapic_unlock(sc);
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
}

CFDRIVER_DECL(NULL, ioapic, &ioapic_cops, DV_DULL, sizeof(struct ioapic_softc));
CFOPS_DECL(ioapic, ioapic_match, ioapic_attach, NULL, NULL);

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

	sc->sc_flags = aaa->flags;
	sc->sc_apicid = aaa->apic_id;

	if (ioapic_find(aaa->apic_id) != NULL) {
		printf("%s: duplicate apic id (ignored)\n", sc->sc_dev.dv_xname);
		return;
	}

	ioapic_add(sc);

	printf("%s: pa 0x%lx", sc->sc_dev.dv_xname, aaa->apic_address);

	if (i386_mem_add_mapping(aaa->apic_address, PAGE_SIZE, 0, &bh) != 0) {
		printf(": map failed\n");
		return;
	}
	sc->sc_reg = (volatile u_int32_t *)(bh + IOAPIC_REG);
	sc->sc_data = (volatile u_int32_t *)(bh + IOAPIC_DATA);

	ioapic_write(sc, IOAPIC_ID,
			(ioapic_read(sc, IOAPIC_ID) & ~APIC_ID_MASK)
					| (sc->sc_apicid << APIC_ID_SHIFT));
}

int
ioapic_activate(struct device *self, int act)
{
	struct ioapic_softc *sc = (struct ioapic_softc *)self;

	switch (act) {
	case DVACT_RESUME:
		/* On resume, reset the APIC id, like we do on boot */
		ioapic_write(sc, IOAPIC_ID, (ioapic_read(sc, IOAPIC_ID) & ~IOAPIC_ID_MASK) | (sc->sc_apicid << IOAPIC_ID_SHIFT));
	}

	return (0);
}


void
ioapic_hwmask(struct pic *pic, int pin)
{
	u_int32_t redlo;
	struct ioapic_softc *sc = (struct ioapic_softc *)pic;

	if (ioapic_cold) {
		return;
	}
	ioapic_lock(sc);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo |= IOAPIC_REDLO_MASK;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_unlock(sc);
}

void
ioapic_hwunmask(struct pic *pic, int pin)
{
	u_int32_t redlo;
	struct ioapic_softc *sc = (struct ioapic_softc *)pic;

	if (ioapic_cold) {
		return;
	}
	ioapic_lock(sc);
	redlo = ioapic_read_ul(sc, IOAPIC_REDLO(pin));
	redlo &= ~IOAPIC_REDLO_MASK;
	ioapic_write_ul(sc, IOAPIC_REDLO(pin), redlo);
	ioapic_unlock(sc);
}
