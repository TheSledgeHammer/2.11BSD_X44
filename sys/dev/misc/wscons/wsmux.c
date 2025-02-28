/*	$NetBSD: wsmux.c,v 1.35 2003/09/23 09:16:07 simonb Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Lennart Augustsson <augustss@carlstedt.se>
 *         Carlstedt Research & Technology
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * wscons mux device.
 *
 * The mux device is a collection of real mice and keyboards and acts as
 * a merge point for all the events from the different real devices.
 */

#include <sys/cdefs.h>
/*__KERNEL_RCSID(0, "$NetBSD: wsmux.c,v 1.35 2003/09/23 09:16:07 simonb Exp $"); */

#include "wsdisplay.h"
#include "wsmux.h"
#include "wskbd.h"
#include "wsmouse.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/device.h>

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wsdisplayvar.h>
#include <dev/misc/wscons/wseventvar.h>
#include <dev/misc/wscons/wscons_callbacks.h>
#include <dev/misc/wscons/wskbdvar.h>
#include <dev/misc/wscons/wsmuxvar.h>

#ifdef WSMUX_DEBUG
#define DPRINTF(x)		if (wsmuxdebug) printf x
#define DPRINTFN(n,x)	if (wsmuxdebug > (n)) printf x
int	wsmuxdebug = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

/*
 * The wsmux pseudo device is used to multiplex events from several wsmouse,
 * wskbd, and/or wsmux devices together.
 * The devices connected together form a tree with muxes in the interior
 * and real devices (mouse and kbd) at the leaves.  The special case of
 * a tree with one node (mux or other) is supported as well.
 * Only the device at the root of the tree can be opened (if a non-root
 * device is opened the subtree rooted at that point is severed from the
 * containing tree).  When the root is opened it allocates a wseventvar
 * struct which all the nodes in the tree will send their events too.
 * An ioctl() performed on the root is propagated to all the nodes.
 * There are also ioctl() operations to add and remove nodes from a tree.
 */

static int wsmux_mux_open(struct wsevsrc *, struct wseventvar *);
static int wsmux_mux_close(struct wsevsrc *);

static void wsmux_do_open(struct wsmux_softc *, struct wseventvar *);

static void wsmux_do_close(struct wsmux_softc *);
#if NWSDISPLAY > 0
static int wsmux_evsrc_set_display(struct device *, struct wsevsrc *);
#else
#define wsmux_evsrc_set_display NULL
#endif

static int wsmux_do_displayioctl(struct device *dev, u_long cmd, caddr_t data, int flag, struct proc *p);
static int wsmux_do_ioctl(struct device *, u_long, caddr_t,int,struct proc *);

static int wsmux_add_mux(int, struct wsmux_softc *);

void 		wsmuxattach(int);

#define WSMUXDEV(n) ((n) & 0x7f)
#define WSMUXCTL(n) ((n) & 0x80)

dev_type_open(wsmuxopen);
dev_type_close(wsmuxclose);
dev_type_read(wsmuxread);
dev_type_ioctl(wsmuxioctl);
dev_type_poll(wsmuxpoll);
dev_type_kqfilter(wsmuxkqfilter);

const struct cdevsw wsmux_cdevsw = {
	.d_open = wsmuxopen,
	.d_close = wsmuxclose,
	.d_read = wsmuxread,
	.d_write = nowrite,
	.d_ioctl = wsmuxioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = wsmuxpoll,
	.d_mmap = nommap,
	.d_discard = nodiscard,
	.d_type = D_OTHER
};

struct wssrcops wsmux_srcops = {
		.type = WSMUX_MUX,
		.dopen = wsmux_mux_open,
		.dclose = wsmux_mux_close,
		.dioctl = wsmux_do_ioctl,
		.ddispioctl = wsmux_do_displayioctl,
		.dsetdisplay = wsmux_evsrc_set_display
};

/* From upper level */
void
wsmuxattach(int n)
{

}

/* Keep track of all muxes that have been allocated */
static int nwsmux = 0;
static struct wsmux_softc **wsmuxdevs;

