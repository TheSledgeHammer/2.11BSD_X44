/*	$NetBSD: isapnp.c,v 1.42 2003/12/04 13:57:30 keihan Exp $	*/

/*
 * Copyright (c) 1996 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

/*
 * ISA PnP bus autoconfiguration.
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <machine/bus.h>

//#include <dev/core/isa/isadmavar.h>
#include <dev/core/isa/isavar.h>
#include <dev/core/isapnp/isapnpreg.h>
#include <dev/core/isapnp/isapnpvar.h>
#include <dev/core/isapnp/isapnpdevs.h>

#ifndef ISAPNP_ALLOC_INTR_MASK
#define ISAPNP_ALLOC_INTR_MASK (~0)
#endif

static void isapnp_init(struct isapnp_softc *);
static __inline u_char isapnp_shift_bit(struct isapnp_softc *);
static int isapnp_findcard(struct isapnp_softc *);
static void isapnp_free_region(bus_space_tag_t, struct isapnp_region *);
static int isapnp_alloc_region(bus_space_tag_t, struct isapnp_region *);
static int isapnp_alloc_irq(isa_chipset_tag_t, struct isapnp_pin *);
static int isapnp_alloc_drq(isa_chipset_tag_t, struct isapnp_pin *);
static int isapnp_testconfig(bus_space_tag_t, bus_space_tag_t, struct isapnp_attach_args *, int);
static struct isapnp_attach_args *isapnp_bestconfig(struct isapnp_softc *, struct isapnp_attach_args **);
static void isapnp_print_region(const char *, struct isapnp_region *, size_t);
static void isapnp_configure(struct isapnp_softc *, const struct isapnp_attach_args *);
static void isapnp_print_pin(const char *, struct isapnp_pin *, size_t);
static int isapnp_print(void *, const char *);
#ifdef _KERNEL
static int isapnp_submatch(struct device *, struct cfdata *, void *);
#endif
static int isapnp_find(struct isapnp_softc *, int);
static int isapnp_match(struct device *, struct cfdata *, void *);
static void isapnp_attach(struct device *, struct device *, void *);
static void isapnp_callback(struct device *);

CFOPS_DECL(isapnp, isapnp_match, isapnp_attach, NULL, NULL);
CFDRIVER_DECL(NULL, isapnp, DV_DULL);
CFATTACH_DECL(isapnp, &isapnp_cd, &isapnp_cops, sizeof(struct isapnp_softc));

/*
 * This keeps track if which ISA's we have been probed on.
 */
struct isapnp_probe_cookie {
	LIST_ENTRY(isapnp_probe_cookie)	ipc_link;
	struct device *ipc_parent;
};
LIST_HEAD(, isapnp_probe_cookie) isapnp_probes =
    LIST_HEAD_INITIALIZER(isapnp_probes);

/* isapnp_init():
 *	Write the PNP initiation key to wake up the cards...
 */
static void
isapnp_init(sc)
	struct isapnp_softc *sc;
{
	int i;
	u_char v = ISAPNP_LFSR_INIT;

	/* First write 0's twice to enter the Wait for Key state */
	ISAPNP_WRITE_ADDR(sc, 0);
	ISAPNP_WRITE_ADDR(sc, 0);

	/* Send the 32 byte sequence to awake the logic */
	for (i = 0; i < ISAPNP_LFSR_LENGTH; i++) {
		ISAPNP_WRITE_ADDR(sc, v);
		v = ISAPNP_LFSR_NEXT(v);
	}
}

/* isapnp_shift_bit():
 *	Read a bit at a time from the config card.
 */
static __inline u_char
isapnp_shift_bit(sc)
	struct isapnp_softc *sc;
{
	u_char c1, c2;

	DELAY(250);
	c1 = ISAPNP_READ_DATA(sc);
	DELAY(250);
	c2 = ISAPNP_READ_DATA(sc);

	if (c1 == 0x55 && c2 == 0xAA)
		return (0x80);
	else
		return (0);
}


