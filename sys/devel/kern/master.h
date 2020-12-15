/*
 * master.h
 *
 *  Created on: 15 Dec 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_KERN_MASTER_H_
#define SYS_DEVEL_KERN_MASTER_H_

struct master {
	int			(*s_attach)(struct devswtable *devsw, dev_t dev, void *data);
	int			(*s_detach)(dev_t dev, void *data);
};

#define	dev_decl(n,t)			__CONCAT(dev_type_,t)(__CONCAT(n,t))
#define	dev_init(n,t)			__CONCAT(n,t)

#define	dev_type_add(n)			void n(struct devswtable *devsw, dev_t dev, void *data);
#define	dev_type_remove(n)		void n(dev_t dev, void *data);



#define	dev_type_attach(n)		int n(struct devswtable *devsw, dev_t dev, void *data)
#define	dev_type_detach(n)		int n(dev_t dev, void *data)

int devswattach(struct devswtable *devsw, dev_t dev, void *data);
int devswdetach(dev_t dev, void *data);

#define dev_type_devswattach int n(struct devswtable *devsw, dev_t dev, void *data)
#define dev_type_devswdetach int n(dev_t dev, void *data)

#define devsw_decl(n)	\
		dev_decl(n, devswattach); dev_decl(n, devswdetach);

#define devsw_init(n)	\
		dev_init(n, devswattach); dev_init(n, devswdetach);




#endif /* SYS_DEVEL_KERN_MASTER_H_ */