/* Return mux n, create if necessary */
struct wsmux_softc *
wsmux_getmux(int n)
{
	struct wsmux_softc *sc;
	int i;
	void *new;

	n = WSMUXDEV(n);	/* limit range */

	/* Make sure there is room for mux n in the table */
	if (n >= nwsmux) {
		i = nwsmux;
		nwsmux = n + 1;
		if (i != 0)
			new = realloc(wsmuxdevs, nwsmux * sizeof (*wsmuxdevs),
				      M_DEVBUF, M_NOWAIT);
		else
			new = malloc(nwsmux * sizeof (*wsmuxdevs),
				     M_DEVBUF, M_NOWAIT);
		if (new == NULL) {
			printf("wsmux_getmux: no memory for mux %d\n", n);
			return (NULL);
		}
		wsmuxdevs = new;
		for (; i < nwsmux; i++)
			wsmuxdevs[i] = NULL;
	}

	sc = wsmuxdevs[n];
	if (sc == NULL) {
		sc = wsmux_create("wsmux", n);
		if (sc == NULL)
			printf("wsmux: attach out of memory\n");
		wsmuxdevs[n] = sc;
	}
	return (sc);
}

/*
 * open() of the pseudo device from device table.
 */
int
wsmuxopen(dev_t dev, int flags, int mode, struct proc *p)
{
	struct wsmux_softc *sc;
	struct wseventvar *evar;
	int minr, unit;

	minr = minor(dev);
	unit = WSMUXDEV(minr);
	sc = wsmux_getmux(unit);
	if (sc == NULL)
		return (ENXIO);

	DPRINTF(("wsmuxopen: %s: sc=%p p=%p\n", sc->sc_base.me_dv.dv_xname,
		 sc, p));

	if (WSMUXCTL(minr)) {
		/* This is the control device which does not allow reads. */
		if (flags & FREAD)
			return (EINVAL);
		return (0);
	}
	if ((flags & (FREAD | FWRITE)) == FWRITE)
		/* Allow write only open */
		return (0);

	if (sc->sc_base.me_parent != NULL) {
		/* Grab the mux out of the greedy hands of the parent mux. */
		DPRINTF(("wsmuxopen: detach\n"));
		wsmux_detach_sc(&sc->sc_base);
	}

	if (sc->sc_base.me_evp != NULL)
		/* Already open. */
		return (EBUSY);

	evar = &sc->sc_base.me_evar;
	wsevent_init(evar);
	evar->io = p;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	sc->sc_rawkbd = 0;
#endif

	wsmux_do_open(sc, evar);

	return (0);
}

/*
 * Open of a mux via the parent mux.
 */
int
wsmux_mux_open(struct wsevsrc *me, struct wseventvar *evar)
{
	struct wsmux_softc *sc = (struct wsmux_softc *)me;

#ifdef DIAGNOSTIC
	if (sc->sc_base.me_evp != NULL) {
		printf("wsmux_mux_open: busy\n");
		return (EBUSY);
	}
	if (sc->sc_base.me_parent == NULL) {
		printf("wsmux_mux_open: no parent\n");
		return (EINVAL);
	}
#endif

	wsmux_do_open(sc, evar);

	return (0);
}

/* Common part of opening a mux. */
void
wsmux_do_open(struct wsmux_softc *sc, struct wseventvar *evar)
{
	struct wsevsrc *me;

	sc->sc_base.me_evp = evar; /* remember event variable, mark as open */

	/* Open all children. */
	CIRCLEQ_FOREACH(me, &sc->sc_cld, me_next) {
		DPRINTF(("wsmuxopen: %s: m=%p dev=%s\n",
						sc->sc_base.me_dv.dv_xname, me, me->me_dv.dv_xname));
#ifdef DIAGNOSTIC
		if (me->me_evp != NULL) {
			printf("wsmuxopen: dev already in use\n");
			continue;
		}
		if (me->me_parent != sc) {
			printf("wsmux_do_open: bad child=%p\n", me);
			continue;
		}
		{
			int error = wsevsrc_open(me, evar);
			if (error) {
				DPRINTF(("wsmuxopen: open failed %d\n", error));
			}
		}
#else
		/* ignore errors, failing children will not be marked open */
		(void) wsevsrc_open(me, evar);
#endif
	}
}

