/*-
 * Copyright (c) 2008 Semihalf, Rafal Jaworowski
 * Copyright (c) 2009 Semihalf, Piotr Ziecik
 * Copyright (c) 2012 Andrey V. Elsukov <ae@FreeBSD.org>
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
 *
 */

/*
 * Block storage I/O routines for U-Boot
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD$"); */

#include <sys/param.h>
#include <sys/disk.h>
#include <sys/stdarg.h>
#include <lib/libsa/stand.h>

#include "api_public.h"
#include "bootstrap.h"
#include "disk.h"
#include "glue.h"
#include "libuboot.h"

#define stor_printf(fmt, args...) do {							\
    printf("%s%d: ", dev->dd.d_dev->dv_name, dev->dd.d_unit);	\
    printf(fmt, ##args);										\
} while (0)

#ifdef DEBUG
#define debugf(fmt, args...) do { printf("%s(): ", __func__);	\
    printf(fmt,##args); } while (0)
#else
#define debugf(fmt, args...)
#endif

static struct {
	int			opened;	/* device is opened */
	int			handle;	/* storage device handle */
	int			type;	/* storage type */
	off_t		blocks;	/* block count */
	u_int		bsize;	/* block size */
} stor_info[UB_MAX_DEV];

//#define	SI(dev)		(stor_info[dev)->dd.d_unit])

static int stor_info_no = 0;
static int stor_opendev(struct uboot_devdesc *);
static int stor_readdev(struct uboot_devdesc *, daddr_t, size_t, char *);

/* devsw I/F */
static int stor_init(void);
static int stor_strategy(void *, int, daddr_t, size_t, char *, size_t *);
static int stor_open(struct open_file *, ...);
static int stor_close(struct open_file *);
static int stor_ioctl(struct open_file *f, u_long cmd, void *data);
static int stor_print(int);
static void stor_cleanup(void);

struct devsw uboot_storage = {
	"disk",
	DEVT_DISK,
	stor_init,
	stor_strategy,
	stor_open,
	stor_close,
	stor_ioctl,
	stor_print,
	stor_cleanup
};

static struct stor_info
SI(dev)
	struct uboot_devdesc *dev;
{
	return (stor_info[dev->dd.d_unit]);
}

static int
stor_init(void)
{
	struct device_info *di;
	int i;

	if (devs_no == 0) {
		printf("No U-Boot devices! Really enumerated?\n");
		return (-1);
	}

	for (i = 0; i < devs_no; i++) {
		di = ub_dev_get(i);
		if ((di != NULL) && (di->type & DEV_TYP_STOR)) {
			if (stor_info_no >= UB_MAX_DEV) {
				printf("Too many storage devices: %d\n",
				    stor_info_no);
				return (-1);
			}
			stor_info[stor_info_no].handle = i;
			stor_info[stor_info_no].opened = 0;
			stor_info[stor_info_no].type = di->type;
			stor_info[stor_info_no].blocks = di->di_stor.block_count;
			stor_info[stor_info_no].bsize = di->di_stor.block_size;
			stor_info_no++;
		}
	}

	if (!stor_info_no) {
		debugf("No storage devices\n");
		return (-1);
	}

	debugf("storage devices found: %d\n", stor_info_no);
	return (0);
}

static void
stor_cleanup(void)
{
	int i;

	for (i = 0; i < stor_info_no; i++)
		if (stor_info[i].opened > 0)
			ub_dev_close(stor_info[i].handle);
}

static int
stor_strategy(void *devdata, int rw, daddr_t blk, size_t size, char *buf, size_t *rsize)
{
	struct uboot_devdesc *dev = (struct uboot_devdesc *)devdata;
	daddr_t bcount;
	int err;

	rw &= F_MASK;
	if (rw != F_READ) {
		stor_printf("write attempt, operation not supported!\n");
		return (EROFS);
	}

	if (size % SI(dev).bsize) {
		stor_printf("size=%zu not multiple of device "
		    "block size=%d\n",
		    size, SI(dev).bsize);
		return (EIO);
	}
	bcount = size / SI(dev).bsize;
	if (rsize)
		*rsize = 0;

	err = stor_readdev(dev, blk + dev->d_kind.d_stor.offset, bcount, buf);
	if (!err && rsize)
		*rsize = size;

	return (err);
}

static int
stor_open(struct open_file *f, ...)
{
	va_list ap;
	struct uboot_devdesc *dev;

	va_start(ap, f);
	dev = va_arg(ap, dev);
	va_end(ap);

	return (stor_opendev(dev));
}

static int
stor_opendev(struct uboot_devdesc *dev)
{
	int err;

	if (dev->dd.d_unit < 0 || dev->dd.d_unit >= stor_info_no)
		return (EIO);

	if (SI(dev).opened == 0) {
		err = ub_dev_open(SI(dev).handle);
		if (err != 0) {
			stor_printf("device open failed with error=%d, "
			    "handle=%d\n", err, SI(dev).handle);
			return (ENXIO);
		}
		SI(dev).opened++;
	}
	return (disk_open(dev, SI(dev).blocks * SI(dev).bsize, SI(dev).bsize));
}

static int
stor_close(struct open_file *f)
{
	struct uboot_devdesc *dev;

	dev = (struct uboot_devdesc *)(f->f_devdata);
	return (disk_close(dev));
}

static int
stor_readdev(struct uboot_devdesc *dev, daddr_t blk, size_t size, char *buf)
{
	lbasize_t real_size;
	int err;

	debugf("reading blk=%d size=%d @ 0x%08x\n", (int)blk, size, (uint32_t)buf);

	err = ub_dev_read(SI(dev).handle, buf, size, blk, &real_size);
	if (err != 0) {
		stor_printf("read failed, error=%d\n", err);
		return (EIO);
	}

	if (real_size != size) {
		stor_printf("real size != size\n");
		err = EIO;
	}

	return (err);
}

static int
stor_print(int verbose)
{
	struct uboot_devdesc dev;
	static char line[80];
	int i, ret = 0;

	if (stor_info_no == 0)
		return (ret);

	printf("%s devices:", uboot_storage.dv_name);
	if ((ret = pager_output("\n")) != 0)
		return (ret);

	for (i = 0; i < stor_info_no; i++) {
		dev.dd.d_dev = &uboot_storage;
		dev.dd.d_unit = i;
		dev.d_kind.d_stor.slice = D_SLICENONE;
		dev.d_kind.d_stor.partition = D_PARTNONE;
		snprintf(line, sizeof(line), "\tdisk%d (%s)\n", i,
		    ub_stor_type(SI(&dev).type));
		if ((ret = pager_output(line)) != 0)
			break;
		if (stor_opendev(&dev) == 0) {
			sprintf(line, "\tdisk%d", i);
			ret = disk_print(&dev, line, verbose);
			disk_close(&dev);
			if (ret != 0)
				break;
		}
	}
	return (ret);
}

static int
stor_ioctl(struct open_file *f, u_long cmd, void *data)
{
	struct uboot_devdesc *dev;
	int rc;

	dev = (struct uboot_devdesc *)f->f_devdata;
	rc = disk_ioctl(dev, cmd, data);
	if (rc != ENOTTY)
		return (rc);

	switch (cmd) {
	case DIOCGSECTORSIZE:
		*(u_int *)data = SI(dev).bsize;
		break;
	case DIOCGMEDIASIZE:
		*(uint64_t *)data = SI(dev).bsize * SI(dev).blocks;
		break;
	default:
		return (ENOTTY);
	}
	return (0);
}


/*
 * Return the device unit number for the given type and type-relative unit
 * number.
 */
int
uboot_diskgetunit(int type, int type_unit)
{
	int local_type_unit;
	int i;

	local_type_unit = 0;
	for (i = 0; i < stor_info_no; i++) {
		if ((stor_info[i].type & type) == type) {
			if (local_type_unit == type_unit) {
				return (i);
			}
			local_type_unit++;
		}
	}

	return (-1);
}

int
uboot_ioctl(struct uboot_devdesc *dev, u_long cmd, void *data)
{
	struct open_disk *od = dev->dd.d_opendata;

	if (od == NULL)
		return (ENOTTY);

	switch (cmd) {
	case DIOCGSECTORSIZE:
		*(u_int *)data = od->sectorsize;
		break;
	case DIOCGMEDIASIZE:
		if (dev->d_kind.d_stor.offset == 0)
			*(uint64_t *)data = od->mediasize;
		else
			*(uint64_t *)data = od->entrysize * od->sectorsize;
		break;
	default:
		return (ENOTTY);
	}

	return (0);
}

int
disk_open(struct uboot_devdesc *dev, uint64_t mediasize, u_int sectorsize)
{
	struct uboot_devdesc partdev;
	struct open_disk *od;
	struct ptable *table;
	struct ptable_entry part;
	int rc, slice, partition;

	if (sectorsize == 0) {
		DPRINTF("unknown sector size");
		return (ENXIO);
	}
	rc = 0;
	od = (struct open_disk *)malloc(sizeof(struct open_disk));
	if (od == NULL) {
		DPRINTF("no memory");
		return (ENOMEM);
	}
	dev->dd.d_opendata = od;
	od->entrysize = 0;
	od->mediasize = mediasize;
	od->sectorsize = sectorsize;
	/*
	 * While we are reading disk metadata, make sure we do it relative
	 * to the start of the disk
	 */
	memcpy(&partdev, dev, sizeof(partdev));
	partdev.d_offset = 0;
	partdev.d_slice = D_SLICENONE;
	partdev.d_partition = D_PARTNONE;

	dev->d_offset = 0;
	table = NULL;
	slice = dev->d_slice;
	partition = dev->d_partition;

	DPRINTF("%s unit %d, slice %d, partition %d => %p", disk_fmtdev(dev), dev->dd.d_unit, dev->d_slice, dev->d_partition, od);

	/* Determine disk layout. */
	od->table = ptable_open(&partdev, mediasize / sectorsize, sectorsize, ptblread);
	if (od->table == NULL) {
		DPRINTF("Can't read partition table");
		rc = ENXIO;
		goto out;
	}

	if (ptable_getsize(od->table, &mediasize) != 0) {
		rc = ENXIO;
		goto out;
	}
	od->mediasize = mediasize;

	if (ptable_gettype(od->table) == PTABLE_BSD &&
	    partition >= 0) {
		/* It doesn't matter what value has d_slice */
		rc = ptable_getpart(od->table, &part, partition);
		if (rc == 0) {
			dev->d_offset = part.start;
			od->entrysize = part.end - part.start + 1;
		}
	} else if (ptable_gettype(od->table) == PTABLE_ISO9660) {
		dev->d_offset = 0;
		od->entrysize = mediasize;
	} else if (slice >= 0) {
		/* Try to get information about partition */
		if (slice == 0)
			rc = ptable_getbestpart(od->table, &part);
		else
			rc = ptable_getpart(od->table, &part, slice);
		if (rc != 0) /* Partition doesn't exist */
			goto out;
		dev->d_offset = part.start;
		od->entrysize = part.end - part.start + 1;
		slice = part.index;
		if (ptable_gettype(od->table) == PTABLE_GPT) {
			partition = D_PARTISGPT;
			goto out; /* Nothing more to do */
		} else if (partition == D_PARTISGPT) {
			/*
			 * When we try to open GPT partition, but partition
			 * table isn't GPT, reset partition value to
			 * D_PARTWILD and try to autodetect appropriate value.
			 */
			partition = D_PARTWILD;
		}

		/*
		 * If partition is D_PARTNONE, then disk_open() was called
		 * to open raw MBR slice.
		 */
		if (partition == D_PARTNONE)
			goto out;

		/*
		 * If partition is D_PARTWILD and we are looking at a BSD slice,
		 * then try to read BSD label, otherwise return the
		 * whole MBR slice.
		 */
		if (partition == D_PARTWILD &&
		    part.type != PART_FREEBSD)
			goto out;
		/* Try to read BSD label */
		table = ptable_open(dev, part.end - part.start + 1, od->sectorsize, ptblread);
		if (table == NULL) {
			DPRINTF("Can't read BSD label");
			rc = ENXIO;
			goto out;
		}
		/*
		 * If slice contains BSD label and partition < 0, then
		 * assume the 'a' partition. Otherwise just return the
		 * whole MBR slice, because it can contain ZFS.
		 */
		if (partition < 0) {
			if (ptable_gettype(table) != PTABLE_BSD)
				goto out;
			partition = 0;
		}
		rc = ptable_getpart(table, &part, partition);
		if (rc != 0)
			goto out;
		dev->d_offset += part.start;
		od->entrysize = part.end - part.start + 1;
	}
out:
	if (table != NULL)
		ptable_close(table);

	if (rc != 0) {
		if (od->table != NULL)
			ptable_close(od->table);
		free(od);
		DPRINTF("%s could not open", disk_fmtdev(dev));
	} else {
		/* Save the slice and partition number to the dev */
		dev->d_slice = slice;
		dev->d_partition = partition;
		DPRINTF("%s offset %lld => %p", disk_fmtdev(dev),
		    (long long)dev->d_offset, od);
	}
	return (rc);
}

int
disk_close(struct uboot_devdesc *dev)
{
	struct open_disk *od;

	od = (struct open_disk *)dev->dd.d_opendata;
	DPRINTF("%s closed => %p", disk_fmtdev(dev), od);
	//ptable_close(od->table);
	free(od);
	return (0);
}

char*
disk_fmtdev(struct uboot_devdesc *dev)
{
	static char buf[128];
	char *cp;

	cp = buf + sprintf(buf, "%s%d", dev->dd.d_dev->dv_name, dev->dd.d_unit);
	if (dev->d_slice > D_SLICENONE) {
#ifdef LOADER_GPT_SUPPORT
		if (dev->d_partition == D_PARTISGPT) {
			sprintf(cp, "p%d:", dev->d_slice);
			return (buf);
		} else
#endif
#ifdef LOADER_MBR_SUPPORT
			cp += sprintf(cp, "s%d", dev->d_slice);
#endif
	}
	if (dev->d_kind->d_stor.partition > D_PARTNONE)
		cp += sprintf(cp, "%c", dev->d_partition + 'a');
	strcat(cp, ":");
	return (buf);
}

