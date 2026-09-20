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

#include <sys/reboot.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/bcache.h>
#include <lib/libkern/libkern.h>

#include <stand/common/bootstrap.h>
#include <stand/common/console.h>
#include <boot/common/smbios.h>

#include "libi386.h"
#include "btxv86.h"

#include <machine/bootinfo.h>
#include <machine/psl.h>

static struct bootargs 		*kargs;
static uint32_t				initial_howto;
static uint32_t				initial_bootdev;
static struct bootinfo		*initial_bootinfo;

static void extract_currdev(struct bootargs *, struct bootinfo *, uint32_t);
static int isa_inb(int);
static void isa_outb(int, int);

struct arch_switch archsw = {
		.arch_autoload = i386_autoload,
		.arch_getdev = i386_getdev,
		.arch_copyin = i386_copyin,
		.arch_copyout = i386_copyout,
		.arch_readin = i386_readin,
		.arch_isainb = isa_inb,
		.arch_isaoutb = isa_outb,
};

int
main(void)
{
	static char malloc[512 * 1024];
	int i;

	/* Pick up arguments */
	kargs = (void *)__args;
	initial_howto = kargs->howto;
	initial_bootdev = kargs->bootdev;
	initial_bootinfo =
			kargs->bootinfo ? (struct bootinfo *)PTOV(kargs->bootinfo) : NULL;

	/* Initialize the v86 register set to a known-good state. */
	bzero(&v86, sizeof(v86));
	v86.efl = PSL_RESERVED_DEFAULT | PSL_I;

	/*
	 * Initialise the heap as early as possible.  Once this is done, malloc() is usable.
	 */
	bios_getmem();

	setheap((void *)malloc, (void *) (malloc + 512 * 1024));

	/*
	 * XXX Chicken-and-egg problem; we want to have console output early, but some
	 * console attributes may depend on reading from eg. the boot device, which we
	 * can't do yet.
	 *
	 * We can use printf() etc. once this is done.
	 * If the previous boot stage has requested a serial console, prefer that.
	 */
	md_setboothowto(initial_howto);
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
		if (kargs->bootflags & KARGS_FLAGS_PXE) {
			//pxe_enable(kargs->pxeinfo ? PTOV(kargs->pxeinfo) : NULL);
		}

		if (kargs->bootflags & KARGS_FLAGS_CD) {
			bc_add(initial_bootdev);
		}
	}

	/* ZFS & GELI SUPPORT Belongs Here */

	/*
	 * March through the device switch probing for things.
	 */
	for (i = 0; devsw[i] != NULL; i++) {
		if (devsw[i]->dv_init != NULL) {
			(devsw[i]->dv_init)();
		}
	}

	printf("BIOS %dkB/%dkB available memory\n", bios_basemem / 1024,
			bios_extmem / 1024);
	if (initial_bootinfo != NULL) {
		initial_bootinfo->bi_basemem = bios_basemem / 1024;
		initial_bootinfo->bi_extmem = bios_extmem / 1024;
	}

	/* detect ACPI for future reference */
	//biosacpi_detect();

	/* detect SMBIOS for future reference */
	smbios_detect(NULL);

	/* detect PCI BIOS for future reference */
	biospci_detect();

	printf("\n%s", bootprog_info);

	extract_currdev(kargs, initial_bootinfo, initial_bootdev);
	setenv("LINES", "24", 1); /* optional */

	bios_getsmap();

	interact();

	/* if we ever get here, it is an error */
	return (1);
}

static void
extract_currdev(struct bootargs *ba, struct bootinfo *bi, uint32_t bootdev)
{
	struct i386_devdesc	currdev;
	struct devsw *dv;
	int biosdev = -1;

	dv = &biosdisk;
	 /* new-style boot loaders such as pxeldr and cdldr */
	if (ba->bootinfo == 0) {
		if ((ba->bootflags & KARGS_FLAGS_CD) != 0) {
			/* we are booting from a CD with cdboot */
			dv = &bioscd;
			currdev.d_kind.dd.d_unit = bc_bios2unit(bootdev);
		} else if ((ba->bootflags & KARGS_FLAGS_PXE) != 0) {
			 /* we are booting from pxeldr */
			dv = &pxedisk;
			currdev.d_kind.dd.d_unit = 0;
		} else {
		    /* we don't know what our boot device is */
			currdev.d_kind.dd.d_slice = -1;
			currdev.d_kind.dd.d_adaptor = -1;
			currdev.d_kind.dd.d_controller = -1;
			currdev.d_kind.dd.d_partition = 0;
		    biosdev = -1;
		}
		/* ZFS SUPPORT Belongs Here */
	} else if ((bootdev & B_MAGICMASK) != B_DEVMAGIC) {
		/* The passed-in boot device is bad */
		currdev.d_kind.dd.d_slice = -1;
		currdev.d_kind.dd.d_adaptor = -1;
		currdev.d_kind.dd.d_controller = -1;
		currdev.d_kind.dd.d_partition = 0;
		currdev = -1;
	} else {
		biosdev = bi->bi_bios_dev;
		disk_setbootdev(dev, biosdev);

		/*
		 * If we are booted by an old bootstrap, we have to guess at the BIOS
		 * unit number.  We will lose if there is more than one disk type
		 * and we are not booting from the lowest-numbered disk type
		 * (ie. SCSI when IDE also exists).
		 */
		if ((biosdev == 0) && (B_TYPE(bootdev) != 2)) {	/* biosdev doesn't match major */
			biosdev = 0x80 + B_UNIT(bootdev);			/* assume harddisk */
		}
	}
	currdev.d_kind.dd.d_dev = dv;
	currdev.d_kind.dd.d_type = currdev.d_kind.dd->d_dev->dv_type;

	/*
	 * If we are booting off of a BIOS disk and we didn't succeed in determining
	 * which one we booted off of, just use disk0: as a reasonable default.
	 */
	if ((currdev.d_kind.dd.d_type == biosdisk.dv_type) &&
		((currdev.d_kind.dd.d_unit = bd_bios2unit(biosdev)) == -1)) {
		printf("Can't work out which disk we are booting from.\n"
		       "Guessed BIOS device 0x%x not found by probes, defaulting to disk0:\n", biosdev);
		currdev.d_kind.dd.d_unit = 0;
	}

	disk_setcurrdev(disk_fmtdev(&currdev.d_kind.dd));
	env_setenv("currdev", EV_VOLATILE, disk_fmtdev(&currdev.d_kind.dd), (ev_sethook_t *)disk_setcurrdev, env_nounset);
	env_setenv("loaddev", EV_VOLATILE, disk_fmtdev(&currdev.d_kind.dd), env_noset, env_nounset);
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
