/* $NetBSD: wsmouse.c,v 1.34 2003/11/28 13:19:46 drochner Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)ms.c	8.1 (Berkeley) 6/11/93
 */

/*
 * Mouse driver.
 */

#include <sys/cdefs.h>

//#include "opt_evdev.h"

#include "wsmouse.h"
#include "wsdisplay.h"
#include "wsmux.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/device.h>
#include <sys/devsw.h>
#include <sys/vnode.h>
#include <sys/callout.h>
#include <sys/malloc.h>

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wsmousevar.h>
#include <dev/misc/wscons/wseventvar.h>
#include <dev/misc/wscons/wsmuxvar.h>

#ifdef EVDEV_SUPPORT
#include <dev/misc/evdev/evdev.h>
#include <dev/misc/evdev/evdev_private.h>
#endif

#if defined(WSMUX_DEBUG) && NWSMUX > 0
#define DPRINTF(x)	if (wsmuxdebug) printf x
#define DPRINTFN(n,x)	if (wsmuxdebug > (n)) printf x
extern int wsmuxdebug;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define INVALID_X	INT_MAX
#define INVALID_Y	INT_MAX
#define INVALID_Z	INT_MAX

struct wsmouse_softc {
	struct wsevsrc					sc_base;
	struct device					sc_dv;

	const struct wsmouse_accessops 	*sc_accessops;
	void							*sc_accesscookie;

	struct wseventvar 				sc_events;	/* event queue state */

	u_int							sc_mb;		/* mouse button state */
	u_int							sc_ub;		/* user button state */
	int								sc_dx;		/* delta-x */
	int								sc_dy;		/* delta-y */
	int								sc_dz;		/* delta-z */
	int								sc_x;		/* absolute-x */
	int								sc_y;		/* absolute-y */
	int								sc_z;		/* absolute-z */

	int								sc_refcnt;
	u_char							sc_dying;	/* device is being detached */

	struct wsmouse_repeat			sc_repeat;
	int								sc_repeat_button;
	struct callout					sc_repeat_callout;
	unsigned int					sc_repeat_delay;

	int								sc_reverse_scroll;
	int								sc_horiz_scroll_dist;
	int								sc_vert_scroll_dist;

#ifdef EVDEV_SUPPORT
	struct evdev_softc				*sc_evsc;	/* pointer to evdev softc */
#endif
};

int			wsmouse_match(struct device *, struct cfdata *, void *);
void		wsmouse_attach(struct device *, struct device *, void *);
int  		wsmouse_detach(struct device *, int);
int  		wsmouse_activate(struct device *, enum devact);

static int  wsmouse_set_params(struct wsmouse_softc *, struct wsmouse_param *, size_t);
static int  wsmouse_get_params(struct wsmouse_softc *, struct wsmouse_param *, size_t);
static int  wsmouse_handle_params(struct wsmouse_softc *, struct wsmouse_parameters *, bool_t);

static int  wsmouse_do_ioctl(struct wsmouse_softc *, u_long, caddr_t, int, struct proc *);
#ifdef EVDEV_SUPPORT
void		evdev_wsmouse_init(struct evdev_softc *);
void		evdev_wsmouse_write(struct wsmouse_softc *, int, int, int, int);
#endif
#if NWSMUX > 0
static int  wsmouse_mux_open(struct wsevsrc *, struct wseventvar *);
static int  wsmouse_mux_close(struct wsevsrc *);
#endif

static int  wsmousedoioctl(struct device *, u_long, caddr_t, int, struct proc *);

static int  wsmousedoopen(struct wsmouse_softc *, struct wseventvar *);
static void wsmouse_repeat(void *v);

CFOPS_DECL(wsmouse, wsmouse_match, wsmouse_attach, wsmouse_detach, wsmouse_activate);
CFDRIVER_DECL(NULL, wsmouse, DV_DULL);
CFATTACH_DECL(wsmouse, &wsmouse_cd, &wsmouse_cops, sizeof(struct wsmouse_softc));

