/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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

/* __FBSDID("$FreeBSD$"); */

/*
 * MD bootstrap main() and assorted miscellaneous
 * commands.
 */
#include <sys/user.h>
#include <sys/stddef.h>
#include <sys/reboot.h>

#include <boot/bootstand.h>
#include <boot/common/bootstrap.h>
#include <i386/common/bootargs.h>
#include <lib/libkern/libkern.h>
#include <machine/bootinfo.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>

#include "../smbios.h"
#include "libi386/libi386.h"
#include "btxv86.h"

/* Arguments passed in from the boot1/boot2 loader */
static struct bootargs *kargs;

static uint32_t				initial_howto;
static uint32_t				initial_bootdev;
static union bootinfo		*bootinfo;
static struct bootinfo_bios initial_bootinfo = bootinfo->bi_bios;

struct arch_switch		archsw;		/* MI/MD interface boundary */

static void				extract_currdev(void);
static int				isa_inb(int port);
static void				isa_outb(int port, int value);
void					exit(int code);

caddr_t
ptov(uintptr_t x)
{
	return (PTOV(x));
}

int
main(void)
{
	static char malloc[512*1024];
	int			i;

	/* Pick up arguments */
	kargs = (void *)__args;
	initial_howto = kargs->howto;
	initial_bootdev = kargs->bootdev;
	initial_bootinfo = kargs->bootinfo ? (struct bootinfo *)PTOV(kargs->bootinfo) : NULL;


    /* Initialize the v86 register set to a known-good state. */
    bzero(&v86, sizeof(v86));
    v86.efl = PSL_RESERVED_DEFAULT | PSL_I;

    /*
     * Initialise the heap as early as possible.  Once this is done, malloc() is usable.
     */
	bios_getmem();

	calloc((void *)malloc, (void *)(malloc + 512*1024));

	/*
	 * XXX Chicken-and-egg problem; we want to have console output early, but some
	 * console attributes may depend on reading from eg. the boot device, which we
	 * can't do yet.
	 *
	 * We can use printf() etc. once this is done.
	 * If the previous boot stage has requested a serial console, prefer that.
	 */
    bi_setboothowto(initial_howto);
    if (initial_howto & RB_MULTIPLE) {
    	if (initial_howto & RB_SERIAL) {
    		 setenv("console", "comconsole vidconsole", 1);
    	} else {
    		 setenv("console", "vidconsole comconsole", 1);
    	}
    } else if (initial_howto & RB_SERIAL) {
    	setenv("console", "comconsole", 1);
    } else if (initial_howto & RB_MUTE) {
    	setenv("console", "nullconsole", 1);
    }
    cons_probe();

    /*
     * Initialise the block cache. Set the upper limit.
     */
    bcache_init(32768, 512);

    /*
	 * Special handling for PXE and CD booting.
	 */
	if (kargs->bootinfo == 0) {
		/*
		 * We only want the PXE disk to try to init itself in the below
		 * walk through devsw if we actually booted off of PXE.
		 */
		if (kargs->bootflags & KARGS_FLAGS_PXE)
			pxe_enable(kargs->pxeinfo ? PTOV(kargs->pxeinfo) : NULL);
		else if (kargs->bootflags & KARGS_FLAGS_CD)
			bc_add(initial_bootdev);
	}


	//archsw.arch_autoload = i386_autoload;
	archsw.arch_getdev = i386_getdev;
	archsw.arch_copyin = i386_copyin;
	archsw.arch_copyout = i386_copyout;
	archsw.arch_readin = i386_readin;
    archsw.arch_isainb = isa_inb;
    archsw.arch_isaoutb = isa_outb;

    /* ZFS & GELI SUPPORT Belongs Here */

    /*
	 * March through the device switch probing for things.
	 */
    for (i = 0; devsw[i] != NULL; i++) {
    	if (devsw[i]->dv_init != NULL) {
    		(devsw[i]->dv_init)();
    	}
    }
    printf("BIOS %dkB/%dkB available memory\n", bios_basemem / 1024, bios_extmem / 1024);
    if (initial_bootinfo != NULL) {
    	initial_bootinfo->bi_basemem = bios_basemem / 1024;
    	initial_bootinfo->bi_extmem = bios_extmem / 1024;
    }

    /* detect SMBIOS for future reference */
    smbios_detect(NULL);

    /* detect PCI BIOS for future reference */
    biospci_detect();

    printf("\n%s", bootprog_info);

    extract_currdev();				/* set $currdev and $loaddev */

    bios_getsmap();

    interact();

    /* if we ever get here, it is an error */
    return (1);
}

