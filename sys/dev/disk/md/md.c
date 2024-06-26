/*	$NetBSD: md.c,v 1.36 2003/06/29 22:30:00 fvdl Exp $	*/

/*
 * Copyright (c) 1995 Gordon W. Ross, Leo Weppelman.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *			Gordon W. Ross and Leo Weppelman.
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
 * This implements a general-purpose memory-disk.
 * See md.h for notes on the config types.
 *
 * Note that this driver provides the same functionality
 * as the MFS filesystem hack, but this is better because
 * you can use this for any filesystem type you'd like!
 *
 * Credit for most of the kmem ramdisk code goes to:
 *   Leo Weppelman (atari) and Phil Nelson (pc532)
 * Credit for the ideas behind the "user space memory" code goes
 * to the authors of the MFS implementation.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: md.c,v 1.36 2003/06/29 22:30:00 fvdl Exp $");

#include "opt_md.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/disklabel.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_extern.h>

#include <dev/disk/md/md.h>

#include "ioconf.h"

/*
 * By default, include the user-space functionality.
 * Use  `options MEMORY_DISK_SERVER=0' to turn it off.
 */
#ifndef MEMORY_DISK_SERVER
#define	MEMORY_DISK_SERVER 1
#endif	/* MEMORY_DISK_SERVER */

/*
 * We should use the raw partition for ioctl.
 */
#define MD_MAX_UNITS	0x10
#define MD_UNIT(unit)	dkunit(unit)

/* autoconfig stuff... */

struct md_softc {
	struct device sc_dev;	/* REQUIRED first entry */
	struct dkdevice sc_dkdev;	/* hook for generic disk handling */
	struct md_conf sc_md;
	struct bufq_state sc_buflist;
};
/* shorthand for fields in sc_md: */
#define sc_addr sc_md.md_addr
#define sc_size sc_md.md_size
#define sc_type sc_md.md_type

void mdattach(int);
static int md_match(struct device *, struct cfdata *, void *);
static void md_attach(struct device *, struct device *, void *);
static int md_detach(struct device *, int);

CFOPS_DECL(md, md_match, md_attach, md_detach, NULL);
CFDRIVER_DECL(NULL, md, DV_DISK);
CFATTACH_DECL(md, &md_cd, &md_cops, sizeof(struct md_softc));

dev_type_open(mdopen);
dev_type_close(mdclose);
dev_type_read(mdread);
dev_type_write(mdwrite);
dev_type_ioctl(mdioctl);
dev_type_strategy(mdstrategy);
dev_type_size(mdsize);

const struct bdevsw md_bdevsw = {
	.d_open = mdopen,
	.d_close = mdclose,
	.d_strategy = mdstrategy,
	.d_ioctl = mdioctl,
	.d_dump = nodump,
	.d_psize = mdsize,
	.d_type = D_DISK
};

const struct cdevsw md_cdevsw = {
	.d_open = mdopen,
	.d_close = mdclose,
	.d_read = mdread,
	.d_write = mdwrite,
	.d_ioctl = mdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_type = D_DISK
};

struct dkdriver mddkdriver = {
		mdstrategy
};

static int   ramdisk_ndevs;
static void *ramdisk_devs[MD_MAX_UNITS];

/*
 * This is called if we are configured as a pseudo-device
 */
void
mdattach(n)
	int n;
{
	struct md_softc *sc;
	int i;

#ifdef	DIAGNOSTIC
	if (ramdisk_ndevs) {
		printf("ramdisk: multiple attach calls?\n");
		return;
	}
#endif

	/* XXX:  Are we supposed to provide a default? */
	if (n <= 1)
		n = 1;
	if (n > MD_MAX_UNITS)
		n = MD_MAX_UNITS;
	ramdisk_ndevs = n;

	/* Attach as if by autoconfig. */
	for (i = 0; i < n; i++) {

		sc = malloc(sizeof(*sc), M_DEVBUF, M_NOWAIT|M_ZERO);
		if (!sc) {
			printf("ramdisk: malloc for attach failed!\n");
			return;
		}
		ramdisk_devs[i] = sc;
		sc->sc_dev.dv_unit = i;
		sprintf(sc->sc_dev.dv_xname, "md%d", i);
		md_attach(NULL, &sc->sc_dev, NULL);
	}
}

int
md_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

static void
md_attach(parent, self, aux)
	struct device	*parent, *self;
	void		*aux;
{
	struct md_softc *sc = (struct md_softc *)self;

	bufq_alloc(&sc->sc_buflist, BUFQ_FCFS);

	/* XXX - Could accept aux info here to set the config. */
#ifdef	MEMORY_DISK_HOOKS
	/*
	 * This external function might setup a pre-loaded disk.
	 * All it would need to do is setup the md_conf struct.
	 * See sys/dev/md_root.c for an example.
	 */
	md_attach_hook(sc->sc_dev.dv_unit, &sc->sc_md);
#endif

	/*
	 * Initialize and attach the disk structure.
	 */
	sc->sc_dkdev.dk_driver = &mddkdriver;
	sc->sc_dkdev.dk_name = sc->sc_dev.dv_xname;
	disk_attach(&sc->sc_dkdev, &md_bdevsw, &md_cdevsw);
}


