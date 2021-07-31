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

void
dk_init(dksc, dev, dtype)
	struct dksoftc *dksc;
	struct device *dev;
	int dtype;
{
	memset(dksc, 0x0, sizeof(*dksc));
	dksc->sc_dtype = dtype;
	dksc->sc_dev = dev;

	strlcpy(dksc->sc_xname, dev->d_name, DK_NAMELEN);
	dksc->sc_dkdev.dk_name = dksc->sc_xname;
}

int
dk_open(dksc, dev, flags, devtype, p)
	struct dksoftc *dksc;
	dev_t dev;
	int flags, devtype;
	struct proc *p;
{
	register struct dkdevice *disk;
	register struct dkdriver *driver;
	int unit, part, mask, mode, rv;

	disk = dksc->sc_dkdev;

	unit = dkunit(dev);
	/*
	while (disk->dk_flags & (DKF_OPENING | DKF_CLOSING)) {
		sleep(disk, PRIBIO);
	}
	*/

	if ((disk->dk_flags & DKF_ALIVE) == 0) {
		disk->dk_dev->d_type[unit] = 0;
		disk->dk_flags |= DKF_ALIVE;
	}
	if (disk->dk_openmask == 0) {
		disk->dk_flags |= DKF_OPENING;
		dkgetinfo(disk, dev);
		disk->dk_flags &= ~DKF_OPENING;
		//wakeup(disk);
	}
	disk->dk_label = LABELDESC;
	part = disk->dk_label->d_npartitions;
	if (dkpart(dev) >= part) {
		return (ENXIO);
	}
	mask = 1 << dkpart(dev);
	dkoverlapchk(disk->dk_openmask, dev, disk->dk_label, disk->dk_name);
	if (mode == S_IFCHR) {
		disk->dk_copenmask |= mask;
	} else if (mode == S_IFBLK) {
		disk->dk_bopenmask |= mask;
	} else {
		return (EINVAL);
	}
	disk->dk_openmask |= mask;

	rv = dkdriver_open(disk, dev, flags, devtype, p);
	if(rv != 0) {
		return (-1);
	}
	return (rv);
}
