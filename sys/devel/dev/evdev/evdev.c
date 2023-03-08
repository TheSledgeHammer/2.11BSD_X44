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

#include <dev/misc/evdev/evdev.h>
#include <dev/misc/evdev/evdev_private.h>
#include <dev/misc/evdev/input.h>
#include <dev/misc/evdev/freebsd-bitstring.h>

struct evdev_softc {
	struct device 				sc_device;
	struct evdev_dev 			*sc_evdev;
	int			 				sc_evdev_state;
	int							sc_refcnt;
	u_char						sc_dying;
};

static void evdev_init(struct evdev_dev *, int);

void
evdev_attach_subr(sc, mux_loc)
    struct evdev_softc  *sc;
	int mux_loc;
{
    evdev_init(sc->sc_evdev, mux_loc);
}

int
evdev_doactivate(sc, self, act)
	struct evdev_softc *sc;
	struct device 		*self;
	enum devact 		act;
{
	if (act == DVACT_DEACTIVATE) {
		sc->sc_dying = 1;
	}
	return (0);
}

int
evdev_dodetach(sc, self, flags)
	struct evdev_softc 	*sc;
	struct device 		*self;
	int 				flags;
{
	struct evdev_dev *evdev = sc->sc_evdev;
	struct evdev_client *client = evdev->ev_client;
	struct wseventvar *evar;
	int maj, mn, s;

#if NWSMUX > 0
	/* Tell parent mux we're leaving. */
	if (client->ec_base.me_parent != NULL) {
		DPRINTF(("evdev_detach:\n"));
		wsmux_detach_sc(&client->ec_base);
	}
#endif

	/* If we're open ... */
	evar = client->ec_base.me_evp;
	if (evar != NULL && evar->io != NULL) {
		s = spltty();
		if (--sc->sc_refcnt >= 0) {
			struct wscons_event event;
			struct input_event 	ie;

			/* Wake everyone by generating a dummy event. */
			event.type = 0;
			event.value = 0;
			ie.type = event.type;
			ie.value = event.value;
			if ((wsevent_inject(evar, &event, 1) != 0) && (evdev_wsevent_inject(evar, &ie, 1) != 0)) {
				wsevent_wakeup(evar);
			}
			/* Wait for processes to go away. */
			if (tsleep(sc, PZERO, "evddet", hz * 60)) {
				printf("evdev_detach: %s didn't detach\n",
						client->ec_base.me_dv.dv_xname);
			}
		}
		splx(s);
	}

	/* locate the major number */
	maj = cdevsw_lookup_major(&evdev_cdevsw);

	/* Nuke the vnodes for any open instances (calls close). */
	mn = self->dv_unit;
	vdevgone(maj, mn, mn, VCHR);
	return (0);
}

int
evdev_doread(sc, dev, uio, flags)
	struct evdev_softc *sc;
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct evdev_dev 	*evdev;
	struct evdev_client *client;
	struct wseventvar 	*evar;
	struct wscons_event *we;
	int error, remaining;

	evdev = sc->sc_evdev;
	client = evdev->ev_client;
	evar = client->ec_base.me_evp;

	if (sc->sc_dying) {
		return (EIO);
	}

	if (evar->revoked)
		return (ENODEV);

	/* Zero-sized reads are allowed for error checking */
	if (uio->uio_resid != 0 && uio->uio_resid < sizeof(struct input_event)) {
		return (EINVAL);
	}

	remaining = uio->uio_resid / sizeof(struct input_event);

	EVDEV_CLIENT_LOCKQ(client);
	if (EVDEV_CLIENT_EMPTYQ(client)) {
		if (flags & O_NONBLOCK) {
			error = EWOULDBLOCK;
		} else {
			if (remaining != 0) {
				evar->blocked = TRUE;
				tsleep(client, &client->ec_buffer_lock, PCATCH, "evread", 0);
				if (error == 0 && evar->revoked) {
					error = ENODEV;
				}
			}
		}
	}
	while (error == 0 && !EVDEV_CLIENT_EMPTYQ(client) && remaining > 0) {
		memcpy(&we, &client->ec_buffer[evar->put], sizeof(struct input_event));
		wsevent_put(evar, 1);
		remaining--;

		EVDEV_CLIENT_UNLOCKQ(client);
		sc->sc_refcnt++;
		error = wsevent_read(evar, uio, flags);
		if (--sc->sc_refcnt < 0) {
			wakeup(sc);
			error = EIO;
		}
		EVDEV_CLIENT_LOCKQ(client);
	}
	EVDEV_CLIENT_UNLOCKQ(client);
	return (error);
}