static int
md_detach(self, flags)
	struct device *self;
	int flags;
{
	struct md_softc *sc = (struct md_softc *)self;
	int rc;

	rc = 0;
	if (sc->sc_dkdev.dk_openmask == 0 && sc->sc_type == MD_UNCONFIGURED) {
		;	/* nothing to do */
	} else if ((flags & DETACH_FORCE) == 0) {
		rc = EBUSY;
	}

	if (rc != 0)
		return rc;

	disk_detach(&sc->sc_dkdev);
	bufq_free(&sc->sc_buflist);
	return 0;
}

/*
 * operational routines:
 * open, close, read, write, strategy,
 * ioctl, dump, size
 */

#if MEMORY_DISK_SERVER
static int md_server_loop(struct md_softc *sc);
static int md_ioctl_server(struct md_softc *sc,
		struct md_conf *umd, struct proc *proc);
#endif	/* MEMORY_DISK_SERVER */
static int md_ioctl_kalloc(struct md_softc *sc,
		struct md_conf *umd, struct proc *proc);

int
mdsize(dev_t dev)
{
	int unit;
	struct md_softc *sc;

	unit = MD_UNIT(dev);
	if (unit >= ramdisk_ndevs)
		return 0;
	sc = ramdisk_devs[unit];
	if (sc == NULL)
		return 0;

	if (sc->sc_type == MD_UNCONFIGURED)
		return 0;

	return (sc->sc_size >> DEV_BSHIFT);
}

int
mdopen(dev, flag, fmt, proc)
	dev_t dev;
	int flag, fmt;
	struct proc *proc;
{
	int unit;
	struct md_softc *sc;

	unit = MD_UNIT(dev);
	if (unit >= ramdisk_ndevs)
		return ENXIO;
	sc = ramdisk_devs[unit];
	if (sc == NULL)
		return ENXIO;

	/*
	 * The raw partition is used for ioctl to configure.
	 */
	if (dkpart(dev) == RAW_PART)
		return 0;

#ifdef	MEMORY_DISK_HOOKS
	/* Call the open hook to allow loading the device. */
	md_open_hook(unit, &sc->sc_md);
#endif

	/*
	 * This is a normal, "slave" device, so
	 * enforce initialized.
	 */
	if (sc->sc_type == MD_UNCONFIGURED)
		return ENXIO;

	return 0;
}

int
mdclose(dev, flag, fmt, proc)
	dev_t dev;
	int flag, fmt;
	struct proc *proc;
{
	int unit;

	unit = MD_UNIT(dev);

	if (unit >= ramdisk_ndevs)
		return ENXIO;

	return 0;
}

int
mdread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	int unit;
	struct md_softc *sc;

	unit = MD_UNIT(dev);

	if (unit >= ramdisk_ndevs)
		return ENXIO;

	sc = ramdisk_devs[unit];

	if (sc->sc_type == MD_UNCONFIGURED)
		return ENXIO;

	return (physio(mdstrategy, NULL, dev, B_READ, minphys, uio));
}

