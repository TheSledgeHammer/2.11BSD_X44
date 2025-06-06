/*	$NetBSD: pcivar.h,v 1.28 1997/10/14 07:15:47 sakamoto Exp $	*/

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
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

#ifndef _DEV_PCI_PCIVAR_H_
#define	_DEV_PCI_PCIVAR_H_

/*
 * Definitions for PCI autoconfiguration.
 *
 * This file describes types and functions which are used for PCI
 * configuration.  Some of this information is machine-specific, and is
 * provided by pci_machdep.h.
 */
#include <sys/device.h>
#include <machine/bus.h>
#include <dev/core/pci/pcireg.h>

/*
 * Structures and definitions needed by the machine-dependent header.
 */
typedef u_int32_t pcireg_t;		/* configuration space register XXX */
struct pcibus_attach_args;
struct pci_softc;

/*
 * Descriptions of known PCI classes and subclasses.
 *
 * Subclasses are described in the same way as classes, but have a
 * NULL subclass pointer.
 */
struct pci_class {
	char					*name;
	int						val;		/* as wide as pci_{,sub}class_t */
	const struct pci_class 	*subclasses;
};

#ifdef _KERNEL
/*
 * Machine-dependent definitions.
 */
#include <machine/pci/pci_machdep.h>

/*
 * PCI bus attach arguments.
 */
struct pcibus_attach_args {
	char				*pba_busname;	/* XXX should be common */
	bus_space_tag_t 	pba_iot;		/* pci i/o space tag */
	bus_space_tag_t 	pba_memt;		/* pci mem space tag */
	bus_dma_tag_t 		pba_dmat;		/* DMA tag */
	bus_dma_tag_t 		pba_dmat64;		/* DMA tag */
	pci_chipset_tag_t 	pba_pc;
	int					pba_flags;		/* flags; see below */

	int					pba_bus;		/* PCI bus number */

	/*
	 * Pointer to the pcitag of our parent bridge.  If there is no
	 * parent bridge, then we assume we are a root bus.
	 */
	pcitag_t			*pba_bridgetag;

	/*
	 * Interrupt swizzling information.  These fields
	 * are only used by secondary busses.
	 */
	u_int				pba_intrswiz;	/* how to swizzle pins */
	pcitag_t			pba_intrtag;	/* intr. appears to come from here */
};

/*
 * PCI device attach arguments.
 */
struct pci_attach_args {
	bus_space_tag_t 	pa_iot;		/* pci i/o space tag */
	bus_space_tag_t 	pa_memt;	/* pci mem space tag */
	bus_dma_tag_t 		pa_dmat;	/* DMA tag */
	bus_dma_tag_t 		pa_dmat64;	/* DMA tag */
	pci_chipset_tag_t 	pa_pc;
	int					pa_flags;	/* flags; see below */

	u_int				pa_bus;
	u_int				pa_device;
	u_int				pa_function;
	pcitag_t			pa_tag;
	pcireg_t			pa_id, pa_class;

	/*
	 * Interrupt information.
	 *
	 * "Intrline" is used on systems whose firmware puts
	 * the right routing data into the line register in
	 * configuration space.  The rest are used on systems
	 * that do not.
	 */
	u_int				pa_intrswiz;	/* how to swizzle pins if ppb */
	pcitag_t			pa_intrtag;		/* intr. appears to come from here */
	pci_intr_pin_t		pa_intrpin;		/* intr. appears on this pin */
	pci_intr_line_t		pa_intrline;	/* intr. routing information */
	pci_intr_pin_t  	pa_rawintrpin; 	/* unswizzled pin */
};

/*
 * Flags given in the bus and device attachment args.
 */
#define	PCI_FLAGS_IO_ENABLED	0x01		/* I/O space is enabled */
#define	PCI_FLAGS_MEM_ENABLED	0x02		/* memory space is enabled */
#define	PCI_FLAGS_MRL_OKAY		0x04		/* Memory Read Line okay */
#define	PCI_FLAGS_MRM_OKAY		0x08		/* Memory Read Multiple okay */
#define	PCI_FLAGS_MWI_OKAY		0x10		/* Memory Write and Invalidate
						   	   	   	   	   	   okay */
/*
 * PCI device 'quirks'.
 *
 * In general strange behaviour which can be handled by a driver (e.g.
 * a bridge's inability to pass a type of access correctly) should be.
 * The quirks table should only contain information which impacts
 * the operation of the MI PCI code and which can't be pushed lower
 * (e.g. because it's unacceptable to require a driver to be present
 * for the information to be known).
 */