extern struct cfdriver wsmouse_cd;

dev_type_open(wsmouseopen);
dev_type_close(wsmouseclose);
dev_type_read(wsmouseread);
dev_type_ioctl(wsmouseioctl);
dev_type_poll(wsmousepoll);
dev_type_kqfilter(wsmousekqfilter);

const struct cdevsw wsmouse_cdevsw = {
	.d_open = wsmouseopen,
	.d_close = wsmouseclose,
	.d_read = wsmouseread,
	.d_write = nowrite,
	.d_ioctl = wsmouseioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = wsmousepoll,
	.d_mmap = nommap,
	.d_kqfilter = wsmousekqfilter,
	.d_discard = nodiscard,
	.d_type = D_OTHER
};

#if NWSMUX > 0
struct wssrcops wsmouse_srcops = {
	WSMUX_MOUSE,
	wsmouse_mux_open, wsmouse_mux_close, wsmousedoioctl, NULL, NULL
};
#endif

/*
 * Print function (for parent devices).
 */
int
wsmousedevprint(aux, pnp)
	void *aux;
	const char *pnp;
{

	if (pnp)
		printf("wsmouse at %s", pnp);
	return (UNCONF);
}

int
wsmouse_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	return (1);
}

void
wsmouse_attach(parent, self, aux)
    struct device *parent, *self;
	void *aux;
{
    struct wsmouse_softc *sc = (struct wsmouse_softc *)self;
	struct wsmousedev_attach_args *ap = aux;
#if NWSMUX > 0
	int mux, error;
#endif

	sc->sc_base.me_dv = self;
	sc->sc_accessops = ap->accessops;
	sc->sc_accesscookie = ap->accesscookie;

	/* Initialize button repeating. */
	memset(&sc->sc_repeat, 0, sizeof(sc->sc_repeat));
	sc->sc_repeat_button = -1;
	sc->sc_repeat_delay = 0;
	sc->sc_reverse_scroll = 0;
	sc->sc_horiz_scroll_dist = WSMOUSE_DEFAULT_SCROLL_DIST;
	sc->sc_vert_scroll_dist = WSMOUSE_DEFAULT_SCROLL_DIST;
	callout_init(&sc->sc_repeat_callout);
	callout_setfunc(&sc->sc_repeat_callout, wsmouse_repeat, sc);

#ifdef EVDEV_SUPPORT
	evdev_wsmouse_init(sc->sc_evsc);
#endif

#if NWSMUX > 0
	sc->sc_base.me_ops = &wsmouse_srcops;
	mux = sc->sc_base.me_dv.dv_cfdata->cf_loc[WSMOUSEDEVCF_MUX];
	if (mux >= 0) {
		error = wsmux_attach_sc(wsmux_getmux(mux), &sc->sc_base);
		if (error)
			printf(" attach error=%d", error);
		else
			printf(" mux %d", mux);
	}
#else
	if (sc->sc_base.me_dv.dv_cfdata->cf_loc[WSMOUSEDEVCF_MUX] >= 0)
		printf(" (mux ignored)");
#endif
	printf("\n");
}

#ifdef EVDEV_SUPPORT
void
evdev_wsmouse_init(sc)
	struct evdev_softc *sc;
{
	struct evdev_dev	*wsmouse_evdev;
	int i;

	wsmouse_evdev = evdev_alloc();
	evdev_set_name(wsmouse_evdev, "wsmouse evdev");
	evdev_set_phys(wsmouse_evdev, "wsmouse");
	evdev_set_id(wsmouse_evdev, BUS_VIRTUAL, 0, 0, 0);
	evdev_support_prop(wsmouse_evdev, INPUT_PROP_POINTER);
	evdev_support_event(wsmouse_evdev, EV_SYN);
	evdev_support_event(wsmouse_evdev, EV_REL);
	evdev_support_event(wsmouse_evdev, EV_KEY);
	evdev_support_rel(wsmouse_evdev, REL_X);
	evdev_support_rel(wsmouse_evdev, REL_Y);
	evdev_support_rel(wsmouse_evdev, REL_WHEEL);
	evdev_support_rel(wsmouse_evdev, REL_HWHEEL);
	for (i = 0; i < 8; i++)
		evdev_support_key(wsmouse_evdev, BTN_MOUSE + i);
	if (evdev_register(wsmouse_evdev)) {
		evdev_free(wsmouse_evdev);
		wsmouse_evdev = NULL;
	} else {
		sc->sc_evdev = wskbd_evdev;
	}
	sc->sc_evdev_state = 0;
}
#endif

