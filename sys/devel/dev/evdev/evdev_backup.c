/*-
 * Copyright (c) 2014 Jakub Wojciech Klama <jceel@FreeBSD.org>
 * Copyright (c) 2015-2016 Vladimir Kondratyev <wulf@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/stddef.h>
#include <sys/uio.h>

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wseventvar.h>

#include <dev/evdev/evdev.h>
#include <dev/evdev/evdev_private.h>
#include <dev/evdev/input.h>
#include "freebsd-bitstring.h"

int
evdev_open(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct evdev_softc *sc;
	struct evdev_dev 	*evdev;
	struct evdev_client *client;
	struct wseventvar 	*evar;
	int unit, error;

	unit = minor(dev);
	sc = (struct evdev_softc *)evdev_cd.cd_devs[unit];
	evdev = sc->sc_evdev;
	client = evdev->ev_client;

	if(sc == NULL) {
		return (ENXIO);
	}

	if (client == NULL) {
		client = evdev_client_alloc(evdev, (evdev->ev_report_size * DEF_RING_REPORTS));
	}

	EVDEV_LOCK(evdev);
	evar = evdev_register_wsevent(client, p);

	/* check event buffer */
	if(evdev_buffer_size(evdev) > wsevent_avail(evar)) {
		return (EINVAL);
	}

	EVDEV_UNLOCK(evdev);

	return (error);
}

int
evdev_close(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
	struct evdev_softc *sc;
	struct evdev_dev 	*evdev;
	struct evdev_client *client;
	int unit, error;

	unit = minor(dev);
	sc = (struct evdev_softc *)evdev_cd.cd_devs[unit];
	evdev = sc->sc_evdev;
	client = evdev->ev_client;

	if(sc == NULL) {
		return (ENXIO);
	}

	EVDEV_LOCK(evdev);
	if (client == NULL) {
		if (evdev_event_supported(evdev, EV_REP)
				&& bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
			evdev_stop_repeat(evdev);
		}
	} else {
		evdev_revoke_client(client);
		evdev_unregister_wsevent(client);
		/* free client */
		evdev_client_free(client);
		evdev->ev_client = client;
	}
	evdev_release_client(evdev, client);
	EVDEV_UNLOCK(evdev);

	return (0);
}

int
evdev_register_client(struct evdev_dev *evdev, struct evdev_client *client, dev_t dev, int flags, int mode, int p)
{
	int error = 0;

	debugf(evdev, "adding new client for device %s", evdev->ev_shortname);

	EVDEV_LOCK_ASSERT(evdev);
	if (evdev->ev_client && evdev->ev_methods != NULL
			&& evdev->ev_methods->ev_open != NULL) {
		debugf(evdev, "calling ev_open() on device %s", evdev->ev_shortname);
		error = evdev->ev_methods->ev_open(dev, flags, mode, p);
	}

	if (error == 0) {
		client = evdev->ev_client;
	}

	return (error);
}

int
evdev_dispose_client(struct evdev_dev *evdev, struct evdev_client *client, dev_t dev, int flags, int mode, int p)
{
	int error;

	KASSERT(evdev);
	client = NULL;
	evdev->ev_client = client;
	if(evdev->ev_client == NULL) {
		if (evdev->ev_methods != NULL && evdev->ev_methods->ev_close != NULL) {
			error = evdev->ev_methods->ev_close(dev, flags, mode, p);
		}
		if (evdev_event_supported(evdev, EV_REP) && bit_test(evdev->ev_flags, EVDEV_FLAG_SOFTREPEAT)) {
			evdev_stop_repeat(evdev);
		}
	}
	evdev_release_client(evdev, client);

	return (error);
}
