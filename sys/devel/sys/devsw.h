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

#ifndef _SYS_DEVSW_H_
#define _SYS_DEVSW_H_

#include <sys/conf.h>
#include <sys/queue.h>

/*
 * device switch table
 */
struct devswtable {
    void                			*dv_data;	/* device data */
    dev_t							dv_major;	/* device major */
    struct devswops    				*dv_ops;	/* device switch operations */
};

struct devswtable_head;
TAILQ_HEAD(devswtable_head, devswtable_entry);
struct devswtable_entry {
	TAILQ_ENTRY(devswtable_entry)  	dve_link;
	struct devswtable				*dve_devswtable;
};
typedef struct devswtable_entry		*devswtable_entry_t;

struct devswops {
    int     (*dvop_attach)(struct devswtable *, const char *, struct device *, dev_t);
    int     (*dvop_detach)(struct devswtable *, const char *, struct device *, dev_t);
};

#define DVOP_ATTACH(dv, name, device, major)	(*((dv)->dv_ops->dvop_attach))(dv, name, device, major)
#define DVOP_DETACH(dv, name, device, major)	(*((dv)->dv_ops->dvop_detach))(dv, name, device, major)

struct dvop_attach_args {
    struct devswtable   d_devswtable;
    const char      	*d_name;
    struct device   	*d_device;
    dev_t				d_major;
};

struct dvop_detach_args {
    struct devswtable 	d_devswtable;
    const char      	*d_name;
    struct device   	*d_device;
    dev_t				d_major;
};


#define	MAXDEVSW		512	/* the maximum of major device number */
#define	BDEVSW_SIZE		(sizeof(struct bdevsw *))
#define	CDEVSW_SIZE		(sizeof(struct cdevsw *))
#define	LINESW_SIZE		(sizeof(struct linesw *))

/* devswtable types */
#define BDEVTYPE 		0x01
#define CDEVTYPE 		0x02
#define LINETYPE 		0x04

/* conversion macros */
#define DTOB(dv)  		((struct bdevsw *)(dv)->dv_data)
#define DTOC(dv)  		((struct cdevsw *)(dv)->dv_data)
#define DTOL(dv)  		((struct linesw *)(dv)->dv_data)

//#ifdef _KERNEL
struct bdevsw;
struct cdevsw;
struct linesw;

extern struct devswops 	dvops;	/* device switch table operations */
//#endif

#define	dev_type_open(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_close(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_read(n)		int n(dev_t, struct uio *, int)
#define	dev_type_write(n)		int n(dev_t, struct uio *, int)
#define	dev_type_ioctl(n) 		int n(dev_t, u_long, caddr_t, int, struct proc *)
#define dev_type_start(n)		int n(struct tty *)
#define	dev_type_stop(n)		int n(struct tty *, int)
#define	dev_type_tty(n)			struct tty * n(dev_t)
#define dev_type_select(n)		int n(dev_t, int, struct proc *)
#define	dev_type_poll(n)		int n(dev_t, int, struct proc *)
#define	dev_type_mmap(n)		caddr_t n(dev_t, off_t, int)
#define	dev_type_strategy(n)	int n(dev_t, int, int, struct proc *)
#define dev_type_modem(n)		int n(struct tty *, int)
#define dev_type_rint(n)		int n(int, struct tty *)
#define	dev_type_dump(n)		int n(dev_t)
#define	dev_type_size(n)		daddr_t n(dev_t)

/* bdevsw-specific types */
dev_type_open(bdev_open);
dev_type_close(bdev_close);
dev_type_strategy(bdev_strategy);
dev_type_ioctl(bdev_ioctl);
dev_type_dump(bdev_dump);
dev_type_size(bdev_size);

/* cdevsw-specific types */
dev_type_open(cdev_open);
dev_type_close(cdev_close);
dev_type_read(cdev_read);
dev_type_write(cdev_write);
dev_type_ioctl(cdev_ioctl);
dev_type_stop(cdev_stop);
dev_type_tty(cdev_tty);
dev_type_select(cdev_select);
dev_type_poll(cdev_poll);
dev_type_mmap(cdev_mmap);
dev_type_strategy(cdev_strategy);

/* linesw-specific types */
dev_type_open(line_open);
dev_type_close(line_close);
dev_type_read(line_read);
dev_type_write(line_write);
dev_type_ioctl(line_ioctl);
dev_type_rint(line_rint);
dev_type_start(line_start);
dev_type_modem(line_modem);
dev_type_poll(line_poll);

#endif /* _SYS_DEVSW_H_ */