/*
 * close() of the pseudo device from device table.
 */
int
wsmuxclose(dev_t dev, int flags, int mode, struct proc *p)
{
	int minr = minor(dev);
	struct wsmux_softc *sc = wsmuxdevs[WSMUXDEV(minr)];
	struct wseventvar *evar = sc->sc_base.me_evp;

	if (WSMUXCTL(minr))
		/* control device */
		return (0);
	if (evar == NULL)
		/* Not open for read */
		return (0);

	wsmux_do_close(sc);
	sc->sc_base.me_evp = NULL;
	wsevent_fini(evar);
	return (0);
}

/*
 * Close of a mux via the parent mux.
 */
int
wsmux_mux_close(struct wsevsrc *me)
{
	me->me_evp = NULL;
	wsmux_do_close((struct wsmux_softc *)me);
	return (0);
}

/* Common part of closing a mux. */
void
wsmux_do_close(struct wsmux_softc *sc)
{
	struct wsevsrc *me;

	DPRINTF(("wsmuxclose: %s: sc=%p\n", sc->sc_base.me_dv.dv_xname, sc));

	/* Close all the children. */
	CIRCLEQ_FOREACH(me, &sc->sc_cld, me_next) {
		DPRINTF(("wsmuxclose %s: m=%p dev=%s\n",
			 sc->sc_base.me_dv.dv_xname, me, me->me_dv.dv_xname));
#ifdef DIAGNOSTIC
		if (me->me_parent != sc) {
			printf("wsmuxclose: bad child=%p\n", me);
			continue;
		}
#endif
		(void)wsevsrc_close(me);
		me->me_evp = NULL;
	}
}

/*
 * read() of the pseudo device from device table.
 */
int
wsmuxread(dev_t dev, struct uio *uio, int flags)
{
	int minr = minor(dev);
	struct wsmux_softc *sc = wsmuxdevs[WSMUXDEV(minr)];
	struct wseventvar *evar;
	int error;

	if (WSMUXCTL(minr)) {
		/* control device */
		return (EINVAL);
	}

	evar = sc->sc_base.me_evp;
	if (evar == NULL) {
#ifdef DIAGNOSTIC
		/* XXX can we get here? */
		printf("wsmuxread: not open\n");
#endif
		return (EINVAL);
	}

	DPRINTFN(5,("wsmuxread: %s event read evar=%p\n",
		    sc->sc_base.me_dv.dv_xname, evar));
	error = wsevent_read(evar, uio, flags);
	DPRINTFN(5,("wsmuxread: %s event read ==> error=%d\n",
		    sc->sc_base.me_dv.dv_xname, error));
	return (error);
}

/*
 * ioctl of the pseudo device from device table.
 */
int
wsmuxioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	int u = WSMUXDEV(minor(dev));

	return wsmux_do_ioctl(&wsmuxdevs[u]->sc_base.me_dv, cmd, data, flag, p);
}

/*
 * ioctl of a mux via the parent mux, continuation of wsmuxioctl().
 */
int
wsmux_do_ioctl(struct device *dv, u_long cmd, caddr_t data, int flag,
	       struct proc *p)
{
	struct wsmux_softc *sc = (struct wsmux_softc *)dv;
	struct wsevsrc *me;
	int error, ok;
	int s, put, get, n;
	struct wseventvar *evar;
	struct wscons_event *ev;
	struct timeval thistime;
	struct wsmux_device_list *l;

	DPRINTF(("wsmux_do_ioctl: %s: enter sc=%p, cmd=%08lx\n",
		 sc->sc_base.me_dv.dv_xname, sc, cmd));

