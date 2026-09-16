/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>
#include <sys/disk.h>
//#include <sys/disklabel.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>

#include <bootstrap.h>

#if DISK_SLICES
static int disk_makebootdev2(struct devdesc *);
#else
static int disk_makebootdev1(struct devdesc *);
#endif

void
disk_setbootdev(struct devdesc *dev, uint32_t bootdev)
{
	disk_format(bootdev, dev->d_major, dev->d_adaptor, dev->d_controller,
			dev->d_slice, dev->d_partition);
}

int
disk_makebootdev(struct devdesc *dev)
{
#if DISK_SLICES
	return (disk_makebootdev2(dev));
#else
	return (disk_makebootdev1(dev));
#endif
}

#if DISK_SLICES
static int
disk_makebootdev2(struct devdesc *dev)
{
	if (dev == NULL) {
		return (-1);
	}
	return (MAKEBOOTDEV2(dev->d_major, dev->d_slice, dev->d_unit, dev->d_partition));
}
#else
static int
disk_makebootdev1(struct devdesc *dev)
{
	if (dev == NULL) {
		return (-1);
	}
	return (MAKEBOOTDEV1(dev->d_major, dev->d_adaptor, dev->d_controller, dev->d_unit,
			dev->d_partition));
}
#endif

int
disk_device_type(uint32_t bootdev)
{
	return (B_TYPE(bootdev));
}

int
disk_device_adaptor(uint32_t bootdev)
{
#if DISK_SLICES
	int slice = B_SLICE(bootdev) - 1;
	return (B_SLICE_TO_B_ADAPTOR(slice));
#else
	return (B_ADAPTOR(bootdev) << 4);
#endif
}

int
disk_device_controller(uint32_t bootdev)
{
#if DISK_SLICES
	int slice = B_SLICE(bootdev) - 1;
	return (B_SLICE_TO_B_CONTROLLER(slice));
#else
	return (B_CONTROLLER(bootdev) - 1);
#endif
}

int
disk_device_slice(uint32_t bootdev)
{
#if DISK_SLICES
	return (B_SLICE(bootdev) - 1);
#else
	return (disk_device_adaptor(bootdev) + disk_device_controller(bootdev));
#endif
}

int
disk_device_partition(uint32_t bootdev)
{
	return (B_PARTITION(bootdev));
}

void
disk_format(uint32_t bootdev, int type, int adaptor, int controller, int slice, int partition)
{
	type = disk_device_type(bootdev);
	adaptor = disk_device_adaptor(bootdev);
	controller = disk_device_controller(bootdev);
	slice = disk_device_slice(bootdev);
	partition = disk_device_partition(bootdev);
}

/*
 * Point (dev) at an allocated device specifier for the device matching the
 * path in (devspec). If it contains an explicit device specification,
 * use that.  If not, use the default device.
 */
int
disk_getdev(struct devdesc **dev, const char *devspec, const char **path)
{
	int rv;

	/*
	 * If it looks like this is just a path and no
	 * device, go with the current device.
	 */
	if ((devspec == NULL) || (devspec[0] == '/')
			|| (strchr(devspec, ':') == NULL)) {
		rv = disk_parsedev(dev, getenv("currdev"), NULL);
		if ((rv == 0) && (path != NULL)) {
			*path = devspec;
		}
		return (rv);
	}

	/*
	 * Try to parse the device name off the beginning of the devspec
	 */
	return (disk_parsedev(dev, devspec, path));
}

char *
disk_fmtdev(struct devdesc *dev)
{
	static char buf[128]; /* XXX device length constant? */
	size_t len, buflen = sizeof(buf);

	len = snprintf(buf, buflen, "%s%d", dev->d_dev->dv_name, dev->d_unit);
	if (len > buflen)
		len = buflen;
	if (dev->d_slice > 0) {
		len += snprintf(buf + len, buflen - len, "s%d", dev->d_slice);
		if (len > buflen) {
			len = buflen;
		}
	}
	if (dev->d_adaptor > 0) {
		len += snprintf(buf + len, buflen - len, "s%d", dev->d_adaptor);
		if (len > buflen) {
			len = buflen;
		}
	}
	if (dev->d_controller > 0) {
		len += snprintf(buf + len, buflen - len, "s%d", dev->d_controller);
		if (len > buflen) {
			len = buflen;
		}
	}
	if (dev->d_partition >= 0) {
		len += snprintf(buf + len, buflen - len, "%c", dev->d_partition + 'a');
		if (len > buflen) {
			len = buflen;
		}
	}
	strlcat(buf, ":", buflen - len);
	return (buf);
}


/*
 * Point (dev) at an allocated device specifier matching the string version
 * at the beginning of (devspec).  Return a pointer to the remaining
 * text in (path).
 *
 * In all cases, the beginning of (devspec) is compared to the names
 * of known devices in the device switch, and then any following text
 * is parsed according to the rules applied to the device type.
 *
 * For disk-type devices, the syntax is:
 *
 * disk<unit>[s<slice>][<partition>]:
 *
 */
int
disk_parsedev(struct devdesc **dev, const char *devspec, const char **path)
{
	struct devdesc *idev;
	struct devsw *dv;
	int i, unit, slice, partition, err;
	char *cp;
	const char *np;

	/* minimum length check */
	if (strlen(devspec) < 2) {
		return (EINVAL);
	}

	/* look for a device that matches */
	for (i = 0, dv = NULL; devsw[i] != NULL; i++) {
		if (!strncmp(devspec, devsw[i]->dv_name, strlen(devsw[i]->dv_name))) {
			dv = devsw[i];
			break;
		}
	}

	idev = alloc(sizeof(struct devdesc));
	err = 0;
	np = (devspec + strlen(dv->dv_name));

	unit = -1;
	slice = -1;
	partition = -1;
	if (*np && (*np != ':')) {
		unit = strtol(np, &cp, 10); /* next comes the unit number */
		if (cp == np) {
			err = EUNIT;
			goto fail;
		}
		if (*cp == 's') { /* got a slice number */
			np = cp + 1;
			slice = strtol(np, &cp, 10);
			if (cp == np) {
				err = EPART;
				goto fail;
			}
		}
		if (*cp && (*cp != ':')) {
			partition = *cp - 'a'; /* get a partition number */
			if ((partition < 0) || (partition >= MAXPARTITIONS)) {
				err = EPART;
				goto fail;
			}
			cp++;
		}
	}
	if (cp == NULL) {
		err = EINVAL;
		goto fail;
	}
	if (*cp && (*cp != ':')) {
		err = EINVAL;
		goto fail;
	}

	idev->d_unit = unit;
	idev->d_slice = slice;
	idev->d_adaptor = B_SLICE_TO_B_ADAPTOR(slice);
	idev->d_controller = B_SLICE_TO_B_CONTROLLER(slice);
	idev->d_partition = partition;

	if (path != NULL) {
		*path = (*cp == 0) ? cp : cp + 1;
	}

	idev->d_dev = dv;
	idev->d_type = dv->dv_type;

	if (dev == NULL) {
		free(idev);
	} else {
		*dev = idev;
	}
	return (0);

fail:
	free(idev);
	return (err);
}

/*
 * Set currdev to suit the value being supplied in (value)
 */
int
disk_setcurrdev(struct env_var *ev, int flags, void *value)
{
	struct devdesc *ncurr;
	int rv;

	rv = disk_parsedev(&ncurr, value, NULL);
	if (rv != 0) {
		return (rv);
	}
	free(ncurr);
	env_setenv(ev->ev_name, flags | EV_NOHOOK, value, NULL, NULL);
	return (0);
}