int
wsmouse_activate(struct device *self, enum devact act)
{
	struct wsmouse_softc *sc = (struct wsmouse_softc *)self;

	if (act == DVACT_DEACTIVATE) {
		sc->sc_dying = 1;
	}
	return (0);
}

/*
 * Detach a mouse.  To keep track of users of the softc we keep
 * a reference count that's incremented while inside, e.g., read.
 * If the mouse is active and the reference count is > 0 (0 is the
 * normal state) we post an event and then wait for the process
 * that had the reference to wake us up again.  Then we blow away the
 * vnode and return (which will deallocate the softc).
 */
int
wsmouse_detach(struct device *self, int flags)
{
	struct wsmouse_softc *sc = (struct wsmouse_softc *)self;
	struct wseventvar *evar;
	int maj, mn;
	int s;

#if NWSMUX > 0
	/* Tell parent mux we're leaving. */
	if (sc->sc_base.me_parent != NULL) {
		DPRINTF(("wsmouse_detach:\n"));
		wsmux_detach_sc(&sc->sc_base);
	}
#endif

	/* If we're open ... */
	evar = sc->sc_base.me_evp;
	if (evar != NULL && evar->io != NULL) {
		s = spltty();
		if (--sc->sc_refcnt >= 0) {
			struct wscons_event event;

			/* Wake everyone by generating a dummy event. */
			event.type = 0;
			event.value = 0;
			if (wsevent_inject(evar, &event, 1) != 0)
				wsevent_wakeup(evar);
			/* Wait for processes to go away. */
			if (tsleep(sc, PZERO, "wsmdet", hz * 60))
				printf("wsmouse_detach: %s didn't detach\n",
				       sc->sc_base.me_dv.dv_xname);
		}
		splx(s);
	}

	/* locate the major number */
	maj = cdevsw_lookup_major(&wsmouse_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);

	return (0);
}