	switch (cmd) {
	case WSMUXIO_INJECTEVENT:
		/* Inject an event, e.g., from moused. */
		DPRINTF(("%s: inject\n", sc->sc_base.me_dv.dv_xname));

		evar = sc->sc_base.me_evp;
		if (evar == NULL) {
			/* No event sink, so ignore it. */
			DPRINTF(("wsmux_do_ioctl: event ignored\n"));
			return (0);
		}

		s = spltty();
		get = evar->get;
		put = evar->put;
		ev = &evar->q[put];
		if (++put % WSEVENT_QSIZE == get) {
			put--;
			splx(s);
			return (ENOSPC);
		}
		if (put >= WSEVENT_QSIZE)
			put = 0;
		*ev = *(struct wscons_event *)data;
		microtime(&thistime);
		TIMEVAL_TO_TIMESPEC(&thistime, &ev->time);
		evar->put = put;
		wsevent_wakeup(evar);
		splx(s);
		return (0);
	case WSMUXIO_ADD_DEVICE:
#define d ((struct wsmux_device *)data)
		DPRINTF(("%s: add type=%d, no=%d\n", sc->sc_base.me_dv.dv_xname,
			 d->type, d->idx));
		switch (d->type) {
#if NWSMOUSE > 0
		case WSMUX_MOUSE:
			return (wsmouse_add_mux(d->idx, sc));
#endif
#if NWSKBD > 0
		case WSMUX_KBD:
			return (wskbd_add_mux(d->idx, sc));
#endif
#if NEVDEV > 0
		case WSMUX_EVDEV:
			return (evdev_add_mux(d->idx, sc));
#endif
		case WSMUX_MUX:
			return (wsmux_add_mux(d->idx, sc));
		default:
			return (EINVAL);
		}
	case WSMUXIO_REMOVE_DEVICE:
		DPRINTF(("%s: rem type=%d, no=%d\n", sc->sc_base.me_dv.dv_xname,
			 d->type, d->idx));
		/* Locate the device */
		CIRCLEQ_FOREACH(me, &sc->sc_cld, me_next) {
			if (me->me_ops->type == d->type &&
			    me->me_dv.dv_unit == d->idx) {
				DPRINTF(("wsmux_do_ioctl: detach\n"));
				wsmux_detach_sc(me);
				return (0);
			}
		}
		return (EINVAL);
#undef d

	case WSMUXIO_LIST_DEVICES:
		DPRINTF(("%s: list\n", sc->sc_base.me_dv.dv_xname));
		l = (struct wsmux_device_list *)data;
		n = 0;
		CIRCLEQ_FOREACH(me, &sc->sc_cld, me_next) {
			if (n >= WSMUX_MAXDEV)
				break;
			l->devices[n].type = me->me_ops->type;
			l->devices[n].idx = me->me_dv.dv_unit;
			n++;
		}
		l->ndevices = n;
		return (0);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data;
		DPRINTF(("wsmux_do_ioctl: save rawkbd = %d\n", sc->sc_rawkbd));
		break;
#endif
	case FIONBIO:
		DPRINTF(("%s: FIONBIO\n", sc->sc_base.me_dv.dv_xname));
		return (0);

	case FIOASYNC:
		DPRINTF(("%s: FIOASYNC\n", sc->sc_base.me_dv.dv_xname));
		evar = sc->sc_base.me_evp;
		if (evar == NULL)
			return (EINVAL);
		evar->async = *(int *)data != 0;
		return (0);
	case FIOSETOWN:
		DPRINTF(("%s: FIOSETOWN\n", sc->sc_base.me_dv.dv_xname));
		evar = sc->sc_base.me_evp;
		if (evar == NULL)
			return (EINVAL);
		if (-*(int *)data != evar->io->p_pgid
		    && *(int *)data != evar->io->p_pid)
			return (EPERM);
		return (0);
	case TIOCSPGRP:
		DPRINTF(("%s: TIOCSPGRP\n", sc->sc_base.me_dv.dv_xname));
		evar = sc->sc_base.me_evp;
		if (evar == NULL)
			return (EINVAL);
		if (*(int *)data != evar->io->p_pgid)
			return (EPERM);
		return (0);
	default:
		DPRINTF(("%s: unknown\n", sc->sc_base.me_dv.dv_xname));
		break;
	}

