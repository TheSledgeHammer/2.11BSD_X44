/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 *
 *	@(#)advvm.c 1.0 	7/01/21
 */

/*
 * AdvVM is a logical volume manager for BSD. That is based on the ideas from Tru64 UNIX's AdvFS,
 * as well as incorporating concepts from Vinum and LVM2.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/user.h>

struct advvm_label 		*advlab;
struct advvm_block 		*advblk;

struct advvm_softc {
	struct device		*sc_dev;		/* Self. */
	struct dkdevice		sc_dkdev;		/* hook for generic disk handling */
	char				*sc_name;		/* device name *
	struct buf 			*sc_buflist;
	char			 	sc_dying;		/* device detached */

	struct advvm_header *sc_header;		/* advvm header */
	struct volume 		*sc_volume;		/* advvm volumes */
	struct domain 		*sc_domain;		/* advvm domains */
	struct fileset 		*sc_fileset;	/* advvm filesets */
};

#define advvmunit(dev) dkunit(dev)

static dev_type_open(advvm_open);
static dev_type_close(advvm_close);
static dev_type_read(advvm_read);
static dev_type_write(advvm_write);
static dev_type_ioctl(advvm_ioctl);
static dev_type_strategy(advvm_strategy);
static dev_type_size(advvm_size);

const struct bdevsw advvm_bdev = {
		.d_open = advvm_open,
		.d_close = advvm_close,
		.d_strategy = advvm_strategy,
		.d_ioctl = advvm_ioctl,
		.d_dump = nodump,
		.d_psize = advvm_size,
		.d_discard = nodiscard,
		.d_type = D_DISK
};

const struct cdevsw advvm_cdev = {
		.d_open = advvm_open,
		.d_close = advvm_close,
		.d_read = advvm_read,
		.d_write = advvm_write,
		.d_ioctl = advvm_ioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_select = noselect,
		.d_poll = nopoll,
		.d_mmap = nommap,
		.d_strategy = advvm_strategy,
		.d_discard = nodiscard,
		.d_type = D_DISK
};

const struct dkdriver advvmdkdriver = {
		.d_strategy = advvm_strategy
};

CFOPS_DECL(advvm, advvm_match, advvm_attach, advvm_detach, advvm_activate);
CFDRIVER_DECL(NULL, advvm, DV_DISK);
CFATTACH_DECL(advvm, &advvm_cd, &advvm_cops, sizeof(struct advvm_softc));

struct advvm_header *
advvm_set_header(magic, label, block)
	long magic;
	struct advvm_label *label;
	struct advvm_block *block;
{
	struct advvm_header *header;
	if (header == NULL) {
		advvm_malloc((struct advvm_header *) header, sizeof(struct advvm_header *));
	}
	header->ahd_magic = magic;
	header->ahd_label = label;
	header->ahd_block = block;

	return (header);
}

int
advvm_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct advvm_softc *adv = (struct advvm_softc *)aux;

	if (strcmp(adv->sc_name, match->cf_driver->cd_name)) {
		return (0);
	}
	return (1);
}

void
advvm_attach(struct device *parent, struct device *self, void *aux)
{
	struct advvm_softc *sc = (struct advvm_softc *)(self);
	struct advvm_attach_args *adv = (struct advvm_attach_args *)aux;

	sc->sc_dev = self;

	sc->sc_header = advvm_set_header(ADVVM_MAGIC, &advlab, &advblk);
	adv->ada_name = sc->sc_header->ahd_label->alb_name;
	self->dv_xname = adv->ada_name;

	/* allocate domains, volumes & filesets */
	advvm_malloc((advvm_domain_t *) sc->sc_domain, sizeof(advvm_domain_t *));
	advvm_malloc((advvm_volume_t *) sc->sc_volume, sizeof(advvm_volume_t *));
	advvm_malloc((advvm_fileset_t *) sc->sc_fileset, sizeof(advvm_fileset_t *));

	/* initialize domains, volumes & filesets */
	advvm_domain_init(sc->sc_domain);
	advvm_volume_init(sc->sc_volume);
	advvm_fileset_init(sc->sc_fileset);

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_attach(&sc->sc_dkdev);
}

int
advvm_detach(struct device *self, int flags)
{
	struct advvm_softc *sc = (struct advvm_softc *)self;
	int s, maj, mn;

	/* locate the major number */
	/* character devices */
	for (maj = 0; maj < nchrdev; maj++) {
		if (cdevsw[maj].d_open == advvm_open) {
			break;
		}
	}
	/* block devices */
	for (maj = 0; maj < nblkdev; maj++) {
		if(bdevsw[maj].d_open == advvm_open) {
			break;
		}
	}

	/* Nuke the vnodes for any open instances (calls close). */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);
	vdevgone(maj, mn, mn, VBLK);

	free(sc->sc_buflist, M_DEVBUF);
}

int
advvm_activate(struct device *self, int act)
{
	struct advvm_softc *sc = (struct advvm_softc *)self;

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		break;
	}

	return (0);
}

int
advvm_open(dev_t dev, int oflag, int devtype, struct proc *p)
{
	struct advvm_softc *sc;
	return (0);
}

int
advvm_close(dev_t dev, int fflag, int devtype, struct proc *p)
{
	struct advvm_softc *sc;
	return (0);
}

int
advvm_read(dev_t dev, struct uio *uio, int ioflag)
{
	struct advvm_softc *sc;
	return (0);
}

int
advvm_write(dev_t dev, struct uio *uio, int ioflag)
{
	struct advvm_softc *sc;
	return (0);
}

int
advvm_strategy(dev_t dev, int fflag, int devtype, struct proc *p)
{

	return (0);
}

int
advvm_minphys(dev_t dev, int fflag, int devtype, struct proc *p)
{
	return (0);
}

int
advvm_start()
{
	return (0);
}

int
advvm_ioctl(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p)
{
	return (0);
}

int
advvm_size(dev_t dev)
{
	int size;

	return (size);
}

int
advvm_mklabel()
{
	return (0);
}