int
evdev_dowrite(sc, dev, uio, flags)
	struct evdev_softc *sc;
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct evdev_dev *evdev;
	struct evdev_client *client;
	struct wseventvar 	*evar;
	struct input_event 	event;
	int unit, error, remaining;

	evdev = sc->sc_evdev;
	client = evdev->ev_client;
	evar = client->ec_base.me_evp;

	if (evar->revoked || evdev == NULL) {
		return (ENODEV);
	}
	if (uio->uio_resid % sizeof(struct input_event) != 0) {
		debugf(sc, "write size not multiple of input_event size");
		return (EINVAL);
	}

	while (uio->uio_resid > 0) {
		error = uiomove(&event, sizeof(struct input_event), uio);
		if (error == 0) {
			error = evdev_inject_event(evdev, event.type, event.code, event.value);
			if (error == 0) {
				error = evdev_wsevent_inject(evar, &event, 1);
			}
		}
	}
	return (error);
}

int
evdev_dopoll(sc, dev, events, p)
	struct evdev_softc *sc;
	dev_t dev;
	int events;
	struct proc *p;
{
	struct evdev_dev *evdev;
	struct evdev_client *client;
	int error;

	evdev = sc->sc_evdev;
	client = evdev->ev_client;

	if (client->ec_base.me_evp == NULL) {
		return (EINVAL);
	}

	EVDEV_CLIENT_LOCKQ(client);
	if(!EVDEV_CLIENT_EMPTYQ(client)) {
		error = wsevent_poll(client->ec_base.me_evp, events & (POLLIN | POLLRDNORM), p);
	} else {
		error = wsevent_poll(client->ec_base.me_evp, events, p);
	}
	EVDEV_CLIENT_UNLOCKQ(client);
	return (error);
}

int
evdev_dokqfilter(sc, dev, kn)
	struct evdev_softc *sc;
	dev_t dev;
	struct knote *kn;
{
	struct evdev_dev *evdev;
	struct evdev_client *client;

	evdev = sc->sc_evdev;
	client = evdev->ev_client;
	if (client->ec_base.me_evp == NULL) {
		return (EINVAL);
	}
	return (wsevent_kqfilter(client->ec_base.me_evp, kn));
}

int
evdev_doioctl(sc, cmd, data, fflag, p)
	struct evdev_softc *sc;
	int cmd;
	caddr_t data;
	int fflag;
	struct proc *p;
{
	struct evdev_dev *evdev;
	int error;

	evdev = (struct evdev_dev *)sc->sc_device;
	sc->sc_refcnt++;
	error = evdev_do_ioctl_sc(evdev, cmd, data, fflag, p);
	if (--sc->sc_refcnt < 0) {
		wakeup(sc);
	}
	return (error);
}

static void
evdev_init(evdev, mux_loc)
    struct evdev_dev        *evdev;
	int mux_loc;
{
    struct evdev_client     *client;
    size_t buffer_size;
    int ret;
#if NWSMUX > 0
	int mux, error;
#endif

    /* Initialize internal structures */
    if (evdev == NULL) {
        evdev = evdev_alloc();
    }
    ret = evdev_register(evdev);
    if (ret != 0) {
        printf("evdev_attach: evdev_register error", ret);
    }
    /* Initialize ring buffer */
    buffer_size = evdev_buffer_size(evdev);
	client = evdev_client_alloc(evdev, buffer_size);
    client->ec_buffer_size = buffer_size;
	client->ec_buffer_ready = 0;

    client->ec_evdev = evdev;

#if NWSMUX > 0
	client->ec_base.me_ops = &evdev_srcops;
	mux = client->ec_base.me_dv.dv_cfdata->cf_loc[mux_loc];
	if (mux >= 0) {
		error = wsmux_attach_sc(wsmux_getmux(mux), &client->ec_base);
		if (error) {
			printf(" attach error=%d", error);
		} else {
			printf(" mux %d", mux);
		}
	}
#else
	if (client->ec_base.me_dv.dv_cfdata->cf_loc[mux_loc] >= 0) {
		printf(" (mux ignored)");
	}
#endif
	printf("\n");
    lockinit(&client->ec_buffer_lock, "evclient", 0, LK_CANRECURSE);
}
