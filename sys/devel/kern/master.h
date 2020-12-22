/*
 * master.h
 *
 *  Created on: 15 Dec 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_KERN_MASTER_H_
#define SYS_DEVEL_KERN_MASTER_H_

int devswio_attach (dev_t, struct bdevsw *, struct cdevsw *, struct linesw *);
int devswio_detach (dev_t, struct bdevsw *, struct cdevsw *, struct linesw *);

#define	dev_decl(n,t)		__CONCAT(dev_type_,t)(__CONCAT(n,t))
#define	dev_init(n,t)		__CONCAT(n,t)

#define	dev_type_attach(n)	int n(dev_t, struct bdevsw *, struct cdevsw *, struct linesw *)
#define	dev_type_detach(n)	int n(dev_t, struct bdevsw *, struct cdevsw *, struct linesw *)

dev_type_attach(devswio_attach);
dev_type_detach(devswio_detach);

#endif /* SYS_DEVEL_KERN_MASTER_H_ */
