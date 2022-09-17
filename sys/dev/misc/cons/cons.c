/*	$NetBSD: cons.c,v 1.52 2003/10/18 21:26:22 cdi Exp $	*/

/*
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

/*
 * Copyright (c) 1988 University of Utah.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	 $FreeBSD$
 */

#include <sys/cdefs.h>
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
#include <sys/reboot.h>

//#include <dev/misc/cons/cons.h>

#include <devel/dev/cons1.h>

dev_type_open(cnopen);
dev_type_close(cnclose);
dev_type_read(cnread);
dev_type_write(cnwrite);
dev_type_ioctl(cnioctl);
dev_type_poll(cnpoll);
dev_type_kqfilter(cnkqfilter);

const struct cdevsw cons_cdevsw = {
		.d_open = cnopen,
		.d_close = cnclose,
		.d_read = cnread,
		.d_write = cnwrite,
		.d_ioctl = cnioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_select = noselect,
		.d_poll = cnpoll,
		.d_mmap = nommap,
		.d_kqfilter = cnkqfilter,
		.d_strategy = nostrategy,
		.d_discard = nodiscard,
		.d_type = D_TTY
};

struct tty *constty = NULL;		/* virtual console output device */
struct consdev *cn_tab;			/* physical console device info */
struct vnode *cn_devvp[2];		/* vnode for underlying device. */

struct cn_device {
	SIMPLEQ_ENTRY(cn_device) 	cnd_next;
	struct consdev 				*cnd_cp;
};
#define CNDEVTAB_SIZE	4
static struct cn_device cn_devtab[CNDEVTAB_SIZE];
static SIMPLEQ_HEAD(, cn_device) cn_devlist = SIMPLEQ_HEAD_INITIALIZER(cn_devlist);

int	cons_avail_mask = 0;	/* Bit mask. Each registered low level console
				 	 	 	 * which is currently unavailable for inpit
				 	 	 	 * (i.e., if it is in graphics mode) will have
				 	 	 	 * this bit cleared.
				 	 	 	 */
static bool_t 		console_pausing;	/* pause after each line during probe */
static const char 	console_pausestr[] = "<pause; press any key to proceed to next line or '.' to end pause mode>";
static const char 	*dev_console_filename;
static const struct cdevsw *cn_redirect(dev_t *, int, int *);

void
cninit(void)
{
	struct cn_device *cnd;
	struct consdev *cp, *bestMatch;

	bestMatch = cn_tab = NULL;

	SIMPLEQ_FOREACH(cnd, &cn_devlist, cnd_next) {
		cp = cnd->cnd_cp;
		(*cp->cn_probe)(cp);
		if (cp->cn_pri != CN_DEAD && (bestMatch  == NULL || cp->cn_pri > bestMatch->cn_pri)) {
			bestMatch = cp;
		}
		if (boothowto & RB_MULTIPLE) {
			(*cp->cn_init)(cp);
			cnadd(cp);
		}
	}

	/*
	 * No console, we can handle it.
	 */
	if ((cp = bestMatch) == NULL) {
		return;
	}

	/*
	 * Turn on console.
	 */
	if ((boothowto & RB_MULTIPLE) == 0) {
		(*bestMatch->cn_init)(bestMatch);
		cnadd(bestMatch);
	}

	/*
	 * Make the best console the preferred console.
	 */
	cnselect(bestMatch);
}

void
cninit_finish(void)
{
	console_pausing = FALSE;
}

/* add a new physical console to back the virtual console */
int
cnadd(cp)
	struct consdev 	*cp;
{
	struct cn_device *cnd;
	int i;

