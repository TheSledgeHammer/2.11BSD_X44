/*
 * uinput1.c
 *
 *  Created on: 10 Dec 2021
 *      Author: marti
 */
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/uio.h>

#include <dev/evdev/evdev.h>
#include <dev/evdev/evdev_private.h>
#include <dev/evdev/input.h>
#include <dev/evdev/uinput.h>

#ifdef UINPUT_DEBUG
#define	debugf(state, fmt, args...)	printf("uinput: " fmt "\n", ##args)
#else
#define	debugf(state, fmt, args...)
#endif

#define	UINPUT_BUFFER_SIZE			16

#define	UNIPUTUNIT(dev)				(minor(dev) >> 8)
#define	UINPUT_LOCK(state)			simple_lock(&(state)->ucs_lock)
#define	UINPUT_UNLOCK(state)		simple_unlock(&(state)->ucs_lock)
#define UINPUT_EMPTYQ(state) \
    ((state)->ucs_buffer_head == (state)->ucs_buffer_tail)

enum uinput_state {
	UINPUT_NEW = 0,
	UINPUT_CONFIGURED,
	UINPUT_RUNNING
};

struct uinput_softc {
	u_int				get;	/* get (read) index (modified synchronously) */
	volatile u_int 		put;	/* put (write) index (modified by interrupt) */

	enum uinput_state	ucs_state;
	struct evdev_dev 	*ucs_evdev;
	struct lock_object	ucs_lock;
	size_t				ucs_buffer_head;
	size_t				ucs_buffer_tail;
	struct selinfo		ucs_selp;
	bool				ucs_blocked;
	bool				ucs_selected;
	struct input_event  ucs_buffer[UINPUT_BUFFER_SIZE];
};

static evdev_event_t	uinput_ev_event;

CFDRIVER_DECL(NULL, uinput, &uinput_cops, DV_DULL, sizeof(struct uinput_softc));
CFOPS_DECL(uinput, uinput_probe, uinput_attach, NULL, NULL);

extern struct cfdriver uinput_cd;

dev_type_open(uinput_open);
//dev_type_close(uinput_close);
dev_type_read(uinput_read);
dev_type_write(uinput_write);
dev_type_ioctl(uinput_ioctl);
//dev_type_stop(wsdisplaystop);
//dev_type_tty(wsdisplaytty);
dev_type_poll(uinput_poll);
//dev_type_mmap(wsdisplaymmap);
dev_type_kqfilter(uinput_kqfilter);

const struct cdevsw uinput_cdevsw = {
	.d_open = uinput_open,
	.d_close = noclose,
	.d_read = uinput_read,
	.d_write = uinput_write,
	.d_ioctl = uinput_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = uinput_poll,
	.d_mmap = nommap,
	.d_kqfilter = uinput_kqfilter,
	.d_discard = nodiscard,
	.d_type = D_OTHER
};

static struct evdev_methods uinput_ev_methods = {
	.ev_open = NULL,
	.ev_close = NULL,
	.ev_event = uinput_ev_event,
};

static void uinput_enqueue_event(struct uinput_softc *, uint16_t, uint16_t, int32_t);
static int uinput_setup_provider(struct uinput_softc *, struct uinput_user_dev *);

static void
uinput_knllock(void *arg)
{
	struct lock_object *lo = arg;

	simple_lock(lo);
}

static void
uinput_knlunlock(void *arg)
{
	struct lock_object *lo = arg;

	simple_unlock(lo);
}

static void
uinput_ev_event(evdev, type, code, value)
	struct evdev_dev *evdev;
	uint16_t type, code;
	int32_t value;
{
	struct uinput_softc *state = evdev_get_softc(evdev);

	if (type == EV_LED)
		evdev_push_event(evdev, type, code, value);

	UINPUT_LOCK(state);
	if (state->ucs_state == UINPUT_RUNNING) {
		uinput_enqueue_event(state, type, code, value);
		uinput_notify(state);
	}
	UINPUT_UNLOCK(state);
}

static void
uinput_enqueue_event(state, type, code, value)
	struct uinput_softc *state;
	uint16_t type, code;
	int32_t value;
{
	size_t head, tail;

	head = state->ucs_buffer_head;
	tail = (state->ucs_buffer_tail + 1) % UINPUT_BUFFER_SIZE;

	microtime(&state->ucs_buffer[tail].time);
	state->ucs_buffer[tail].type = type;
	state->ucs_buffer[tail].code = code;
	state->ucs_buffer[tail].value = value;
	state->ucs_buffer_tail = tail;

	/* If queue is full remove oldest event */
	if (tail == head) {
		debugf(state, "state %p: buffer overflow", state);

		head = (head + 1) % UINPUT_BUFFER_SIZE;
		state->ucs_buffer_head = head;
	}
}

