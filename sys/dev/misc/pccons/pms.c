/* $NetBSD: pms.c,v 1.2 2004/03/18 21:05:19 bjh21 Exp $ */

/*-
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL I BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: pms.c,v 1.2 2004/03/18 21:05:19 bjh21 Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/user.h>

#include <machine/bus.h>

#include <dev/misc/pccons/pckbportvar.h>

#include <dev/misc/pccons/pmsreg.h>

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wsmousevar.h>

#ifdef PMSDEBUG
int pmsdebug = 1;
#define DPRINTF(x)      if (pmsdebug) printf x
#else
#define DPRINTF(x)
#endif

enum pms_type { PMS_UNKNOWN, PMS_STANDARD, PMS_SCROLL3, PMS_SCROLL5 };

struct pms_protocol {
	int rates[3];
	int response;
	const char *name;
} pms_protocols[] = {
	{ { 0, 0, 0 }, 0, "unknown protocol" },
	{ { 0, 0, 0 }, 0, "no scroll wheel (3 buttons)" },
	{ { 200, 100, 80 }, 3, "scroll wheel (3 buttons)" },
	{ { 200, 200, 80 }, 4, "scroll wheel (5 buttons)" }
};

enum pms_type tries[] = {
	PMS_SCROLL5, PMS_SCROLL3, PMS_STANDARD, PMS_UNKNOWN
};

struct pms_softc {		/* driver status information */
	struct device 	sc_dev;

	pckbport_tag_t 	sc_kbctag;
	int 			sc_kbcslot;

	int 			sc_enabled;		/* input enabled? */
	int 			inputstate;		/* number of bytes received for this packet */
	u_int 			buttons;		/* mouse button status */
	enum pms_type 	protocol;
	unsigned char 	packet[4];
	struct timeval 	last, current;

	struct device 	*sc_wsmousedev;
};

int pmsprobe(struct device *, struct cfdata *, void *);
void pmsattach(struct device *, struct device *, void *);
void pmsinput(void *, int);

CFOPS_DECL(pms, pmsprobe, pmsattach, NULL, NULL);
CFDRIVER_DECL(NULL, pms, DV_DULL);
CFATTACH_DECL(pms, &pms_cd, &pms_cops, sizeof(struct pms_softc));

static int	pms_protocol(pckbport_tag_t, pckbport_slot_t);
static void	do_enable(struct pms_softc *);
static void	do_disable(struct pms_softc *);
int			pms_enable(void *);
int			pms_ioctl(void *, u_long, caddr_t, int, struct proc *);
void		pms_disable(void *);

const struct wsmouse_accessops pms_accessops = {
		.enable = pms_enable,
		.ioctl = pms_ioctl,
		.disable = pms_disable,
};

static int
pms_protocol(pckbport_tag_t tag, pckbport_slot_t slot)
{
	u_char cmd[2], resp[1];
	int i, j, res;
	struct pms_protocol *p;

	for (j = 0; j < sizeof(tries) / sizeof(tries[0]); ++j) {
		p = &pms_protocols[tries[j]];
		if (!p->rates[0])
			break;
		cmd[0] = PMS_SET_SAMPLE;
		for (i = 0; i < 3; i++) {
			cmd[1] = p->rates[i];
			res = pckbport_enqueue_cmd(tag, slot, cmd, 2, 0, 1, 0);
			if (res)
				return PMS_STANDARD;
		}

		cmd[0] = PMS_SEND_DEV_ID;
		res = pckbport_enqueue_cmd(tag, slot, cmd, 1, 1, 1, resp);
		if (res)
			return PMS_UNKNOWN;
		if (resp[0] == p->response) {
			DPRINTF(("pms_protocol: found mouse protocol %d\n",
				tries[j]));
			return tries[j];
		}
	}
	DPRINTF(("pms_protocol: standard PS/2 protocol (no scroll wheel)\n"));
	return PMS_STANDARD;
}

int
pmsprobe(struct device *parent, struct cfdata *match, void *aux)
{
	struct pckbport_attach_args *pa = aux;
	u_char cmd[1], resp[2];
	int res;

	if (pa->pa_slot != PCKBPORT_AUX_SLOT)
		return 0;

	/* Flush any garbage. */
	pckbport_flush(pa->pa_tag, pa->pa_slot);

	/* reset the device */
	cmd[0] = PMS_RESET;
	res = pckbport_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 2, resp, 1);
	if (res) {
#ifdef DEBUG
		printf("pmsprobe: reset error %d\n", res);
#endif
		return 0;
	}
	if (resp[0] != PMS_RSTDONE) {
		printf("pmsprobe: reset response 0x%x\n", resp[0]);
		return 0;
	}

	/* get type number (0 = mouse) */
	if (resp[1] != 0) {
#ifdef DEBUG
		printf("pmsprobe: type 0x%x\n", resp[1]);
#endif
		return 0;
	}

	return 10;
}