	if (sc->sc_base.me_evp == NULL
#if NWSDISPLAY > 0
	    && sc->sc_base.me_dispdv == NULL
#endif
	    )
		return (EACCES);

	/* Return 0 if any of the ioctl() succeeds, otherwise the last error */
	error = 0;
	ok = 0;
	CIRCLEQ_FOREACH(me, &sc->sc_cld, me_next) {
#ifdef DIAGNOSTIC
		/* XXX check evp? */
		if (me->me_parent != sc) {
			printf("wsmux_do_ioctl: bad child %p\n", me);
			continue;
		}
#endif
		error = wsevsrc_ioctl(me, cmd, data, flag, p);
		DPRINTF(("wsmux_do_ioctl: %s: me=%p dev=%s ==> %d\n",
			 sc->sc_base.me_dv.dv_xname, me, me->me_dv.dv_xname,
			 error));
		if (!error)
			ok = 1;
	}
	if (ok)
		error = 0;

	return (error);
}

/*
 * poll() of the pseudo device from device table.
 */
int
wsmuxpoll(dev_t dev, int events, struct proc *p)
{
	int minr = minor(dev);
	struct wsmux_softc *sc = wsmuxdevs[WSMUXDEV(minr)];

	if (WSMUXCTL(minr)) {
		/* control device */
		return (EINVAL);
	}

	if (sc->sc_base.me_evp == NULL) {
#ifdef DIAGNOSTIC
		printf("wsmuxpoll: not open\n");
#endif
		return (EACCES);
	}

	return (wsevent_poll(sc->sc_base.me_evp, events, p));
}

/*
 * kqfilter() of the pseudo device from device table.
 */
int
wsmuxkqfilter(dev_t dev, struct knote *kn)
{
	int minr = minor(dev);
	struct wsmux_softc *sc = wsmuxdevs[WSMUXDEV(minr)];

	if (WSMUXCTL(minr)) {
		/* control device */
		return (1);
	}

	if (sc->sc_base.me_evp == NULL) {
#ifdef DIAGNOSTIC
		printf("wsmuxkqfilter: not open\n");
#endif
		return (1);
	}

	return (wsevent_kqfilter(sc->sc_base.me_evp, kn));
}

/*
 * Add mux unit as a child to muxsc.
 */
int
wsmux_add_mux(int unit, struct wsmux_softc *muxsc)
{
	struct wsmux_softc *sc, *m;

	sc = wsmux_getmux(unit);
	if (sc == NULL)
		return (ENXIO);

	DPRINTF(("wsmux_add_mux: %s(%p) to %s(%p)\n",
		 sc->sc_base.me_dv.dv_xname, sc, muxsc->sc_base.me_dv.dv_xname,
		 muxsc));

	if (sc->sc_base.me_parent != NULL || sc->sc_base.me_evp != NULL)
		return (EBUSY);

	/* The mux we are adding must not be an ancestor of itself. */
	for (m = muxsc; m != NULL ; m = m->sc_base.me_parent)
		if (m == sc)
			return (EINVAL);

	return (wsmux_attach_sc(muxsc, &sc->sc_base));
}

/* Create a new mux softc. */
struct wsmux_softc *
wsmux_create(const char *name, int unit)
{
	struct wsmux_softc *sc;

	DPRINTF(("wsmux_create: allocating\n"));
	sc = malloc(sizeof *sc, M_DEVBUF, M_NOWAIT|M_ZERO);
	if (sc == NULL)
		return (NULL);
	CIRCLEQ_INIT(&sc->sc_cld);
	snprintf(sc->sc_base.me_dv.dv_xname, sizeof sc->sc_base.me_dv.dv_xname,
		 "%s%d", name, unit);
	sc->sc_base.me_dv.dv_unit = unit;
	sc->sc_base.me_ops = &wsmux_srcops;
	return (sc);
}

