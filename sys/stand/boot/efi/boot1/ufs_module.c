/*-
 * Copyright (c) 1998 Robert Nordier
 * All rights reserved.
 * Copyright (c) 2001 Robert Drehmel
 * All rights reserved.
 * Copyright (c) 2014 Nathan Whitehorn
 * All rights reserved.
 * Copyright (c) 2015 Eric McCorkle
 * All rights reverved.
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
 *
 * $FreeBSD$
 */

#include <stdarg.h>
#include <stdbool.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <efi.h>

#include "boot_module.h"

static dev_info_t *devinfo;
static dev_info_t *devices;

static int
dskread(void *buf, uint64_t lba, int nblk)
{
	int size;
	EFI_STATUS status;

	lba += devinfo->partoff;
	lba = lba / (devinfo->dev->Media->BlockSize / DEV_BSIZE);
	size = nblk * DEV_BSIZE;

	status = devinfo->dev->ReadBlocks(devinfo->dev,
	    devinfo->dev->Media->MediaId, lba, size, buf);

	if (status != EFI_SUCCESS) {
		DPRINTF("dskread: failed dev: %p, id: %u, lba: %ju, size: %d, "
		    "status: %lu\n", devinfo->dev,
		    devinfo->dev->Media->MediaId, (uintmax_t)lba, size,
		    EFI_ERROR_CODE(status));
		return (-1);
	}

	return (0);
}

static struct ufs_dmadat __dmadat;
static struct ufs_dmadat *boot2_dmadat;

#include "ufsread.c"

#define fsread boot2_ufs_read
#define fsread_size boot2_ufs_read_size
#define lookup boot2_ufs_lookup

static int
init_dev(dev_info_t* dev)
{
	struct disklabel *dl;
	EFI_STATUS status;
	size_t label_size;
	uint64_t bs;
	int ok;

	devinfo = dev;
	boot2_dmadat = &__dmadat;

	/*
	 * First try offset 0. This is the typical GPT case where we have no
	 * further partitioning (as well as the degenerate MBR case where
	 * the bsdlabel has a 0 offset).
	 */
	devinfo->partoff = 0;
	ok = fsread(0, NULL, 0);
	if (ok >= 0) {
		return (ok);
	}

	status = BS->AllocatePool(EfiLoaderData, label_size, (void **)&dl);
	if (status == EFI_SUCCESS) {
		if (dskread(dl, 0, label_size / DEV_BSIZE) == 0 &&
		dl->d_magic == BSD_MAGIC && dl->d_npartitions > 0 &&
		dl->d_partitions[0].p_fstype == FS_BSDFFS) {
			devinfo->partoff = dl->d_partitions[0].p_offset;
		}
		BS->FreePool(dl);
	}
	return (fsread(0, NULL, 0));
}

static EFI_STATUS
probe(dev_info_t* dev)
{

	if (init_dev(dev) < 0)
		return (EFI_UNSUPPORTED);

	add_device(&devices, dev);

	return (EFI_SUCCESS);
}

static EFI_STATUS
load(const char *filepath, dev_info_t *dev, void **bufp, size_t *bufsize)
{
	ufs_ino_t ino;
	size_t size;
	ssize_t read;
	void *buf;

#ifdef EFI_DEBUG
	{
		CHAR16 *text = efi_devpath_name(dev->devpath);
		DPRINTF("UFS Loading '%s' from %S\n", filepath, text);
		efi_free_devpath_name(text);
	}
#endif
	if (init_dev(dev) < 0) {
		DPRINTF("Failed to init device\n");
		return (EFI_UNSUPPORTED);
	}

	if ((ino = lookup(filepath)) == 0) {
		DPRINTF("Failed to lookup '%s' (file not found?)\n", filepath);
		return (EFI_NOT_FOUND);
	}

	if (fsread_size(ino, NULL, 0, &size) < 0 || size <= 0) {
		printf("Failed to read size of '%s' ino: %d\n", filepath, ino);
		return (EFI_INVALID_PARAMETER);
	}

	buf = malloc(size);
	if (buf == NULL) {
		printf("Failed to allocate read buffer %zu for '%s'\n",
		    size, filepath);
		return (EFI_OUT_OF_RESOURCES);
	}

	read = fsread(ino, buf, size);
	if ((size_t)read != size) {
		printf("Failed to read '%s' (%zd != %zu)\n", filepath, read,
		    size);
		free(buf);
		return (EFI_INVALID_PARAMETER);
	}

	DPRINTF("Load complete\n");

	*bufp = buf;
	*bufsize = size;

	return (EFI_SUCCESS);
}

static void
status(void)
{
	int i;
	dev_info_t *dev;

	for (dev = devices, i = 0; dev != NULL; dev = dev->next, i++)
		;

	printf("%s found ", ufs_module.name);
	switch (i) {
	case 0:
		printf("no partitions\n");
		break;
	case 1:
		printf("%d partition\n", i);
		break;
	default:
		printf("%d partitions\n", i);
	}
}

static dev_info_t *
_devices(void)
{

	return (devices);
}

const boot_module_t ufs_module =
{
	.name = "UFS",
	.probe = probe,
	.load = load,
	.status = status,
	.devices = _devices
};
