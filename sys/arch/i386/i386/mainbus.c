/*	$NetBSD: mainbus.c,v 1.19.2.2 1997/11/28 08:13:24 mellon Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/user.h>

#include <dev/core/eisa/eisavar.h>
#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>
#include <dev/core/pci/pcivar.h>

#if NMCA > 0
#include <dev/core/mca/mcavar.h>
#endif

#include <machine/bus.h>

#include <machine/apic/apic.h>
#include <machine/apic/ioapicvar.h>
#include <machine/cpuvar.h>
#include <machine/mpbiosvar.h>

#include <machine/eisa/eisa_machdep.h>
#include <machine/isa/isa_machdep.h>
#include <machine/mca/mca_machdep.h>

int	 mainbus_match (struct device *, struct cfdata *, void *);
void mainbus_attach (struct device *, struct device *, void *);

CFOPS_DECL(mainbus, mainbus_match, mainbus_attach, NULL, NULL);
CFDRIVER_DECL(NULL, mainbus, &mainbus_cops, DV_DULL, sizeof(struct device));

int	mainbus_print (void *, const char *);

union mainbus_attach_args {
	const char 					*mba_busname;		/* first elem of all */
	struct pcibus_attach_args 	mba_pba;
	struct eisabus_attach_args 	mba_eba;
	struct isabus_attach_args 	mba_iba;
#if NMCA > 0
	struct mcabus_attach_args	mba_mba;
#endif
	struct cpu_attach_args 		mba_caa;
	struct apic_attach_args 	aaa_caa;
};

/*
 * This is set when the ISA bus is attached.  If it's not set by the
 * time it's checked below, then mainbus attempts to attach an ISA.
 */
int	isa_has_been_seen;
/*
 * Same as above, but for EISA.
 */
int eisa_has_been_seen;

#if defined(MPBIOS) || defined(MPACPI)
struct mp_bus *mp_busses;
int mp_nbus;
struct mp_intr_map *mp_intrs;
int mp_nintr;

int mp_isa_bus = -1;            /* XXX */
int mp_eisa_bus = -1;           /* XXX */

#ifdef MPVERBOSE
int mp_verbose = 1;
#else
int mp_verbose = 0;
#endif
#endif


/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union mainbus_attach_args mba;

#ifdef MPBIOS
	int mpbios_present = 0;
#endif

	printf("\n");

#ifdef MPBIOS
	mpbios_present = mpbios_probe(self);
	if (mpbios_present) {
		mpbios_scan(self);
	} else
#endif
	{
		struct cpu_attach_args caa;

		bzero(&caa, sizeof(caa));
		caa.caa_name = "cpu";
		caa.cpu_number = 0;
		caa.cpu_role = CPU_ROLE_SP;
		caa.cpu_ops = 0;

		config_found(self, &caa, mainbus_print);
	}

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
	pci_mode = pci_mode_detect();
	if (pci_mode != 0) {
		mba.mba_pba.pba_busname = "pci";
		mba.mba_pba.pba_iot = I386_BUS_SPACE_IO;
		mba.mba_pba.pba_memt = I386_BUS_SPACE_MEM;
		mba.mba_pba.pba_dmat = &pci_bus_dma_tag;
		mba.mba_pba.pba_dmat64 = NULL;
		mba.mba_pba.pba_pc = NULL;
		mba.mba_pba.pba_flags =
		    PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
		mba.mba_pba.pba_bus = 0;
		mba.mba_pba.pba_bridgetag = NULL;

#if defined(MPBIOS) && defined(MPBIOS_SCANPCI)
		if (mpbios_scanned != 0) {
			mpbios_scan_pci(self, &mba.mba_pba, mainbus_print);
		} else {
#endif
			config_found(self, &mba.mba_pba, mainbus_print);
	}
#endif

#if NMCA > 0
	/* Note: MCA bus probe is done in i386/machdep.c */
	if (MCA_system) {
		mba.mba_mba.mba_busname = "mca";
		mba.mba_mba.mba_iot = X86_BUS_SPACE_IO;
		mba.mba_mba.mba_memt = X86_BUS_SPACE_MEM;
		mba.mba_mba.mba_dmat = &mca_bus_dma_tag;
		mba.mba_mba.mba_mc = NULL;
		mba.mba_mba.mba_bus = 0;
		config_found(self, &mba.mba_mba, mainbus_print);
	}
#endif

	if (bcmp(ISA_HOLE_VADDR(EISA_ID_PADDR), EISA_ID, EISA_ID_LEN) == 0
			&& eisa_has_been_seen == 0) {
		mba.mba_eba.eba_busname = "eisa";
		mba.mba_eba.eba_iot = I386_BUS_SPACE_IO;
		mba.mba_eba.eba_memt = I386_BUS_SPACE_MEM;
#if NEISA > 0
		mba.mba_eba.eba_dmat = &eisa_bus_dma_tag;
#endif
		config_found(self, &mba.mba_eba, mainbus_print);
	}

	if (isa_has_been_seen == 0) {
		mba.mba_iba.iba_busname = "isa";
		mba.mba_iba.iba_iot = I386_BUS_SPACE_IO;
		mba.mba_iba.iba_memt = I386_BUS_SPACE_MEM;
#if NISA > 0
		mba.mba_iba.iba_dmat = &isa_bus_dma_tag;
#endif
		config_found(self, &mba.mba_iba, mainbus_print);
	}
}

int
mainbus_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	union mainbus_attach_args *mba = aux;

	if (pnp)
		printf("%s at %s", mba->mba_busname, pnp);
	if (!strcmp(mba->mba_busname, "pci"))
		printf(" bus %d", mba->mba_pba.pba_bus);
	return (UNCONF);
}