	SIMPLEQ_FOREACH(cnd, &cn_devlist, cnd_next) {
		if (cnd->cnd_cp == cp) {
			return (0);
		}
	}
	for (i = 0; i < CNDEVTAB_SIZE; i++) {
		cnd = &cn_devtab[i];
		if (cnd->cnd_cp == NULL) {
			break;
		}
	}
	if (cnd->cnd_cp == NULL) {
		return (ENOMEM);
	}
	cnd->cnd_cp = cp;
	if (cp->cn_name[0] == '\0') {
		/* XXX: it is unclear if/where this print might output */
		printf("WARNING: console at %p has no name\n", cp);
	}
	SIMPLEQ_INSERT_TAIL(&cn_devlist, cnd, cnd_next);
	cn_tab = cp;
	if (SIMPLEQ_FIRST(&cn_devlist) == cnd) {
		ttyconsdev_select(cnd->cnd_cp->cn_name);
	}
	/* Add device to the active mask. */
	cnavailable(cp, (cp->cn_flags & CN_FLAG_NOAVAIL) == 0);

	return (0);
}

void
cnremove(cp)
	struct consdev *cp;
{
	struct cn_device *cnd;
	int i;

	SIMPLEQ_FOREACH(cnd, &cn_devlist, cnd_next)	{
		if (cnd->cnd_cp == cp) {
			continue;
		}
		if (SIMPLEQ_FIRST(&cn_devlist) == cnd) {
			ttyconsdev_select(NULL);
		}
		SIMPLEQ_REMOVE(&cn_devlist, cnd, cn_device, cnd_next);
		cnd->cnd_cp = NULL;

		/* Remove this device from available mask. */
		for (i = 0; i < CNDEVTAB_SIZE; i++) {
			if (cnd == &cn_devtab[i]) {
				cons_avail_mask &= ~(1 << i);
				break;
			}
		}
		return;
	}
}

struct consdev *
cnsearch(cp)
	struct consdev *cp;
{
	struct cn_device *cnd;

	SIMPLEQ_FOREACH(cnd, &cn_devlist, cnd_next) {
		if(cp == SIMPLEQ_FIRST(&cn_devlist)) {
			return (cp);
		}
	}
	return (NULL);
}

void
cnselect(cp)
	struct consdev *cp;
{
	struct cn_device *cnd;

	SIMPLEQ_FOREACH(cnd, &cn_devlist, cnd_next) {
		if (cnd->cnd_cp != cp) {
			continue;
		}
		if (cnd == SIMPLEQ_FIRST(&cn_devlist)) {
			return;
		}
		SIMPLEQ_REMOVE(&cn_devlist, cnd, cn_device, cnd_next);
		SIMPLEQ_INSERT_HEAD(&cn_devlist, cnd, cnd_next);
		ttyconsdev_select(cnd->cnd_cp->cn_name);
		return;
	}
}

void
cnavailable(cp, available)
	struct consdev *cp;
	int available;
{
	int i;

	for (i = 0; i < CNDEVTAB_SIZE; i++) {
		if (cn_devtab[i].cnd_cp == cp) {
			break;
		}
	}
	if (available) {
		if (i < CNDEVTAB_SIZE) {
			cons_avail_mask |= (1 << i);
		}
		cp->cn_flags &= ~CN_FLAG_NOAVAIL;
	} else {
		if (i < CNDEVTAB_SIZE) {
			cons_avail_mask &= ~(1 << i);
		}
		cp->cn_flags |= CN_FLAG_NOAVAIL;
	}
}

int
cnunavailable(void)
{
	return (cons_avail_mask == 0);
}

int
cngetc(void)
{
	struct consdev *cp;
	int c;

	cp = cnsearch(cn_tab);
	if(cp == NULL) {
		return (0);
	}
	if(!(cp->cn_flags & CN_FLAG_NODEBUG)) {
		c = cp->cn_getc(cp->cn_dev);
		if (c != -1) {
			return (c);
		}
	}
	return (-1);
}

int
cngetsn(cp, size)
	char *cp;
	int size;
{
	char *lp;
	int c, len;

	cnpollc(1);

	lp = cp;
	len = 0;
	for (;;) {
		c = cngetc();
		switch (c) {
		case '\n':
		case '\r':
			printf("\n");
			*lp++ = '\0';
			cnpollc(0);
			return (len);
		case '\b':
		case '\177':
		case '#':
			if (len) {
				--len;
				--lp;
				printf("\b \b");
			}
			continue;
		case '@':
		case 'u'&037:	/* CTRL-u */
			len = 0;
			lp = cp;
			printf("\n");
			continue;
		default:
			if (len + 1 >= size || c < ' ') {
				printf("\007");
				continue;
			}
			printf("%c", c);
			++len;
			*lp++ = c;
		}
	}
}

