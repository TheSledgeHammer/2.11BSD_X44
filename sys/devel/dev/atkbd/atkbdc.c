/*
 * atkbdc.c
 *
 *  Created on: 11 Jan 2021
 *      Author: marti
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <devel/dev/kbio.h>

#include <machine/clock.h>

#include "atkbdcreg.h"
#include "kbdreg.h"

#include <dev/core/isa/isareg.h>

/* constants */

#define MAXKBDC			MAX(NATKBDC, 1)		/* XXX */

/* macros */

#ifndef MAX
#define MAX(x, y)		((x) > (y) ? (x) : (y))
#endif

#define kbdcp(p)		((atkbdc_softc_t *)(p))
#define nextq(i)		(((i) + 1) % KBDQ_BUFSIZE)
#define availq(q)		((q)->head != (q)->tail)
#if KBDIO_DEBUG >= 2
#define emptyq(q)		((q)->tail = (q)->head = (q)->qcount = 0)
#else
#define emptyq(q)		((q)->tail = (q)->head = 0)
#endif

#define read_data(k)	(bus_space_read_1((k)->iot, (k)->ioh0, 0))
#define read_status(k)	(bus_space_read_1((k)->iot, (k)->ioh1, 0))
#define write_data(k, d)	\
	(bus_space_write_1((k)->iot, (k)->ioh0, 0, (d)))
#define write_command(k, d)	\
	(bus_space_write_1((k)->iot, (k)->ioh1, 0, (d)))

/* local variables */

/*
 * We always need at least one copy of the kbdc_softc struct for the
 * low-level console.  As the low-level console accesses the keyboard
 * controller before kbdc, and all other devices, is probed, we
 * statically allocate one entry. XXX
 */
static atkbdc_softc_t default_kbdc;
static atkbdc_softc_t *atkbdc_softc[MAXKBDC] = { &default_kbdc };

static int verbose = KBDIO_DEBUG;

/* function prototypes */

static int atkbdc_setup(atkbdc_softc_t *sc, bus_space_tag_t tag, bus_space_handle_t h0, bus_space_handle_t h1);
static int addq(kbdkqueue *q, int c);
static int removeq(kbdkqueue *q);
static int wait_while_controller_busy(atkbdc_softc_t *kbdc);
static int wait_for_data(atkbdc_softc_t *kbdc);
static int wait_for_kbd_data(atkbdc_softc_t *kbdc);
static int wait_for_kbd_ack(atkbdc_softc_t *kbdc);
static int wait_for_aux_data(atkbdc_softc_t *kbdc);
static int wait_for_aux_ack(atkbdc_softc_t *kbdc);

atkbdc_softc_t *
atkbdc_get_softc(int unit)
{
	atkbdc_softc_t *sc;

	if (unit >= NELEM(atkbdc_softc))
		return (NULL);
	sc = atkbdc_softc[unit];
	if (sc == NULL) {
		sc = malloc(sizeof(*sc), M_DEVBUF, M_WAITOK | M_ZERO);
		atkbdc_softc[unit] = sc;
	}
	return (sc);
}

int
atkbdc_probe_unit(int unit, struct resource *port0, struct resource *port1)
{
	if (rman_get_start(port0) <= 0)
		return (ENXIO);
	if (rman_get_start(port1) <= 0)
		return (ENXIO);
	return (0);
}

int
atkbdc_attach_unit(int unit, atkbdc_softc_t *sc, bus_space_handle_t port0, bus_space_handle_t port1)
{

	return atkbdc_setup(sc, port0, port0, port1);
}

static int
atkbdc_setup(atkbdc_softc_t *sc, bus_space_tag_t tag, bus_space_handle_t h0, bus_space_handle_t h1)
{
	if (sc->ioh0 == 0) { /* XXX */
		sc->command_byte = -1;
		sc->lock = FALSE;
		sc->kbd.head = sc->kbd.tail = 0;
		sc->aux.head = sc->aux.tail = 0;
//#if KBDIO_DEBUG >= 2
	    sc->kbd.call_count = 0;
	    sc->kbd.qcount = sc->kbd.max_qcount = 0;
	    sc->aux.call_count = 0;
	    sc->aux.qcount = sc->aux.max_qcount = 0;
#endif
	}
	sc->iot = tag;
	sc->ioh0 = h0;
	sc->ioh1 = h1;
	return (0);
}
