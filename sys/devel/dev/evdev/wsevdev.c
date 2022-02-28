/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/malloc.h>
#include <dev/misc/wscons/wsksymdef.h>
#include <dev/misc/wscons/wsksymvar.h>

#include <devel/dev/evdev/evdev.h>
#include <devel/dev/evdev/evdev_private.h>

static const struct evdev_methods default_evdev_methods = {
		.ev_open = evdev_open,
		.ev_close = evdev_close,
		.ev_event = NULL,
};

//static struct evdev_dev	*wskbd_evdev;

void
wskbd_evdev_init(sc)
	struct wskbd_softc *sc;
{
	keyboard_t			*kbd = NULL;
	struct evdev_dev 	*evdev;
	char		 		phys_loc[NAMELEN];

	/* register as evdev provider */
	evdev = evdev_alloc();
	evdev_set_name(evdev, "wskbd evdev");
	snprintf(phys_loc, NAMELEN, KEYBOARD_NAME"%d", unit);

	evdev_set_phys(evdev, phys_loc);
	evdev_set_id(evdev, BUS_VIRTUAL, 0, 0, 0);
	evdev_set_methods(evdev, kbd, &wskbd_evdev_methods);
	evdev_support_event(evdev, EV_SYN);
	evdev_support_event(evdev, EV_KEY);
	evdev_support_event(evdev, EV_LED);
	evdev_support_event(evdev, EV_REP);
	evdev_support_all_known_keys(evdev);
	evdev_support_led(evdev, LED_NUML);
	evdev_support_led(evdev, LED_CAPSL);
	evdev_support_led(evdev, LED_SCROLLL);

	if (evdev_register(evdev)) {
		evdev_free(evdev);
	} else {
		sc->sc_evdev = evdev;
	}
	sc->sc_evdev_state = 0;
}

static struct evdev_dev	*wsmouse_evdev;

void
evdev_wsmouse_init(void)
{
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
	}
}

void
evdev_wsmouse_write(x, y, z, buttons)
	int x, y, z, buttons;
{
	if (wsmouse_evdev == NULL || !(evdev_rcpt_mask & EVDEV_RCPT_SYSMOUSE))
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

void
wskbd_read_char(sc)
	struct wskbd_softc *sc;
{
	u_int		 action;
	int		 scancode, keycode;
#ifdef EVDEV_SUPPORT
	/* push evdev event */
	if ((evdev_rcpt_mask & EVDEV_RCPT_KBDMUX) && sc->sc_evdev != NULL) {
		uint16_t key = evdev_scancode2key(&sc->sc_evdev_state, scancode);

		if (key != KEY_RESERVED) {
			evdev_push_event(sc->sc_evdev, EV_KEY, key, scancode & 0x80 ? 0 : 1);
			evdev_sync(sc->sc_evdev);
		}
	}
#endif
}

int
wskbd_do_ioctl(dv, cmd, data, flag, p)
	struct device *dv;
	u_long cmd;
	caddr_t data;
	int 	flag;
	struct proc *p;
{
	struct wskbd_softc *sc = (struct wskbd_softc*) dv;
	int error;

	sc->sc_refcnt++;
#ifdef EVDEV_SUPPORT
	error = evdev_do_ioctl(sc->sc_evdev, cmd, data, fflag, p);
#else
	error = wskbd_do_ioctl_sc(sc, cmd, data, flag, p);
#endif
	if (--sc->sc_refcnt < 0)
		wakeup(sc);
	return (error);
}

void
wskbd_ioctl(u_long cmd)
{
	struct wskbd_softc *sc;
	switch (cmd) {
	case WSKBDIO_SETLED:		 /* set keyboard LED */
#ifdef EVDEV_SUPPORT
		if (sc->sc_evdev != NULL && (evdev_rcpt_mask & EVDEV_RCPT_KBDMUX)) {
			evdev_push_leds(sc->sc_evdev, *(int*) arg);
		}
#endif
		break;
	case WSKBDIO_SETKEYREPEAT:
#ifdef EVDEV_SUPPORT
		if (sc->sc_evdev != NULL && (evdev_rcpt_mask & EVDEV_RCPT_KBDMUX)) {
			evdev_push_repeats(sc->sc_evdev, ukdp);
		}
#endif
}
