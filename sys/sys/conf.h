/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)conf.h	1.3 (2.11BSD Berkeley) 12/23/92
 */
#ifndef _SYS_CONF_H_
#define _SYS_CONF_H_

#include <sys/devsw.h>

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
	int			(*d_strategy)(dev_t dev, int fflag, int devtype, struct proc *p);
	int			(*d_ioctl)(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p);
	int			(*d_root)();		/* parameters vary by architecture */
	int			(*d_dump)(dev_t dev);
	daddr_t		(*d_psize)(dev_t dev);
	int			(*d_discard)(dev_t dev, off_t pos, off_t len);
	int			d_type;
};

#ifdef KERNEL
extern struct bdevsw bdevsw[];
#endif

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
	struct tty 	(*d_tty)(dev_t dev);
	int			(*d_select)(dev_t dev, int which, struct proc *p);
	int			(*d_poll)(dev_t dev, int events, struct proc *p);
	caddr_t		(*d_mmap)(dev_t dev, off_t off, int flag);
	int			(*d_strategy)(struct buf *bp);
	int			(*d_discard)(dev_t dev, off_t pos, off_t len);
	int			d_type;
};
#ifdef KERNEL
extern struct cdevsw cdevsw[];

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
	int			(*l_rend)();
	int			(*l_meta)();
	int			(*l_start)(struct tty *tp);
	int			(*l_modem)(struct tty *tp, int flag);
	int			(*l_poll)(struct tty *tp, int flag, struct proc *p);
};
#ifdef KERNEL
extern struct 	linesw linesw[];
#endif

/*
 * Swap device table
 */
struct swdevt {
	dev_t			sw_dev;
	int				sw_flags;
	int				sw_nblks;
	struct vnode 	*sw_vp;
};

#define	SW_FREED		0x01
#define	SW_SEQUENTIAL	0x02
#define sw_freed		sw_flags	/* XXX compat */

#ifdef KERNEL
extern struct swdevt swdevt[];
#endif

/* kern_conf.c */
extern void	audio_init(struct devswtable *devsw);
extern void	console_init(struct devswtable *devsw);
extern void	disk_init(struct devswtable *devsw);
extern void	misc_init(struct devswtable *devsw);
extern void	network_init(struct devswtable *devsw);
extern void	tty_init(struct devswtable *devsw);
extern void	usb_init(struct devswtable *devsw);
extern void	video_init(struct devswtable *devsw);
extern void	wscons_init(struct devswtable *devsw);
#endif