/* Attach me as a child to sc. */
int
wsmux_attach_sc(struct wsmux_softc *sc, struct wsevsrc *me)
{
	int error;

	if (sc == NULL)
		return (EINVAL);

	DPRINTF(("wsmux_attach_sc: %s(%p): type=%d\n",
		 sc->sc_base.me_dv.dv_xname, sc, me->me_ops->type));

#ifdef DIAGNOSTIC
	if (me->me_parent != NULL) {
		printf("wsmux_attach_sc: busy\n");
		return (EBUSY);
	}
#endif
	me->me_parent = sc;
	CIRCLEQ_INSERT_TAIL(&sc->sc_cld, me, me_next);

	error = 0;
#if NWSDISPLAY > 0
	if (sc->sc_base.me_dispdv != NULL) {
		/* This is a display mux, so attach the new device to it. */
		DPRINTF(("wsmux_attach_sc: %s: set display %p\n",
			 sc->sc_base.me_dv.dv_xname, sc->sc_base.me_dispdv));
		if (me->me_ops->dsetdisplay != NULL) {
			error = wsevsrc_set_display(me, &sc->sc_base);
			/* Ignore that the console already has a display. */
			if (error == EBUSY)
				error = 0;
			if (!error) {
#ifdef WSDISPLAY_COMPAT_RAWKBD
				DPRINTF(("wsmux_attach_sc: %s set rawkbd=%d\n",
					 me->me_dv.dv_xname, sc->sc_rawkbd));
				(void)wsevsrc_ioctl(me, WSKBDIO_SETMODE,
						    &sc->sc_rawkbd, 0, 0);
#endif
			}
		}
	}
#endif
	if (sc->sc_base.me_evp != NULL) {
		/* Mux is open, so open the new subdevice */
		DPRINTF(("wsmux_attach_sc: %s: calling open of %s\n",
			 sc->sc_base.me_dv.dv_xname, me->me_dv.dv_xname));
		error = wsevsrc_open(me, sc->sc_base.me_evp);
	} else {
		DPRINTF(("wsmux_attach_sc: %s not open\n",
			 sc->sc_base.me_dv.dv_xname));
	}

	if (error) {
		me->me_parent = NULL;
		CIRCLEQ_REMOVE(&sc->sc_cld, me, me_next);
	}

	DPRINTF(("wsmux_attach_sc: %s(%p) done, error=%d\n",
		 sc->sc_base.me_dv.dv_xname, sc, error));
	return (error);
}

/* Remove me from the parent. */
void
wsmux_detach_sc(struct wsevsrc *me)
{
	struct wsmux_softc *sc = me->me_parent;

	DPRINTF(("wsmux_detach_sc: %s(%p) parent=%p\n",
		 me->me_dv.dv_xname, me, sc));

#ifdef DIAGNOSTIC
	if (sc == NULL) {
		printf("wsmux_detach_sc: %s has no parent\n",
		       me->me_dv.dv_xname);
		return;
	}
#endif

#if NWSDISPLAY > 0
	if (sc->sc_base.me_dispdv != NULL) {
		if (me->me_ops->dsetdisplay != NULL)
			/* ignore error, there's nothing we can do */
			(void)wsevsrc_set_display(me, NULL);
	} else
#endif
		if (me->me_evp != NULL) {
		DPRINTF(("wsmux_detach_sc: close\n"));
		/* mux device is open, so close multiplexee */
		(void)wsevsrc_close(me);
	}

	CIRCLEQ_REMOVE(&sc->sc_cld, me, me_next);
	me->me_parent = NULL;

	DPRINTF(("wsmux_detach_sc: done sc=%p\n", sc));
}

/*
 * Display ioctl() of a mux via the parent mux.
 */
int
wsmux_do_displayioctl(struct device *dv, u_long cmd, caddr_t data, int flag,
		      struct proc *p)
{
	struct wsmux_softc *sc = (struct wsmux_softc *)dv;
	struct wsevsrc *me;
	int error, ok;