/* isapnp_findcard():
 *	Attempt to read the vendor/serial/checksum for a card
 *	If a card is found [the checksum matches], assign the
 *	next card number to it and return 1
 */
static int
isapnp_findcard(sc)
	struct isapnp_softc *sc;
{
	u_char v = ISAPNP_LFSR_INIT, csum, w;
	int i, b;

	if (sc->sc_ncards == ISAPNP_MAX_CARDS) {
		printf("%s: Too many pnp cards\n", sc->sc_dev.dv_xname);
		return (0);
	}

	/* Set the read port */
	isapnp_write_reg(sc, ISAPNP_WAKE, 0);
	isapnp_write_reg(sc, ISAPNP_SET_RD_PORT, sc->sc_read_port >> 2);
	sc->sc_read_port |= 3;
	DELAY(1000);

	ISAPNP_WRITE_ADDR(sc, ISAPNP_SERIAL_ISOLATION);
	DELAY(1000);

	/* Read the 8 bytes of the Vendor ID and Serial Number */
	for (i = 0; i < 8; i++) {
		/* Read each bit separately */
		for (w = 0, b = 0; b < 8; b++) {
			u_char neg = isapnp_shift_bit(sc);

			w >>= 1;
			w |= neg;
			v = ISAPNP_LFSR_NEXT(v) ^ neg;
		}
		sc->sc_id[sc->sc_ncards][i] = w;
	}

	/* Read the remaining checksum byte */
	for (csum = 0, b = 0; b < 8; b++) {
		u_char neg = isapnp_shift_bit(sc);

		csum >>= 1;
		csum |= neg;
	}
	sc->sc_id[sc->sc_ncards][8] = csum;

	if (csum == v) {
		sc->sc_ncards++;
		isapnp_write_reg(sc, ISAPNP_CARD_SELECT_NUM, sc->sc_ncards);
		return (1);
	}
	return (0);
}

/* isapnp_free_region():
 *	Free a region
 */
static void
isapnp_free_region(t, r)
	bus_space_tag_t t;
	struct isapnp_region *r;
{
#ifdef _KERNEL
	bus_space_unmap(t, r->h, r->length);
#endif
}

/* isapnp_alloc_region():
 *	Allocate a single region if possible
 */
static int
isapnp_alloc_region(t, r)
	bus_space_tag_t t;
	struct isapnp_region *r;
{
	int error = 0;

	for (r->base = r->minbase; r->base <= r->maxbase; r->base += r->align) {
#ifdef _KERNEL
		error = bus_space_map(t, r->base, r->length, 0, &r->h);
#endif
		if (error == 0)
			return (0);
		if (r->align == 0)
			break;
	}
	return (error);
}

/* isapnp_alloc_irq():
 *	Allocate an irq
 */
static int
isapnp_alloc_irq(ic, i)
	isa_chipset_tag_t ic;
	struct isapnp_pin *i;
{
	int irq;
#define LEVEL_IRQ (ISAPNP_IRQTYPE_LEVEL_PLUS|ISAPNP_IRQTYPE_LEVEL_MINUS)
	i->type = (i->flags & LEVEL_IRQ) ? IST_LEVEL : IST_EDGE;

	if (i->bits == 0) {
		i->num = 0;
		return (0);
	}

	if (isa_intr_alloc(ic, i->bits, i->type, &irq) == 0) {
		i->num = irq;
		return (0);
	}

	return (EINVAL);
}

/* isapnp_alloc_drq():
 *	Allocate a drq
 */
static int
isapnp_alloc_drq(ic, i)
	isa_chipset_tag_t ic;
	struct isapnp_pin *i;
{
#if NISADMA > 0
	int b;

	if (i->bits == 0) {
		i->num = 0;
		return (0);
	}

	for (b = 0; b < 16; b++)
		if ((i->bits & (1 << b)) && isa_drq_isfree(isa, b)) {
			i->num = b;
			return (0);
		}
#endif

