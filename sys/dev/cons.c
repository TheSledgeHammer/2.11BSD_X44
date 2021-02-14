/*	$OpenBSD: cons.c,v 1.29 2020/04/03 08:24:52 mpi Exp $	*/
/*	$NetBSD: cons.c,v 1.30 1996/04/08 19:57:30 jonathan Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: cons.c 1.7 92/01/21$
 *
 *	@(#)cons.c	8.2 (Berkeley) 1/12/94
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/vnode.h>
#include <sys/poll.h>

#include <dev/cons.h>

/*
dev_type_cnprobe(cnprobe);
dev_type_cninit(cninit);
dev_type_cngetc(cngetc);
dev_type_cnputc(cnputc);
dev_type_cnpollc(cnpollc);

const struct consdev cons = {
		.cn_probe = cnprobe,
		.cn_init = cninit,
		.cn_getc = cngetc,
		.cn_putc = cnputc,
		.cn_pollc = cnpollc,
		.cn_dev = CONSMAJOR,
		.cn_pri= 0,
};
*/

dev_type_open(cnopen);
dev_type_close(cnclose);
dev_type_read(cnread);
dev_type_write(cnwrite);
dev_type_stop(cnstop);
dev_type_ioctl(cnioctl);
dev_type_poll(cnpoll);

const struct cdevsw cons_cdevsw = {
		.d_open = cnopen,
		.d_close = cnclose,
		.d_read = cnread,
		.d_write = cnwrite,
		.d_ioctl = cnioctl,
		.d_stop = cnstop,
		.d_tty = notty,
		.d_select = noselect,
		.d_poll = cnpoll,
		.d_mmap = nommap,
		.d_strategy = nostrategy,
		.d_discard = nodiscard,
		.d_type = D_TTY
};

struct tty *constty = NULL;			/* virtual console output device */
struct consdev 	*cn_tab = NULL;
struct vnode *cn_devvp = NULLVP;	/* vnode for underlying device. */

void
cnprobe(cp)
	struct consdev *cp;
{
	/*
	 * Collect information about all possible consoles
	 * and find the one with highest priority.
	 */
	register struct consdev *cpp;
	for (cpp = constab; cpp->cn_probe; cpp++) {
		if (cp == cpp) {
			(*cp->cn_probe)(cp);
			if (cp->cn_pri != CN_DEAD && (cn_tab == NULL || cp->cn_pri > cn_tab->cn_pri)) {
				cn_tab = cp;
			}
		}
	}

	/*
	 * No console, we can handle it.
	 */
	if ((cp = cn_tab) == NULL) {
		return;
	}
}

void
cninit(void)
{
	struct consdev *cp;

	/* probe consdev */
	cnprobe(cp);

	/*
	 * Turn on console.
	 */
	(*cp->cn_init)(cp);
}

int
cnopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	dev_t cndev;

	if (cn_tab == NULL)
		return (0);

	/*
	 * always open the 'real' console device, so we don't get nailed
	 * later.  This follows normal device semantics; they always get
	 * open() calls.
	 */
	cndev  = cn_tab->cn_dev;
	if (cndev == NODEV) {
		return (ENXIO);
#ifdef DIAGNOSTIC
	if (cndev == dev)
		panic("cnopen: recursive");
#endif
	if (cn_devvp == NULLVP) {
		/* try to get a reference on its vnode, but fail silently */
		cdevvp(dev, &cn_devvp);
	}
	return (cdev_open(cndev, flag, mode, p));
}

int
cnclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct vnode *vp;

	if (cn_tab == NULL)
		return (0);

	/*
	 * If the real console isn't otherwise open, close it.
	 * If it's otherwise open, don't close it, because that'll
	 * screw up others who have it open.
	 */
	dev = cn_tab->cn_dev;
	if (cn_devvp != NULLVP) {
		/* release our reference to real dev's vnode */
		vrele(cn_devvp);
		cn_devvp = NULLVP;
	}
	if (vfinddev(dev, VCHR, &vp) && vcount(vp))
		return (0);
	return (cdev_close(dev, flag, mode, p));
}

int
cnread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{

	/*
	 * If we would redirect input, punt.  This will keep strange
	 * things from happening to people who are using the real
	 * console.  Nothing should be using /dev/console for
	 * input (except a shell in single-user mode, but then,
	 * one wouldn't TIOCCONS then).
	 */
	if (constty != NULL && (cn_tab == NULL || cn_tab->cn_pri != CN_REMOTE))
		return 0;
	else if (cn_tab == NULL)
		return ENXIO;

	dev = cn_tab->cn_dev;
	return (cdev_read(dev, uio, flag));
}

int
cnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{

	/*
	 * Redirect output, if that's appropriate.
	 * If there's no real console, return ENXIO.
	 */
	if (constty != NULL && (cn_tab == NULL || cn_tab->cn_pri != CN_REMOTE))
		dev = constty->t_dev;
	else if (cn_tab == NULL)
		return ENXIO;
	else
		dev = cn_tab->cn_dev;
	return (cdev_write(dev, uio, flag));
}

int
cnstop(tp, flag)
	struct tty *tp;
	int flag;
{
	return (cdev_stop(tp, flag));
}

int
cnioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error;

	/*
	 * Superuser can always use this to wrest control of console
	 * output from the "virtual" console.
	 */
	if (cmd == TIOCCONS && constty != NULL) {
		error = suser1(p->p_ucred, (u_short *) NULL);
		if (error)
			return (error);
		constty = NULL;
		return (0);
	}

	/*
	 * Redirect the ioctl, if that's appropriate.
	 * Note that strange things can happen, if a program does
	 * ioctls on /dev/console, then the console is redirected
	 * out from under it.
	 */
	if (constty != NULL && (cn_tab == NULL || cn_tab->cn_pri != CN_REMOTE))
		dev = constty->t_dev;
	else if (cn_tab == NULL)
		return ENXIO;
	else
		dev = cn_tab->cn_dev;
	return (cdev_ioctl(dev, cmd, data, flag, p));
}

/*ARGSUSED*/
int
cnpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	/*
	 * Redirect the ioctl, if that's appropriate.
	 * I don't want to think of the possible side effects
	 * of console redirection here.
	 */
	if (constty != NULL && (cn_tab == NULL || cn_tab->cn_pri != CN_REMOTE))
		dev = constty->t_dev;
	else if (cn_tab == NULL)
		return ENXIO;
	else
		dev = cn_tab->cn_dev;
	return (cdev_poll(dev, events, p));
}

int
cngetc()
{
	if (cn_tab == NULL)
		return (0);
	return ((*cn_tab->cn_getc)(cn_tab->cn_dev));
}

void
cnputc(c)
	register int c;
{

	if (cn_tab == NULL)
		return;

	if (c) {
		(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
		if (c == '\n')
			(*cn_tab->cn_putc)(cn_tab->cn_dev, '\r');
	}
}

void
cnpollc(on)
	int on;
{
	static int refcount = 0;

	if (cn_tab == NULL)
		return;
	if (!on)
		--refcount;
	if (refcount == 0)
		(*cn_tab->cn_pollc)(cn_tab->cn_dev, on);
	if (on)
		++refcount;
}

void
nullcnpollc(dev, on)
	dev_t dev;
	int on;
{

}