void
pmsattach(struct device *parent, struct device *self, void *aux)
{
	struct pms_softc *sc = (void *)self;
	struct pckbport_attach_args *pa = aux;
	struct wsmousedev_attach_args a;
	u_char cmd[1], resp[2];
	int res;

	sc->sc_kbctag = pa->pa_tag;
	sc->sc_kbcslot = pa->pa_slot;

	printf("\n");

	/* Flush any garbage. */
	pckbport_flush(pa->pa_tag, pa->pa_slot);

	/* reset the device */
	cmd[0] = PMS_RESET;
	res = pckbport_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 2, resp, 1);
#ifdef DEBUG
	if (res || resp[0] != PMS_RSTDONE || resp[1] != 0) {
		printf("pmsattach: reset error\n");
		return;
	}
#endif
	sc->inputstate = 0;
	sc->buttons = 0;
	sc->protocol = PMS_UNKNOWN;

	pckbport_set_inputhandler(sc->sc_kbctag, sc->sc_kbcslot, pmsinput, sc, sc->sc_dev.dv_xname);

	a.accessops = &pms_accessops;
	a.accesscookie = sc;

	/*
	 * Attach the wsmouse, saving a handle to it.
	 * Note that we don't need to check this pointer against NULL
	 * here or in pmsintr, because if this fails pms_enable() will
	 * never be called, so pmsinput() will never be called.
	 */
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);

	/* no interrupts until enabled */
	cmd[0] = PMS_DEV_DISABLE;
	res = pckbport_poll_cmd(pa->pa_tag, pa->pa_slot, cmd, 1, 0, 0, 0);
	if (res)
		printf("pmsattach: disable error\n");
	pckbport_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
}

static void
do_enable(struct pms_softc *sc)
{
	u_char cmd[1];
	int res;

	sc->inputstate = 0;
	sc->buttons = 0;

	pckbport_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 1);

	cmd[0] = PMS_DEV_ENABLE;
	res = pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, cmd, 1, 0, 1, 0);
	if (res)
		printf("pms_enable: command error %d\n", res);

	if (sc->protocol == PMS_UNKNOWN)
		sc->protocol = pms_protocol(sc->sc_kbctag, sc->sc_kbcslot);
	DPRINTF(("pms_enable: using %s protocol\n",
	    pms_protocols[sc->protocol].name));
#if 0
	{
		u_char scmd[2];

		scmd[0] = PMS_SET_RES;
		scmd[1] = 3; /* 8 counts/mm */
		res = pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
		    2, 0, 1, 0);
		if (res)
			printf("pms_enable: setup error1 (%d)\n", res);

		scmd[0] = PMS_SET_SCALE21;
		res = pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
		    1, 0, 1, 0);
		if (res)
			printf("pms_enable: setup error2 (%d)\n", res);

		scmd[0] = PMS_SET_SAMPLE;
		scmd[1] = 100; /* 100 samples/sec */
		res = pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, scmd,
		    2, 0, 1, 0);
		if (res)
			printf("pms_enable: setup error3 (%d)\n", res);
	}
#endif
}

static void
do_disable(struct pms_softc *sc)
{
	u_char cmd[1];
	int res;

	cmd[0] = PMS_DEV_DISABLE;
	res = pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, cmd, 1, 0, 1, 0);
	if (res)
		printf("pms_disable: command error\n");

	pckbport_slot_enable(sc->sc_kbctag, sc->sc_kbcslot, 0);
}

int
pms_enable(void *v)
{
	struct pms_softc *sc = v;
	int s;

	if (sc->sc_enabled)
		return EBUSY;

	do_enable(sc);

	s = spltty();
	sc->sc_enabled = 1;
	splx(s);

	return 0;
}

void
pms_disable(void *v)
{
	struct pms_softc *sc = v;
	int s;

	do_disable(sc);

	s = spltty();
	sc->sc_enabled = 0;
	splx(s);
}

int
pms_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct pms_softc *sc = v;
	u_char kbcmd[2];
	int i;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		break;
		
	case WSMOUSEIO_SRES:
		i = (*(u_int *)data - 12) / 25;
		
		if (i < 0)
			i = 0;
			
		if (i > 3)
			i = 3;

		kbcmd[0] = PMS_SET_RES;
		kbcmd[1] = i;			
		i = pckbport_enqueue_cmd(sc->sc_kbctag, sc->sc_kbcslot, kbcmd, 
		    2, 0, 1, 0);
		
		if (i)
			printf("pms_ioctl: SET_RES command error\n");
		break;
		
	default:
		return ENOIOCTL;
	}
	return 0;
}

/* Masks for the first byte of a packet */
#define PMS_LBUTMASK 0x01
#define PMS_RBUTMASK 0x02
#define PMS_MBUTMASK 0x04
#define PMS_4BUTMASK 0x10
#define PMS_5BUTMASK 0x20