	return (EINVAL);
}

/* isapnp_testconfig():
 *	Test/Allocate the regions used
 */
static int
isapnp_testconfig(iot, memt, ipa, alloc)
	bus_space_tag_t iot, memt;
	struct isapnp_attach_args *ipa;
	int alloc;
{
	int nio = 0, nmem = 0, nmem32 = 0, nirq = 0, ndrq = 0;
	int error = 0;

#ifdef DEBUG_ISAPNP
	isapnp_print_attach(ipa);
#endif

	for (; nio < ipa->ipa_nio; nio++) {
		error = isapnp_alloc_region(iot, &ipa->ipa_io[nio]);
		if (error)
			goto bad;
	}

	for (; nmem < ipa->ipa_nmem; nmem++) {
		error = isapnp_alloc_region(memt, &ipa->ipa_mem[nmem]);
		if (error)
			goto bad;
	}

	for (; nmem32 < ipa->ipa_nmem32; nmem32++) {
		error = isapnp_alloc_region(memt, &ipa->ipa_mem32[nmem32]);
		if (error)
			goto bad;
	}

	for (; nirq < ipa->ipa_nirq; nirq++) {
		error = isapnp_alloc_irq(ipa->ipa_ic, &ipa->ipa_irq[nirq]);
		if (error)
			goto bad;
	}

	for (; ndrq < ipa->ipa_ndrq; ndrq++) {
		error = isapnp_alloc_drq(ipa->ipa_ic, &ipa->ipa_drq[ndrq]);
		if (error)
			goto bad;
	}

	if (alloc)
		return error;

bad:
#ifdef notyet
	for (ndrq--; ndrq >= 0; ndrq--)
		isapnp_free_pin(&ipa->ipa_drq[ndrq]);

	for (nirq--; nirq >= 0; nirq--)
		isapnp_free_pin(&ipa->ipa_irq[nirq]);
#endif

	for (nmem32--; nmem32 >= 0; nmem32--)
		isapnp_free_region(memt, &ipa->ipa_mem32[nmem32]);

	for (nmem--; nmem >= 0; nmem--)
		isapnp_free_region(memt, &ipa->ipa_mem[nmem]);

	for (nio--; nio >= 0; nio--)
		isapnp_free_region(iot, &ipa->ipa_io[nio]);

	return error;
}


/* isapnp_config():
 *	Test/Allocate the regions used
 */
int
isapnp_config(iot, memt, ipa)
	bus_space_tag_t iot, memt;
	struct isapnp_attach_args *ipa;
{
	return isapnp_testconfig(iot, memt, ipa, 1);
}


/* isapnp_unconfig():
 *	Free the regions used
 */
void
isapnp_unconfig(iot, memt, ipa)
	bus_space_tag_t iot, memt;
	struct isapnp_attach_args *ipa;
{
	int i;

#ifdef notyet
	for (i = 0; i < ipa->ipa_ndrq; i++)
		isapnp_free_pin(&ipa->ipa_drq[i]);

	for (i = 0; i < ipa->ipa_nirq; i++)
		isapnp_free_pin(&ipa->ipa_irq[i]);
#endif

	for (i = 0; i < ipa->ipa_nmem32; i++)
		isapnp_free_region(memt, &ipa->ipa_mem32[i]);

	for (i = 0; i < ipa->ipa_nmem; i++)
		isapnp_free_region(memt, &ipa->ipa_mem[i]);

	for (i = 0; i < ipa->ipa_nio; i++)
		isapnp_free_region(iot, &ipa->ipa_io[i]);
}


/* isapnp_bestconfig():
 *	Return the best configuration for each logical device, remove and
 *	free all other configurations.
 */
static struct isapnp_attach_args *
isapnp_bestconfig(sc, ipa)
	struct isapnp_softc *sc;
	struct isapnp_attach_args **ipa;
{
	struct isapnp_attach_args *c, *best, *f = *ipa;
	int error;

