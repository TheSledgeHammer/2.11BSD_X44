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
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/dk.h>
#include <sys/conf.h>
#include <sys/dmap.h>
#include <sys/reboot.h>
#include <sys/null.h>
#include <sys/devsw.h>

#include <machine/pte.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

#if NIOAPIC > 0
#include <machine/apic/ioapicvar.h>
#endif

#if NLAPIC > 0
#include <machine/apic/lapicvar.h>
#endif

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

	i386_proc0_tss_ldt_init();

	if(config_rootfound("mainbus", NULL) == NULL) {
		panic("cpu_configure: mainbus not configured");
	}

#if NIOAPIC > 0
	ioapic_enable();
#endif

	/*
	 * Configure device structures
	 */
	mi_device_init(&sys_devsw);
	md_device_init(&sys_devsw);

	/*
	 * Configure swap area and related system
	 * parameter based on device(s) used.
	 */
	swapconf();

	spl0();

#if NLAPIC > 0
	lapic_write_tpri(0);
#endif

	cold = 0;
}

/*
 * Configure MD (Machine-Dependent) Devices
 */
void
md_device_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &cmos_cdevsw, NULL);			/* CMOS Interface */
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &apm_cdevsw, NULL);				/* Power Management (APM) Interface */
}

/*
 * Generic configuration;  all in one
 */
dev_t	rootdev = makedev(0,0);
dev_t	dumpdev = makedev(0,1);
int		nswap;

struct swdevt swdevt[] = {
		{ 1, 0,	0 },
		{ NODEV, 1,	0 },
};

long dumplo;
int	dmmin, dmmax, dmtext;

/*
 * Configure swap space and related parameters.
 */
void
swapconf()
{
	register struct swdevt *swp;
	register int nblks;
	extern int Maxmem;

	for (swp = swdevt; swp->sw_dev != NODEV; swp++)	{
		if ( (u_int)swp->sw_dev >= nblkdev ){
			break;	/* XXX */
		}
		if (bdevsw[major(swp->sw_dev)].d_psize) {
			nblks = (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			if (nblks != -1 && (swp->sw_nblks == 0 || swp->sw_nblks > nblks)) {
				swp->sw_nblks = nblks;
			}
		}
	}
	if (dumplo == 0 && bdevsw[major(dumpdev)].d_psize) {
		dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) - (Maxmem * NBPG/512);
	}
	if (dumplo < 0) {
		dumplo = 0;
	}
}