void
wsmouse_input(wsmousedev, btns, x, y, z, flags)
	struct device *wsmousedev;
	u_int btns;			/* 0 is up */
	int x, y, z;
	u_int flags;
{
	struct wsmouse_softc *sc = (struct wsmouse_softc *)wsmousedev;
	struct wscons_event *ev;
	struct wseventvar *evar;
	int mb, ub, d, get, put, any;

	/*
	 * Discard input if not open.
	 */
	evar = sc->sc_base.me_evp;
	if (evar == NULL)
		return;

#ifdef DIAGNOSTIC
	if (evar->q == NULL) {
		printf("wsmouse_input: evar->q=NULL\n");
		return;
	}
#endif

#if NWSMUX > 0
	DPRINTFN(5,("wsmouse_input: %s mux=%p, evar=%p\n", sc->sc_base.me_dv.dv_xname, sc->sc_base.me_parent, evar));
#endif

	sc->sc_mb = btns;
	if (!(flags & WSMOUSE_INPUT_ABSOLUTE_X))
		sc->sc_dx += x;
	if (!(flags & WSMOUSE_INPUT_ABSOLUTE_Y))
		sc->sc_dy += y;
	if (!(flags & WSMOUSE_INPUT_ABSOLUTE_Z))
		sc->sc_dz += z;

	/*
	 * We have at least one event (mouse button, delta-X, or
	 * delta-Y; possibly all three, and possibly three separate
	 * button events).  Deliver these events until we are out
	 * of changes or out of room.  As events get delivered,
	 * mark them `unchanged'.
	 */
	ub = sc->sc_ub;
	any = 0;
	get = evar->get;
	put = evar->put;
	ev = &evar->q[put];

	/* NEXT prepares to put the next event, backing off if necessary */
#define	NEXT									\
	if ((++put) % WSEVENT_QSIZE == get) {		\
		put--;									\
		goto out;								\
	}
	/* ADVANCE completes the `put' of the event */
#define	ADVANCE									\
	ev++;										\
	if (put >= WSEVENT_QSIZE) {					\
		put = 0;								\
		ev = &evar->q[0];						\
	}											\
	any = 1
	/* TIMESTAMP sets `time' field of the event to the current time */
#define TIMESTAMP								\
	do {										\
		int s;									\
		s = splhigh();							\
		TIMEVAL_TO_TIMESPEC(&time, &ev->time);	\
		splx(s);								\
	} while (0)

	if (flags & WSMOUSE_INPUT_ABSOLUTE_X) {
		if (sc->sc_x != x) {
			NEXT;
			ev->type = WSCONS_EVENT_MOUSE_ABSOLUTE_X;
			ev->value = x;
			TIMESTAMP;
			ADVANCE;
			sc->sc_x = x;
		}
	} else {
		if (sc->sc_dx) {
			NEXT;
			ev->type = WSCONS_EVENT_MOUSE_DELTA_X;
			ev->value = sc->sc_dx;
			TIMESTAMP;
			ADVANCE;
			sc->sc_dx = 0;
		}
	}
	if (flags & WSMOUSE_INPUT_ABSOLUTE_Y) {
		if (sc->sc_y != y) {
			NEXT;
			ev->type = WSCONS_EVENT_MOUSE_ABSOLUTE_Y;
			ev->value = y;
			TIMESTAMP;
			ADVANCE;
			sc->sc_y = y;
		}
	} else {
		if (sc->sc_dy) {
			NEXT;
			ev->type = WSCONS_EVENT_MOUSE_DELTA_Y;
			ev->value = sc->sc_dy;
			TIMESTAMP;
			ADVANCE;
			sc->sc_dy = 0;
		}
	}
	if (flags & WSMOUSE_INPUT_ABSOLUTE_Z) {
		if (sc->sc_z != z) {
			NEXT;
			ev->type = WSCONS_EVENT_MOUSE_ABSOLUTE_Z;
			ev->value = z;
			TIMESTAMP;
			ADVANCE;
			sc->sc_z = z;
		}
	} else {
		if (sc->sc_dz) {
			NEXT;
			ev->type = WSCONS_EVENT_MOUSE_DELTA_Z;
			ev->value = sc->sc_dz;
			TIMESTAMP;
			ADVANCE;
			sc->sc_dz = 0;
		}
	}

	mb = sc->sc_mb;
	while ((d = mb ^ ub) != 0) {
		/*
		 * Cancel button repeating if button status changed.
		 */
		if (sc->sc_repeat_button != -1) {
			KASSERT(sc->sc_repeat_button >= 0);
			KASSERT(sc->sc_repeat.wr_buttons & (1 << sc->sc_repeat_button));
			ub &= ~(1 << sc->sc_repeat_button);
			sc->sc_repeat_button = -1;
			callout_stop(&sc->sc_repeat_callout);
		}

		/*
		 * Mouse button change.  Find the first change and drop
		 * it into the event queue.
		 */
		NEXT;
		ev->value = ffs(d) - 1;

		KASSERT(ev->value >= 0);

		d = 1 << ev->value;
		ev->type = (mb & d) ? WSCONS_EVENT_MOUSE_DOWN : WSCONS_EVENT_MOUSE_UP;
		TIMESTAMP
		;
		ADVANCE
		;
		ub ^= d;

		/*
		 * Program button repeating if configured for this button.
		 */
		if ((mb & d) && (sc->sc_repeat.wr_buttons & (1 << ev->value))
				&& sc->sc_repeat.wr_delay_first > 0) {
			sc->sc_repeat_button = ev->value;
			sc->sc_repeat_delay = sc->sc_repeat.wr_delay_first;
			callout_schedule(&sc->sc_repeat_callout,
					mstohz(sc->sc_repeat_delay));
		}
	}

#ifdef EVDEV_SUPPORT
	evdev_wsmouse_write(sc, x, y, z, btns);
#endif

out:
	if (any) {
		sc->sc_ub = ub;
		evar->put = put;
		wsevent_wakeup(evar);
#if NWSMUX > 0
		DPRINTFN(5,("wsmouse_input: %s wakeup evar=%p\n",
			    sc->sc_base.me_dv.dv_xname, evar));
#endif
	}
}