static void
extract_currdev(void)
{
	struct i386_devdesc	currdev;	/* our current device */
	int					biosdev = -1;

	currdev.dd.d_dev = &bioshd;

	 /* new-style boot loaders such as pxeldr and cdldr */
	if (kargs->bootinfo == 0) {
		if ((kargs->bootflags & KARGS_FLAGS_CD) != 0) {
			/* we are booting from a CD with cdboot */
			currdev.dd.d_dev = &bioscd;
			currdev.dd.d_unit = bd_bios2unit(initial_bootdev);
		} else if ((kargs->bootflags & KARGS_FLAGS_PXE) != 0) {
			 /* we are booting from pxeldr */
			currdev.dd.d_dev = &pxedisk;
			currdev.dd.d_unit = 0;
		} else {
		    /* we don't know what our boot device is */
		    currdev.d_kind.biosdisk.slice = -1;
		    currdev.d_kind.biosdisk.partition = 0;
		    biosdev = -1;
		}
		/* ZFS SUPPORT Belongs Here */
	} else if ((initial_bootdev & B_MAGICMASK) != B_DEVMAGIC) {
		/* The passed-in boot device is bad */
	    currdev.d_kind.biosdisk.slice = -1;
	    currdev.d_kind.biosdisk.partition = 0;
		biosdev = -1;
	} else {
	    currdev.d_kind.biosdisk.slice = B_SLICE(initial_bootdev) - 1;
	    currdev.d_kind.biosdisk.partition = B_PARTITION(initial_bootdev);
		biosdev = initial_bootinfo->bi_bios_dev;


		/*
		 * If we are booted by an old bootstrap, we have to guess at the BIOS
		 * unit number.  We will lose if there is more than one disk type
		 * and we are not booting from the lowest-numbered disk type
		 * (ie. SCSI when IDE also exists).
		 */
		if ((biosdev == 0) && (B_TYPE(initial_bootdev) != 2))	/* biosdev doesn't match major */
			biosdev = 0x80 + B_UNIT(initial_bootdev);			/* assume harddisk */
	}

	/*
	 * If we are booting off of a BIOS disk and we didn't succeed in determining
	 * which one we booted off of, just use disk0: as a reasonable default.
	 */
	if ((currdev.dd.d_dev->dv_type == bioshd.dv_type) &&
		((currdev.dd.d_unit = bd_bios2unit(biosdev)) == -1)) {
		printf("Can't work out which disk we are booting from.\n"
		       "Guessed BIOS device 0x%x not found by probes, defaulting to disk0:\n", biosdev);
		currdev.dd.d_unit = 0;
	}

	env_setenv("currdev", EV_VOLATILE, i386_fmtdev(&currdev), (ev_sethook_t *) i386_setcurrdev, env_nounset);
	env_setenv("loaddev", EV_VOLATILE, i386_fmtdev(&currdev), env_noset, env_nounset);
}

/* provide this for panic, as it's not in the startup code */
void
exit(int code)
{
    __exit(code);
}

/* ISA bus access functions for PnP. */
static int
isa_inb(int port)
{
	return (inb(port));
}

static void
isa_outb(int port, int value)
{
	outb(port, value);
}
