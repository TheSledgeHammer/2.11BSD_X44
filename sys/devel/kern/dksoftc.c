/*
 * dksoftc.c
 *
 *  Created on: 7 Nov 2021
 *      Author: marti
 */
#include <sys/disk.h>
#include <sys/null.h>
#include <sys/malloc.h>
#include <sys/queue.h>

struct dksoftc {
	struct dkdevice 	sc_dkdev;
	struct dkdriver		sc_dkdriver;
	dev_t				sc_dev;
	char				*sc_xname;
	struct disklabel 	*sc_label;
	struct diskslices 	*sc_slices;
	struct diskslice 	*sc_slice;


	struct lock_object 	sc_iolock;
};

void
dk_init(dksc, dkdev)
	struct dksoftc 	*dksc;
	struct dkdevice *dkdev;
{
	memset(dksc, 0x0, sizeof(*dksc));
	dksc->sc_dkdev = dkdev;

	strlcpy(dksc->sc_xname, dkdev->dk_name, DK_XNAME_SIZE);
	dksc->sc_dkdev.dk_name = dksc->sc_xname;
}

dk_dev()
{

}

dk_open(dksc)
	struct dksoftc *dksc;
{

}

dk_close(dksc)
	struct dksoftc *dksc;
{

}

dk_ioctl()
{

}

char *
disk_name(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_name);
}

struct dkdriver *
disk_driver(disk, dev)
	struct dkdevice *disk;
	dev_t 			dev;
{
	return (disk[dkunit(dev)].dk_driver);
}