void
wsmouse_precision_scroll(wsmousedev, x, y)
	struct device *wsmousedev;
	int x, y;
{
	struct wsmouse_softc *sc = (struct wsmouse_softc *)wsmousedev;
	struct wseventvar *evar;
	struct wscons_event events[2];
	int nevents = 0;

	sc =
	evar = sc->sc_base.me_evp;
	if (evar == NULL)
		return;

	if (sc->sc_reverse_scroll) {
		x = -x;
		y = -y;
	}

	x = (x * 4096) / sc->sc_horiz_scroll_dist;
	y = (y * 4096) / sc->sc_vert_scroll_dist;

	if (x != 0) {
		events[nevents].type = WSCONS_EVENT_HSCROLL;
		events[nevents].value = x;
		nevents++;
	}

	if (y != 0) {
		events[nevents].type = WSCONS_EVENT_VSCROLL;
		events[nevents].value = y;
		nevents++;
	}

	(void)wsevent_inject(evar, events, nevents);
}

static void
wsmouse_repeat(void *v)
{
	int oldspl;
	unsigned int newdelay;
	struct wsmouse_softc *sc;
	struct wscons_event events[2];

	oldspl = spltty();
	sc = (struct wsmouse_softc *)v;

	if (sc->sc_repeat_button == -1) {
		/* Race condition: a "button up" event came in when
		 * this function was already called but did not do
		 * spltty() yet. */
		splx(oldspl);
		return;
	}
	KASSERT(sc->sc_repeat_button >= 0);

	KASSERT(sc->sc_repeat.wr_buttons & (1 << sc->sc_repeat_button));

	newdelay = sc->sc_repeat_delay;

	events[0].type = WSCONS_EVENT_MOUSE_UP;
	events[0].value = sc->sc_repeat_button;
	events[1].type = WSCONS_EVENT_MOUSE_DOWN;
	events[1].value = sc->sc_repeat_button;

	if (wsevent_inject(sc->sc_base.me_evp, events, 2) == 0) {
		sc->sc_ub = 1 << sc->sc_repeat_button;

		if (newdelay - sc->sc_repeat.wr_delay_decrement <
		    sc->sc_repeat.wr_delay_minimum)
			newdelay = sc->sc_repeat.wr_delay_minimum;
		else if (newdelay > sc->sc_repeat.wr_delay_minimum)
			newdelay -= sc->sc_repeat.wr_delay_decrement;
		KASSERT(newdelay >= sc->sc_repeat.wr_delay_minimum);
		KASSERT(newdelay <= sc->sc_repeat.wr_delay_first);
	}

	/*
	 * Reprogram the repeating event.
	 */
	sc->sc_repeat_delay = newdelay;
	callout_schedule(&sc->sc_repeat_callout, mstohz(newdelay));

	splx(oldspl);
}

static int
wsmouse_set_params(sc, buf, nparams)
	struct wsmouse_softc *sc;
	struct wsmouse_param *buf;
	size_t nparams;
{
	size_t i = 0;

	for (i = 0; i < nparams; ++i) {
		switch (buf[i].key) {
		case WSMOUSECFG_REVERSE_SCROLLING:
			sc->sc_reverse_scroll = (buf[i].value != 0);
			break;
		case WSMOUSECFG_HORIZSCROLLDIST:
			sc->sc_horiz_scroll_dist = buf[i].value;
			break;
		case WSMOUSECFG_VERTSCROLLDIST:
			sc->sc_vert_scroll_dist = buf[i].value;
			break;
		}
	}
	return 0;
}

