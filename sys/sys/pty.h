/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)pty.h	1.3 (2.11BSD GTE) 1997/5/2
 */

#ifndef _SYS_PTY_H_
#define _SYS_PTY_H_

#define PF_RCOLL    0x01
#define PF_WCOLL    0x02
#define PF_PKT      0x08        /* packet mode */
#define PF_STOPPED  0x10        /* user told stopped */
#define PF_REMOTE   0x20        /* remote and flow controlled input */
#define PF_NOSTOP   0x40
#define PF_UCNTL    0x80        /* user control mode */

#ifdef KERNEL

#include <sys/uio.h>
#include <sys/tty.h>
#include <sys/conf.h>

#ifndef PTY_NUNITS
#define PTY_NUNITS 4            			/* 4 units by default */
#endif

extern struct tty pt_tty[];
extern int npty;

extern int ptsopen(dev_t dev, int flag, int mode);
extern int ptsclose(dev_t dev, int flag, int mode);
extern int ptsread(dev_t dev, register struct uio *uio, int flag);
extern int ptswrite(dev_t dev, register struct uio *uio, int flag);
extern void ptsstart(struct tty *tp);
extern void ptcwakeup(struct tty *tp, int flag);
extern int ptcopen(dev_t dev, int flag, int mode);
extern int ptcclose(dev_t dev, int flag, int mode);
extern int ptcread(dev_t dev, register struct uio *uio, int flag);
extern void ptsstop(register struct tty *tp, int flush);
extern int ptcselect(dev_t dev, int rw);
extern int ptcwrite(dev_t dev, register struct uio *uio, int flag);
extern int ptyioctl(dev_t dev, u_int cmd, caddr_t data, int flag);

#endif

#endif /* _SYS_PTY_H_ */
