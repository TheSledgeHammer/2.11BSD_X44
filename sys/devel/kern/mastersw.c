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
	int						(*do_attach)(dev_t, struct bdevsw *, struct cdevsw *, struct linesw *);
	int						(*do_detach)(dev_t, struct bdevsw *, struct cdevsw *, struct linesw *);
};

#define DEVSWIO_ATTACH(drv, dev, bdev, cdev, line) (*((drv)->do_attach))(dev, bdev, cdev, line)
#define DEVSWIO_DETACH(drv, dev, bdev, cdev, line) (*((drv)->do_detach))(dev, bdev, cdev, line)

struct devswio_config 	sys_dwconf;

struct devswio_driver ksyms = {
		.do_attach = devsw_io_attach(0, NULL, NULL, NULL),
		.do_detach = devsw_io_detach(0, NULL, NULL, NULL),
};

struct devswio_config conf = {
		{ "ksyms", ksyms }
};


devsw_configure(dev, bdev, cdev, line)
{
	register struct devswio_driver *driver;

	int rv, error;

	error = devsw_io_attach(dev, bdev, cdev, line);
	if(error != 0) {
		return (devsw_io_detach(dev, bdev, cdev, line));
	}

	rv = DEVSWIO_ATTACH(driver, dev, bdev, cdev, line);

	DEVSWIO_ATTACH(driver, dev, bdev, cdev, line);
}