	for (;;) {
		if (f == NULL)
			return (NULL);

#define SAMEDEV(a, b) (strcmp((a)->ipa_devlogic, (b)->ipa_devlogic) == 0)

		/* Find the best config */
		for (best = c = f; c != NULL; c = c->ipa_sibling) {
			if (!SAMEDEV(c, f))
				continue;
			if (c->ipa_pref < best->ipa_pref)
				best = c;
		}

		/*
		 * Make sure the ISA chipset is initialized!  We need
		 * it to test the best config!
		 */
		best->ipa_ic = sc->sc_ic;

		/* Test the best config */
		error = isapnp_testconfig(sc->sc_iot, sc->sc_memt, best, 0);

		/* Remove this config from the list */
		if (best == f)
			f = f->ipa_sibling;
		else {
			for (c = f; c->ipa_sibling != best; c = c->ipa_sibling)
				continue;
			c->ipa_sibling = best->ipa_sibling;
		}

		if (error) {
			best->ipa_pref = ISAPNP_DEP_CONFLICTING;

			for (c = f; c != NULL; c = c->ipa_sibling)
				if (c != best && SAMEDEV(c, best))
					break;
			/* Last config for this logical device is conflicting */
			if (c == NULL) {
				*ipa = f;
				return (best);
			}

			ISAPNP_FREE(best);
			continue;
		} else {
			/* Remove all other configs for this device */
			struct isapnp_attach_args *l = NULL, *n = NULL, *d;

			for (c = f; c;) {
				if (c == best)
					continue;
				d = c->ipa_sibling;
				if (SAMEDEV(c, best))
					ISAPNP_FREE(c);
				else {
					if (n)
						n->ipa_sibling = c;

					else
						l = c;
					n = c;
					c->ipa_sibling = NULL;
				}
				c = d;
			}
			f = l;
		}
		*ipa = f;
		return (best);
	}
	return (best);
}


/* isapnp_id_to_vendor():
 *	Convert a pnp ``compressed ascii'' vendor id to a string
 */
char *
isapnp_id_to_vendor(v, id)
	char   *v;
	const u_char *id;
{
	static const char hex[] = "0123456789ABCDEF";
	char *p = v;
	
	*p++ = 'A' + (id[0] >> 2) - 1;
	*p++ = 'A' + ((id[0] & 3) << 3) + (id[1] >> 5) - 1;
	*p++ = 'A' + (id[1] & 0x1f) - 1;
	*p++ = hex[id[2] >> 4];
	*p++ = hex[id[2] & 0x0f];
	*p++ = hex[id[3] >> 4];
	*p++ = hex[id[3] & 0x0f];
	*p = '\0';

	return (v);
}


/* isapnp_print_region():
 *	Print a region allocation
 */
static void
isapnp_print_region(str, r, n)
	const char *str;
	struct isapnp_region *r;
	size_t n;
{
	size_t i;

	if (n == 0)
		return;

	printf(" %s ", str);
	for (i = 0; i < n; i++, r++) {
		printf("0x%x", r->base);
		if (r->length)
			printf("/%d", r->length);
		if (i != n - 1)
			printf(",");
	}
}


/* isapnp_print_pin():
 *	Print an irq/drq assignment
 */
static void
isapnp_print_pin(str, p, n)
	const char *str;
	struct isapnp_pin *p;
	size_t n;
{
	size_t i;

	if (n == 0)
		return;

	printf(" %s ", str);
	for (i = 0; i < n; i++, p++) {
		printf("%d", p->num);
		if (i != n - 1)
			printf(",");
	}
}


/* isapnp_print():
 *	Print the configuration line for an ISA PnP card.
 */
