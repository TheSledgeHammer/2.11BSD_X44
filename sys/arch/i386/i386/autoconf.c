/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)autoconf.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */
#include <sys/param.h>

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/dk.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/dmap.h>
#include <sys/reboot.h>
#include <sys/null.h>
#include <sys/devsw.h>

#ifdef DISK_SLICES
#include <sys/diskslice.h>
#endif

#include <machine/bios.h>
#include <machine/cpu.h>
#include <machine/pte.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

#include <dev/misc/cons/cons.h>

#if NIOAPIC > 0
#include <machine/apic/ioapicvar.h>
#endif

#if NLAPIC > 0
#include <machine/apic/lapicvar.h>
#endif

#include "opt_pcibios.h"
#ifdef PCIBIOS
#include <dev/core/pci/pcireg.h>
#include <dev/core/pci/pcivar.h>
#include <i386/pci/pcibios.h>
#endif

void	swapconf(void);
void	setroot(void);

/*
 * Generic configuration;  all in one
 */
//dev_t	rootdev = makedev(0,0);
//dev_t	dumpdev = makedev(0,1);
dev_t	swapdev = makedev(1,0);
u_long	bootdev = 0;

//int		nswap;
int		dmmin, dmmax, dmtext;

struct swdevt swdevt[] = {
		{ 1, 0,	0 },
		{ NODEV, 1,	0 },
};

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
extern int	cold;		/* cold start flag initialized in locore.s */

/*
 * Determine i/o configuration for a machine.
 */
void
configure()
{
	startrtclock();

	bios32_init();
	k6_mem_drvinit();

#ifdef PCIBIOS
	pcibios_init();
#endif

	i386_proc0_tss_ldt_init();

	if(config_rootfound("mainbus", NULL) == NULL) {
		panic("cpu_configure: mainbus not configured");
	}

#if NIOAPIC > 0
	ioapic_enable();
#endif

	setroot();
	/*
	 * Configure device structures
	 */
	conf_init(&sys_devsw);

	/*
	 * Configure swap area and related system
	 * parameter based on device(s) used.
	 */
	swapconf();

	spl0();

#if NLAPIC > 0
	lapic_write_tpri(0);
#endif

	cninit_finish();
	cold = 0;
}

/*
 * Configure swap space and related parameters.
 */
void
swapconf(void)
{
	register struct swdevt *swp;
	const struct bdevsw *bdev;
	register int nblks;
	extern int Maxmem;

	for (swp = swdevt; swp->sw_dev != NODEV; swp++)	{
		if ( (u_int)swp->sw_dev >= nblkdev ){
			break;	/* XXX */
		}
		bdev = bdevsw_lookup(swp->sw_dev);
		if (bdev->d_psize) {
			nblks = (*bdev->d_psize)(swp->sw_dev);
			if (nblks != -1 && (swp->sw_nblks == 0 || swp->sw_nblks > nblks)) {
				swp->sw_nblks = nblks;
			}
		}
	}
	bdev = bdevsw_lookup(dumpdev);
	if (dumplo == 0 && bdev->d_psize) {
		dumplo = (*bdev->d_psize)(dumpdev) - (Maxmem * NBPG/512);
	}
	if (dumplo < 0) {
		dumplo = 0;
	}
}

void
setswap(majdev, mindev)
	int majdev, mindev;
{
	struct swdevt *swp;
	dev_t temp;
	int swmaj, swmin;

	temp = 0;
	swmaj = major(swp->sw_dev);
	swmin = minor(swp->sw_dev);
	for (swp = swdevt; swp->sw_dev != NODEV; swp++) {
		if (majdev == swmaj && (mindev == swmin)) {
			temp = swdevt[0].sw_dev;
			swdevt[0].sw_dev = swp->sw_dev;
			swp->sw_dev = temp;
			break;
		}
	}
	if (swp->sw_dev == NODEV) {
		return;
	}
	if (temp == dumpdev) {
		dumpdev = swdevt[0].sw_dev;
	}
}

#ifdef DISK_SLICES
int
setroot_slices(void)
{
	int mindev, unit, part, slice;

	unit = B_UNIT(bootdev);
	part = B_PARTITION(bootdev);
	slice = B_SLICE(bootdev);
	mindev = dkmakeminor(unit, slice, part);

	return (mindev);
}
#else
int
setroot_traditional(void)
{
	int mindev, unit, part, adaptor;

	unit = B_UNIT(bootdev);
	part = B_PARTITION(bootdev);
	adaptor = B_ADAPTOR(bootdev);
	mindev = dkminor(unit, part);

	return (mindev);
}
#endif

void
setroot(void)
{
	int  majdev, mindev;
	dev_t orootdev;

	if ((boothowto & RB_DFLTROOT) || (bootdev & B_MAGICMASK) != (u_long)B_DEVMAGIC) {
		return;
	}
	majdev = B_TYPE(bootdev);
#ifdef DISK_SLICES
	mindev = setroot_slices();

#else
	mindev = setroot_traditional();
#endif
	//mindev = mindev * MAXPARTITIONS;// (unit * MAXPARTITIONS) + part;
	orootdev = rootdev;
	rootdev = makedev(majdev, mindev);
	if (rootdev == orootdev) {
		return;
	}
	setswap(majdev, mindev);
}
