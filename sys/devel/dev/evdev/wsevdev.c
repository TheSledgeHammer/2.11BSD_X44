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

#include <dev/misc/wscons/wsksymdef.h>
#include <dev/misc/wscons/wsksymvar.h>

#include <devel/dev/evdev/evdev.h>
#include <devel/dev/evdev/evdev_private.h>

static const struct evdev_methods kbdmux_evdev_methods = {
		.ev_event = evdev_ev_kbd_event,
};

struct wskbd_softc {
	struct evdev_dev *	ks_evdev;
	int			 		ks_evdev_state;
};

static int
evdev_kbd_init()
{
	keyboard_t			*kbd = NULL;
	struct wskbd_softc *state = NULL;
	struct evdev_dev 	*evdev;
	char		 		phys_loc[NAMELEN];

	/* register as evdev provider */
	evdev = evdev_alloc();
	evdev_set_name(evdev, "System keyboard multiplexer");
	ksnprintf(phys_loc, NAMELEN, KEYBOARD_NAME"%d", unit);

	evdev_set_phys(evdev, phys_loc);
	evdev_set_id(evdev, BUS_VIRTUAL, 0, 0, 0);
	evdev_set_methods(evdev, kbd, &kbdmux_evdev_methods);
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
		state->ks_evdev = evdev;
	}
	state->ks_evdev_state = 0;

	return (0);
}

static u_int
wskbd_read_char()
{
	struct wskbd_softc *state;
	u_int		 action;
	int		 scancode, keycode;
#ifdef EVDEV_SUPPORT
	/* push evdev event */
	if ((evdev_rcpt_mask & EVDEV_RCPT_KBDMUX) && state->ks_evdev != NULL) {
		uint16_t key = evdev_scancode2key(&state->ks_evdev_state, scancode);

		if (key != KEY_RESERVED) {
			evdev_push_event(state->ks_evdev, EV_KEY, key, scancode & 0x80 ? 0 : 1);
			evdev_sync(state->ks_evdev);
		}
	}
#endif
	return (action);
}

int
wskbd_ioctl(u_long cmd)
{
	struct wskbd_softc *state;
	switch (cmd) {
	case KDSETLED:		 /* set keyboard LED */
//#ifdef EVDEV_SUPPORT
		if (state->ks_evdev != NULL && (evdev_rcpt_mask & EVDEV_RCPT_KBDMUX))
			evdev_push_leds(state->ks_evdev, *(int*) arg);
#endif
		break;
	case KDSETREPEAT: /* set keyboard repeat rate (new interface) */
		if (state->ks_evdev != NULL && (evdev_rcpt_mask & EVDEV_RCPT_KBDMUX))
			evdev_push_repeats(state->ks_evdev, kbd);
		break;

	}
}