static int
isapnp_print(aux, str)
	void *aux;
	const char *str;
{
	struct isapnp_attach_args *ipa = aux;

	if (str != NULL)
		printf("%s: <%s, %s, %s, %s>", str, ipa->ipa_devident,
				ipa->ipa_devlogic, ipa->ipa_devcompat, ipa->ipa_devclass);

	isapnp_print_region("port", ipa->ipa_io, ipa->ipa_nio);
	isapnp_print_region("mem", ipa->ipa_mem, ipa->ipa_nmem);
	isapnp_print_region("mem32", ipa->ipa_mem32, ipa->ipa_nmem32);
	isapnp_print_pin("irq", ipa->ipa_irq, ipa->ipa_nirq);
	isapnp_print_pin("drq", ipa->ipa_drq, ipa->ipa_ndrq);

	return (UNCONF);
}

/* isapnp_submatch():
 * Special case.
 * A lot of com devices do not have the PNPxxx identifiers
 * they should have.  If it looks like a modem..... let's try it.
 */
/*
int
isapnp_com_submatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf = match;
	struct isapnp_attach_args *ipa = aux;

	ipa->ipa_nmem;

	if (strcmp("com", cf->cf_driver->cd_name) == 0 && ipa->ipa_nio == 1 && ipa->ipa_nirq == 1 &&
	    ipa->ipa_ndrq == 0 && ipa->ipa_nmem == 0 && ipa->ipa_io == 8) {
		if (isapnp_config(ipa->ipa_iot, ipa->ipa_memt, ipa)) {
			printf("%s: error in region allocation\n", cf->cf_driver->cd_name);
			return (0);
		}
		return (config_match(parent, match, ipa));
	}
	return (0);
}
*/

#ifdef _KERNEL
/* isapnp_submatch():
 *	Probe the logical device...
 */
static int
isapnp_submatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (config_match(parent, match, aux));
}

/* isapnp_devmatch():
 *	Match a probed device with the information from the driver
 */
int
isapnp_devmatch(ipa, dinfo, variant)
	const struct isapnp_attach_args *ipa;
	const struct isapnp_devinfo *dinfo;
	int *variant;
{
	const struct isapnp_matchinfo *match;
	int n;

	for (match = dinfo->devlogic, n = dinfo->nlogic; n--; match++)
		if (strcmp(match->name, ipa->ipa_devlogic) == 0) {
			*variant = match->variant;
			return (1);
		}

	for (match = dinfo->devcompat, n = dinfo->ncompat; n--; match++)
		if (strcmp(match->name, ipa->ipa_devcompat) == 0) {
			*variant = match->variant;
			return (1);
		}

	return (0);
}

/* isapnp_isa_attach_hook():
 *	This routine is called from the isa attach code and
 *	is a kludge; we are resetting all the cards here in order
 *	to undo any card configuration that the bios did for us, in order
 *	to avoid having the PnP devices match an isa probe. The correct
 *	way of doing this is to read the PnP BIOS and find the card settings
 *	from there. Unfortunately it is not as easy as it sounds.
 */
void
isapnp_isa_attach_hook(isa_sc)
	struct isa_softc *isa_sc;
{
	struct isapnp_softc sc;

	sc.sc_iot = isa_sc->sc_iot;
	sc.sc_ncards = 0;

	if (isapnp_map(&sc))
		return;

#if NWSS_ISAPNP > 0
	/*
	 * XXX XXX
	 * This a totally disgusting hack, but I can't figure out another way.
	 * It seems that many CS audio chips have a bug (as far as I can
	 * understand).  The reset below does not really reset the chip, it
	 * remains in a catatonic state and will not respond when probed.
	 * The chip can be used both as a WSS and as a SB device, and a
	 * single read at the WSS address (0x534) takes it out of this
	 * non-responsive state.
	 * The read has to happen at this point in time (or earlier) so
	 * it cannot be moved to the wss_isapnp.c driver.
	 * (BTW, We're not alone in having problems with these chips:
	 * Windoze 98 couldn't detect the sound chip on a Dell when I tried.)
	 *
	 *     Lennart Augustsson <augustss@NetBSD.org>
	 *
	 * (Implementation from John Kohl <jtk@kolvir.arlington.ma.us>)
	 */
	{
		bus_space_handle_t ioh;
		int rv;
		if ((rv = bus_space_map(sc.sc_iot, 0x534, 1, 0, &ioh)) == 0) {
			DPRINTF(("wss probe kludge\n"));
			(void)bus_space_read_1(sc.sc_iot, ioh, 0);
			bus_space_unmap(sc.sc_iot, ioh, 1);
		} else {
			DPRINTF(("wss probe kludge failed to map: %d\n", rv));
		}
	}
#endif

