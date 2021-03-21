/*	$NetBSD: usb.c,v 1.80 2003/11/07 17:03:25 wiz Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (augustss@carlstedt.se) at
 * Carlstedt Research & Technology.
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
 * USB specifications and other documentation can be found at
 * http://www.usb.org/developers/data/ and
 * http://www.usb.org/developers/index.html .
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/user.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_quirks.h>

#include <machine/bus_space.h>
#include <machine/bus_dma.h>

#ifdef USB_DEBUG
#define DPRINTF(x)		if (usbdebug) printf x
#define DPRINTFN(n,x)	if (usbdebug>(n)) printf x
int	usbdebug = 0;
int	uhcidebug;
int	ohcidebug;
/*
 * 0  - do usual exploration
 * 1  - do not use timeout exploration
 * >1 - do no exploration
 */
int	usb_noexplore = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif
#if defined(UHCI_DEBUG) && NUHCI > 0
extern int	uhcidebug;
#endif
#if defined(OHCI_DEBUG) && NOHCI > 0
extern int	ohcidebug;
#endif
#if defined(EHCI_DEBUG) && NEHCI > 0
extern int	ehcidebug;
#endif

struct usb_softc {
	struct device		sc_dev;			/* base device */
	usbd_bus_handle 	sc_bus;			/* USB controller */
	struct usbd_port 	sc_port;		/* dummy port for root hub */

	int		 			sc_speed;
	char 				sc_running;
	struct selinfo 		sc_consel;		/* waiting for connect change */

	//struct proc			*sc_event_thread;
};

TAILQ_HEAD(, usb_task) usb_all_tasks;

static void	usb_discover(struct usb_softc *);

int usbopen (dev_t, int, int, struct proc *);
int usbclose (dev_t, int, int, struct proc *);
int usbioctl (dev_t, u_long, caddr_t, int, struct proc *);
int usbpoll (dev_t, int, struct proc *);

struct cdevsw usb_cdevsw = {
		.d_open = usbopen,
		.d_close = usbclose,
		.d_read = noread,
		.d_write = nowrite,
		.d_ioctl = usbioctl,
		.d_stop = nullstop,
		.d_reset = nullreset,
		.d_tty = nodevtotty,
		.d_poll = usbpoll,
		.d_mmap = nommap,
		.d_strategy = nostrategy,
		.d_discard = nodiscard,
		.d_type = D_OTHER
};

#define USBUNIT(dev) 	(minor(dev))
static const char *usbrev_str[] = USBREV_STR;
/*
struct cfdriver usb_cd = {
	NULL, "usb", usb_match, usb_attach, DV_DULL, sizeof(struct usb_softc)
};
*/
CFDRIVER_DECL(NULL, usb, &usb_cops, DV_DULL, sizeof(struct usb_softc));
CFOPS_DECL(usb, usb_match, usb_attach, usb_detach, usb_activate);

int
usb_match(struct device *parent, struct cfdata *match, void *aux)
{
	DPRINTF(("usbd_match\n"));
	return (UMATCH_GENERIC);
}

void
usb_attach(struct device *parent, struct device *self, void *aux)
{
	struct usb_softc *sc = (struct usb_softc *)self;
	usbd_device_handle dev;
	usbd_status err;
	int usbrev;

	DPRINTF(("usbd_attach\n"));

	usbd_init();
	sc->sc_bus = aux;
	sc->sc_bus->usbctl = sc;
	sc->sc_running = 1;
	sc->sc_bus->use_polling = 0;
	sc->sc_port.power = USB_MAX_POWER;
	err = usbd_new_device(&sc->sc_dev, sc->sc_bus, 0, sc->sc_speed, 0, &sc->sc_port);
	usbrev = sc->sc_bus->usbrev;
	printf(": USB revision %s", usbrev_str[usbrev]);
	switch (usbrev) {
	case USBREV_1_0:
	case USBREV_1_1:
		sc->sc_speed = USB_SPEED_FULL;
		break;
	case USBREV_2_0:
		sc->sc_speed = USB_SPEED_HIGH;
		break;
	case USBREV_3_0:
		sc->sc_speed = USB_SPEED_SUPER;
		break;
	default:
		printf(", not supported\n");
		sc->sc_bus->dying = 1;
		return;
	}
	printf("\n");

	if (err == USBD_NORMAL_COMPLETION) {
		dev = sc->sc_port.device;
		if (!dev->hub) {
			sc->sc_running = 0;
			printf("%s: root device is not a hub\n", sc->sc_dev.dv_xname);
			return;
		}
		sc->sc_bus->root_hub = dev;
		dev->hub->explore(sc->sc_bus->root_hub);
	} else {
		printf("%s: root hub problem, error=%d\n", sc->sc_dev.dv_xname, err);
		sc->sc_running = 0;
	}
	sc->sc_bus->use_polling = 0;

	return;
}

int
usbopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = USBUNIT(dev);
	struct usb_softc *sc;

	if (unit >= usb_cd.cd_ndevs)
		return (ENXIO);

	sc = usb_cd.cd_devs[unit];

	if (sc == NULL)
		return (ENXIO);

	if (sc->sc_bus->dying)
		return (EIO);

	if (sc == 0 || !sc->sc_running)
		return (ENXIO);

	return (0);
}

int
usbclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (0);
}