	DPRINTF(("wsmux_displayioctl: %s: sc=%p, cmd=%08lx\n",
		 sc->sc_base.me_dv.dv_xname, sc, cmd));

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (cmd == WSKBDIO_SETMODE) {
		sc->sc_rawkbd = *(int *)data;
		DPRINTF(("wsmux_displayioctl: rawkbd = %d\n", sc->sc_rawkbd));
	}
#endif

	/*
	 * Return 0 if any of the ioctl() succeeds, otherwise the last error.
	 * Return EPASSTHROUGH if no mux component accepts the ioctl.
	 */
	error = EPASSTHROUGH;
	ok = 0;
	CIRCLEQ_FOREACH(me, &sc->sc_cld, me_next) {
		DPRINTF(("wsmux_displayioctl: me=%p\n", me));
#ifdef DIAGNOSTIC
		if (me->me_parent != sc) {
			printf("wsmux_displayioctl: bad child %p\n", me);
			continue;
		}
#endif
		if (me->me_ops->ddispioctl != NULL) {
			error = wsevsrc_display_ioctl(me, cmd, data, flag, p);
			DPRINTF(("wsmux_displayioctl: me=%p dev=%s ==> %d\n",
				 me, me->me_dv.dv_xname, error));
			if (!error)
				ok = 1;
		}
	}
	if (ok)
		error = 0;

	return (error);
}

#if NWSDISPLAY > 0
/*
 * Set display of a mux via the parent mux.
 */
int
wsmux_evsrc_set_display(struct device *dv, struct wsevsrc *ame)
{
	struct wsmux_softc *muxsc = (struct wsmux_softc *)ame;
	struct wsmux_softc *sc = (struct wsmux_softc *)dv;
	struct device *displaydv = muxsc ? muxsc->sc_base.me_dispdv : NULL;

	DPRINTF(("wsmux_set_display: %s: displaydv=%p\n",
		 sc->sc_base.me_dv.dv_xname, displaydv));

	if (displaydv != NULL) {
		if (sc->sc_base.me_dispdv != NULL)
			return (EBUSY);
	} else {
		if (sc->sc_base.me_dispdv == NULL)
			return (ENXIO);
	}

	return wsmux_set_display(sc, displaydv);
}

int
wsmux_set_display(struct wsmux_softc *sc, struct device *displaydv)
{
	struct device *odisplaydv;
	struct wsevsrc *me;
	struct wsmux_softc *nsc = displaydv ? sc : NULL;
	int error, ok;

	odisplaydv = sc->sc_base.me_dispdv;
	sc->sc_base.me_dispdv = displaydv;

	if (displaydv)
		printf("%s: connecting to %s\n",
		       sc->sc_base.me_dv.dv_xname, displaydv->dv_xname);
	ok = 0;
	error = 0;
	CIRCLEQ_FOREACH(me, &sc->sc_cld,me_next) {
#ifdef DIAGNOSTIC
		if (me->me_parent != sc) {
			printf("wsmux_set_display: bad child parent %p\n", me);
			continue;
		}
#endif
		if (me->me_ops->dsetdisplay != NULL) {
			error = wsevsrc_set_display(me, &nsc->sc_base);
			DPRINTF(("wsmux_set_display: m=%p dev=%s error=%d\n",
				 me, me->me_dv.dv_xname, error));
			if (!error) {
				ok = 1;
#ifdef WSDISPLAY_COMPAT_RAWKBD
				DPRINTF(("wsmux_set_display: %s set rawkbd=%d\n",
					 me->me_dv.dv_xname, sc->sc_rawkbd));
				(void)wsevsrc_ioctl(me, WSKBDIO_SETMODE,
						    &sc->sc_rawkbd, 0, 0);
#endif
			}
		}
	}
	if (ok)
		error = 0;

	if (displaydv == NULL)
		printf("%s: disconnecting from %s\n",
		       sc->sc_base.me_dv.dv_xname, odisplaydv->dv_xname);

	return (error);
}
#endif /* NWSDISPLAY > 0 */