	isapnp_init(&sc);

	isapnp_write_reg(&sc, ISAPNP_CONFIG_CONTROL, ISAPNP_CC_RESET_DRV);
	DELAY(2000);

	isapnp_unmap(&sc);
}
#endif

/* isapnp_find():
 *	Probe and add cards
 */
static int
isapnp_find(sc, all)
	struct isapnp_softc *sc;
	int all;
{
	int p;

	isapnp_init(sc);

	isapnp_write_reg(sc, ISAPNP_CONFIG_CONTROL, ISAPNP_CC_RESET_DRV);
	DELAY(2000);

	isapnp_init(sc);
	DELAY(2000);

	for (p = ISAPNP_RDDATA_MIN; p <= ISAPNP_RDDATA_MAX; p += 4) {
		sc->sc_read_port = p;
		if (isapnp_map_readport(sc))
			continue;DPRINTF(("%s: Trying port %x\n", sc->sc_dev.dv_xname, p));
		if (isapnp_findcard(sc))
			break;
		isapnp_unmap_readport(sc);
	}

	if (p > ISAPNP_RDDATA_MAX) {
		sc->sc_read_port = 0;
		return (0);
	}

	if (all)
		while (isapnp_findcard(sc))
			continue;

	return (1);
}


/* isapnp_configure():
 *	Configure a PnP card
 *	XXX: The memory configuration code is wrong. We need to check the
 *	     range/length bit an do appropriate sets.
 */