void
pmsinput(void *vsc, int data)
{
	struct pms_softc *sc = vsc;
	u_int changed;
	int dx, dy, dz = 0;
	int newbuttons = 0;
	int s;

	if (!sc->sc_enabled) {
		/* Interrupts are not expected.	 Discard the byte. */
		return;
	}

	s = splclock();
	sc->current = mono_time;
	splx(s);

	if (sc->inputstate > 0) {
		struct timeval diff;

		timersub(&sc->current, &sc->last, &diff);
		/*
		 * Empirically, the delay should be about 1700us on a standard
		 * PS/2 port.  I have seen delays as large as 4500us (rarely)
		 * in regular use.  When using a confused mouse, I generally
		 * see delays at least as large as 30,000us.  -seebs
		 *
		 * The thinkpad trackball returns at 22-23ms. So we use
		 * >= 40ms. In the future, I'll implement adaptable timeout
		 * by increasing the timeout if the mouse reset happens
		 * too frequently -christos
		 */
		if (diff.tv_sec > 0 || diff.tv_usec >= 40000) {
			DPRINTF(("pms_input: unusual delay (%ld.%06ld s), "
			    "scheduling reset\n",
			    (long)diff.tv_sec, (long)diff.tv_usec));
			sc->inputstate = 0;
			sc->sc_enabled = 0;
			wakeup(&sc->sc_enabled);
			return;
		}
	}
	sc->last = sc->current;

	if (sc->inputstate == 0) {
		/*
		 * Some devices (seen on trackballs anytime, and on
		 * some mice shortly after reset) output garbage bytes
		 * between packets.  Just ignore them.
		 */
		if ((data & 0xc0) != 0)
			return;	/* not in sync yet, discard input */
	}

	sc->packet[sc->inputstate++] = data & 0xff;
	switch (sc->inputstate) {
	case 0:
		/* no useful processing can be done yet */
		break;

	case 1:
		/*
		 * Why should we test for bit 0x8 and insist on it here?
		 * The old (psm.c and psm_intelli.c) drivers didn't do
		 * it, and there are devices where it does harm (that's
		 * why it is not used if using PMS_STANDARD protocol).
		 * Anyway, it does not to cause any harm to accept packets
		 * without this bit.
		 */
#if 0
		if (sc->protocol == PMS_STANDARD)
			break;
		if (!(sc->packet[0] & 0x8)) {
			DPRINTF(("pmsinput: 0x8 not set in first byte "
			    "[0x%02x], resetting\n", sc->packet[0]));
			sc->inputstate = 0;
			sc->sc_enabled = 0;
			wakeup(&sc->sc_enabled);
			return;
		}
#endif
		break;

	case 2:
		break;

	case 4:
		/* Case 4 is a superset of case 3. This is *not* an accident. */
		if (sc->protocol == PMS_SCROLL3) {
			dz = sc->packet[3];
			if (dz >= 128)
				dz -= 256;
			if (dz == -128)
				dz = -127;
		} else if (sc->protocol == PMS_SCROLL5) {
			dz = sc->packet[3] & 0xf;
			if (dz >= 8)
				dz -= 16;
                	if (sc->packet[3] & PMS_4BUTMASK)
				newbuttons |= 0x8;
                	if (sc->packet[3] & PMS_5BUTMASK)
				newbuttons |= 0x10;
		} else {
			DPRINTF(("pmsinput: why am I looking at this byte?\n"));
			dz = 0;
		}
		/* FALLTHROUGH */
	case 3:
		/*
		 * This is only an endpoint for scroll protocols with 4
		 * bytes, or the standard protocol with 3.
		 */
		if (sc->protocol != PMS_STANDARD && sc->inputstate == 3)
			break;

		newbuttons |= ((sc->packet[0] & PMS_LBUTMASK) ? 0x1 : 0) |
		    ((sc->packet[0] & PMS_MBUTMASK) ? 0x2 : 0) |
		    ((sc->packet[0] & PMS_RBUTMASK) ? 0x4 : 0);

		dx = sc->packet[1];
		if (dx >= 128)
			dx -= 256;
		if (dx == -128)
			dx = -127;

		dy = sc->packet[2];
		if (dy >= 128)
			dy -= 256;
		if (dy == -128)
			dy = -127;
		
		sc->inputstate = 0;
		changed = (sc->buttons ^ newbuttons);
		sc->buttons = newbuttons;

#ifdef PMSDEBUG
		if (sc->protocol == PMS_STANDARD) {
			DPRINTF(("pms: packet: 0x%02x%02x%02x\n",
			    sc->packet[0], sc->packet[1], sc->packet[2]));
		} else {
			DPRINTF(("pms: packet: 0x%02x%02x%02x%02x\n",
			    sc->packet[0], sc->packet[1], sc->packet[2],
			    sc->packet[3]));
		}
#endif
		if (dx || dy || dz || changed) {
#ifdef PMSDEBUG
			DPRINTF(("pms: x %+03d y %+03d z %+03d "
			    "buttons 0x%02x\n",	dx, dy, dz, sc->buttons));
#endif
			wsmouse_input(sc->sc_wsmousedev, sc->buttons, dx, dy, dz, WSMOUSE_INPUT_DELTA);
		}
		memset(sc->packet, 0, 4);
		break;

	/* If we get here, we have problems. */
	default:
		printf("pmsinput: very confused.  resetting.\n");
		sc->inputstate = 0;
		sc->sc_enabled = 0;
		wakeup(&sc->sc_enabled);
		return;
	}
}