int
usbioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = USBUNIT(dev);
	struct usb_softc *sc;
	int error;

	sc = usb_cd.cd_devs[unit];

	if (sc->sc_bus->dying)
		return (EIO);

	switch (cmd) {
#ifdef USB_DEBUG
	case USB_SETDEBUG:
		usbdebug  = ((*(unsigned int *)data) & 0x000000ff);
#if defined(UHCI_DEBUG) && NUHCI > 0
		uhcidebug = ((*(unsigned int *)data) & 0x0000ff00) >> 8;
#endif
#if defined(OHCI_DEBUG) && NOHCI > 0
		ohcidebug = ((*(unsigned int *)data) & 0x00ff0000) >> 16;
#endif
#if defined(EHCI_DEBUG) && NEHCI > 0
		ehcidebug = ((*(unsigned int *)data) & 0xff000000) >> 24;
#endif
		break;
#endif

	case USB_REQUEST:
	{
		struct usb_ctl_request *ur = (void *)data;
		int len = UGETW(ur->request.wLength);
		struct iovec iov;
		struct uio uio;
		void *ptr = 0;
		int addr = ur->addr;
		usbd_status r;
		int error = 0;

		DPRINTF(("usbioctl: USB_REQUEST addr=%d len=%d\n", addr, len));
		if (len < 0 || len > 32768) {
			return (EINVAL);
		}
		if (addr < 0 || addr >= USB_MAX_DEVICES || sc->sc_bus->devices[addr] == 0) {
			return (EINVAL);
		}
		if (len != 0) {
			iov.iov_base = (caddr_t)ur->data;
			iov.iov_len = len;
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_resid = len;
			uio.uio_offset = 0;
			uio.uio_segflg = UIO_USERSPACE;
			uio.uio_rw =
			ur->request.bmRequestType & UT_READ ? UIO_READ : UIO_WRITE;
			uio.uio_procp = p;
			ptr = malloc(len, M_TEMP, M_WAITOK);
			if (uio.uio_rw == UIO_WRITE) {
				error = uiomove(ptr, len, &uio);
				if (error) {
					goto ret;
				}
			}
		}
		r = usbd_do_request_flags(sc->sc_bus->devices[addr], &ur->request, ptr, ur->flags, &ur->actlen);
		if (r) {
			error = EIO;
			goto ret;
		}
		if (len != 0) {
			if (uio.uio_rw == UIO_READ) {
				error = uiomove(ptr, len, &uio);
				if (error) {
					goto ret;
				}
			}
		}
	ret:
		free(ptr, M_TEMP);
		return (error);
	}

	case USB_DEVICEINFO:
	{
		struct usb_device_info *di = (void *)data;
		int addr = di->addr;
		usbd_device_handle dev;

		if (addr < 1 || addr >= USB_MAX_DEVICES) {
			return (EINVAL);
		}
		dev = sc->sc_bus->devices[addr];
		if (dev == 0) {
			return (ENXIO);
		}
		usbd_fill_deviceinfo(dev, di);
		break;
	}

	case USB_DEVICESTATS:
		*(struct usb_device_stats *)data = sc->sc_bus->stats;
		break;

	default:
		return (EINVAL);
	}
	return (0);
}

int
usbpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	int unit = USBUNIT(dev);
	struct usb_softc *sc;
	int revents, s;
	sc = usb_cd.cd_devs[unit];


	DPRINTFN(2, ("usbpoll: sc=%p events=0x%x\n", sc, events));
	s = splusb();
	revents = 0;
	if (events & (POLLOUT | POLLWRNORM)) {
		if (sc->sc_bus->needs_explore) {
			revents |= events & (POLLOUT | POLLWRNORM);
		}
	}
	DPRINTFN(2, ("usbpoll: revents=0x%x\n", revents));
	if (revents == 0) {
		if (events & (POLLOUT | POLLWRNORM)) {
			DPRINTFN(2, ("usbpoll: selrecord\n"));
			selrecord(p, &sc->sc_consel);
		}
	}
	splx(s);
	return (revents);
}

#if 0
int
usb_bus_count()
{
	int i, n;

	for (i = n = 0; i < usb_cd.cd_ndevs; i++)
		if (usb_cd.cd_devs[i])
			n++;
	return (n);
}
#endif

#if 0
usbd_status
usb_get_bus_handle(n, h)
	int n;
	usbd_bus_handle *h;
{
	int i;

	for (i = 0; i < usb_cd.cd_ndevs; i++)
		if (usb_cd.cd_devs[i] && n-- == 0) {
			*h = usb_cd.cd_devs[i];
			return (USBD_NORMAL_COMPLETION);
		}
	return (USBD_INVAL);
}
#endif

static void
usb_discover(sc)
	struct usb_softc *sc;
{
	DPRINTFN(2,("usb_discover\n"));
#ifdef USB_DEBUG
	if (usb_noexplore > 1)
		return;
#endif
	/*
	 * We need mutual exclusion while traversing the device tree,
	 * but this is guaranteed since this function is only called
	 * from the event thread for the controller.
	 */
	while (sc->sc_bus->needs_explore && !sc->sc_bus->dying) {
			sc->sc_bus->needs_explore = 0;
			sc->sc_bus->root_hub->hub->explore(sc->sc_bus->root_hub);
	}
}

void
usb_needs_explore(bus)
	usbd_bus_handle bus;
{
	DPRINTFN(2,("usb_needs_explore\n"));
	bus->needs_explore = 1;
	selwakeup(&bus->usbctl->sc_consel);
}