static void
isapnp_configure(sc, ipa)
	struct isapnp_softc *sc;
	const struct isapnp_attach_args *ipa;
{
	int i;
	static u_char isapnp_mem_range[] = ISAPNP_MEM_DESC;
	static u_char isapnp_io_range[] = ISAPNP_IO_DESC;
	static u_char isapnp_irq_range[] = ISAPNP_IRQ_DESC;
	static u_char isapnp_drq_range[] = ISAPNP_DRQ_DESC;
	static u_char isapnp_mem32_range[] = ISAPNP_MEM32_DESC;
	const struct isapnp_region *r;
	const struct isapnp_pin *p;
	struct isapnp_region rz;
	struct isapnp_pin pz;

	memset(&pz, 0, sizeof(pz));
	memset(&rz, 0, sizeof(rz));

#define B0(a) ((a) & 0xff)
#define B1(a) (((a) >> 8) & 0xff)
#define B2(a) (((a) >> 16) & 0xff)
#define B3(a) (((a) >> 24) & 0xff)

	for (i = 0; i < sizeof(isapnp_io_range); i++) {
		if (i < ipa->ipa_nio)
			r = &ipa->ipa_io[i];
		else
			r = &rz;

		isapnp_write_reg(sc, isapnp_io_range[i] + ISAPNP_IO_BASE_15_8,
				B1(r->base));
		isapnp_write_reg(sc, isapnp_io_range[i] + ISAPNP_IO_BASE_7_0,
				B0(r->base));
	}

	for (i = 0; i < sizeof(isapnp_mem_range); i++) {
		if (i < ipa->ipa_nmem)
			r = &ipa->ipa_mem[i];
		else
			r = &rz;

		isapnp_write_reg(sc, isapnp_mem_range[i] + ISAPNP_MEM_BASE_23_16,
				B2(r->base));
		isapnp_write_reg(sc, isapnp_mem_range[i] + ISAPNP_MEM_BASE_15_8,
				B1(r->base));

		isapnp_write_reg(sc, isapnp_mem_range[i] + ISAPNP_MEM_LRANGE_23_16,
				B2(r->length));
		isapnp_write_reg(sc, isapnp_mem_range[i] + ISAPNP_MEM_LRANGE_15_8,
				B1(r->length));
	}

	for (i = 0; i < sizeof(isapnp_irq_range); i++) {
		u_char v;

		if (i < ipa->ipa_nirq)
			p = &ipa->ipa_irq[i];
		else
			p = &pz;

		isapnp_write_reg(sc, isapnp_irq_range[i] + ISAPNP_IRQ_NUMBER, p->num);

		switch (p->flags) {
		case ISAPNP_IRQTYPE_LEVEL_PLUS:
			v = ISAPNP_IRQ_LEVEL | ISAPNP_IRQ_HIGH;
			break;

		case ISAPNP_IRQTYPE_EDGE_PLUS:
			v = ISAPNP_IRQ_HIGH;
			break;

		case ISAPNP_IRQTYPE_LEVEL_MINUS:
			v = ISAPNP_IRQ_LEVEL;
			break;

		default:
		case ISAPNP_IRQTYPE_EDGE_MINUS:
			v = 0;
			break;
		}
		isapnp_write_reg(sc, isapnp_irq_range[i] + ISAPNP_IRQ_CONTROL, v);
	}

	for (i = 0; i < sizeof(isapnp_drq_range); i++) {
		u_char v;

		if (i < ipa->ipa_ndrq)
			v = ipa->ipa_drq[i].num;
		else
			v = 4;

		isapnp_write_reg(sc, isapnp_drq_range[i], v);
	}

	for (i = 0; i < sizeof(isapnp_mem32_range); i++) {
		if (i < ipa->ipa_nmem32)
			r = &ipa->ipa_mem32[i];
		else
			r = &rz;

		isapnp_write_reg(sc, isapnp_mem32_range[i] + ISAPNP_MEM32_BASE_31_24,
				B3(r->base));
		isapnp_write_reg(sc, isapnp_mem32_range[i] + ISAPNP_MEM32_BASE_23_16,
				B2(r->base));
		isapnp_write_reg(sc, isapnp_mem32_range[i] + ISAPNP_MEM32_BASE_15_8,
				B1(r->base));
		isapnp_write_reg(sc, isapnp_mem32_range[i] + ISAPNP_MEM32_BASE_7_0,
				B0(r->base));

		isapnp_write_reg(sc, isapnp_mem32_range[i] + ISAPNP_MEM32_LRANGE_31_24,
				B3(r->length));
		isapnp_write_reg(sc, isapnp_mem32_range[i] + ISAPNP_MEM32_LRANGE_23_16,
				B2(r->length));
		isapnp_write_reg(sc, isapnp_mem32_range[i] + ISAPNP_MEM32_LRANGE_15_8,
				B1(r->length));
		isapnp_write_reg(sc, isapnp_mem32_range[i] + ISAPNP_MEM32_LRANGE_7_0,
				B0(r->length));
	}
}


/* isapnp_match():
 *	Probe routine
 */
static int
isapnp_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isapnp_softc sc;
	struct isa_attach_args *ia = aux;
	struct isapnp_probe_cookie *ipc;

	/*
	 * Ensure we only probe ISA PnP once; we don't actually consume
	 * bus resources, so we have to prevent being cloned forever.
	 */
	for (ipc = LIST_FIRST(&isapnp_probes); ipc != NULL;
			ipc = LIST_NEXT(ipc, ipc_link))
		if (ipc->ipc_parent == parent)
			return (0);

	ipc = malloc(sizeof(*ipc), M_DEVBUF, M_NOWAIT);
	if (ipc == NULL)
		panic("isapnp_match: can't allocate probe cookie");

	ipc->ipc_parent = parent;
	LIST_INSERT_HEAD(&isapnp_probes, ipc, ipc_link);

	sc.sc_iot = ia->ia_iot;
	(void) strcpy(sc.sc_dev.dv_xname, "(isapnp probe)");

	if (isapnp_map(&sc))
		return (0);

	isapnp_unmap(&sc);

	ia->ia_iobase = ISAPNP_ADDR;
	ia->ia_iosize = 1;


	ia->ia_niomem = 0;
	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return (1);
}