void
cnputc(c)
	register int c;
{
	struct consdev *cp;
	const char *cstr;

	cp = cnsearch(cn_tab);
	if (cp == NULL) {
		return;
	}
	if(!(cp->cn_flags & CN_FLAG_NODEBUG)) {
		if (c) {
			(*cp->cn_putc)(cp->cn_dev, c);
			if (c == '\n') {
				(*cp->cn_putc)(cp->cn_dev, '\r');
			}
		}
	}
	if (console_pausing && c == '\n'){
		for (cstr = console_pausestr; *cstr != '\0'; cstr++) {
			cnputc(*cstr);
		}
		if (cngetc() == '.') {
			console_pausing = FALSE;
		}
		cnputc('\r');
		for (cstr = console_pausestr; *cstr != '\0'; cstr++) {
			cnputc(' ');
		}
		cnputc('\r');
	}
}

int
cnopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	const struct cdevsw *cdev;
	struct consdev *cp;
	dev_t cndev;
	int unit;

	unit = minor(dev);
	if (unit > 1)
		return ENODEV;

	cp = cnsearch(cn_tab);
	if (cp == NULL) {
		return (0);
	}

	/*
	 * always open the 'real' console device, so we don't get nailed
	 * later.  This follows normal device semantics; they always get
	 * open() calls.
	 */
	cndev = cp->cn_dev;
	if (cndev == NODEV) {
		/*
		 * This is most likely an error in the console attach
		 * code. Panicing looks better than jumping into nowhere
		 * through cdevsw below....
		 */
		panic("cnopen: no console device");
	}
	if (dev == cndev) {
		/*
		 * This causes cnopen() to be called recursively, which
		 * is generally a bad thing.  It is often caused when
		 * dev == 0 and cn_dev has not been set, but was probably
		 * initialised to 0.
		 */
		panic("cnopen: cp->cn_dev == dev");
	}
	cdev = cdevsw_lookup(cndev);
	if (cdev == NULL) {
		return (ENXIO);
	}

	if (cn_devvp[unit] == NULLVP) {
		/* try to get a reference on its vnode, but fail silently */
		cdevvp(cndev, &cn_devvp[unit]);
	}
	return ((*cdev->d_open)(cndev, flag, mode, p));
}

int
cnclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	const struct cdevsw *cdev;
	struct consdev *cp;
	struct vnode *vp;
	int unit;

	unit = minor(dev);

	cp = cnsearch(cn_tab);
	if (cp == NULL) {
		return (0);
	}

	/*
	 * If the real console isn't otherwise open, close it.
	 * If it's otherwise open, don't close it, because that'll
	 * screw up others who have it open.
	 */
	dev = cp->cn_dev;
	cdev = cdevsw_lookup(dev);
	if (cdev == NULL) {
		return (ENXIO);
	}
	if (cn_devvp[unit] != NULLVP) {
		/* release our reference to real dev's vnode */
		vrele(cn_devvp[unit]);
		cn_devvp[unit] = NULLVP;
	}
	if (vfinddev(dev, VCHR, &vp) && vcount(vp)) {
		return (0);
	}
	return ((*cdev->d_close)(dev, flag, mode, p));
}

int
cnread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	const struct cdevsw *cdev;
	int error;

	/*
	 * If we would redirect input, punt.  This will keep strange
	 * things from happening to people who are using the real
	 * console.  Nothing should be using /dev/console for
	 * input (except a shell in single-user mode, but then,
	 * one wouldn't TIOCCONS then).
	 */
	cdev = cn_redirect(&dev, 1, &error);
	if (cdev == NULL) {
		return (error);
	}
	return ((*cdev->d_read)(dev, uio, flag));
}

