/*
 * mastersw.c
 *
 *  Created on: 15 Dec 2020
 *      Author: marti
 */


#include <sys/conf.h>
#include <sys/devsw.h>
#include <master.h>
#include <sys/user.h>
#include <sys/map.h>

struct devswio_config {
	const char 				*do_name;
	struct devswio_driver 	*do_driver;
};

struct devswio_driver {
	attach_t				do_attach;
	detach_t				do_detach;
};

struct devswio_config 	sys_dwconf;

int
devswio_attach(dev, bdev, cdev, line)
	dev_t 					dev;
	struct bdevsw 			*bdev;
	struct cdevsw 			*cdev;
	struct linesw 			*line;
{
	struct devswio_driver *dwdriver;
	int rv, error;

	dwdriver = &sys_dwconf->do_driver;
	error = devsw_io_attach(dev, bdev, cdev, line);

	if(dwdriver == NULL || error != 0) {
		return (ENXIO);
	}

	rv = (*dwdriver->do_attach)(dev, bdev, cdev, line);

	return (rv);
}

int
devswio_detach(dev, bdev, cdev, line)
	dev_t 					dev;
	struct bdevsw 			*bdev;
	struct cdevsw 			*cdev;
	struct linesw 			*line;
{
	struct devswio_driver *dwdriver;
	int rv, error;

	dwdriver = &sys_dwconf->do_driver;
	error = devsw_io_detach(dev, bdev, cdev, line);

	if(dwdriver == NULL || error != 0) {
		return (ENXIO);
	}

	rv = (*dwdriver->do_detach)(dev, bdev, cdev, line);

	return (rv);
}

devswio_setup(dev, bdev, cdev, line)
	dev_t 					dev;
	struct bdevsw 			*bdev;
	struct cdevsw 			*cdev;
	struct linesw 			*line;
{
	struct devswio_config *dwconf;
	attach_t attach = devswio_attach(dev, bdev, cdev, line);
	detach_t detach = devswio_detach(dev, bdev, cdev, line);

	if(dwconf->do_driver->do_attach != attach) {

	}
}


struct devswio_config dwconf[] = {
		devsw_init("ksyms", ksyms_driver),
};

devswio_configure()
{
	struct devswio_config *dwconf = &sys_dwconf[0];
	int i;
	for(i = 0; i < MAXDEVSW; i++) {
		dwconf = sys_dwconf[i];
		if(dwconf->do_name != NULL ) {

		}
	}
}

/* Ksyms Example */
struct devswio_driver ksyms_driver = {
		.do_attach = devsw_io_attach,
		.do_detach = devsw_io_detach
};

struct devswio_devices ksyms_device = {
		.do_name = "ksyms",
		.do_driver = ksyms_driver,
};
