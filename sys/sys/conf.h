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
	int			(*d_open)(dev_t, int, int, struct proc *);
	int			(*d_close)(dev_t, int, int, struct proc *);
	void		(*d_strategy)(struct buf *);
	int			(*d_ioctl)(dev_t, u_long, caddr_t, int, struct proc *);
	int			(*d_root)(void);		/* parameters vary by architecture */
	int			(*d_dump)(dev_t, daddr_t, caddr_t, size_t);
	int		(*d_psize)(dev_t);
	int			(*d_discard)(dev_t, off_t, off_t);
	int			d_type;
};

/*
 * Character device switch.
 */
struct cdevsw {
	int			(*d_open)(dev_t, int, int, struct proc *);
	int			(*d_close)(dev_t, int, int, struct proc *);
	int			(*d_read)(dev_t, struct uio *, int);
	int			(*d_write)(dev_t, struct uio *, int);
	int			(*d_ioctl)(dev_t, u_long, caddr_t, int, struct proc *);
	int			(*d_stop)(struct tty *, int);
	int			(*d_reset)(int);	/* XXX */
	struct tty 	*(*d_tty)(dev_t);
	int			(*d_select)(dev_t, int, struct proc *);
	int			(*d_poll)(dev_t, int, struct proc *);
	caddr_t		(*d_mmap)(dev_t, off_t, int);
	void		(*d_strategy)(struct buf *);
	int			(*d_kqfilter)(dev_t, struct knote *);
	int			(*d_discard)(dev_t, off_t, off_t);
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
	int			(*l_open)(dev_t, struct tty *);
	int			(*l_close)(struct tty *, int);
	int			(*l_read)(struct tty *, struct uio *, int);
	int			(*l_write)(struct tty *, struct uio *, int);
	int			(*l_ioctl)(struct tty *, u_long, caddr_t, int, struct proc *);
	int			(*l_rint)(int, struct tty *);
	int			(*l_rend)(void);
	int			(*l_meta)(void);
	int			(*l_start)(struct tty *);
	int			(*l_modem)(struct tty *, int);
	int			(*l_poll)(struct tty *, int, struct proc *);
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
typedef int 		dev_type_open_t(dev_t, int, int, struct proc *);
typedef int 		dev_type_close_t(dev_t, int, int, struct proc *);
typedef int 		dev_type_read_t(dev_t, struct uio *, int);
typedef int 		dev_type_write_t(dev_t, struct uio *, int);
typedef int 		dev_type_ioctl_t(dev_t, u_long, caddr_t, int, struct proc *);
typedef int 		dev_type_root_t(void);
typedef int 		dev_type_start_t(struct tty *);
typedef int 		dev_type_stop_t(struct tty *, int);
typedef struct tty 	*dev_type_tty_t(dev_t);
typedef int 		dev_type_select_t(dev_t, int, struct proc *);
typedef int 		dev_type_poll_t(dev_t, int, struct proc *);
typedef caddr_t 	dev_type_mmap_t(dev_t, off_t, int);
typedef void 		dev_type_strategy_t(struct buf *);
typedef int 		dev_type_modem_t(struct tty *, int);
typedef int 		dev_type_rint_t(int, struct tty *);
typedef int 		dev_type_rend_t(void);
typedef int 		dev_type_meta_t(void);
typedef int 		dev_type_reset_t(int);
typedef int 		dev_type_kqfilter_t(dev_t, struct knote *);
typedef int 		dev_type_discard_t(dev_t, off_t, off_t);
typedef int 		dev_type_dump_t(dev_t, daddr_t, caddr_t, size_t);
typedef int 		dev_type_size_t(dev_t);
/* tty specific */
typedef int 		dev_type_tty_open_t(dev_t, struct tty *);
typedef int 		dev_type_tty_close_t(struct tty *, int);
typedef int 		dev_type_tty_read_t(struct tty *, struct uio *, int);
typedef int 		dev_type_tty_write_t(struct tty *, struct uio *, int);
typedef int 		dev_type_tty_ioctl_t(struct tty *, u_long, caddr_t, int, struct proc *);
typedef int 		dev_type_tty_poll_t(struct tty *, int, struct proc *);

#define	dev_type_open(n)	    dev_type_open_t n
#define	dev_type_close(n)	    dev_type_close_t n
#define	dev_type_read(n)	    dev_type_read_t n
#define	dev_type_write(n)	    dev_type_write_t n
#define	dev_type_ioctl(n)	    dev_type_ioctl_t n
#define dev_type_root(n)		dev_type_root_t n
#define	dev_type_start(n)       dev_type_start_t
#define	dev_type_stop(n)	    dev_type_stop_t n
#define	dev_type_tty(n)		    dev_type_tty_t n
#define	dev_type_select(n)      dev_type_select_t n
#define	dev_type_poll(n)	    dev_type_poll_t n
#define	dev_type_mmap(n)	    dev_type_mmap_t n
#define	dev_type_strategy(n)    dev_type_strategy_t n
#define	dev_type_modem(n)       dev_type_modem_t n
#define	dev_type_rint(n)        dev_type_rint_t n
#define	dev_type_rend(n)        dev_type_rend_t n
#define	dev_type_meta(n)        dev_type_meta_t n
#define dev_type_reset(n)       dev_type_reset_t n
#define	dev_type_kqfilter(n)    dev_type_kqfilter_t n
#define dev_type_discard(n)	    dev_type_discard_t n
#define	dev_type_dump(n)	    dev_type_dump_t n
#define	dev_type_size(n)	    dev_type_size_t n
/* tty specific */
#define	dev_type_tty_open(n)	dev_type_tty_open_t n
#define	dev_type_tty_close(n)	dev_type_tty_close_t n
#define	dev_type_tty_read(n)	dev_type_tty_read_t n
#define	dev_type_tty_write(n)	dev_type_tty_write_t n
#define	dev_type_tty_ioctl(n)	dev_type_tty_ioctl_t n
#define	dev_type_tty_poll(n)	dev_type_tty_poll_t n

/* bdevsw-specific types */
dev_type_open(bdev_open);
dev_type_close(bdev_close);
dev_type_ioctl(bdev_ioctl);
dev_type_strategy(bdev_strategy);
//dev_type_discard(bdev_discard);
dev_type_root(bdev_root);
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
dev_type_reset(cdev_reset);
dev_type_strategy(cdev_strategy);
//dev_type_discard(cdev_discard);
dev_type_kqfilter(cdev_kqfilter);

/* linesw-specific types */
dev_type_tty_open(line_open);
dev_type_tty_close(line_close);
dev_type_tty_read(line_read);
dev_type_tty_write(line_write);
dev_type_tty_ioctl(line_ioctl);
dev_type_rint(line_rint);
dev_type_rend(line_rend);
dev_type_meta(line_meta);
//dev_type_start(line_start);
dev_type_modem(line_modem);

/* no dev routines */
#define	noopen				((dev_type_open_t *)enodev)
#define	noclose				((dev_type_close_t *)enodev)
#define	noread				((dev_type_read_t *)enodev)
#define	nowrite				((dev_type_write_t *)enodev)
#define	noioctl				((dev_type_ioctl_t *)enodev)
#define	nostart				((dev_type_start_t *)enodev)
#define	nostop				((dev_type_stop_t *)enodev)
#define	notty				((dev_type_tty_t *)enodev)
#define	noselect			((dev_type_select_t *)enodev)
#define	nopoll				((dev_type_poll_t *)enodev)
#define	nommap				((dev_type_mmap_t *)enodev)
#define	nostrategy			((dev_type_strategy_t *)enodev)
#define	nomodem				((dev_type_modem_t *)enodev)
#define	norint				((dev_type_rint_t *)enodev)
#define	norend				((dev_type_rend_t *)enodev)
#define	nometa				((dev_type_meta_t *)enodev)
#define	nokqfilter			seltrue_kqfilter
#define	nodiscard			((dev_type_discard_t *)enodev)
#define	nodump				((dev_type_dump_t *)enodev)
#define	nosize				((dev_type_size_t *)enodev)
/* tty specific */
#define nottyopen           ((dev_type_tty_open_t *)enodev)
#define nottyclose          ((dev_type_tty_close_t *)enodev)
#define nottyread           ((dev_type_tty_read_t *)enodev)
#define nottywrite          ((dev_type_tty_write_t *)enodev)
#define nottyioctl          ((dev_type_tty_ioctl_t *)enodev)
#define nottypoll           ((dev_type_tty_poll_t *)enodev)

/* null dev routines */
#define	nullopen			((dev_type_open_t *)nullop)
#define	nullclose			((dev_type_close_t *)nullop)
#define	nullread			((dev_type_read_t *)nullop)
#define	nullwrite			((dev_type_write_t *)nullop)
#define	nullioctl			((dev_type_ioctl_t *)nullop)
#define	nullstart			((dev_type_start_t *)nullop)
#define	nullstop			((dev_type_stop_t *)nullop)
#define	nulltty				((dev_type_tty_t *)nullop)
#define	nullselect			((dev_type_select_t *)nullop)
#define	nullpoll			((dev_type_poll_t *)nullop)
#define	nullmmap			((dev_type_mmap_t *)nullop)
#define	nullstrategy		((dev_type_strategy_t *)nullop)
#define	nullrint			((dev_type_rint_t *)nullop)
#define	nullrend			((dev_type_rend_t *)nullop)
#define	nullmeta			((dev_type_meta_t *)nullop)
#define	nullkqfilter		((dev_type_kqfilter_t *)nullop)
#define	nulldiscard			((dev_type_discard_t *)nullop)
#define	nulldump			((dev_type_dump_t *)nullop)
#define	nullsize			((dev_type_size_t *)nullop)
/* tty specific */
#define nullttyopen         ((dev_type_tty_open_t *)nullop)
#define nullttyclose        ((dev_type_tty_close_t *)nullop)
#define nullttyread         ((dev_type_tty_read_t *)nullop)
#define nullttywrite        ((dev_type_tty_write_t *)nullop)
#define nullttyioctl        ((dev_type_tty_ioctl_t *)nullop)
#define nullttypoll         ((dev_type_tty_poll_t *)nullop)
#endif
#endif