int
cnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	const struct cdevsw *cdev;
	int error;

	/* Redirect output, if that's appropriate. */
	cdev = cn_redirect(&dev, 0, &error);
	if (cdev == NULL) {
		return error;
	}
	return ((*cdev->d_write)(dev, uio, flag));
}

int
cnioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	const struct cdevsw *cdev;
	int error;

	/*
	 * Superuser can always use this to wrest control of console
	 * output from the "virtual" console.
	 */
	if (cmd == TIOCCONS && constty != NULL) {
		error = suser1(p->p_ucred, (u_short*) NULL);
		if (error) {
			return (error);
		}
		constty = NULL;
		return (0);
	}

	/*
	 * Redirect the ioctl, if that's appropriate.
	 * Note that strange things can happen, if a program does
	 * ioctls on /dev/console, then the console is redirected
	 * out from under it.
	 */
	cdev = cn_redirect(&dev, 0, &error);
	if (cdev == NULL) {
		return error;
	}
	return ((*cdev->d_ioctl)(dev, cmd, data, flag, p));
}

/*ARGSUSED*/
int
cnpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	const struct cdevsw *cdev;
	int error;

	/*
	 * Redirect the poll, if that's appropriate.
	 * I don't want to think of the possible side effects
	 * of console redirection here.
	 */
	cdev = cn_redirect(&dev, 0, &error);
	if (cdev == NULL) {
		return (error);
	}
	return ((*cdev->d_poll)(dev, events, p));
}

int
cnkqfilter(dev, kn)
	dev_t dev;
	struct knote *kn;
{
	const struct cdevsw *cdev;
	int error;

	/*
	 * Redirect the kqfilter, if that's appropriate.
	 * I don't want to think of the possible side effects
	 * of console redirection here.
	 */
	cdev = cn_redirect(&dev, 0, &error);
	if (cdev == NULL) {
		return (error);
	}
	return ((*cdev->d_kqfilter)(dev, kn));
}

void
cnpollc(on)
	int on;
{
	struct consdev *cp;
	static int refcount = 0;

	cp = cnsearch(cn_tab);
	if (cp == NULL) {
		return;
	}
	if (!on) {
		--refcount;
	}
	if (refcount == 0) {
		(*cp->cn_pollc)(cp->cn_dev, on);
	}
	if (on) {
		++refcount;
	}
}

void
cnbell(pitch, period, volume)
	u_int pitch, period, volume;
{
	struct consdev *cp;
	cp = cnsearch(cn_tab);
	if (cp == NULL || cp->cn_bell == NULL) {
		return;
	}
	(*cp->cn_bell)(cp->cn_dev, pitch, period, volume);
}

void
cnflush(void)
{
	struct consdev *cp;

	cp = cnsearch(cn_tab);
	if (cp == NULL || cp->cn_flush == NULL) {
		return;
	}
	(*cp->cn_flush)(cp->cn_dev);
}

void
cnhalt(void)
{
	struct consdev *cp;

	cp = cnsearch(cn_tab);
	if (cp == NULL || cp->cn_halt == NULL) {
		return;
	}
	(*cp->cn_halt)(cp->cn_dev);
}

static const struct cdevsw *
cn_redirect(devp, is_read, error)
	dev_t *devp;
	int is_read, *error;
{
	struct consdev *cp;
	dev_t dev;

	dev = *devp;
	cp = cnsearch(cn_tab);
	/*
	 * Redirect output, if that's appropriate.
	 * If there's no real console, return ENXIO.
	 */
	*error = ENXIO;
	if (constty != NULL && minor(dev) == 0 &&
	    (cp == NULL || (cp->cn_pri != CN_REMOTE ))) {
		if (is_read) {
			*error = 0;
			return NULL;
		}
		dev = constty->t_dev;
	} else if (cp == NULL)
		return NULL;
	else
		dev = cp->cn_dev;
	*devp = dev;
	return (cdevsw_lookup(dev));
}

void
ttyconsdev_select(name)
	const char *name;
{
	dev_console_filename = name;
}