int
uinput_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

}

void
uinput_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct uinput_softc *state = (void *)self;

	state->ucs_evdev = evdev_alloc();
	simple_lock_init(&state->ucs_lock, "uinput_lock");
}

int
uinput_open(dev, flags, fmt, p)
	dev_t dev;
	int flags;
	int fmt;
	struct proc *p;
{
	struct uinput_softc *state;
	int unit;

	unit = UNIPUTUNIT(dev);
	state = uinput_cd.cd_devs[unit];

	if (state == NULL)	{		/* make sure it was attached */
		return (ENXIO);
	}

	state = malloc(sizeof(struct uinput_softc), M_EVDEV, M_WAITOK | M_ZERO);
	state->ucs_evdev = evdev_alloc();

	simple_lock_init(&state->ucs_lock, "uinput");
	knlist_init(&state->ucs_selp.si_note, &state->ucs_lock, uinput_knllock, uinput_knlunlock, uinput_knl_assert_lock);

	return (0);
}

int
uinput_close(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct uinput_softc *state;
	int unit;

	unit = UNIPUTUNIT(dev);
	state = uinput_cd.cd_devs[unit];

	evdev_free(state->ucs_evdev);

	knlist_clear(&state->ucs_selp.si_note, 0);
	seldrain(&state->ucs_selp);
	knlist_destroy(&state->ucs_selp.si_note);
	sx_destroy(&state->ucs_lock);
	free(data, M_EVDEV);
}

int
uinput_read(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct uinput_softc *state;
	struct input_event *event;
	int remaining, ret, unit;

	unit = UNIPUTUNIT(dev);
	state = uinput_cd.cd_devs[unit];

	if (state == NULL) {
		return (ENODEV);
	}

	debugf(state, "read %zd bytes by proc %d", uio->uio_resid, uio->uio_procp->p_pid);

	/* Zero-sized reads are allowed for error checking */
	if (uio->uio_resid != 0 && uio->uio_resid < sizeof(struct input_event))
		return (EINVAL);

	remaining = uio->uio_resid / sizeof(struct input_event);

	UINPUT_LOCK(state);

	if (state->ucs_state != UINPUT_RUNNING) {
		ret = EINVAL;
	}

	if (ret == 0 && UINPUT_EMPTYQ(state)) {
		if (flags & O_NONBLOCK)
			ret = EWOULDBLOCK;
		else {
			if (remaining != 0) {
				state->ucs_blocked = TRUE;
				ret = sx_sleep(state, &state->ucs_lock, PCATCH, "uiread", 0);
			}
		}
	}

	while (ret == 0 && !UINPUT_EMPTYQ(state) && remaining > 0) {
		event = &state->ucs_buffer[state->ucs_buffer_head];
		state->ucs_buffer_head = (state->ucs_buffer_head + 1) %
		UINPUT_BUFFER_SIZE;
		remaining--;
		ret = uiomove(event, sizeof(struct input_event), uio);
	}

	UINPUT_UNLOCK(state);

	return (ret);
}

int
uinput_write(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct uinput_softc *state;
	struct uinput_user_dev userdev;
	struct input_event event;
	int unit, ret;

	ret = 0;
	unit = UNIPUTUNIT(dev);
	state = uinput_cd.cd_devs[unit];

	if (state == NULL) {
		return (ENODEV);
	}

	debugf(state, "write %zd bytes by proc %d", uio->uio_resid,	uio->uio_procp->p_pid);

	UINPUT_LOCK(state);

	if (state->ucs_state != UINPUT_RUNNING) {
		/* Process written struct uinput_user_dev */
		if (uio->uio_resid != sizeof(struct uinput_user_dev)) {
			debugf(state, "write size not multiple of "
					"struct uinput_user_dev size");
			ret = EINVAL;
		} else {
			ret = uiomove(&userdev, sizeof(struct uinput_user_dev), uio);
			if (ret == 0)
				uinput_setup_provider(state, &userdev);
		}
	} else {
		/* Process written event */
		if (uio->uio_resid % sizeof(struct input_event) != 0) {
			debugf(state, "write size not multiple of "
					"struct input_event size");
			ret = EINVAL;
		}

		while (ret == 0 && uio->uio_resid > 0) {
			uiomove(&event, sizeof(struct input_event), uio);
			ret = evdev_push_event(state->ucs_evdev, event.type, event.code,
					event.value);
		}
	}

	UINPUT_UNLOCK(state);

	return (ret);
}

