/* $NetBSD: vga_isa.c,v 1.15 2002/10/02 03:10:50 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: vga_isa.c,v 1.15 2002/10/02 03:10:50 thorpej Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/user.h>

#include <dev/core/isa/isavar.h>
#include <dev/core/ic/mc6845reg.h>
#include <dev/misc/pccons/pcdisplayvar.h>
#include <dev/video/vga/vgareg.h>
#include <dev/video/vga/vgavar.h>
#include <dev/video/vga/vga_isavar.h>

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wsdisplayvar.h>

int		vga_isa_match(struct device *, struct cfdata *, void *);
void	vga_isa_attach(struct device *, struct device *, void *);

extern struct cfdriver vga_cd;
CFOPS_DECL(vga_isa, vga_isa_match, vga_isa_attach, NULL, NULL);
CFATTACH_DECL(vga_isa, &vga_cd, &vga_isa_cops, sizeof(struct vga_softc));

int
vga_isa_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct isa_attach_args *ia = aux;

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	/* If values are hardwired to something that they can't be, punt. */
	if (ia->ia_nio < 1 || (ia->ia_iobase != IOBASEUNK && ia->ia_iobase != 0x3b0))
		return (0);

	if (ia->ia_niomem < 1 || (ia->ia_maddr != MADDRUNK && ia->ia_maddr != 0xa0000))
		return (0);

	if (ia->ia_msize != 0 && ia->ia_msize != 0x20000)
		return (0);

	if (ia->ia_nirq > 0 && ia->ia_irq != IRQUNK)
		return (0);

	if (ia->ia_ndrq > 0 && ia->ia_drq != DRQUNK)
		return (0);

	if (!vga_is_console(ia->ia_iot, WSDISPLAY_TYPE_ISAVGA) && !vga_common_probe(ia->ia_iot, ia->ia_memt))
		return (0);

	ia->ia_nio = 1;
	ia->ia_iobase = 0x3b0;	/* XXX mono 0x3b0 color 0x3c0 */
	ia->ia_iosize = 0x30;	/* XXX 0x20 */

	ia->ia_niomem = 1;
	ia->ia_maddr = 0xa0000;
	ia->ia_msize = 0x20000;

	ia->ia_nirq = 0;
	ia->ia_ndrq = 0;

	return (2);	/* more than generic pcdisplay */
}

void
vga_isa_attach(struct device *parent, struct device *self, void *aux)
{
	struct vga_softc *sc = (void *) self;
	struct isa_attach_args *ia = aux;

	printf("\n");

	vga_common_attach(sc, ia->ia_iot, ia->ia_memt, WSDISPLAY_TYPE_ISAVGA, 0, NULL);
}

int
vga_isa_cnattach(bus_space_tag_t iot, bus_space_tag_t memt)
{

	return (vga_cnattach(iot, memt, WSDISPLAY_TYPE_ISAVGA, 1));
}
