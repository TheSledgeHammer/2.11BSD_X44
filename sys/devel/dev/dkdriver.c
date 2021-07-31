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
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/dk.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <devel/dev/dkvar.h>

struct dkdriver dkd = {
		.d_strategy = 	dkdriver_strategy,
		.d_minphys = 	dkdriver_minphys,
		.d_open = 		dkdriver_open,
		.d_close = 		dkdriver_close,
		.d_dump = 		dkdriver_dump,
		.d_start = 		dkdriver_start,
		.d_mklabel = 	dkdriver_mklabel
};

void
dkdriver_strategy(disk, bp)
	struct dkdevice *disk;
	struct buf *bp;
{
	register struct dkdriver *driver;
	int rv;

	driver = disk->dk_driver;

	if (driver->d_strategy) {
		driver->d_strategy(bp);
	}
}

void
dkdriver_minphys(disk, bp)
	struct dkdevice *disk;
	struct buf *bp;
{
	register struct dkdriver *driver;
	int rv;

	driver = disk->dk_driver;

	if (driver->d_minphys) {
		driver->d_minphys(bp);
	}
}

int
dkdriver_open(disk, dev, flags, mode, p)
	struct dkdevice *disk;
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register struct dkdriver *driver;
	int rv;

	driver = disk->dk_driver;

	rv = (*driver->d_open)(dev, flags, mode, p);
	if (rv != 0) {
		return (-1);
	}
	return (rv);
}

int
dkdriver_close(disk, dev, flags, mode, p)
	struct dkdevice *disk;
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	register struct dkdriver *driver;
	int rv;

	driver = disk->dk_driver;

	rv = (*driver->d_close)(dev, flags, mode, p);
	if (rv != 0) {
		return (-1);
	}
	return (rv);
}

int
dkdriver_ioctl(disk, dev, cmd, data, flag, p)
	struct dkdevice *disk;
	dev_t dev;
	int cmd, flag;
	caddr_t data;
	struct proc *p;
{
	register struct dkdriver *driver;
	int rv;

	driver = disk->dk_driver;

	rv = (*driver->d_ioctl)(dev, cmd, data, flag, p);
	if (rv != 0) {
		return (-1);
	}
	return (rv);
}

int
dkdriver_dump(disk, dev)
	struct dkdevice *disk;
	dev_t dev;
{
	register struct dkdriver *driver;
	int rv;

	driver = disk->dk_driver;

	rv = (*driver->d_dump)(dev);
	if (rv != 0) {
		return (-1);
	}
	return (rv);
}

void
dkdriver_start(disk, bp, addr)
	struct dkdevice *disk;
	struct buf *bp;
	daddr_t 	addr;
{
	register struct dkdriver *driver;

	driver = disk->dk_driver;

	if (driver->d_start) {
		driver->d_start(bp, addr);
	}
}

void
dkdriver_mklabel(disk, fstype)
	struct dkdevice *disk;
	u_int8_t fstype;
{
	register struct dkdriver *driver;
	register struct  disklabel *lp;

	driver = disk->dk_driver;
	lp = disk->dk_label;

	strlcpy(lp->d_packname, "default label", sizeof(lp->d_packname));

	if (driver->d_mklabel) {
		driver->d_mklabel(disk);
	} else {
		lp->d_partitions[RAW_PART].p_fstype = fstype;
	}
	lp->d_checksum = dkcksum(lp);
}
