/*
 * genkbds.c
 *
 *  Created on: 16 Jan 2021
 *      Author: marti
 */

dev_type_open(gkopen);
dev_type_close(gkclose);
dev_type_read(gkread);
dev_type_write(gkwrite);
dev_type_ioctl(gkioctl);
dev_type_poll(gkpoll);

const struct cdevsw genkbd_cdevsw = {
		.d_open =	gkopen,
		.d_close =	gkclose,
		.d_read =	gkread,
		.d_write =	gkwrite,
		.d_ioctl =	gkioctl,
		.d_stop = 	nostop,
		.d_tty = 	notty,
		.d_poll =	gkpoll,
		.d_mmap = 	nommap,
		.d_discard = nodiscard,
		.d_type = 	D_OTHER
};

struct genkbddev 			*gk_tab;

static kbd_callback_func_t 	genkbd_event;

static int
gkopen(dev_t dev, int oflags, int devtype, struct proc *p)
{
	keyboard_t 		*kbd;
	genkbd_softc_t 	*sc;

	int s;
	int i;

	s = spltty();
	sc = gk_tab->gk_devs[KBD_INDEX(dev)];
	kbd = kbd_get_keyboard(KBD_INDEX(dev));
	if ((sc == NULL) || (kbd == NULL) || !KBD_IS_VALID(kbd)) {
		splx(s);
		return (ENXIO);
	}
	i = kbd_allocate(kbd->kb_name, kbd->kb_unit, sc, genkbd_event, (void *)sc);
	if (i < 0) {
		splx(s);
		return (EBUSY);
	}
	/* assert(i == kbd->kb_index) */
	/* assert(kbd == kbd_get_keyboard(i)) */

	/*
	 * NOTE: even when we have successfully claimed a keyboard,
	 * the device may still be missing (!KBD_HAS_DEVICE(kbd)).
	 */

	sc->gkb_q_length = 0;
	splx(s);

	return (cdev_open(KBD_INDEX(dev), oflags, devtype, p));
}

static int
gkclose(dev_t dev, int fflag, int devtype, struct proc *p)
{
	keyboard_t *kbd;
	genkbd_softc_t *sc;
	int s;

	/*
	 * NOTE: the device may have already become invalid.
	 * kbd == NULL || !KBD_IS_VALID(kbd)
	 */
	s = spltty();
	sc = gk_tab->gk_devs[KBD_INDEX(dev)];
	kbd = kbd_get_keyboard(KBD_INDEX(dev));
	if ((sc == NULL) || (kbd == NULL) || !KBD_IS_VALID(kbd)) {
		/* XXX: we shall be forgiving and don't report error... */
	} else {
		kbd_release(kbd, (void *)sc);
	}
	splx(s);
	return (cdev_close(KBD_INDEX(dev), fflag, devtype, p));
}

static int
gkread(dev_t dev, struct uio *uio, int ioflag)
{
	keyboard_t *kbd;
	genkbd_softc_t *sc;
	u_char buffer[KB_BUFSIZE];
	int len;
	int error;
	int s;

	/* wait for input */
	s = spltty();
	sc = gk_tab->gk_devs[KBD_INDEX(dev)];
	kbd = kbd_get_keyboard(KBD_INDEX(dev));
	if ((sc == NULL) || (kbd == NULL) || !KBD_IS_VALID(kbd)) {
		splx(s);
		return (ENXIO);
	}
	while (sc->gkb_q_length == 0) {
		if (ioflag & O_NONBLOCK) {
			splx(s);
			return (EWOULDBLOCK);
		}
		sc->gkb_flags |= KB_ASLEEP;
		error = tsleep(sc, PZERO | PCATCH, "kbdrea", 0);
		kbd = kbd_get_keyboard(KBD_INDEX(dev));
		if ((kbd == NULL) || !KBD_IS_VALID(kbd)) {
			splx(s);
			return (ENXIO);	/* our keyboard has gone... */
		}
		if (error) {
			sc->gkb_flags &= ~KB_ASLEEP;
			splx(s);
			return (error);
		}
	}
	splx(s);

	/* copy as much input as possible */
	error = 0;
	while (uio->uio_resid > 0) {
		len = imin(uio->uio_resid, sizeof(buffer));
		len = genkbd_getc(sc, buffer, len);
		if (len <= 0)
			break;
		error = uiomove(buffer, len, uio);
		if (error)
			return (error);
	}

	return (cdev_read(KBD_INDEX(dev), uio, ioflag));
}

static int
gkwrite(dev_t dev, struct uio *uio, int ioflag)
{
	keyboard_t *kbd;

	kbd = kbd_get_keyboard(KBD_INDEX(dev));
	if ((kbd == NULL) || !KBD_IS_VALID(kbd))
		return (ENXIO);
	return (cdev_write(KBD_INDEX(dev), uio, ioflag));
}

static int
gkioctl(dev_t dev, int cmd, caddr_t data, int fflag, struct proc *p)
{
	keyboard_t *kbd;
	int error;

	kbd = kbd_get_keyboard(KBD_INDEX(dev));
	if ((kbd == NULL) || !KBD_IS_VALID(kbd))
		return (ENXIO);
	error = kbdd_ioctl(kbd, cmd, data);
	if (error == ENOIOCTL) {
		error = ENODEV;
		return (error);
	}

	return (cdev_ioctl(KBD_INDEX(dev), cmd, data, fflag, p));
}

static int
gkpoll(dev_t dev, int events, struct proc *p)
{
	keyboard_t *kbd;
	genkbd_softc_t *sc;
	int revents;
	int s;

	revents = 0;
	s = spltty();
	sc = gk_tab->gk_devs[KBD_INDEX(dev)];
	kbd = kbd_get_keyboard(KBD_INDEX(dev));
	if ((sc == NULL) || (kbd == NULL) || !KBD_IS_VALID(kbd)) {
		revents =  POLLHUP;	/* the keyboard has gone */
	} else if (events & (POLLIN | POLLRDNORM)) {
		if (sc->gkb_q_length > 0)
			revents = events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->gkb_rsel);
	}
	splx(s);
	if(revents)
		return (revents);
	return (cdev_poll(KBD_INDEX(dev), events, p));
}

static void
gkputc(genkbd_softc_t *sc, char c)
{
	unsigned int p;

	if (sc->gkb_q_length == KB_QSIZE)
		return;

	p = (sc->gkb_q_start + sc->gkb_q_length) % KB_QSIZE;
	sc->gkb_q[p] = c;
	sc->gkb_q_length++;
}

static size_t
gkgetc(genkbd_softc_t *sc, char *buf, size_t len)
{

	/* Determine copy size. */
	if (sc->gkb_q_length == 0)
		return (0);
	if (len >= sc->gkb_q_length)
		len = sc->gkb_q_length;
	if (len >= KB_QSIZE - sc->gkb_q_start)
		len = KB_QSIZE - sc->gkb_q_start;

	/* Copy out data and progress offset. */
	memcpy(buf, sc->gkb_q + sc->gkb_q_start, len);
	sc->gkb_q_start = (sc->gkb_q_start + len) % KB_QSIZE;
	sc->gkb_q_length -= len;

	return (len);
}