/* isapnp_attach
 *	Find and attach PnP cards.
 */
static void
isapnp_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isapnp_softc *sc = (struct isapnp_softc *) self;
	struct isa_attach_args *ia = aux;
	void *match;
	int c, d;

	sc->sc_iot = ia->ia_iot;
	sc->sc_memt = ia->ia_memt;
	sc->sc_ic = ia->ia_ic;
	sc->sc_dmat = ia->ia_dmat;
	sc->sc_ncards = 0;

	if (isapnp_map(sc))
		panic("%s: bus map failed\n", sc->sc_dev.dv_xname);
		return;

#ifdef _KERNEL
	/*
	 * Defer configuration until the rest of the ISA devices have
	 * attached themselves.
	 */
	config_defer(self, isapnp_callback);
#else
	isapnp_callback(self);
#endif
}

/* isapnp_callback
 *	Find and attach PnP cards.
 */
void
isapnp_callback(self)
	struct device *self;
{
	struct isapnp_softc *sc = (struct isapnp_softc *)self;
	struct isapnp_attach_args *ipa, *lpa;
	int c, d;

	/*
	 * Look for cards.  If none are found, we say so and just return.
	 */
	if (isapnp_find(sc, 1) == 0) {
		printf("%s: no ISA Plug 'n Play devices found\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	printf("%s: read port 0x%x\n", sc->sc_dev.dv_xname, sc->sc_read_port);

	/*
	 * Now configure all of the cards.
	 */
	for (c = 0; c < sc->sc_ncards; c++) {
		/* Good morning card c */
		isapnp_write_reg(sc, ISAPNP_WAKE, c + 1);

		if ((ipa = isapnp_get_resource(sc, c)) == NULL)
			continue;

		DPRINTF(("Selecting attachments\n"));
		for (d = 0; (lpa = isapnp_bestconfig(sc, &ipa)) != NULL; d++) {
			isapnp_write_reg(sc, ISAPNP_LOGICAL_DEV_NUM, d);
			isapnp_configure(sc, lpa);
#ifdef DEBUG_ISAPNP
			{
				struct isapnp_attach_args pa;

				isapnp_get_config(sc, &pa);
				isapnp_print_config(&pa);
			}
#endif

			DPRINTF(("%s: configuring <%s, %s, %s, %s>\n",
							sc->sc_dev.dv_xname,
							lpa->ipa_devident, lpa->ipa_devlogic,
							lpa->ipa_devcompat, lpa->ipa_devclass));
			if (lpa->ipa_pref == ISAPNP_DEP_CONFLICTING) {
				printf("%s: <%s, %s, %s, %s> ignored; %s\n",
						sc->sc_dev.dv_xname, lpa->ipa_devident,
						lpa->ipa_devlogic, lpa->ipa_devcompat,
						lpa->ipa_devclass, "resource conflict");
				ISAPNP_FREE(lpa);
				continue;
			}

			lpa->ipa_ic = sc->sc_ic;
			lpa->ipa_iot = sc->sc_iot;
			lpa->ipa_memt = sc->sc_memt;
			lpa->ipa_dmat = sc->sc_dmat;

			isapnp_write_reg(sc, ISAPNP_ACTIVATE, 1);
#ifdef _KERNEL
			if (config_found_sm(self, lpa, isapnp_print,
					isapnp_submatch) == NULL)
				isapnp_write_reg(sc, ISAPNP_ACTIVATE, 0);
#else
			isapnp_print(lpa, NULL);
			printf("\n");
#endif
			ISAPNP_FREE(lpa);
		}
		isapnp_write_reg(sc, ISAPNP_WAKE, 0); /* Good night cards */
	}
}
