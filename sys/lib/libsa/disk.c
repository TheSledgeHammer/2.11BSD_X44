/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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
 */

#include <disklabel.h>
#include <lib/libsa/stand.h>

#ifdef DISK_DEBUG
# define DPRINTF(fmt, args...)	printf("%s: " fmt "\n" , __func__ , ## args)
#else
# define DPRINTF(fmt, args...)	((void)0)
#endif

#define	D_PARTNONE	-1

struct disk_devdesc {
	struct devdesc	dd;
	int				d_slice;
	int				d_partition;
	uint64_t		d_offset;
};

struct open_disk {
	u_int			od_sector;
	uint64_t		od_entry;
	uint64_t		od_media;

	int 			od_dkunit;		/* disk unit number */
	int 			od_unit;		/* BIOS unit number */
	int				od_cyl;			/* BIOS geometry */
	int				od_hds;
	int				od_sec;
	int				od_boff;		/* block offset from beginning of BIOS disk */
	int				od_flags;
};

int
disk_read(dev, buf, offset, blocks)
	struct disk_devdesc *dev;
	void *buf;
	uint64_t offset;
	u_int blocks;
{
	struct open_disk *od;
	int ret;

	od = (struct open_disk *)dev->dd.d_opendata;
	ret = dev->dd.d_dev->dv_strategy(dev, F_READ, dev->d_offset + offset, blocks * od->od_sector, buf, NULL);
	return (ret);
}

int
disk_write(dev, buf, offset, blocks)
	struct disk_devdesc *dev;
	void *buf;
	uint64_t offset;
	u_int blocks;
{
	struct open_disk *od;
	int ret;

	od = (struct open_disk*) dev->dd.d_opendata;
	ret = dev->dd.d_dev->dv_strategy(dev, F_READ, dev->d_offset + offset, blocks * od->od_sector, buf, NULL);
	return (ret);
}

int
disk_ioctl(dev, cmd, data)
	struct disk_devdesc *dev;
	u_long cmd;
	void *data;
{
	struct open_disk *od;

	od = dev->dd.d_opendata;
	if (od == NULL) {
		return (ENOTTY);
	}

	switch (cmd) {
	case DIOCGSECTORSIZE:
		*(u_int *)data = od->od_sector;
		break;
	case DIOCGMEDIASIZE:
		if (dev->d_offset == 0) {
			*(uint64_t*) data = od->od_media;
		} else {
			*(uint64_t*) data = od->od_entry * od->od_sector;
		}
		break;
	default:
		return (ENOTTY);
	}

	return (0);
}

int
disk_open(dev, media, sector)
	struct disk_devdesc *dev;
	uint64_t 			media;
	u_int 				sector;
{
	struct open_disk 	*od;
	struct disklabel	*lp;
	int rc, slice, partition;

	if (sectorsize == 0) {
		DPRINTF("unknown sector size");
		return (ENXIO);
	}
	rc = 0;
	od = (struct open_disk*) malloc(sizeof(struct open_disk));
	if (od == NULL) {
		DPRINTF("no memory");
		return (ENOMEM);
	}
	dev->dd.d_opendata = od;
	od->od_entry = 0;
	od->od_media = media;
	od->od_sector = sector;
	/*
	 * While we are reading disk metadata, make sure we do it relative
	 * to the start of the disk
	 */
	dev->d_offset = 0;
	slice = dev->d_slice;
	partition = dev->d_partition;

	od->od_media = media;

	return (rc);
}

int
disk_close(dev)
	struct disk_devdesc *dev;
{
	struct open_disk *od;

	od = (struct open_disk *)dev->dd.d_opendata;
	DPRINTF("%s closed => %p", disk_fmtdev(dev), od);
	free(od);
	return (0);
}

char*
disk_fmtdev(dev)
	struct disk_devdesc *dev;
{
	static char buf[128];
	char *cp;

	cp = buf + sprintf(buf, "%s%d", dev->dd.d_dev->dv_name, dev->dd.d_unit);

	if (dev->d_partition > D_PARTNONE) {
		cp += sprintf(cp, "%c", dev->d_partition + 'a');
	}
	strcat(cp, ":");
	return (buf);
}

