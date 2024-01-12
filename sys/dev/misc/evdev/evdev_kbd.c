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

#include "opt_evdev.h"

#include "evkbd.h"
#include "wsmux.h"

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/misc/wscons/wskbdvar.h>

#include <dev/misc/evdev/evdev_private.h>
#include <dev/misc/evdev/evdev.h>

struct evdev_kbd_softc {
	struct evdev_softc 				sc;
    const struct wskbd_accessops  	*sc_accessops;
	void							*sc_accesscookie;
};

int evdev_kbd_match(struct device *, struct cfdata *, void *);
void evdev_kbd_attach(struct device *, struct device *, void *);

extern struct cfdriver evdev_cd;
CFOPS_DECL(evdev_kbd, evdev_kbd_match, evdev_kbd_attach, NULL, NULL);
CFATTACH_DECL(evdev_kbd, &evdev_cd, &evdev_kbd_cops, sizeof(struct evdev_kbd_softc));

#if NWSMUX > 0
int evdev_kbd_mux_open(struct wsevsrc *, struct wseventvar *);
int evdev_kbd_mux_close(struct wsevsrc *);

struct wssrcops evmouse_srcops = {
		.type = WSMUX_EVDEV,
		.dopen = evdev_kbd_mux_open,
		.dclose = evdev_kbd_mux_close,
		.dioctl = evdev_do_ioctl,
		.ddispioctl = NULL,
		.dsetdisplay = NULL,
};
#endif

int
evdev_kbd_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return (1);
}

void
evdev_kbd_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct evdev_kbd_softc 			*ksc;
	struct evdev_softc				*sc;
	struct wskbddev_attach_args 	*ap;

	ksc = (struct evdev_kbd_softc *)self;
	sc = &ksc->sc;
	ap = (struct wskbddev_attach_args *)aux;

	ksc->sc_accessops = ap->accessops;
	ksc->sc_accesscookie = ap->accesscookie;

	evdev_attach_subr(sc, WSKBDDEVCF_MUX);
}

int
evdev_kbd_enable(ksc, on)
	struct evdev_kbd_softc *ksc;
	int on;
{
	return ((*ksc->sc_accessops->enable)(ksc->sc_accesscookie, on));
}

#if NWSMUX > 0
int
evdev_kbd_mux_open(me, evp)
	struct wsevsrc *me;
	struct wseventvar *evp;
{
	struct evdev_kbd_softc 	*ksc;
    struct evdev_softc			*sc;
	struct evdev_dev 			*evdev;
	struct evdev_client			*client;

	ksc = (struct evdev_kbd_softc *)me;
    sc = &ksc->sc;
	evdev = sc->sc_evdev;
	client = evdev->ev_client;

	if (client->ec_base.me_evp != NULL) {
		return (EBUSY);
	}

	return (evdev_doopen(evdev));
}

int
evdev_kbd_mux_close(me)
	struct wsevsrc *me;
{
	struct evdev_kbd_softc 	*ksc;
    struct evdev_softc		*sc;
	struct evdev_dev 		*evdev;
	struct evdev_client		*client;

	ksc = (struct evdev_kbd_softc *)me;
    sc = &ksc->sc;
	evdev = sc->sc_evdev;
	client = evdev->ev_client;

	client->ec_base.me_evp = NULL;
	(void)evdev_kbd_enable(ksc, 0);

	return (0);
}

int
evdev_kbd_add_mux(unit, muxsc)
	int unit;
	struct wsmux_softc *muxsc;
{
	struct evdev_kbd_softc 	    *ksc;
    struct evdev_softc			*sc;
	struct evdev_dev 			*evdev;
	struct evdev_client			*client;

	ksc = evdev_cd.cd_devs[unit];
	if (unit < 0 || unit >= evdev_cd.cd_ndevs || ksc == NULL) {
		return (ENXIO);
	}
    sc = &ksc->sc;
	evdev = sc->sc_evdev;
	client = evdev->ev_client;

	if (client->ec_base.me_parent != NULL || client->ec_base.me_evp != NULL) {
		return (EBUSY);
	}
	return (wsmux_attach_sc(muxsc, &client->ec_base));
}
#endif
