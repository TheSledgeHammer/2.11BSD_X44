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

#ifdef _KERNEL

#include <sys/uio.h>
#include <sys/tty.h>
#include <sys/conf.h>

#ifndef PTY_NUNITS
#define PTY_NUNITS 4            			/* 4 units by default */
#endif

extern struct tty pt_tty[];
extern int npty;

int ptsopen(dev_t, int, int, struct proc *);
int ptsclose(dev_t, int, int, struct proc *);
int ptsread(dev_t, struct uio *, int);
int ptswrite(dev_t, struct uio *, int);
void ptsstart(struct tty *);
void ptcwakeup(struct tty *, int);
int ptcopen(dev_t, int, int, struct proc *);
int ptcclose(dev_t, int, int, struct proc *);
int ptcread(dev_t, struct uio *, int);
int ptsstop(struct tty *, int);
int ptcwrite(dev_t, struct uio *, int);
int ptyioctl(dev_t, u_long, caddr_t, int, struct proc *);
struct tty 	*ptytty(dev_t);
void ptyattach(int);
#endif

#endif /* _SYS_PTY_H_ */
