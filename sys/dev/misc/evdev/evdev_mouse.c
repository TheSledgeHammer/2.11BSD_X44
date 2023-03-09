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

/* TODO:
 * - Fix evdev mouse mux & ioctls
 */

#include <sys/conf.h>
#include <sys/device.h>

#include <dev/misc/wscons/wseventvar.h>
#include <dev/misc/wscons/wsmousevar.h>
#include <dev/misc/wscons/wsmuxvar.h>

#include <dev/misc/evdev/evdev_private.h>

struct evdev_mouse_softc {
	struct evdev_softc 				sc_evdev;
    const struct wsmouse_accessops  *sc_accessops;
	void							*sc_accesscookie;
};

extern struct cfdriver evdev_cd;
CFOPS_DECL(evdev_mouse, evdev_mouse_match, evdev_mouse_attach, NULL, NULL);
//CFDRIVER_DECL(NULL, evdev_mouse, DV_DULL);
CFATTACH_DECL(evdev_mouse, &evdev_cd, &evdev_mouse_cops, sizeof(struct evdev_mouse_softc));

#if NWSMUX > 0
struct wssrcops evmouse_srcops = {
		.type = WSMUX_EVMOUSE,
		.dopen = evdev_mouse_mux_open,
		.dclose = evdev_mouse_mux_close,
		.dioctl = evdev_do_ioctl,
		.ddispioctl = NULL,
		.dsetdisplay = NULL,
};
#endif

int
evdev_mouse_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct wsmousedev_attach_args 	*ap;

	return (1);
}

void
evdev_mouse_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct evdev_mouse_softc 		*msc;
	struct evdev_softc				*sc;
	struct wsmousedev_attach_args 	*ap;

	msc = (struct evdev_mouse_softc *)self;
	sc = &msc->sc_evdev;
	ap = (struct wsmousedev_attach_args)aux;

	msc->sc_accessops = ap->accessops;
	msc->sc_accesscookie = ap->accesscookie;

	evdev_attach_subr(sc, WSMOUSEDEVCF_MUX);
}

int
evdev_kbd_enable(msc)
	struct evdev_mouse_softc *msc;
{
	return ((*msc->sc_accessops->enable)(msc->sc_accesscookie));
}

int
evdev_kbd_disable(msc)
	struct evdev_mouse_softc *msc;
{
	return ((*msc->sc_accessops->disable)(msc->sc_accesscookie));
}

#if NWSMUX > 0
int
evdev_mouse_mux_open(me, evp)
	struct wsevsrc *me;
	struct wseventvar *evp;
{
	struct evdev_mouse_softc 	*msc;
	struct evdev_dev 			*evdev;
	struct evdev_client			*client;

	msc = (struct evdev_mouse_softc *)me;
	evdev = &msc->sc_evdev;
	client = evdev->ev_client;

	if (client->ec_base.me_evp != NULL) {
		return (EBUSY);
	}

	return (evdev_doopen(evdev));
}

int
evdev_mouse_mux_close(me)
	struct wsevsrc *me;
{
	struct evdev_mouse_softc 	*msc;
	struct evdev_dev 			*evdev;
	struct evdev_client			*client;

	msc = (struct evdev_mouse_softc *)me;
	evdev = &msc->sc_evdev;
	client = evdev->ev_client;

	client->ec_base.me_evp = NULL;
	(void)evdev_kbd_disable(msc);

	return (0);
}

int
evdev_mouse_add_mux(unit, muxsc)
	int unit;
	struct wsmux_softc *muxsc;
{
	struct evdev_mouse_softc 	*msc;
	struct evdev_dev 			*evdev;
	struct evdev_client			*client;

	msc = evdev_cd.cd_devs[unit];
	if (unit < 0 || unit >= evdev_cd.cd_ndevs || sc == NULL) {
		return (ENXIO);
	}

	evdev = &msc->sc_evdev;
	client = evdev->ev_client;

	if (client->ec_base.me_parent != NULL || client->ec_base.me_evp != NULL) {
		return (EBUSY);
	}
	return (wsmux_attach_sc(muxsc, &client->ec_base));
}
#endif
