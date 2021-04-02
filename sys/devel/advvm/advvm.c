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

/* AdvVM is a logical volume manager for BSD. That is based on the ideas from Tru64 UNIX's AdvFS,
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

struct advvm_label 	*advlab;
struct advvm_block 	*advblk;

struct advvm_softc {
	struct device		*sc_dev;		/* Self. */
	struct dkdevice		sc_dkdev;		/* hook for generic disk handling */
	//struct lock		sc_lock;
	struct buf 			*sc_buflist;

	struct advvm_header *sc_header;		/* advvm header */
	struct volume 		*sc_volume;		/* advvm volumes */
	struct domain 		*sc_domain;		/* advvm domains */
	struct fileset 		*sc_fileset;	/* advvm filesets */
};

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

static const struct dkdriver advvm_driver = {

};

const struct cfdriver advvm_cd = {
		NULL, "advvm", advvm_probe, advvm_attach, DV_DISK, sizeof(struct advvm_softc)
};

struct advvm_header *
advvm_set_header(magic, label, block)
	long magic;
	struct advvm_label *label;
	struct advvm_block *block;
{
	struct advvm_header *header;
	if(header == NULL) {
		advvm_malloc((struct advvm_header *) header, sizeof(struct advvm_header *));
	}
	header->ahd_magic = magic;
	header->ahd_label = label;
	header->ahd_block = block;

	return (header);
}

int
advvm_probe(struct device *parent, struct cfdata *match, void *aux)
{
	return (0);
}

void
advvm_attach(struct device *parent, struct device *self, void *aux)
{
	struct advvm_softc *sc = (void *)(self);

	sc->sc_dev = self;

	sc->sc_header = advvm_set_header(129, &advlab, &advblk); /* TODO: generate magic number */
	self->dv_xname = sc->sc_header->ahd_label->alb_name;

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_attach(&sc->sc_dkdev);
}

int
advvm_open(dev_t dev, int oflag, int devtype, struct proc *p)
{
	return (0);
}

int
advvm_close(dev_t dev, int fflag, int devtype, struct proc *p)
{
	return (0);
}

int
advvm_read(dev_t dev, struct uio *uio, int ioflag)
{
	return (0);
}

int
advvm_write(dev_t dev, struct uio *uio, int ioflag)
{
	return (0);
}

int
advvm_strategy(dev_t dev, int fflag, int devtype, struct proc *p)
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