static int
wsmouse_get_params(sc, buf, nparams)
	struct wsmouse_softc *sc;
	struct wsmouse_param *buf;
	size_t nparams;
{
	size_t i = 0;

	for (i = 0; i < nparams; ++i) {
		switch (buf[i].key) {
		case WSMOUSECFG_REVERSE_SCROLLING:
			buf[i].value = sc->sc_reverse_scroll;
			break;
		case WSMOUSECFG_HORIZSCROLLDIST:
			buf[i].value = sc->sc_horiz_scroll_dist;
			break;
		case WSMOUSECFG_VERTSCROLLDIST:
			buf[i].value = sc->sc_vert_scroll_dist;
			break;
		}
	}
	return 0;
}

static int
wsmouse_handle_params(sc, upl, set)
	struct wsmouse_softc *sc;
	struct wsmouse_parameters *upl;
	bool_t set;
{
	size_t len;
	struct wsmouse_param *buf;
	int error = 0;

	if (upl->params == NULL || upl->nparams > WSMOUSECFG_MAX)
		return EINVAL;
	if (upl->nparams == 0)
		return 0;

	len = upl->nparams * sizeof(struct wsmouse_param);

	buf = (struct wsmouse_param *)malloc(len, M_DEVBUF, M_WAITOK);
	if (buf == NULL)
		return ENOMEM;
	if ((error = copyin(upl->params, buf, len)) != 0)
		goto error;

	if (set) {
		error = wsmouse_set_params(sc, buf, upl->nparams);
		if (error != 0)
			goto error;
	} else {
		error = wsmouse_get_params(sc, buf, upl->nparams);
		if (error != 0)
			goto error;
		if ((error = copyout(buf, upl->params, len)) != 0)
			goto error;
	}

error:
	free(buf, M_DEVBUF);
	return error;
}

#ifdef EVDEV_SUPPORT
void
evdev_wsmouse_write(sc, x, y, z, buttons)
	struct wsmouse_softc *sc;
	int x, y, z, buttons;
{
	struct evdev_softc 	*evsc;
	struct evdev_dev	*wsmouse_evdev;

	evsc = sc->sc_evsc;
	wsmouse_evdev = evsc->sc_evdev;

	if (wsmouse_evdev == NULL || !(evdev_rcpt_mask & EVDEV_RCPT_WSMOUSE))
		return;

	evdev_push_event(wsmouse_evdev, EV_REL, REL_X, x);
	evdev_push_event(wsmouse_evdev, EV_REL, REL_Y, y);
	switch (evdev_sysmouse_t_axis) {
	case EVDEV_SYSMOUSE_T_AXIS_PSM:
		switch (z) {
		case 1:
		case -1:
			evdev_push_rel(wsmouse_evdev, REL_WHEEL, -z);
			break;
		case 2:
		case -2:
			evdev_push_rel(wsmouse_evdev, REL_HWHEEL, z / 2);
			break;
		}
		break;
	case EVDEV_SYSMOUSE_T_AXIS_UMS:
		if (buttons & (1 << 6))
			evdev_push_rel(wsmouse_evdev, REL_HWHEEL, 1);
		else if (buttons & (1 << 5))
			evdev_push_rel(wsmouse_evdev, REL_HWHEEL, -1);
		buttons &= ~((1 << 5) | (1 << 6));
		/* PASSTHROUGH */
	case EVDEV_SYSMOUSE_T_AXIS_NONE:
	default:
		evdev_push_rel(wsmouse_evdev, REL_WHEEL, -z);
	}
	evdev_push_mouse_btn(wsmouse_evdev, buttons);
	evdev_sync(wsmouse_evdev);
}
#endif