int
uinput_poll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct uinput_softc *state;
	int unit, revents, s;

	unit = UNIPUTUNIT(dev);
	state = uinput_cd.cd_devs[unit];

	revents = 0;
	s = splwsevent();

	if(state == NULL) {
		return (ENIVAL);
	}

	if (events & (POLLIN | POLLRDNORM)) {
		if (state->get != state->put)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &state->ucs_selp);
	}

	splx(s);
	return (revents);
}

static void
filt_uinputrdetach(kn)
	struct knote *kn;
{
	struct uinput_softc *state = kn->kn_hook;
	int s;

	s = splwsevent();
	SLIST_REMOVE(&state->ucs_selp.si_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_uinputread(kn, hint)
	struct knote *kn;
	long hint;
{
	struct uinput_softc *state = kn->kn_hook;

	if (state->get == state->put)
		return (0);

	if (state->get < state->put)
		kn->kn_data = state->put - state->get;
	else
		kn->kn_data = (UINPUT_BUFFER_SIZE - state->get) + state->put;

	kn->kn_data *= sizeof(struct uinput_softc);

	return (1);
}

static const struct filterops uinput_filtops =
	{ 1, NULL, filt_uinputrdetach, filt_uinputread };

int
uinput_kqfilter(state, kn)
	struct uinput_softc *state;
	struct knote *kn;
{
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &state->ucs_selp.si_klist;
		kn->kn_fop = &uinput_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = state;

	s = splwsevent();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

static int
uinput_setup_dev(state, id, name, ff_effects_max)
	struct uinput_softc *state;
	struct input_id *id;
	char *name;
	uint32_t ff_effects_max;
{
	if (name[0] == 0)
		return (EINVAL);

	evdev_set_name(state->ucs_evdev, name);
	evdev_set_id(state->ucs_evdev, id->bustype, id->vendor, id->product,
			id->version);
	state->ucs_state = UINPUT_CONFIGURED;

	return (0);
}

static int
uinput_setup_provider(state, udev)
	struct uinput_softc *state;
	struct uinput_user_dev *udev;
{
	struct input_absinfo absinfo;
	int i, ret;

	debugf(state, "setup_provider called, udev=%p", udev);

	ret = uinput_setup_dev(state, &udev->id, udev->name,
	    udev->ff_effects_max);
	if (ret)
		return (ret);

	bzero(&absinfo, sizeof(struct input_absinfo));
	for (i = 0; i < ABS_CNT; i++) {
		if (!bit_test(state->ucs_evdev->ev_abs_flags, i))
			continue;

		absinfo.minimum = udev->absmin[i];
		absinfo.maximum = udev->absmax[i];
		absinfo.fuzz = udev->absfuzz[i];
		absinfo.flat = udev->absflat[i];
		evdev_set_absinfo(state->ucs_evdev, i, &absinfo);
	}

	return (0);
}

static int
uinput_ioctl_sub(state, cmd, data)
	struct uinput_softc *state;
	int cmd;
	caddr_t data;
{
	struct uinput_setup *us;
	struct uinput_abs_setup *uabs;
	int ret, len, intdata;
	char buf[NAMELEN];

	UINPUT_LOCK_ASSERT(state);

	len = IOCPARM_LEN(cmd);
	if ((cmd & IOC_DIRMASK) == IOC_VOID && len == sizeof(int))
		intdata = *(int*) data;

	switch (IOCBASECMD(cmd)) {
	case UI_GET_SYSNAME(0):
		if (state->ucs_state != UINPUT_RUNNING)
			return (ENOENT);
		if (len == 0)
			return (EINVAL);
		snprintf(data, len, "event%d", state->ucs_evdev->ev_unit);
		return (0);
	}

	switch (cmd) {
	case UI_DEV_CREATE:
		if (state->ucs_state != UINPUT_CONFIGURED)
			return (EINVAL);

		evdev_set_methods(state->ucs_evdev, state, &uinput_ev_methods);
		evdev_set_flag(state->ucs_evdev, EVDEV_FLAG_SOFTREPEAT);
		ret = evdev_register(state->ucs_evdev);
		if (ret == 0)
			state->ucs_state = UINPUT_RUNNING;
		return (ret);

	case UI_DEV_DESTROY:
		if (state->ucs_state != UINPUT_RUNNING)
			return (0);

		evdev_unregister(state->ucs_evdev);
		bzero(state->ucs_evdev, sizeof(struct evdev_dev));
		state->ucs_state = UINPUT_NEW;
		return (0);

	case UI_DEV_SETUP:
		if (state->ucs_state == UINPUT_RUNNING)
			return (EINVAL);

		us = (struct uinput_setup*) data;
		return (uinput_setup_dev(state, &us->id, us->name, us->ff_effects_max));

	case UI_ABS_SETUP:
		if (state->ucs_state == UINPUT_RUNNING)
			return (EINVAL);

		uabs = (struct uinput_abs_setup*) data;
		if (uabs->code > ABS_MAX)
			return (EINVAL);

		evdev_set_abs_bit(state->ucs_evdev, uabs->code);
		evdev_set_absinfo(state->ucs_evdev, uabs->code, &uabs->absinfo);
		return (0);

	case UI_SET_EVBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > EV_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_support_event(state->ucs_evdev, intdata);
		return (0);

	case UI_SET_KEYBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > KEY_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_support_key(state->ucs_evdev, intdata);
		return (0);

	case UI_SET_RELBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > REL_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_support_rel(state->ucs_evdev, intdata);
		return (0);

	case UI_SET_ABSBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > ABS_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_set_abs_bit(state->ucs_evdev, intdata);
		return (0);

	case UI_SET_MSCBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > MSC_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_support_msc(state->ucs_evdev, intdata);
		return (0);

	case UI_SET_LEDBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > LED_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_support_led(state->ucs_evdev, intdata);
		return (0);

	case UI_SET_SNDBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > SND_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_support_snd(state->ucs_evdev, intdata);
		return (0);

	case UI_SET_FFBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > FF_MAX
				|| intdata < 0)
			return (EINVAL);
		/* Fake unsupported ioctl */
		return (0);

	case UI_SET_PHYS:
		if (state->ucs_state == UINPUT_RUNNING)
			return (EINVAL);
		ret = copyinstr(*(void**) data, buf, sizeof(buf), NULL);
		/* Linux returns EINVAL when string does not fit the buffer */
		if (ret == ENAMETOOLONG)
			ret = EINVAL;
		if (ret != 0)
			return (ret);
		evdev_set_phys(state->ucs_evdev, buf);
		return (0);

	case UI_SET_BSDUNIQ:
		if (state->ucs_state == UINPUT_RUNNING)
			return (EINVAL);
		ret = copyinstr(*(void**) data, buf, sizeof(buf), NULL);
		if (ret != 0)
			return (ret);
		evdev_set_serial(state->ucs_evdev, buf);
		return (0);

	case UI_SET_SWBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > SW_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_support_sw(state->ucs_evdev, intdata);
		return (0);

	case UI_SET_PROPBIT:
		if (state->ucs_state == UINPUT_RUNNING || intdata > INPUT_PROP_MAX
				|| intdata < 0)
			return (EINVAL);
		evdev_support_prop(state->ucs_evdev, intdata);
		return (0);

	case UI_BEGIN_FF_UPLOAD:
	case UI_END_FF_UPLOAD:
	case UI_BEGIN_FF_ERASE:
	case UI_END_FF_ERASE:
		if (state->ucs_state == UINPUT_RUNNING)
			return (EINVAL);
		/* Fake unsupported ioctl */
		return (0);

	case UI_GET_VERSION:
		*(unsigned int*) data = UINPUT_VERSION;
		return (0);
	}

	return (EINVAL);
}

int
uinput_ioctl(dev, cmd, data, fflag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int fflag;
	struct proc *p;
{
	struct uinput_softc *state;
	int unit, ret;

	unit = UNIPUTUNIT(dev);
	state = uinput_cd.cd_devs[unit];

	if (state == NULL) {
		return (ENIOCTL);
	}

	debugf(state, "ioctl called: cmd=0x%08lx, data=%p", cmd, data);

	UINPUT_LOCK(state);
	ret = uinput_ioctl_sub(state, cmd, data);
	UINPUT_UNLOCK(state);

	return (ret);
}