struct pci_quirkdata {
	pci_vendor_id_t		vendor;		/* Vendor ID */
	pci_product_id_t	product;	/* Product ID */
	int					quirks;		/* quirks; see below */
};
#define	PCI_QUIRK_MULTIFUNCTION		1
#define	PCI_QUIRK_MONOFUNCTION		2
#define	PCI_QUIRK_SKIP_FUNC(n)		(4 << n)
#define	PCI_QUIRK_SKIP_FUNC0		PCI_QUIRK_SKIP_FUNC(0)
#define	PCI_QUIRK_SKIP_FUNC1		PCI_QUIRK_SKIP_FUNC(1)
#define	PCI_QUIRK_SKIP_FUNC2		PCI_QUIRK_SKIP_FUNC(2)
#define	PCI_QUIRK_SKIP_FUNC3		PCI_QUIRK_SKIP_FUNC(3)
#define	PCI_QUIRK_SKIP_FUNC4		PCI_QUIRK_SKIP_FUNC(4)
#define	PCI_QUIRK_SKIP_FUNC5		PCI_QUIRK_SKIP_FUNC(5)
#define	PCI_QUIRK_SKIP_FUNC6		PCI_QUIRK_SKIP_FUNC(6)
#define	PCI_QUIRK_SKIP_FUNC7		PCI_QUIRK_SKIP_FUNC(7)

struct pci_softc {
	struct device 		sc_dev;
	bus_space_tag_t 	sc_iot, sc_memt;
	bus_dma_tag_t 		sc_dmat;
	bus_dma_tag_t 		sc_dmat64;
	pci_chipset_tag_t 	sc_pc;
	int 				sc_bus, sc_maxndevs;
	pcitag_t 			*sc_bridgetag;
	u_int 				sc_intrswiz;
	pcitag_t 			sc_intrtag;
	int 				sc_flags;
};

#include "locators.h"

/*
 * Locators devices that attach to 'pcibus', as specified to config.
 */
//#define PCIBUSCF_BUS				0
//#define PCIBUSCF_BUS_DEFAULT		-1
#define	PCIBUS_UNK_BUS				PCIBUSCF_BUS_DEFAULT	/* wildcarded 'bus' */

/*
 * Locators for PCI devices, as specified to config.
 */
//#define PCICF_DEV					1
//#define PCICF_DEV_DEFAULT			-1
#define	PCI_UNK_DEV					PCICF_DEV_DEFAULT		/* wildcarded 'dev' */

//#define PCICF_FUNCTION			0
//#define PCICF_FUNCTION_DEFAULT	-1
#define	PCI_UNK_FUNCTION			PCICF_FUNCTION_DEFAULT /* wildcarded 'function' */

/*
 * Configuration space access and utility functions.  (Note that most,
 * e.g. make_tag, conf_read, conf_write are declared by pci_machdep.h.)
 */
int		pci_mapreg_probe(pci_chipset_tag_t, pcitag_t, int, pcireg_t *);
pcireg_t pci_mapreg_type(pci_chipset_tag_t, pcitag_t, int);
int		pci_mapreg_info(pci_chipset_tag_t, pcitag_t, int, pcireg_t, bus_addr_t *, bus_size_t *, int *);
int		pci_mapreg_map(struct pci_attach_args *, int, pcireg_t, int, bus_space_tag_t *, bus_space_handle_t *, bus_addr_t *, bus_size_t *);


int 	pci_get_capability(pci_chipset_tag_t, pcitag_t, int, int *, pcireg_t *);
/*
 * Helper functions for autoconfiguration.
 */
int		pci_enumerate_bus_generic(struct pci_softc *, int (*)(struct pci_attach_args *), struct pci_attach_args *);
int		pci_probe_device(struct pci_softc *, pcitag_t tag, int (*)(struct pci_attach_args *), struct pci_attach_args *);
void	pci_devinfo(pcireg_t, pcireg_t, int, char *);
void 	pci_knowndev(const pcireg_t *, pcireg_t);
void	pci_conf_print(pci_chipset_tag_t, pcitag_t, void (*)(pci_chipset_tag_t, pcitag_t, const pcireg_t *));
const struct pci_quirkdata *pci_lookup_quirkdata(pci_vendor_id_t, pci_product_id_t);

#define	pci_enumerate_bus(sc, m, p)					\
	pci_enumerate_bus_generic((sc), (m), (p))

/*
 * Helper functions for user access to the PCI bus.
 */
int		pci_devioctl(pci_chipset_tag_t, pcitag_t, u_long, caddr_t, int flag, struct proc *);

/*
 * Power Management (PCI 2.2)
 */

#define PCI_PWR_D0	0
#define PCI_PWR_D1	1
#define PCI_PWR_D2	2
#define PCI_PWR_D3	3

int	pci_set_powerstate(pci_chipset_tag_t, pcitag_t, int);
int	pci_get_powerstate(pci_chipset_tag_t, pcitag_t);

/*
 * Misc.
 */
int		pci_find_device(struct pci_attach_args *, int (*match)(struct pci_attach_args *));
void	set_pci_isa_bridge_callback(void (*)(void *), void *);
			
#endif /* _KERNEL */
#endif /* _DEV_PCI_PCIVAR_H_ */