int
wsmouseopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct wsmouse_softc *sc;
	struct wseventvar *evar;
	int error, unit;

	unit = minor(dev);
	if (unit >= wsmouse_cd.cd_ndevs ||	/* make sure it was attached */
	    (sc = wsmouse_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

#if NWSMUX > 0
	DPRINTF(("wsmouseopen: %s mux=%p p=%p\n", sc->sc_base.me_dv.dv_xname,
		 sc->sc_base.me_parent, p));
#endif

	if (sc->sc_dying)
		return (EIO);

	if ((flags & (FREAD | FWRITE)) == FWRITE)
		return (0); /* always allow open for write
		 so ioctl() is possible. */

	if (sc->sc_base.me_evp != NULL)
		return (EBUSY);

	evar = &sc->sc_base.me_evar;
	wsevent_init(evar);
	sc->sc_base.me_evp = evar;
	evar->io = p;

	error = wsmousedoopen(sc, evar);
	if (error) {
		DPRINTF(("wsmouseopen: %s open failed\n",
						sc->sc_base.me_dv.dv_xname));
		sc->sc_base.me_evp = NULL;
		wsevent_fini(evar);
	}
	return (error);
}

int
wsmouseclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct wsmouse_softc *sc = (struct wsmouse_softc*) wsmouse_cd.cd_devs[minor(
			dev)];
	struct wseventvar *evar = sc->sc_base.me_evp;

	if (evar == NULL)
		/* not open for read */
		return (0);
	sc->sc_base.me_evp = NULL;
	(*sc->sc_accessops->disable)(sc->sc_accesscookie);
	wsevent_fini(evar);

	return (0);
}

int
wsmousedoopen(sc, evp)
	struct wsmouse_softc *sc;
	struct wseventvar *evp;
{
	sc->sc_base.me_evp = evp;
	sc->sc_x = INVALID_X;
	sc->sc_y = INVALID_Y;
	sc->sc_z = INVALID_Z;

	/* enable the device, and punt if that's not possible */
	return (*sc->sc_accessops->enable)(sc->sc_accesscookie);
}

int
wsmouseread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct wsmouse_softc *sc = wsmouse_cd.cd_devs[minor(dev)];
	int error;

	if (sc->sc_dying)
		return (EIO);

#ifdef DIAGNOSTIC
	if (sc->sc_base.me_evp == NULL) {
		printf("wsmouseread: evp == NULL\n");
		return (EINVAL);
	}
#endif

	sc->sc_refcnt++;
	error = wsevent_read(sc->sc_base.me_evp, uio, flags);
	if (--sc->sc_refcnt < 0) {
		wakeup(sc);
		error = EIO;
	}
	return (error);
}

int
wsmouseioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return (wsmousedoioctl(wsmouse_cd.cd_devs[minor(dev)], cmd, data, flag, p));
}

int
wsmousedoioctl(dv, cmd, data, flag, p)
	struct device *dv;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct wsmouse_softc *sc = (struct wsmouse_softc*) dv;
	int error;

	sc->sc_refcnt++;
	error = wsmouse_do_ioctl(sc, cmd, data, flag, p);
	if (--sc->sc_refcnt < 0)
		wakeup(sc);
	return (error);
}

