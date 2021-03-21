/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1999 Doug Rabson
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
 */

/*
 * Experimenting with idea of FreeBSD's device hints.
 * Basic infrastructure:
 * Supported:
 * - subr_hints.c & kern_environment.c: provides support for the dynamic & static environmet
 * - devices: most device routines have a relatively easy workaround
 *
 * Unsupported:
 * - Bus Mapping: Using the existing isa & bus infrastructure
 */
/* int bus_set_resource(device_t dev, int type, int rid, rman_res_t start, rman_res_t count) */
/*
 * Idea 1:
 * - Two pronged approach:
 * 	- one: (existing) maps the bus space layout
 * 	- two: (new) maps the bus from the device & softc struct (in this case softc = isa_softc)
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <dev/core/isa/isavar.h>

void
isa_hinted_child(parent, name, unit)
	struct device *parent;
	const char *name;
	int unit;
{
	struct device 	*child;
	int				sensitive, start, count;
	int				order;

	/* device-specific flag overrides any wildcard */
	sensitive = 0;
	if (resource_int_value(name, unit, "sensitive", &sensitive) != 0) {
		resource_int_value(name, -1, "sensitive", &sensitive);
	}

	child = BUS_ADD_CHILD(parent, order, name, unit);
	if (child == 0) {
		return;
	}

	start = 0;
	count = 0;
	resource_int_value(name, unit, "port", &start);
	resource_int_value(name, unit, "portsize", &count);
	if (start > 0 || count > 0) {
			bus_set_resource(child, SYS_RES_IOPORT, 0, start, count);
	}

	start = 0;
	count = 0;
	resource_int_value(name, unit, "maddr", &start);
	resource_int_value(name, unit, "msize", &count);
	if (start > 0 || count > 0) {
		bus_set_resource(child, SYS_RES_MEMORY, 0, start, count);
	}

	if (resource_int_value(name, unit, "irq", &start) == 0 && start > 0) {
		bus_set_resource(child, SYS_RES_IRQ, 0, start, 1);
	}

	if (resource_int_value(name, unit, "drq", &start) == 0 && start >= 0) {
		bus_set_resource(child, SYS_RES_DRQ, 0, start, 1);
	}

	if (resource_disabled(name, unit)) {
		device_disable(child);
	}

	isa_set_configattr(child, (isa_get_configattr(child) | ISACF_HINTS));
}

static int
isa_match_resource_hint(dev, type, value)
	struct device *dev;
	int type, value;
{
	struct isa_device* idev = DEVTOISA(dev);
	struct resource_list *rl = &idev->id_resources;
	struct resource_list_entry *rle;

	STAILQ_FOREACH(rle, rl, link) {
		if (rle->type != type)
			continue;
		if (rle->start <= value && rle->end >= value)
			return (1);
	}
	return (0);
}



void
isa_hint_device_unit(bus, child, name, unitp)
	struct device *bus, child;
	const char *name;
	int *unitp;
{
	const char *s;
	long value;
	int line, matches, unit;

	line = 0;
	for (;;) {
		if (resource_find_dev(&line, name, &unit, "at", NULL) != 0)
			break;

		/* Must have an "at" for isa. */
		resource_string_value(name, unit, "at", &s);
		if (!(strcmp(s, device_get_nameunit(bus)) == 0 || strcmp(s, device_get_name(bus)) == 0)) {
			continue;
		}

		/*
		 * Check for matching resources.  We must have at
		 * least one match.  Since I/O and memory resources
		 * cannot be shared, if we get a match on either of
		 * those, ignore any mismatches in IRQs or DRQs.
		 *
		 * XXX: We may want to revisit this to be more lenient
		 * and wire as long as it gets one match.
		 */
		matches = 0;
		if (resource_long_value(name, unit, "port", &value) == 0) {
			/*
			 * Floppy drive controllers are notorious for
			 * having a wide variety of resources not all
			 * of which include the first port that is
			 * specified by the hint (typically 0x3f0)
			 * (see the comment above
			 * fdc_isa_alloc_resources() in fdc_isa.c).
			 * However, they do all seem to include port +
			 * 2 (e.g. 0x3f2) so for a floppy device, look
			 * for 'value + 2' in the port resources
			 * instead of the hint value.
			 */
			if (strcmp(name, "fdc") == 0)
				value += 2;
			if (isa_match_resource_hint(child, SYS_RES_IOPORT, value))
				matches++;
			else
				continue;
		}
		if (resource_long_value(name, unit, "maddr", &value) == 0) {
			if (isa_match_resource_hint(child, SYS_RES_MEMORY, value))
				matches++;
			else
				continue;
		}
		if (matches > 0)
			goto matched;
		if (resource_long_value(name, unit, "irq", &value) == 0) {
			if (isa_match_resource_hint(child, SYS_RES_IRQ, value))
				matches++;
			else
				continue;
		}
		if (resource_long_value(name, unit, "drq", &value) == 0) {
			if (isa_match_resource_hint(child, SYS_RES_DRQ, value))
				matches++;
			else
				continue;
		}

	matched:
		if (matches > 0) {
			/* We have a winner! */
			*unitp = unit;
			break;
		}
	}
}

/* Freebsd's isavar.h */
#define ISACF_HINTS			(1 << 3)	/* source of config is hints */

/* Freebsd's machine/resource.h */
/*
 * Definitions of resource types for Intel Architecture machines
 * with support for legacy ISA devices and drivers.
 */
#define	SYS_RES_IRQ			1	/* interrupt lines */
#define	SYS_RES_DRQ			2	/* isa dma lines */
#define	SYS_RES_MEMORY		3	/* i/o memory */
#define	SYS_RES_IOPORT		4	/* i/o ports */
#ifdef NEW_PCIB
#define	PCI_RES_BUS			5	/* PCI bus numbers */
#endif


struct cfhint {
	struct cfdata		*ch_data;		/* config data */
	struct cfdriver 	*ch_driver;		/* config driver */
	char				*ch_name;		/* device name */
	int					ch_unit;		/**< current unit number */
	char*				ch_nameunit;	/**< name+unit e.g. foodev0 */
	int					ch_flags;
	enum devclass		ch_class;

	int					ch_type;
	short				*ch_parents;	/* potential parents */
};

typedef int 		(*cfhint_t)(struct device *, struct cfdata *, void *);

extern struct cfhint cfhint[];


void
config_set_hints(ch, data, driver, name, unit, class, flags)
	struct cfhint *ch;
	struct cfdata *data;
	struct cfdriver *driver;
	char *name;
	int unit, flags;
	enum devclass class;
{
	ch->ch_data = data;
	ch->ch_driver = driver;
	ch->ch_name = name;
	ch->ch_unit = unit;
	ch->ch_class = class;
	ch->ch_flags = flags;
}


config_hint(driver, parent)
	struct cfdriver 		*driver;
	register struct device *parent;
{
	struct cfhint *ch;
	register short *p;
	for (ch = cfhint; ch->ch_driver; ch++) {

		for (p = ch->ch_parents; *p >= 0; p++) {
			ch->ch_nameunit = parent->dv_xname;
		}
	}
}
