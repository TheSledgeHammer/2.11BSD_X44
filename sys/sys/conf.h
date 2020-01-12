/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)conf.h	1.3 (2.11BSD Berkeley) 12/23/92
 */
#ifndef _SYS_CONF_H_
#define _SYS_CONF_H_

struct buf;
struct proc;
struct tty;
struct uio;
struct vnode;

/*
 * Types for d_type.
 */
#define	D_TAPE	1
#define	D_DISK	2
#define	D_TTY	3

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 */
struct bdevsw
{
	int	(*d_open)(dev_t dev, int oflags, int devtype, struct proc *p);
	int	(*d_close)(dev_t dev, int fflag, int devtype, struct proc *p);
	int	(*d_strategy)(dev_t dev, int fflag, int devtype, struct proc *p);
	int	(*d_ioctl)(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p);
	int	(*d_root)();		/* parameters vary by architecture */
	daddr_t	(*d_psize)(dev_t dev);
	int	d_flags;
};

#ifdef KERNEL
extern struct	bdevsw bdevsw[];
#endif

/*
 * Character device switch.
 */
struct cdevsw
{
	int	(*d_open)(dev_t dev, int oflags, int devtype, struct proc *p);
	int	(*d_close)(dev_t dev, int fflag, int devtype, struct proc *);
	int	(*d_read)(dev_t dev, struct uio *uio, int ioflag);
	int	(*d_write)(dev_t dev, struct uio *uio, int ioflag);
	int	(*d_ioctl)(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p);
	int	(*d_stop)(struct tty *tp, int rw);
	int	(*d_reset)(int uban);	/* XXX */
	struct tty *d_ttys;
	int	(*d_select)(dev_t dev, int which, struct proc *p);
	int	(*d_mmap)();
	int	(*d_strategy)(struct buf *bp);
};
#ifdef KERNEL
extern struct	cdevsw cdevsw[];
#endif

/*
 * tty line control switch.
 */
struct linesw
{
	int	(*l_open)(dev_t dev, struct tty *tp);
	int	(*l_close)(struct tty *tp, int flag);
	int	(*l_read)(struct tty *tp, struct uio *uio, int flag);
	int	(*l_write)(struct tty *tp, struct uio *uio, int flag);
	int	(*l_ioctl)(struct tty *tp, int cmd, caddr_t data, int flag, struct proc *p);
	int	(*l_rint)(int c, struct tty *tp);
	int	(*l_rend)();
	int	(*l_meta)();
	int	(*l_start)(struct tty *tp);
	int	(*l_modem)(struct tty *tp, int flag);
};
#ifdef KERNEL
extern struct	linesw linesw[];
#endif

struct swdevt {
	dev_t	sw_dev;
	int		sw_flags;
	int		sw_nblks;
	struct	vnode *sw_vp;
};
#define	SW_FREED		0x01
#define	SW_SEQUENTIAL	0x02
#define sw_freed		sw_flags	/* XXX compat */

#ifdef KERNEL
extern struct swdevt swdevt[];

int	iskmemdev __P((dev_t));
int	iszerodev __P((dev_t));
int	isdisk __P((dev_t, int));
int	chrtoblk __P((dev_t));
#endif

#endif