int
wsmouse_do_ioctl(sc, cmd, data, flag, p)
	struct wsmouse_softc *sc;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct wsmouse_repeat *wr;
	int error;

	if (sc->sc_dying)
		return (EIO);

	/*
	 * Try the generic ioctls that the wsmouse interface supports.
	 */
	switch (cmd) {
	case FIONBIO: /* we will remove this someday (soon???) */
		return (0);

	case FIOASYNC:
		if (sc->sc_base.me_evp == NULL)
			return (EINVAL);
		sc->sc_base.me_evp->async = *(int*) data != 0;
		return (0);

	case FIOSETOWN:
		if (sc->sc_base.me_evp == NULL)
			return (EINVAL);
		if (-*(int*) data != sc->sc_base.me_evp->io->p_pgid
				&& *(int*) data != sc->sc_base.me_evp->io->p_pid)
			return (EPERM);
		return (0);

	case TIOCSPGRP:
		if (sc->sc_base.me_evp == NULL)
			return (EINVAL);
		if (*(int*) data != sc->sc_base.me_evp->io->p_pgid)
			return (EPERM);
		return (0);
	}

	/*
	 * Try the wsmouse specific ioctls.
	 */
	switch (cmd) {
	case WSMOUSEIO_GETREPEAT:
		wr = (struct wsmouse_repeat *)data;
		memcpy(wr, &sc->sc_repeat, sizeof(sc->sc_repeat));
		return 0;

	case WSMOUSEIO_SETREPEAT:
		if ((flag & FWRITE) == 0)
			return EACCES;

		/* Validate input data. */
		wr = (struct wsmouse_repeat *)data;
		if (wr->wr_delay_first != 0 &&
		    (wr->wr_delay_first < wr->wr_delay_decrement ||
		     wr->wr_delay_first < wr->wr_delay_minimum ||
		     wr->wr_delay_first < wr->wr_delay_minimum +
		     wr->wr_delay_decrement))
			return EINVAL;

		/* Stop current repeating and set new data. */
		sc->sc_repeat_button = -1;
		callout_stop(&sc->sc_repeat_callout);
		memcpy(&sc->sc_repeat, wr, sizeof(sc->sc_repeat));

		return 0;

	case WSMOUSEIO_GETPARAMS:
		return wsmouse_handle_params(sc,
		    (struct wsmouse_parameters *)data, FALSE);

	case WSMOUSEIO_SETPARAMS:
		if ((flag & FWRITE) == 0)
			return EACCES;
		return wsmouse_handle_params(sc,
		    (struct wsmouse_parameters *)data, TRUE);
	}

	/*
	 * Try the mouse driver for WSMOUSEIO ioctls.  It returns -1
	 * if it didn't recognize the request.
	 */
	error = (*sc->sc_accessops->ioctl)(sc->sc_accesscookie, cmd, data, flag, p);
	return (error); /* may be EPASSTHROUGH */
}

int
wsmousepoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct wsmouse_softc *sc = wsmouse_cd.cd_devs[minor(dev)];

	if (sc->sc_base.me_evp == NULL)
		return (EINVAL);
	return (wsevent_poll(sc->sc_base.me_evp, events, p));
}

int
wsmousekqfilter(dev_t dev, struct knote *kn)
{
	struct wsmouse_softc *sc = wsmouse_cd.cd_devs[minor(dev)];

	if (sc->sc_base.me_evp == NULL)
		return (1);
	return (wsevent_kqfilter(sc->sc_base.me_evp, kn));
}

#if NWSMUX > 0
int
wsmouse_mux_open(me, evp)
	struct wsevsrc *me;
	struct wseventvar *evp;
{
	struct wsmouse_softc *sc = (struct wsmouse_softc *)me;

	if (sc->sc_base.me_evp != NULL) {
		return (EBUSY);
	}

	return wsmousedoopen(sc, evp);
}

int
wsmouse_mux_close(me)
	struct wsevsrc *me;
{
	struct wsmouse_softc *sc = (struct wsmouse_softc *)me;

	sc->sc_base.me_evp = NULL;
	(*sc->sc_accessops->disable)(sc->sc_accesscookie);

	return (0);
}

int
wsmouse_add_mux(unit, muxsc)
	int unit;
	struct wsmux_softc *muxsc;
{
	struct wsmouse_softc *sc;

	if (unit < 0 || unit >= wsmouse_cd.cd_ndevs ||
	    (sc = wsmouse_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	if (sc->sc_base.me_parent != NULL || sc->sc_base.me_evp != NULL)
		return (EBUSY);

	return (wsmux_attach_sc(muxsc, &sc->sc_base));
}
#endif /* NWSMUX > 0 */
