/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2003-2004 Poul-Henning Kamp
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <devel/sys/timetc.h>

#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcidevs.h>

#include <dev/led/led.h>

#include <machine/bus.h>
#include <machine/bios.h>

#include <devel/arch/i386/pci/geodescreg.h>

struct geodesc_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
};

static struct bios_oem bios_soekris = {
		{ 0xf0000, 0xf1000 },
		{
				{ "Soekris", 0, 8 },			/* Soekris Engineering. */
				{ "net4", 0, 8 },				/* net45xx */
				{ "comBIOS", 0, 54 },			/* comBIOS ver. 1.26a  20040819 ... */
				{ NULL, 0, 0 },
		}
};

static struct bios_oem bios_soekris_55 = {
		{ 0xf0000, 0xf1000 },
		{
				{ "Soekris", 0, 8 },			/* Soekris Engineering. */
				{ "net5", 0, 8 },				/* net5xxx */
				{ "comBIOS", 0, 54 },			/* comBIOS ver. 1.26a  20040819 ... */
				{ NULL, 0, 0 },
		}
};

static struct bios_oem bios_pcengines = {
		{ 0xf9000, 0xfa000 },
		{
				{ "PC Engines WRAP", 0, 28 },	/* PC Engines WRAP.1C v1.03 */
				{ "tinyBIOS", 0, 28 },			/* tinyBIOS V1.4a (C)1997-2003 */
				{ NULL, 0, 0 },
		}
};

static struct bios_oem bios_pcengines_55 = {
		{ 0xf9000, 0xfa000 },
		{
				{ "PC Engines ALIX", 0, 28 },	/* PC Engines ALIX */
				{ "tinyBIOS", 0, 28 },			/* tinyBIOS V1.4a (C)1997-2005 */
				{ NULL, 0, 0 },
		}
};

static struct bios_oem bios_advantech = {
		{ 0xfe000, 0xff000 },
		{
				{ "**** PCM-582", 5, 33 },		/* PCM-5823 BIOS V1.12 ... */
				{ "GXm-Cx5530",	-11, 35 },		/* 06/07/2002-GXm-Cx5530... */
				{ NULL, 0, 0 },
		}
};

static unsigned	cba;
static unsigned	gpio;
static unsigned	geode_counter;

static void
led_func(void *ptr, int onoff)
{
	uint32_t u;
	int bit;

	bit = *(int *)ptr;
	if (bit < 0) {
		bit = -bit;
		onoff = !onoff;
	}

	u = inl(gpio + 4);
	if (onoff)
		u |= 1 << bit;
	else
		u &= ~(1 << bit);
	outl(gpio, u);
}

static void
cs5536_led_func(void *ptr, int onoff)
{
	int bit;
	uint16_t a;

	bit = *(int *)ptr;
	if (bit < 0) {
		bit = -bit;
		onoff = !onoff;
	}

	a = rdmsr(0x5140000c);
	if (bit >= 16) {
		a += 0x80;
		bit -= 16;
	}

	if (onoff)
		outl(a, 1 << bit);
	else
		outl(a, 1 << (bit + 16));
}

static unsigned
geode_get_timecount(struct timecounter *tc)
{
	return (inl(geode_counter));
}

static struct timecounter geode_timecounter = {
	.tc_get_timecount = geode_get_timecount,
	NULL,
	.tc_counter_mask = 0xffffffff,
	.tc_frequency = 27000000,
	.tc_name = "Geode",
	1000
};

static uint64_t
geode_cputicks(void)
{
	unsigned c;
	static unsigned last;
	static uint64_t offset;

	c = inl(geode_counter);
	if (c < last)
		offset += (1LL << 32);
	last = c;
	return (offset | c);
}


/*
 * The GEODE watchdog runs from a 32kHz frequency.  One period of that is
 * 31250 nanoseconds which we round down to 2^14 nanoseconds.  The watchdog
 * consists of a power-of-two prescaler and a 16 bit counter, so the math
 * is quite simple.  The max timeout is 14 + 16 + 13 = 2^43 nsec ~= 2h26m.
 */
static void
geode_watchdog(void *foo, u_int cmd, int *error)
{
	u_int u, p, r;

	u = cmd & WD_INTERVAL;
	if (u >= 14 && u <= 43) {
		u -= 14;
		if (u > 16) {
			p = u - 16;
			u -= p;
		} else {
			p = 0;
		}
		if (u == 16)
			u = (1 << u) - 1;
		else
			u = 1 << u;
		r = inw(cba + 2) & 0xff00;
		outw(cba + 2, p | 0xf0 | r);
		outw(cba, u);
		*error = 0;
	} else {
		outw(cba, 0);
	}
}

int
geode_probe(self, pa)
	struct device *self;
	struct pci_attach_args *pa;
{
#define BIOS_OEM_MAXLEN 80
	static u_char bios_oem[BIOS_OEM_MAXLEN] = "\0";

	switch (pci_get_devid(self)) {
	case 0x0515100b:
		if (geode_counter == 0) {
			cba = pci_conf_read(self, 0x64, 4);
			if (bootverbose) {
				printf("Geode CBA@ 0x%x\n", cba);
			}
			geode_counter = cba + 0x08;
			outl(cba + 0x0d, 2);
			if (bootverbose) {
				printf("Geode rev: %02x %02x\n", inb(cba + 0x3c), inb(cba + 0x3d));
			}
			tc_init(&geode_timecounter);
		}
		break;
	case 0x0510100b:
		gpio = pci_read_config(self, PCIR_BAR(0), 4);
		gpio &= ~0x1f;
		if (bootverbose) {
			printf("Geode GPIO@ = %x\n", gpio);
		}
		if (bios_oem_strings(&bios_soekris, bios_oem, sizeof(bios_oem)) > 0 ) {

		} else if (bios_oem_strings(&bios_pcengines, bios_oem, sizeof(bios_oem)) > 0 ) {

		}
		if (*bios_oem) {
			printf("Geode %s\n", bios_oem);
		}
		break;
	case 0x01011078:
		if (bios_oem_strings(&bios_advantech, bios_oem, sizeof(bios_oem)) > 0 ) {
			printf("Geode %s\n", bios_oem);
		}
		break;
	case 0x20801022:
		if (bios_oem_strings(&bios_soekris_55, bios_oem, sizeof(bios_oem)) > 0 ) {

		} else if (bios_oem_strings(&bios_pcengines_55, bios_oem, sizeof(bios_oem)) > 0 ) {

		}
		if (*bios_oem) {
			printf("Geode LX: %s\n", bios_oem);
		}
		if (bootverbose) {
			printf("MFGPT bar: %jx\n", rdmsr(0x5140000d));
		}
		break;
	}
}

int
geodesc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_NS &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NS_SC1100_XBUS ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_NS_SCX200_XBUS)) {
		return (1);
	}
	return (0);
}

#define WDSTSBITS "\20\x04WDRST\x03WDSMI\x02WDINT\x01WDOVF"

void
geodesc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{

}
