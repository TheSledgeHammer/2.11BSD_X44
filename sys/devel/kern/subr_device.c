/*
 * subr_device.c
 *
 *  Created on: 20 Mar 2021
 *      Author: marti
 */

#include <sys/device.h>

enum devclass
device_get_devclass(dev)
	struct device *dev;
{
	return (dev->dv_class);
}

struct device *
device_get_next(dev)
	struct device *dev;
{
	return (dev->dv_next);
}

struct cfdata *
device_get_cfdata(dev)
	struct device *dev;
{
	return (dev->dv_cfdata);
}

int
device_get_unit(dev)
	struct device *dev;
{
	return (dev->dv_unit);
}

char *
device_get_name(dev)
	struct device *dev;
{
	return (dev->dv_xname);
}

struct device *
device_get_parent(dev)
	struct device *dev;
{
	return (dev->dv_parent);
}

int
device_get_flags(dev)
	struct device *dev;
{
	return (dev->dv_flags);
}

void
device_set_flags(dev, flags)
	struct device *dev;
	int flags;
{
	dev->dv_flags = flags;
}

void
device_active(dev)
	struct device *dev;
{
	dev->dv_flags |= DVF_ACTIVE;
}

void
device_inactive(dev)
	struct device *dev;
{
	dev->dv_flags &= ~DVF_ACTIVE;
}
