/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)conf.h	1.3 (2.11BSD Berkeley) 12/23/92
 */
#ifndef _SYS_CONF_H_
#define _SYS_CONF_H_

/*
 * Types for d_type.
 */
#define	D_TAPE	1
#define	D_DISK	2
#define	D_TTY	3
#define	D_OTHER	4

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
/*
 * Block device switch
 */
struct bdevsw {
	int			(*d_open)(dev_t dev, int oflags, int devtype, struct proc *p);
	int			(*d_close)(dev_t dev, int fflag, int devtype, struct proc *p);
	int			(*d_strategy)(struct buf *bp);
	int			(*d_ioctl)(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p);
	int			(*d_root)(void);		/* parameters vary by architecture */
	int			(*d_dump)(dev_t dev);
	daddr_t		(*d_psize)(dev_t dev);
	int			(*d_discard)(dev_t dev, off_t pos, off_t len);
	int			d_type;
};

/*
 * Character device switch.
 */
struct cdevsw {
	int			(*d_open)(dev_t dev, int oflags, int devtype, struct proc *p);
	int			(*d_close)(dev_t dev, int fflag, int devtype, struct proc *p);
	int			(*d_read)(dev_t dev, struct uio *uio, int ioflag);
	int			(*d_write)(dev_t dev, struct uio *uio, int ioflag);
	int			(*d_ioctl)(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p);
	int			(*d_stop)(struct tty *tp, int rw);
	int			(*d_reset)(int uban);	/* XXX */
	struct tty 	*(*d_tty)(dev_t dev);
	int			(*d_select)(dev_t dev, int which, struct proc *p);
	int			(*d_poll)(dev_t dev, int events, struct proc *p);
	caddr_t		(*d_mmap)(dev_t dev, off_t off, int flag);
	int			(*d_strategy)(struct buf *bp);
	int			(*d_kqfilter)(dev_t, struct knote *);
	int			(*d_discard)(dev_t dev, off_t pos, off_t len);
	int			d_type;
};
#ifdef _KERNEL
/* symbolic sleep message strings */
extern char devopn[], devio[], devwait[], devin[], devout[];
extern char devioc[], devcls[];
#endif

/*
 * tty line control switch.
 */
struct linesw {
	int			(*l_open)(dev_t dev, struct tty *tp);
	int			(*l_close)(struct tty *tp, int flag);
	int			(*l_read)(struct tty *tp, struct uio *uio, int flag);
	int			(*l_write)(struct tty *tp, struct uio *uio, int flag);
	int			(*l_ioctl)(struct tty *tp, int cmd, caddr_t data, int flag, struct proc *p);
	int			(*l_rint)(int c, struct tty *tp);
	int			(*l_rend)(void);
	int			(*l_meta)(void);
	int			(*l_start)(struct tty *tp);
	int			(*l_modem)(struct tty *tp, int flag);
	int			(*l_poll)(struct tty *tp, int flag, struct proc *p);
};

/*
 * Swap device table
 */
struct swdevt {
	dev_t			sw_dev;					/* device id */
	int				sw_flags;				/* flags */
	int				sw_nblks;				/* total blocks */
	int				sw_inuse;				/* blocks in use */
	struct vnode 	*sw_vp;					/* swap vnode */
	struct swapdev 	*sw_swapdev;			/* swapdrum device */
	char			sw_path[PATH_MAX+1]; 	/* path name */
};

#define	SW_FREED		0x01
#define	SW_SEQUENTIAL	0x02
#define sw_freed		sw_flags	/* XXX compat */

#ifdef _KERNEL
extern struct swdevt swdevt[];

/* dev types */
#define	dev_type_open(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_close(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_read(n)		int n(dev_t, struct uio *, int)
#define	dev_type_write(n)		int n(dev_t, struct uio *, int)
#define	dev_type_ioctl(n) 		int n(dev_t, u_long, caddr_t, int, struct proc *)
#define dev_type_start(n)		int n(struct tty *)
#define	dev_type_stop(n)		int n(struct tty *, int)
#define	dev_type_tty(n)			struct tty *n(dev_t)
#define dev_type_select(n)		int n(dev_t, int, struct proc *)
#define	dev_type_poll(n)		int n(dev_t, int, struct proc *)
#define	dev_type_mmap(n)		caddr_t n(dev_t, off_t, int)
#define	dev_type_strategy(n)	int n(struct buf *)
#define dev_type_modem(n)		int n(struct tty *, int)
#define dev_type_rint(n)		int n(int, struct tty *)
#define dev_type_kqfilter(n)	int n(dev_t, struct knote *)
#define dev_type_discard(n)		int n(dev_t, off_t, off_t)
#define	dev_type_dump(n)		int n(dev_t)
#define	dev_type_size(n)		daddr_t n(dev_t)

/* bdevsw-specific types */
dev_type_open(bdev_open);
dev_type_close(bdev_close);
dev_type_strategy(bdev_strategy);
//dev_type_ioctl(bdev_ioctl);
dev_type_dump(bdev_dump);
dev_type_size(bdev_size);
//dev_type_discard(bdev_discard);

/* cdevsw-specific types */
dev_type_open(cdev_open);
dev_type_close(cdev_close);
dev_type_read(cdev_read);
dev_type_write(cdev_write);
//dev_type_ioctl(cdev_ioctl);
dev_type_stop(cdev_stop);
dev_type_tty(cdev_tty);
dev_type_select(cdev_select);
dev_type_poll(cdev_poll);
dev_type_mmap(cdev_mmap);
dev_type_strategy(cdev_strategy);
dev_type_kqfilter(cdev_kqfilter);
//dev_type_discard(cdev_discard);

/* linesw-specific types */
//dev_type_open(line_open);
//dev_type_close(line_close);
//dev_type_read(line_read);
//dev_type_write(line_write);
//dev_type_ioctl(line_ioctl);
dev_type_rint(line_rint);
dev_type_start(line_start);
dev_type_modem(line_modem);
//dev_type_poll(line_poll);

/* no dev routines */
#define	noopen				(enodev)
#define	noclose				(enodev)
#define	noread				(enodev)
#define	nowrite				(enodev)
#define	noioctl				(enodev)
#define	nostart				(enodev)
#define	nostop				(enodev)
#define	notty				(enodev)
#define	noselect			(enodev)
#define	nopoll				(enodev)
#define	nommap				(enodev)
#define	nostrategy			(enodev)
#define	nomodem				(enodev)
#define	norint				(enodev)
#define	nokqfilter			seltrue_kqfilter
#define	nodiscard			(enodev)
#define	nodump				(enodev)
#define	nosize				(enodev)

/* null dev routines */
#define	nullopen			(nullop)
#define	nullclose			(nullop)
#define	nullread			(nullop)
#define	nullwrite			(nullop)
#define	nullioctl			(nullop)
#define	nullstart			(nullop)
#define	nullstop			(nullop)
#define	nulltty				(nullop)
#define	nullselect			(nullop)
#define	nullpoll			(nullop)
#define	nullmmap			(nullop)
#define	nullstrategy		(nullop)
#define	nullrint			(nullop)
#define	nullkqfilter		(nullop)
#define	nulldiscard			(nullop)
#define	nulldump			(nullop)
#define	nullsize			(nullop)
#endif
#endif