int
mdwrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	int unit;
	struct md_softc *sc;

	unit = MD_UNIT(dev);

	if (unit >= ramdisk_ndevs)
		return ENXIO;

	sc = ramdisk_devs[unit];

	if (sc->sc_type == MD_UNCONFIGURED)
		return ENXIO;

	return (physio(mdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

/*
 * Handle I/O requests, either directly, or
 * by passing them to the server process.
 */
void
mdstrategy(bp)
	struct buf *bp;
{
	int unit;
	struct md_softc	*sc;
	caddr_t	addr;
	size_t off, xfer;

	unit = MD_UNIT(bp->b_dev);
	sc = ramdisk_devs[unit];

	if (sc->sc_type == MD_UNCONFIGURED) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}

	switch (sc->sc_type) {
#if MEMORY_DISK_SERVER
	case MD_UMEM_SERVER:
		/* Just add this job to the server's queue. */
		BUFQ_PUT(&sc->sc_buflist, bp);
		wakeup((caddr_t)sc);
		/* see md_server_loop() */
		/* no biodone in this case */
		return;
#endif	/* MEMORY_DISK_SERVER */

	case MD_KMEM_FIXED:
	case MD_KMEM_ALLOCATED:
		/* These are in kernel space.  Access directly. */
		bp->b_resid = bp->b_bcount;
		off = (bp->b_blkno << DEV_BSHIFT);
		if (off >= sc->sc_size) {
			if (bp->b_flags & B_READ)
				break;	/* EOF */
			goto set_eio;
		}
		xfer = bp->b_resid;
		if (xfer > (sc->sc_size - off))
			xfer = (sc->sc_size - off);
		addr = sc->sc_addr + off;
		if (bp->b_flags & B_READ)
			memcpy(bp->b_data, addr, xfer);
		else
			memcpy(addr, bp->b_data, xfer);
		bp->b_resid -= xfer;
		break;

	default:
		bp->b_resid = bp->b_bcount;
	set_eio:
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		break;
	}
 done:
	biodone(bp);
}

int
mdioctl(dev, cmd, data, flag, proc)
	dev_t dev;
	u_long cmd;
	int flag;
	caddr_t data;
	struct proc *proc;
{
	int unit;
	struct md_softc *sc;
	struct md_conf *umd;

	unit = MD_UNIT(dev);
	sc = ramdisk_devs[unit];

	/* If this is not the raw partition, punt! */
	if (dkpart(dev) != RAW_PART)
		return ENOTTY;

	umd = (struct md_conf *)data;
	switch (cmd) {
	case MD_GETCONF:
		*umd = sc->sc_md;
		return 0;

	case MD_SETCONF:
		/* Can only set it once. */
		if (sc->sc_type != MD_UNCONFIGURED)
			break;
		switch (umd->md_type) {
		case MD_KMEM_ALLOCATED:
			return md_ioctl_kalloc(sc, umd, proc);
#if MEMORY_DISK_SERVER
		case MD_UMEM_SERVER:
			return md_ioctl_server(sc, umd, proc);
#endif	/* MEMORY_DISK_SERVER */
		default:
			break;
		}
		break;
	}
	return EINVAL;
}

/*
 * Handle ioctl MD_SETCONF for (sc_type == MD_KMEM_ALLOCATED)
 * Just allocate some kernel memory and return.
 */
static int
md_ioctl_kalloc(sc, umd, proc)
	struct md_softc *sc;
	struct md_conf *umd;
	struct proc *proc;
{
	vm_offset_t addr;
	vm_size_t size;

	/* Sanity check the size. */
	size = umd->md_size;
	addr = kmem_alloc(kernel_map, size);
	if (!addr)
		return ENOMEM;

	/* This unit is now configured. */
	sc->sc_addr = (caddr_t)addr; 	/* kernel space */
	sc->sc_size = (size_t)size;
	sc->sc_type = MD_KMEM_ALLOCATED;
	return 0;
}	

#if MEMORY_DISK_SERVER

/*
 * Handle ioctl MD_SETCONF for (sc_type == MD_UMEM_SERVER)
 * Set config, then become the I/O server for this unit.
 */
static int
md_ioctl_server(sc, umd, proc)
	struct md_softc *sc;
	struct md_conf *umd;
	struct proc *proc;
{
	vm_offset_t end;
	int error;

	/* Sanity check addr, size. */
	end = (vm_offset_t) (umd->md_addr + umd->md_size);

	if ((end >= VM_MAXUSER_ADDRESS) ||
		(end < ((vm_offset_t) umd->md_addr)) )
		return EINVAL;

	/* This unit is now configured. */
	sc->sc_addr = umd->md_addr; 	/* user space */
	sc->sc_size = umd->md_size;
	sc->sc_type = MD_UMEM_SERVER;

	/* Become the server daemon */
	error = md_server_loop(sc);

	/* This server is now going away! */
	sc->sc_type = MD_UNCONFIGURED;
	sc->sc_addr = 0;
	sc->sc_size = 0;

	return (error);
}	

int md_sleep_pri = PWAIT | PCATCH;

static int
md_server_loop(sc)
	struct md_softc *sc;
{
	struct buf *bp;
	caddr_t addr;	/* user space address */
	size_t off;	/* offset into "device" */
	size_t xfer;	/* amount to transfer */
	int error;

	for (;;) {
		/* Wait for some work to arrive. */
		while ((bp = BUFQ_GET(&sc->sc_buflist)) == NULL) {
			error = tsleep((caddr_t)sc, md_sleep_pri, "md_idle", 0);
			if (error)
				return error;
		}

		/* Do the transfer to/from user space. */
		error = 0;
		bp->b_resid = bp->b_bcount;
		off = (bp->b_blkno << DEV_BSHIFT);
		if (off >= sc->sc_size) {
			if (bp->b_flags & B_READ)
				goto done;	/* EOF (not an error) */
			error = EIO;
			goto done;
		}
		xfer = bp->b_resid;
		if (xfer > (sc->sc_size - off))
			xfer = (sc->sc_size - off);
		addr = sc->sc_addr + off;
		if (bp->b_flags & B_READ)
			error = copyin(addr, bp->b_data, xfer);
		else
			error = copyout(bp->b_data, addr, xfer);
		if (!error)
			bp->b_resid -= xfer;

	done:
		if (error) {
			bp->b_error = error;
			bp->b_flags |= B_ERROR;
		}
		biodone(bp);
	}
}
#endif	/* MEMORY_DISK_SERVER */
