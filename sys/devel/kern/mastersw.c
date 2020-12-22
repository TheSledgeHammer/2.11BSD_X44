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

struct devswio_driver *sys_devswio;

int
devswio_attach(dev, bdev, cdev, line)
	dev_t 			dev;
	struct bdevsw 	*bdev;
	struct cdevsw 	*cdev;
	struct linesw 	*line;
{
	struct devswio *dd;
	int rv, error;

	dd = &sys_devswio;
	error = devsw_io_attach(dev, bdev, cdev, line);

	if (error != 0 && dd != NULL) {
		return (error);
	}

	rv = (*dd->do_attach)(dev, bdev, cdev, line);

	return (rv);
}

int
devswio_detach(dev, bdev, cdev, line)
	dev_t 			dev;
	struct bdevsw 	*bdev;
	struct cdevsw 	*cdev;
	struct linesw 	*line;
{
	struct devswio *dd;
	int rv, error;

	dd = &sys_devswio;
	error = devsw_io_detach(dev, bdev, cdev, line);

	if(error != 0 && dd != NULL) {
		return (error);
	}

	rv = (*dd->do_detach)(dev, bdev, cdev, line);

	return (rv);
}

void
devsw_io_configure(dd, name, dev, bdev, cdev, line)
	struct devswio 			*dd;
	const char 				*name;
	dev_t 					dev;
	struct bdevsw 			*bdev;
	struct cdevsw 			*cdev;
	struct linesw 			*line;
{
	dd->do_name = name;
	devswio_attach(dev, bdev, cdev, line);
	if(dev == nodev && name == NULL) {
		devswio_detach(dev, bdev, cdev, line);
	}
}

struct devswio drive[] = {
		.do_name = "ksyms",
		.do_attach = devswio_attach(0, NULL, &ksyms_cdevsw, NULL),
		.do_detach = devswio_detach(0, NULL, &ksyms_cdevsw, NULL)
};
