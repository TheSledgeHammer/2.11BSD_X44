/* $NetBSD: wsdisplay.c,v 1.22.2.1 1999/10/19 16:39:20 he Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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

static const char _copyright[] __attribute__ ((unused)) =
    "Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.";
static const char _rcsid[] __attribute__ ((unused)) =
    "$NetBSD: wsdisplay.c,v 1.22.2.1 1999/10/19 16:39:20 he Exp $";

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>
#include <sys/null.h>

#include <dev/misc/wscons/wsconsio.h>
#include <dev/misc/wscons/wsdisplayvar.h>
#include <dev/misc/wscons/wsksymvar.h>
#include <dev/misc/wscons/wsksymdef.h>
#include <dev/misc/wscons/wsemulvar.h>
#include <dev/misc/wscons/wscons_callbacks.h>
#include <dev/misc/cons/cons.h>

#include "wsdisplay.h"

struct wsscreen_internal {
	const struct wsdisplay_emulops *emulops;
	void						*emulcookie;

	const struct wsscreen_descr *scrdata;

	const struct wsemul_ops 	*wsemul;
	void						*wsemulcookie;
};

struct wsscreen {
	struct wsscreen_internal 	*scr_dconf;

	struct tty 					*scr_tty;
	int							scr_hold_screen;		/* hold tty output */

	int 						scr_flags;
#define SCR_OPEN 				1						/* is it open? */
#define SCR_WAITACTIVE 			2						/* someone waiting on activation */
#define SCR_GRAPHICS 			4						/* graphics mode, no text (emulation) output */
	const struct wscons_syncops *scr_syncops;
	void 						*scr_synccookie;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	int 						scr_rawkbd;
#endif

	struct wsdisplay_softc 		*sc;
};

struct wsscreen *wsscreen_attach (struct wsdisplay_softc *, int, const char *, const struct wsscreen_descr *, void *, int, int, long);
void wsscreen_detach (struct wsscreen *);
static const struct wsscreen_descr *
wsdisplay_screentype_pick (const struct wsscreen_list *, char *);
int wsdisplay_addscreen (struct wsdisplay_softc *, int, char *, char *);
static void wsdisplay_closescreen (struct wsdisplay_softc *, struct wsscreen *);
int wsdisplay_delscreen (struct wsdisplay_softc *, int, int);

#define WSDISPLAY_MAXSCREEN 8

struct wsdisplay_softc {
	struct device sc_dv;

	const struct wsdisplay_accessops *sc_accessops;
	void	*sc_accesscookie;

	const struct wsscreen_list *sc_scrdata;

	struct wsscreen *sc_scr[WSDISPLAY_MAXSCREEN];
	int sc_focusidx;
	struct wsscreen *sc_focus;

	int	sc_isconsole;
	struct device *sc_kbddv;

	int sc_flags;
#define SC_SWITCHPENDING 1
	int sc_screenwanted, sc_oldscreen; /* valid with SC_SWITCHPENDING */

#ifdef WSDISPLAY_COMPAT_RAWKBD
	int sc_rawkbd;
#endif
};

#if NWSDISPLAY > 0
extern struct cfdriver wsdisplay_cd;
#endif /* NWSDISPLAY > 0 */

/* Autoconfiguration definitions. */
static int wsdisplay_emul_match (struct device *, struct cfdata *, void *);
static void wsdisplay_emul_attach (struct device *, struct device *, void *);
static int wsdisplay_noemul_match (struct device *, struct cfdata *, void *);
static void wsdisplay_noemul_attach (struct device *, struct device *, void *);

struct cfdriver wsdisplay_emul_cd = {
		NULL,
		"wsdisplay_emul",
		wsdisplay_emul_match,
		wsdisplay_emul_attach,
		DV_DULL,
		sizeof (struct wsdisplay_softc),
};

struct cfdriver wsdisplay_emul_cd = {
		NULL,
		"wsdisplay_noemul",
		wsdisplay_noemul_match,
		wsdisplay_noemul_attach,
		DV_DULL,
		sizeof (struct wsdisplay_softc),
};

/* Exported tty- and cdevsw-related functions. */
dev_type_open(wsdisplayopen);
dev_type_close(wsdisplayclose);
dev_type_read(wsdisplayread);
dev_type_write(wsdisplaywrite);
dev_type_ioctl(wsdisplayioctl);
dev_type_stop(wsdisplaystop);
dev_type_tty(wsdisplaytty);
dev_type_poll(wsdisplaypoll);
dev_type_mmap(wsdisplaymmap);

const struct cdevsw wsdisplay_cdevsw = {
	.d_open = wsdisplayopen,
	.d_close = wsdisplayclose,
	.d_read = wsdisplayread,
	.d_write = wsdisplaywrite,
	.d_ioctl = wsdisplayioctl,
	.d_stop = wsdisplaystop,
	.d_tty = wsdisplaytty,
	.d_poll = wsdisplaypoll,
	.d_mmap = wsdisplaymmap,
	.d_discard = nodiscard,
	.d_type = D_TTY
};

#if NWSDISPLAY > 0
static void wsdisplaystart (struct tty *);
static int wsdisplayparam (struct tty *, struct termios *);
#endif /* NWSDISPLAY > 0 */


/* Internal macros, functions, and variables. */
#define	SET(t, f)	(t) |= (f)
#define	CLR(t, f)	(t) &= ~(f)
#define	ISSET(t, f)	((t) & (f))

#define	WSDISPLAYUNIT(dev)				(minor(dev) >> 8)
#define	WSDISPLAYSCREEN(dev)			(minor(dev) & 0xff)
#define ISWSDISPLAYCTL(dev)				(WSDISPLAYSCREEN(dev) == 255)
#define WSDISPLAYMINOR(unit, screen)	(((unit) << 8) | (screen))

#define	WSSCREEN_HAS_EMULATOR(scr)		((scr)->scr_dconf->wsemul != NULL)
#define	WSSCREEN_HAS_TTY(scr)			((scr)->scr_tty != NULL)

static void wsdisplay_common_attach (struct wsdisplay_softc *sc, int console, const struct wsscreen_list *, const struct wsdisplay_accessops *accessops, void *accesscookie);

#ifdef WSDISPLAY_COMPAT_RAWKBD
int wsdisplay_update_rawkbd (struct wsdisplay_softc *, struct wsscreen *);
#endif

static int wsdisplay_console_initted;
static struct wsdisplay_softc *wsdisplay_console_device;
static struct wsscreen_internal wsdisplay_console_conf;

static int wsdisplay_getc_dummy (dev_t);
static void wsdisplay_pollc_dummy (dev_t, int);

static struct consdev wsdisplay_cons = {
		.cn_probe = NULL,
		.cn_init = NULL,
		.cn_getc = wsdisplay_getc_dummy,
		.cn_putc = wsdisplay_cnputc,
		.cn_pollc = wsdisplay_pollc_dummy,
		.cn_dev = NODEV,
		.cn_pri = CN_NORMAL
};

int wsdisplay_switch1 (void *, int, int);
int wsdisplay_switch3 (void *, int, int);

int wsdisplay_clearonclose;

struct wsscreen *
wsscreen_attach(sc, console, emul, type, cookie, ccol, crow, defattr)
	struct wsdisplay_softc *sc;
	int console;
	const char *emul;
	const struct wsscreen_descr *type;
	void *cookie;
	int ccol, crow;
	long defattr;
{
	struct wsscreen_internal *dconf;
	struct wsscreen *scr;

	scr = malloc(sizeof(struct wsscreen), M_DEVBUF, M_WAITOK);
	if (!scr)
		return (NULL);

	if (console) {
		dconf = &wsdisplay_console_conf;
		/*
		 * If there's an emulation, tell it about the callback argument.
		 * The other stuff is already there.
		 */
		if (dconf->wsemul != NULL)
			(*dconf->wsemul->attach)(1, 0, 0, 0, 0, scr, 0);
	} else { /* not console */
		dconf = malloc(sizeof(struct wsscreen_internal),
		M_DEVBUF, M_NOWAIT);
		dconf->emulops = type->textops;
		dconf->emulcookie = cookie;
		if (dconf->emulops) {
			dconf->wsemul = wsemul_pick(emul);
			if (dconf->wsemul == NULL) {
				free(dconf, M_DEVBUF);
				free(scr, M_DEVBUF);
				return (NULL);
			}
			dconf->wsemulcookie = (*dconf->wsemul->attach)(0, type, cookie,
					ccol, crow, scr, defattr);
		} else
			dconf->wsemul = NULL;
		dconf->scrdata = type;
	}

	scr->scr_dconf = dconf;

	scr->scr_tty = ttymalloc();

	scr->scr_hold_screen = 0;
	if (WSSCREEN_HAS_EMULATOR(scr))
		scr->scr_flags = 0;
	else
		scr->scr_flags = SCR_GRAPHICS;

	scr->scr_syncops = 0;
	scr->sc = sc;
#ifdef WSDISPLAY_COMPAT_RAWKBD
	scr->scr_rawkbd = 0;
#endif
	return (scr);
}

void
wsscreen_detach(scr)
	struct wsscreen *scr;
{
	int ccol, crow; /* XXX */

	if (WSSCREEN_HAS_TTY(scr)) {
		ttyfree(scr->scr_tty);
	}
	if (WSSCREEN_HAS_EMULATOR(scr))
		(*scr->scr_dconf->wsemul->detach)(scr->scr_dconf->wsemulcookie, &ccol,
				&crow);
	free(scr->scr_dconf, M_DEVBUF);
	free(scr, M_DEVBUF);
}

static const struct wsscreen_descr *
wsdisplay_screentype_pick(scrdata, name)
	const struct wsscreen_list *scrdata;
	char *name;
{
	int i;
	const struct wsscreen_descr *scr;

	KASSERT(scrdata->nscreens > 0);

	if (name == NULL)
		return (scrdata->screens[0]);

	for (i = 0; i < scrdata->nscreens; i++) {
		scr = scrdata->screens[i];
		if (!strcmp(name, scr->name))
			return (scr);
	}

	return (0);
}

int
wsdisplay_addscreen(sc, idx, screentype, emul)
	struct wsdisplay_softc *sc;
	int idx;
	char *screentype, *emul;
{
	const struct wsscreen_descr *scrdesc;
	int error;
	void *cookie;
	int ccol, crow;
	long defattr;
	struct wsscreen *scr;
	int s;

	if (idx < 0 || idx >= WSDISPLAY_MAXSCREEN)
		return (EINVAL);
	if (sc->sc_scr[idx] != NULL)
		return (EBUSY);

	scrdesc = wsdisplay_screentype_pick(sc->sc_scrdata, screentype);
	if (!scrdesc)
		return (ENXIO);
	error = (*sc->sc_accessops->alloc_screen)(sc->sc_accesscookie, scrdesc,
			&cookie, &ccol, &crow, &defattr);
	if (error)
		return (error);

	scr = wsscreen_attach(sc, 0, emul, scrdesc, cookie, ccol, crow, defattr);
	if (scr == NULL) {
		(*sc->sc_accessops->free_screen)(sc->sc_accesscookie, cookie);
		return (ENXIO);
	}

	sc->sc_scr[idx] = scr;

	printf("%s: screen %d added (%s", sc->sc_dv.dv_xname, idx, scrdesc->name);
	if (WSSCREEN_HAS_EMULATOR(scr))
		printf(", %s emulation", scr->scr_dconf->wsemul->name);
	printf(")\n");

	/* if no screen has focus yet, activate the first we get */
	s = spltty();
	if (!sc->sc_focus) {
		(*sc->sc_accessops->show_screen)(sc->sc_accesscookie,
				scr->scr_dconf->emulcookie);
		sc->sc_focusidx = idx;
		sc->sc_focus = scr;
	}
	splx(s);
	return (0);
}

static void
wsdisplay_closescreen(sc, scr)
	struct wsdisplay_softc *sc;
	struct wsscreen *scr;
{
	int maj, mn, idx;

	/* hangup */
	if (WSSCREEN_HAS_TTY(scr)) {
		struct tty *tp = scr->scr_tty;
		(*linesw[tp->t_line].l_modem)(tp, 0);
	}

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == wsdisplayopen)
			break;
	/* locate the screen index */
	for (idx = 0; idx < WSDISPLAY_MAXSCREEN; idx++)
		if (scr == sc->sc_scr[idx])
			break;
#ifdef DIAGNOSTIC
	if (idx == WSDISPLAY_MAXSCREEN)
		panic("wsdisplay_forceclose: bad screen");
#endif

	/* nuke the vnodes */
	mn = WSDISPLAYMINOR(sc->sc_dv.dv_unit, idx);
	vdevgone(maj, mn, mn, VCHR);
}

int
wsdisplay_delscreen(sc, idx, flags)
	struct wsdisplay_softc *sc;
	int idx, flags;
{
	struct wsscreen *scr;
	int s;
	void *cookie;

	if (idx < 0 || idx >= WSDISPLAY_MAXSCREEN)
		return (EINVAL);
	scr = sc->sc_scr[idx];
	if (!scr)
		return (ENXIO);

	if (scr->scr_dconf == &wsdisplay_console_conf ||
	    scr->scr_syncops ||
	    ((scr->scr_flags & SCR_OPEN) && !(flags & WSDISPLAY_DELSCR_FORCE)))
		return(EBUSY);

	wsdisplay_closescreen(sc, scr);

	/*
	 * delete pointers, so neither device entries
	 * nor keyboard input can reference it anymore
	 */
	s = spltty();
	if (sc->sc_focus == scr) {
		sc->sc_focus = 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
		wsdisplay_update_rawkbd(sc, 0);
#endif
	}
	sc->sc_scr[idx] = 0;
	splx(s);

	/*
	 * Wake up processes waiting for the screen to
	 * be activated. Sleepers must check whether
	 * the screen still exists.
	 */
	if (scr->scr_flags & SCR_WAITACTIVE)
		wakeup(scr);

	/* save a reference to the graphics screen */
	cookie = scr->scr_dconf->emulcookie;

	wsscreen_detach(scr);

	(*sc->sc_accessops->free_screen)(sc->sc_accesscookie,
					 cookie);

	printf("%s: screen %d deleted\n", sc->sc_dv.dv_xname, idx);
	return (0);
}

/*
 * Autoconfiguration functions.
 */
int
wsdisplay_emul_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct wsemuldisplaydev_attach_args *ap = aux;

	if (match->cf_loc[WSEMULDISPLAYDEVCF_CONSOLE] != WSEMULDISPLAYDEVCF_CONSOLE_UNK) {
		/*
		 * If console-ness of device specified, either match
		 * exactly (at high priority), or fail.
		 */
		if (match->cf_loc[WSEMULDISPLAYDEVCF_CONSOLE] != 0 && ap->console != 0)
			return (10);
		else
			return (0);
	}

	/* If console-ness unspecified, it wins. */
	return (1);
}

void
wsdisplay_emul_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)self;
	struct wsemuldisplaydev_attach_args *ap = aux;

	wsdisplay_common_attach(sc, ap->console, ap->scrdata,
				ap->accessops, ap->accesscookie);

	if (ap->console) {
		int maj;

		/* locate the major number */
		for (maj = 0; maj < nchrdev; maj++)
			if (cdevsw[maj].d_open == wsdisplayopen)
				break;

		cn_tab->cn_dev = makedev(maj, WSDISPLAYMINOR(self->dv_unit, 0));
	}
}

/* Print function (for parent devices). */
int
wsemuldisplaydevprint(aux, pnp)
	void *aux;
	const char *pnp;
{
#if 0 /* -Wunused */
	struct wsemuldisplaydev_attach_args *ap = aux;
#endif

	if (pnp)
		printf("wsdisplay at %s", pnp);
#if 0 /* don't bother; it's ugly */
	printf(" console %d", ap->console);
#endif

	return (UNCONF);
}

int
wsdisplay_noemul_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
#if 0 /* -Wunused */
	struct wsdisplaydev_attach_args *ap = aux;
#endif

	/* Always match. */
	return (1);
}

void
wsdisplay_noemul_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)self;
	struct wsdisplaydev_attach_args *ap = aux;

	wsdisplay_common_attach(sc, 0, NULL, ap->accessops, ap->accesscookie);
}

/* Print function (for parent devices). */
int
wsdisplaydevprint(aux, pnp)
	void *aux;
	const char *pnp;
{
#if 0 /* -Wunused */
	struct wsdisplaydev_attach_args *ap = aux;
#endif

	if (pnp)
		printf("wsdisplay at %s", pnp);

	return (UNCONF);
}

static void
wsdisplay_common_attach(sc, console, scrdata, accessops, accesscookie)
	struct wsdisplay_softc *sc;
	int console;
	const struct wsscreen_list *scrdata;
	const struct wsdisplay_accessops *accessops;
	void *accesscookie;
{
	int i = 0;

	if (console) {
		KASSERT(wsdisplay_console_initted);
		KASSERT(wsdisplay_console_device == NULL);

		sc->sc_scr[0] = wsscreen_attach(sc, 1, 0, 0, 0, 0, 0, 0);
		wsdisplay_console_device = sc;

		printf(": console (%s, %s emulation)",
		       wsdisplay_console_conf.scrdata->name,
		       wsdisplay_console_conf.wsemul->name);

		sc->sc_focusidx = 0;
		sc->sc_focus = sc->sc_scr[0];
		i++;
	}

	printf("\n");

	sc->sc_accessops = accessops;
	sc->sc_accesscookie = accesscookie;
	sc->sc_scrdata = scrdata;

	sc->sc_isconsole = console;
	sc->sc_kbddv = NULL;

	wscons_glue_set_callback();

	/*
	 * Set up a number of virtual screens if possible. The
	 * WSDISPLAYIO_ADDSCREEN ioctl is more flexible, so this code
	 * should go away as soon as we have a userspace utility.
	 */
#ifdef WSDISPLAY_DEFAULTSCREENS
	for (; i < WSDISPLAY_DEFAULTSCREENS; i++)
		if (wsdisplay_addscreen(sc, i, 0, 0))
			break;
#endif
}

void
wsdisplay_cnattach(type, cookie, ccol, crow, defattr)
	const struct wsscreen_descr *type;
	void *cookie;
	int ccol, crow;
	long defattr;
{
	const struct wsemul_ops *wsemul;

	KASSERT(!wsdisplay_console_initted);
	KASSERT(type->nrows > 0);
	KASSERT(type->ncols > 0);
	KASSERT(crow < type->nrows);
	KASSERT(ccol < type->ncols);

	wsdisplay_console_conf.emulops = type->textops;
	wsdisplay_console_conf.emulcookie = cookie;
	wsdisplay_console_conf.scrdata = type;

	wsemul = wsemul_pick(0); /* default */
	wsdisplay_console_conf.wsemul = wsemul;
	wsdisplay_console_conf.wsemulcookie = (*wsemul->cnattach)(type, cookie,
								  ccol, crow,
								  defattr);

	cn_tab = &wsdisplay_cons;

	wsdisplay_console_initted = 1;
}

/*
 * Tty and cdevsw functions.
 */
int
wsdisplayopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
#if NWSDISPLAY > 0
	struct wsdisplay_softc *sc;
	struct tty *tp;
	int unit, newopen, error;
	struct wsscreen *scr;

	unit = WSDISPLAYUNIT(dev);
	if (unit >= wsdisplay_cd.cd_ndevs ||	/* make sure it was attached */
	    (sc = wsdisplay_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	if (ISWSDISPLAYCTL(dev))
		return (0);

	if (WSDISPLAYSCREEN(dev) >= WSDISPLAY_MAXSCREEN)
		return (ENXIO);
	scr = sc->sc_scr[WSDISPLAYSCREEN(dev)];
	if (!scr)
		return (ENXIO);

	if (WSSCREEN_HAS_TTY(scr)) {
		tp = scr->scr_tty;
		tp->t_oproc = wsdisplaystart;
		tp->t_param = wsdisplayparam;
		tp->t_dev = dev;
		newopen = (tp->t_state & TS_ISOPEN) == 0;
		if (newopen) {
			ttychars(tp);
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
			wsdisplayparam(tp, &tp->t_termios);
			ttsetwater(tp);
		} else if ((tp->t_state & TS_XCLUDE) != 0 &&
			   p->p_ucred->cr_uid != 0)
			return EBUSY;
		tp->t_state |= TS_CARR_ON;

		error = ((*linesw[tp->t_line].l_open)(dev, tp));
		if (error)
			return (error);

		if (newopen && WSSCREEN_HAS_EMULATOR(scr)) {
			/* set window sizes as appropriate, and reset
			 the emulation */
			tp->t_winsize.ws_row = scr->scr_dconf->scrdata->nrows;
			tp->t_winsize.ws_col = scr->scr_dconf->scrdata->ncols;

			/* wsdisplay_set_emulation() */
		}
	}

	scr->scr_flags |= SCR_OPEN;
	return (0);
#else
	return (ENXIO);
#endif /* NWSDISPLAY > 0 */
}

int
wsdisplayclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
#if NWSDISPLAY > 0
	struct wsdisplay_softc *sc;
	struct tty *tp;
	int unit;
	struct wsscreen *scr;

	unit = WSDISPLAYUNIT(dev);
	sc = wsdisplay_cd.cd_devs[unit];

	if (ISWSDISPLAYCTL(dev))
		return (0);

	scr = sc->sc_scr[WSDISPLAYSCREEN(dev)];

	if (WSSCREEN_HAS_TTY(scr)) {
		if (scr->scr_hold_screen) {
			int s;

			/* XXX RESET KEYBOARD LEDS, etc. */
			s = spltty();	/* avoid conflict with keyboard */
			wsdisplay_kbdholdscreen((struct device *)sc, 0);
			splx(s);
		}
		tp = scr->scr_tty;
		(*linesw[tp->t_line].l_close)(tp, flag);
		ttyclose(tp);
	}

	if (scr->scr_syncops)
		(*scr->scr_syncops->destroy)(scr->scr_synccookie);

	if (WSSCREEN_HAS_EMULATOR(scr)) {
		scr->scr_flags &= ~SCR_GRAPHICS;
		(*scr->scr_dconf->wsemul->reset)(scr->scr_dconf->wsemulcookie,
						 WSEMUL_RESET);
		if (wsdisplay_clearonclose)
			(*scr->scr_dconf->wsemul->reset)
				(scr->scr_dconf->wsemulcookie,
				 WSEMUL_CLEARSCREEN);
	}

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (scr->scr_rawkbd) {
		int kbmode = WSKBD_TRANSLATED;
		(void) wsdisplay_internal_ioctl(sc, scr, WSKBDIO_SETMODE,
						(caddr_t)&kbmode, 0, p);
	}
#endif

	scr->scr_flags &= ~SCR_OPEN;

	return (0);
#else
	return (ENXIO);
#endif /* NWSDISPLAY > 0 */
}

int
wsdisplayread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
#if NWSDISPLAY > 0
	struct wsdisplay_softc *sc;
	struct tty *tp;
	int unit;
	struct wsscreen *scr;

	unit = WSDISPLAYUNIT(dev);
	sc = wsdisplay_cd.cd_devs[unit];

	if (ISWSDISPLAYCTL(dev))
		return (0);

	scr = sc->sc_scr[WSDISPLAYSCREEN(dev)];

	if (!WSSCREEN_HAS_TTY(scr))
		return (ENODEV);

	tp = scr->scr_tty;
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
#else
	return (ENXIO);
#endif /* NWSDISPLAY > 0 */
}

int
wsdisplaywrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
#if NWSDISPLAY > 0
	struct wsdisplay_softc *sc;
	struct tty *tp;
	int unit;
	struct wsscreen *scr;

	unit = WSDISPLAYUNIT(dev);
	sc = wsdisplay_cd.cd_devs[unit];

	if (ISWSDISPLAYCTL(dev))
		return (0);

	scr = sc->sc_scr[WSDISPLAYSCREEN(dev)];

	if (!WSSCREEN_HAS_TTY(scr))
		return (ENODEV);

	tp = scr->scr_tty;
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
#else
	return (ENXIO);
#endif /* NWSDISPLAY > 0 */
}

struct tty *
wsdisplaytty(dev)
	dev_t dev;
{
#if NWSDISPLAY > 0
	struct wsdisplay_softc *sc;
	int unit;
	struct wsscreen *scr;

	unit = WSDISPLAYUNIT(dev);
	sc = wsdisplay_cd.cd_devs[unit];

	if (ISWSDISPLAYCTL(dev))
		panic("wsdisplaytty() on ctl device");

	scr = sc->sc_scr[WSDISPLAYSCREEN(dev)];

	return (scr->scr_tty);
#else
	return (NULL);
#endif /* NWSDISPLAY > 0 */
}

int
wsdisplayioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
#if NWSDISPLAY > 0
	struct wsdisplay_softc *sc;
	struct tty *tp;
	int unit, error;
	struct wsscreen *scr;

	unit = WSDISPLAYUNIT(dev);
	sc = wsdisplay_cd.cd_devs[unit];

#ifdef WSDISPLAY_COMPAT_USL
	error = wsdisplay_usl_ioctl1(sc, cmd, data, flag, p);
	if (error >= 0)
		return (error);
#endif

	if (ISWSDISPLAYCTL(dev))
		return (wsdisplay_cfg_ioctl(sc, cmd, data, flag, p));

	scr = sc->sc_scr[WSDISPLAYSCREEN(dev)];

	if (WSSCREEN_HAS_TTY(scr)) {
		tp = scr->scr_tty;

/* printf("disc\n"); */
		/* do the line discipline ioctls first */
		error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
		if (error >= 0)
			return error;

/* printf("tty\n"); */
		/* then the tty ioctls */
		error = ttioctl(tp, cmd, data, flag, p);
		if (error >= 0)
			return error;
	}

#ifdef WSDISPLAY_COMPAT_USL
	error = wsdisplay_usl_ioctl2(sc, scr, cmd, data, flag, p);
	if (error >= 0)
		return (error);
#endif

	error = wsdisplay_internal_ioctl(sc, scr, cmd, data, flag, p);
	return (error != -1 ? error : ENOTTY);
#else
	return (ENXIO);
#endif /* NWSDISPLAY > 0 */
}

int
wsdisplay_internal_ioctl(sc, scr, cmd, data, flag, p)
	struct wsdisplay_softc *sc;
	struct wsscreen *scr;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error;
	char namebuf[16];
	struct wsdisplay_font fd;

	if (sc->sc_kbddv != NULL) {
		/* check ioctls for keyboard */
#ifdef WSDISPLAY_COMPAT_RAWKBD
		switch (cmd) {
		    case WSKBDIO_SETMODE:
			scr->scr_rawkbd = (*(int *)data == WSKBD_RAW);
			return (wsdisplay_update_rawkbd(sc, scr));
		    case WSKBDIO_GETMODE:
			*(int *)data = (scr->scr_rawkbd ?
					WSKBD_RAW : WSKBD_TRANSLATED);
			return (0);
		}
#endif
/* printf("kbdcallback\n"); */
		error = wskbd_displayioctl(sc->sc_kbddv, cmd, data, flag, p);
		if (error >= 0)
			return error;
	}

/* printf("display\n"); */
	switch (cmd) {
	case WSDISPLAYIO_GMODE:
		*(u_int *)data = (scr->scr_flags & SCR_GRAPHICS ?
				  WSDISPLAYIO_MODE_MAPPED :
				  WSDISPLAYIO_MODE_EMUL);
		return (0);

	case WSDISPLAYIO_SMODE:
#define d (*(int *)data)
		if (d != WSDISPLAYIO_MODE_EMUL &&
		    d != WSDISPLAYIO_MODE_MAPPED)
			return (EINVAL);

	    if (WSSCREEN_HAS_EMULATOR(scr)) {
		    scr->scr_flags &= ~SCR_GRAPHICS;
		    if (d == WSDISPLAYIO_MODE_MAPPED)
			    scr->scr_flags |= SCR_GRAPHICS;
	    } else if (d == WSDISPLAYIO_MODE_EMUL)
		    return (EINVAL);
	    return (0);
#undef d

	case WSDISPLAYIO_USEFONT:
#define d ((struct wsdisplay_usefontdata *)data)
		if (!sc->sc_accessops->load_font)
			return (EINVAL);
		if (d->name) {
			error = copyinstr(d->name, namebuf, sizeof(namebuf), 0);
			if (error)
				return (error);
			fd.name = namebuf;
		} else
			fd.name = 0;
		fd.data = 0;
		error = (*sc->sc_accessops->load_font)(sc->sc_accesscookie,
					scr->scr_dconf->emulcookie, &fd);
		if (!error && WSSCREEN_HAS_EMULATOR(scr))
			(*scr->scr_dconf->wsemul->reset)
				(scr->scr_dconf->wsemulcookie, WSEMUL_SYNCFONT);
		return (error);
#undef d
	}

	/* check ioctls for display */
	return ((*sc->sc_accessops->ioctl)(sc->sc_accesscookie, cmd, data,
	    flag, p));
}

int
wsdisplay_cfg_ioctl(sc, cmd, data, flag, p)
	struct wsdisplay_softc *sc;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int error;
	char *type, typebuf[16], *emul, emulbuf[16];
	void *buf;

	switch (cmd) {
	case WSDISPLAYIO_ADDSCREEN:
#define d ((struct wsdisplay_addscreendata *)data)
		if (d->screentype) {
			error = copyinstr(d->screentype, typebuf,
					  sizeof(typebuf), 0);
			if (error)
				return (error);
			type = typebuf;
		} else
			type = 0;
		if (d->emul) {
			error = copyinstr(d->emul, emulbuf, sizeof(emulbuf), 0);
			if (error)
				return (error);
			emul = emulbuf;
		} else
			emul = 0;

		return (wsdisplay_addscreen(sc, d->idx, type, emul));
#undef d
	case WSDISPLAYIO_DELSCREEN:
#define d ((struct wsdisplay_delscreendata *)data)
		return (wsdisplay_delscreen(sc, d->idx, d->flags));
#undef d
	case WSDISPLAYIO_LDFONT:
#define d ((struct wsdisplay_font *)data)
		if (!sc->sc_accessops->load_font)
			return (EINVAL);
		if (d->name) {
			error = copyinstr(d->name, typebuf, sizeof(typebuf), 0);
			if (error)
				return (error);
			d->name = typebuf;
		} else
			d->name = "loaded"; /* ??? */
		buf = malloc(d->fontheight * d->stride * d->numchars,
			     M_DEVBUF, M_WAITOK);
		error = copyin(d->data, buf,
			       d->fontheight * d->stride * d->numchars);
		if (error) {
			free(buf, M_DEVBUF);
			return (error);
		}
		d->data = buf;
		error =
		  (*sc->sc_accessops->load_font)(sc->sc_accesscookie, 0, d);
		free(buf, M_DEVBUF);
#undef d
		return (error);
	}
	return (EINVAL);
}

int
wsdisplaymmap(dev, offset, prot)
	dev_t dev;
	int offset;		/* XXX */
	int prot;
{
#if NWSDISPLAY > 0
	struct wsdisplay_softc *sc = wsdisplay_cd.cd_devs[WSDISPLAYUNIT(dev)];
	struct wsscreen *scr;

	if (ISWSDISPLAYCTL(dev))
		return (-1);

	scr = sc->sc_scr[WSDISPLAYSCREEN(dev)];

	if (!(scr->scr_flags & SCR_GRAPHICS))
		return (-1);

	/* pass mmap to display */
	return ((*sc->sc_accessops->mmap)(sc->sc_accesscookie, offset, prot));
#else
	return (-1);
#endif /* NWSDISPLAY > 0 */
}

int
wsdisplaypoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
#if NWSDISPLAY > 0
	struct wsdisplay_softc *sc = wsdisplay_cd.cd_devs[WSDISPLAYUNIT(dev)];
	struct wsscreen *scr;

	if (ISWSDISPLAYCTL(dev))
		return (0);

	scr = sc->sc_scr[WSDISPLAYSCREEN(dev)];

	if (WSSCREEN_HAS_TTY(scr))
		return (ttpoll(dev, events, p));
	else
		return (0);
#else
	return (0);
#endif /* NWSDISPLAY > 0 */
}

#if NWSDISPLAY > 0
void
wsdisplaystart(tp)
	register struct tty *tp;
{
	struct wsdisplay_softc *sc;
	struct wsscreen *scr;
	register int s, n;
	u_char *buf;
		
	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)) {
		splx(s);
		return;
	}
	sc = wsdisplay_cd.cd_devs[WSDISPLAYUNIT(tp->t_dev)];
	scr = sc->sc_scr[WSDISPLAYSCREEN(tp->t_dev)];
	if (scr->scr_hold_screen) {
		tp->t_state |= TS_TIMEOUT;
		splx(s);
		return;
	}
	tp->t_state |= TS_BUSY; 
	splx(s);
	
	/*
	 * Drain output from ring buffer.
	 * The output will normally be in one contiguous chunk, but when the 
	 * ring wraps, it will be in two pieces.. one at the end of the ring, 
	 * the other at the start.  For performance, rather than loop here, 
	 * we output one chunk, see if there's another one, and if so, output 
	 * it too.
	 */

	n = ndqb(&tp->t_outq, 0);
	buf = tp->t_outq.c_cf;

	if (!(scr->scr_flags & SCR_GRAPHICS)) {
		KASSERT(WSSCREEN_HAS_EMULATOR(scr));
		(*scr->scr_dconf->wsemul->output)(scr->scr_dconf->wsemulcookie,
						  buf, n, 0);
	}
	ndflush(&tp->t_outq, n);

	if ((n = ndqb(&tp->t_outq, 0)) > 0) {
		buf = tp->t_outq.c_cf;

		if (!(scr->scr_flags & SCR_GRAPHICS)) {
			KASSERT(WSSCREEN_HAS_EMULATOR(scr));
			(*scr->scr_dconf->wsemul->output)(scr->scr_dconf->wsemulcookie,
							  buf, n, 0);
		}
		ndflush(&tp->t_outq, n);
	}

	s = spltty();
	tp->t_state &= ~TS_BUSY;
	/* Come back if there's more to do */
	if (tp->t_outq.c_cc) {
		tp->t_state |= TS_TIMEOUT;
		timeout(ttrstrt, tp, (hz > 128) ? (hz / 128) : 1);
	}
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	splx(s);
}
#endif /* NWSDISPLAY > 0 */

void
wsdisplaystop(tp, flag)
	struct tty *tp;
	int flag;
{
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY))
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	splx(s);
}

#if NWSDISPLAY > 0
/* Set line parameters. */
int
wsdisplayparam(tp, t)
	struct tty *tp;
	struct termios *t;
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}
#endif /* NWSDISPLAY > 0 */

/*
 * Callbacks for the emulation code.
 */
void
wsdisplay_emulbell(v)
	void *v;
{
	struct wsscreen *scr = v;

	if (scr == NULL)		/* console, before real attach */
		return;

	if (scr->scr_flags & SCR_GRAPHICS) /* can this happen? */
		return;

	(void) wsdisplay_internal_ioctl(scr->sc, scr, WSKBDIO_BELL, NULL, FWRITE, NULL);
}

void
wsdisplay_emulinput(v, data, count)
	void *v;
	const u_char *data;
	u_int count;
{
	struct wsscreen *scr = v;
	struct tty *tp;

	if (v == NULL)			/* console, before real attach */
		return;

	if (scr->scr_flags & SCR_GRAPHICS) /* XXX can't happen */
		return;
	if (!WSSCREEN_HAS_TTY(scr))
		return;

	tp = scr->scr_tty;
	while (count-- > 0)
		(*linesw[tp->t_line].l_rint)(*data++, tp);
};

/*
 * Calls from the keyboard interface.
 */
void
wsdisplay_kbdinput(dev, ks)
	struct device *dev;
	keysym_t ks;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	struct wsscreen *scr;
	char *dp;
	int count;
	struct tty *tp;

	KASSERT(sc != NULL);

	scr = sc->sc_focus;

	if (!scr || !WSSCREEN_HAS_TTY(scr))
		return;

	tp = scr->scr_tty;

	if (KS_GROUP(ks) == KS_GROUP_Ascii)
		(*linesw[tp->t_line].l_rint)(KS_VALUE(ks), tp);
	else if (WSSCREEN_HAS_EMULATOR(scr)) {
		count = (*scr->scr_dconf->wsemul->translate)
		    (scr->scr_dconf->wsemulcookie, ks, &dp);
		while (count-- > 0)
			(*linesw[tp->t_line].l_rint)(*dp++, tp);
	}
}

#ifdef WSDISPLAY_COMPAT_RAWKBD
int
wsdisplay_update_rawkbd(sc, scr)
	struct wsdisplay_softc *sc;
	struct wsscreen *scr;
{
	int s, raw, data, error;
	s = spltty();

	raw = (scr ? scr->scr_rawkbd : 0);

	if (!sc->sc_kbddv ||
	    scr != sc->sc_focus ||
	    sc->sc_rawkbd == raw) {
		splx(s);
		return (0);
	}

	data = (raw ? WSKBD_RAW : WSKBD_TRANSLATED);
	error = wskbd_displayioctl(sc->sc_kbddv, WSKBDIO_SETMODE,
				   (caddr_t)&data, 0, 0);
	if (!error)
		sc->sc_rawkbd = raw;
	splx(s);
	return (error);
}
#endif

int
wsdisplay_switch3(arg, error, waitok)
	void *arg;
	int error, waitok;
{
	struct wsdisplay_softc *sc = arg;
	int no;
	struct wsscreen *scr;

	if (!(sc->sc_flags & SC_SWITCHPENDING)) {
		printf("wsdisplay_switch3: not switching\n");
		return (EINVAL);
	}

	no = sc->sc_screenwanted;
	if (no < 0 || no >= WSDISPLAY_MAXSCREEN)
		panic("wsdisplay_switch3: invalid screen %d", no);
	scr = sc->sc_scr[no];
	if (!scr) {
		printf("wsdisplay_switch3: screen %d disappeared\n", no);
		error = ENXIO;
	}

	if (error) {
		/* try to recover, avoid recursion */

		if (sc->sc_oldscreen == -1) {
			printf("wsdisplay_switch3: giving up\n");
			sc->sc_focus = 0;
#ifdef WSDISPLAY_COMPAT_RAWKBD
			wsdisplay_update_rawkbd(sc, 0);
#endif
			sc->sc_flags &= ~SC_SWITCHPENDING;
			return (error);
		}

		sc->sc_screenwanted = sc->sc_oldscreen;
		sc->sc_oldscreen = -1;
		return (wsdisplay_switch1(arg, 0, waitok));
	}

	sc->sc_flags &= ~SC_SWITCHPENDING;

	if (!error && (scr->scr_flags & SCR_WAITACTIVE))
		wakeup(scr);
	return (error);
}

int
wsdisplay_switch1(arg, error, waitok)
	void *arg;
	int error, waitok;
{
	struct wsdisplay_softc *sc = arg;
	int no;
	struct wsscreen *scr;

	if (!(sc->sc_flags & SC_SWITCHPENDING)) {
		printf("wsdisplay_switch1: not switching\n");
		return (EINVAL);
	}

	no = sc->sc_screenwanted;
	if (no < 0 || no >= WSDISPLAY_MAXSCREEN)
		panic("wsdisplay_switch1: invalid screen %d", no);
	scr = sc->sc_scr[no];
	if (!scr) {
		printf("wsdisplay_switch1: screen %d disappeared\n", no);
		error = ENXIO;
	}

	if (error) {
		sc->sc_flags &= ~SC_SWITCHPENDING;
		return (error);
	}

	(*sc->sc_accessops->show_screen)(sc->sc_accesscookie,
					 scr->scr_dconf->emulcookie);
	sc->sc_focusidx = no;
	sc->sc_focus = scr;

#ifdef WSDISPLAY_COMPAT_RAWKBD
	(void) wsdisplay_update_rawkbd(sc, scr);
#endif
	/* keyboard map??? */

#define wsswitch_cb3 ((void (*) __P((void *, int, int)))wsdisplay_switch3)
	if (scr->scr_syncops) {
		error = (*scr->scr_syncops->attach)(scr->scr_synccookie, waitok,
						    wsswitch_cb3, sc);
		if (error == EAGAIN) {
			/* switch will be done asynchronously */
			return (0);
		}
	}

	return (wsdisplay_switch3(sc, error, waitok));
}

int
wsdisplay_switch(dev, no, waitok)
	struct device *dev;
	int no, waitok;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	int s, res = 0;
	struct wsscreen *scr;

	if (no < 0 || no >= WSDISPLAY_MAXSCREEN || !sc->sc_scr[no])
		return (ENXIO);

	s = spltty();

	if (sc->sc_focus && no == sc->sc_focusidx) {
		splx(s);
		return (0);
	}

	if (sc->sc_flags & SC_SWITCHPENDING) {
		splx(s);
		return (EBUSY);
	}

	sc->sc_flags |= SC_SWITCHPENDING;
	sc->sc_screenwanted = no;

	splx(s);

	scr = sc->sc_focus;
	if (!scr) {
		sc->sc_oldscreen = -1;
		return (wsdisplay_switch1(sc, 0, waitok));
	} else
		sc->sc_oldscreen = sc->sc_focusidx;

#define wsswitch_cb1 ((void (*) __P((void *, int, int)))wsdisplay_switch1)
	if (scr->scr_syncops) {
		res = (*scr->scr_syncops->detach)(scr->scr_synccookie, waitok,
						  wsswitch_cb1, sc);
		if (res == EAGAIN) {
			/* switch will be done asynchronously */
			return (0);
		}
	} else if (scr->scr_flags & SCR_GRAPHICS) {
		/* no way to save state */
		res = EBUSY;
	}

	return (wsdisplay_switch1(sc, res, waitok));
}

void
wsdisplay_reset(dev, op)
	struct device *dev;
	enum wsdisplay_resetops op;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	struct wsscreen *scr;

	KASSERT(sc != NULL);
	scr = sc->sc_focus;

	if (!scr)
		return;

	switch (op) {
	case WSDISPLAY_RESETEMUL:
		if (!WSSCREEN_HAS_EMULATOR(scr))
			break;
		(*scr->scr_dconf->wsemul->reset)(scr->scr_dconf->wsemulcookie,
						 WSEMUL_RESET);
		break;
	case WSDISPLAY_RESETCLOSE:
		wsdisplay_closescreen(sc, scr);
		break;
	}
}

/*
 * Interface for (external) VT switch / process synchronization code
 */
int
wsscreen_attach_sync(scr, ops, cookie)
	struct wsscreen *scr;
	const struct wscons_syncops *ops;
	void *cookie;
{
	if (scr->scr_syncops) {
		/*
		 * The screen is already claimed.
		 * Check if the owner is still alive.
		 */
		if ((*scr->scr_syncops->check)(scr->scr_synccookie))
			return (EBUSY);
	}
	scr->scr_syncops = ops;
	scr->scr_synccookie = cookie;
	return (0);
}

int
wsscreen_detach_sync(scr)
	struct wsscreen *scr;
{
	if (!scr->scr_syncops)
		return (EINVAL);
	scr->scr_syncops = 0;
	return (0);
}

int
wsscreen_lookup_sync(scr, ops, cookiep)
	struct wsscreen *scr;
	const struct wscons_syncops *ops; /* used as ID */
	void **cookiep;
{
	if (!scr->scr_syncops || ops != scr->scr_syncops)
		return (EINVAL);
	*cookiep = scr->scr_synccookie;
	return (0);
}

/*
 * Interface to virtual screen stuff
 */
int
wsdisplay_maxscreenidx(sc)
	struct wsdisplay_softc *sc;
{
	return (WSDISPLAY_MAXSCREEN - 1);
}

int
wsdisplay_screenstate(sc, idx)
	struct wsdisplay_softc *sc;
	int idx;
{
	if (idx >= WSDISPLAY_MAXSCREEN)
		return (EINVAL);
	if (!sc->sc_scr[idx])
		return (ENXIO);
	return ((sc->sc_scr[idx]->scr_flags & SCR_OPEN) ? EBUSY : 0);
}

int
wsdisplay_getactivescreen(sc)
	struct wsdisplay_softc *sc;
{
	return (sc->sc_focusidx);
}

int
wsscreen_switchwait(sc, no)
	struct wsdisplay_softc *sc;
	int no;
{
	struct wsscreen *scr;
	int s, res = 0;

	if (no < 0 || no >= WSDISPLAY_MAXSCREEN)
		return (ENXIO);
	scr = sc->sc_scr[no];
	if (!scr)
		return (ENXIO);

	s = spltty();
	if (scr != sc->sc_focus) {
		scr->scr_flags |= SCR_WAITACTIVE;
		res = tsleep(scr, PCATCH, "wswait", 0);
		if (scr != sc->sc_scr[no])
			res = ENXIO; /* disappeared in the meantime */
		else
			scr->scr_flags &= ~SCR_WAITACTIVE;
	}
	splx(s);
	return (res);
}

void
wsdisplay_kbdholdscreen(dev, hold)
	struct device *dev;
	int hold;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dev;
	struct wsscreen *scr;

	scr = sc->sc_focus;

	if (hold)
		scr->scr_hold_screen = 1;
	else {
		scr->scr_hold_screen = 0;
		timeout(ttrstrt, scr->scr_tty, 0);	/* "immediate" */
	}
}

/*
 * Calls from the glue code.
 */
int
wsdisplay_is_console(dv)
	struct device *dv;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dv;

	KASSERT(sc != NULL);
	return (sc->sc_isconsole);
}

struct device *
wsdisplay_kbd(dv)
	struct device *dv;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dv;

	KASSERT(sc != NULL);
	return (sc->sc_kbddv);
}

void
wsdisplay_set_kbd(dv, kbddv)
	struct device *dv, *kbddv;
{
	struct wsdisplay_softc *sc = (struct wsdisplay_softc *)dv;

	KASSERT(sc != NULL);
	if (sc->sc_kbddv) {
		/* disable old keyboard */
		wskbd_enable(sc->sc_kbddv, 0);
	}
	if (kbddv) {
		/* enable new keyboard */
		wskbd_enable(kbddv, 1);
	}
	sc->sc_kbddv = kbddv;
}

/*
 * Console interface.
 */
void
wsdisplay_cnputc(dev, i)
	dev_t dev;
	int i;
{
	struct wsscreen_internal *dc;
	char c = i;

	if (!wsdisplay_console_initted)
		return;

	if (wsdisplay_console_device != NULL &&
	    (wsdisplay_console_device->sc_scr[0]->scr_flags & SCR_GRAPHICS))
		return;

	dc = &wsdisplay_console_conf;
	(*dc->wsemul->output)(dc->wsemulcookie, &c, 1, 1);
}

static int
wsdisplay_getc_dummy(dev)
	dev_t dev;
{
	/* panic? */
	return (0);
}

static void
wsdisplay_pollc_dummy(dev, on)
	dev_t dev;
	int on;
{
}

void
wsdisplay_set_cons_kbd(get, poll)
	int (*get) (dev_t);
	void (*poll) (dev_t, int);
{
	wsdisplay_cons.cn_getc = get;
	wsdisplay_cons.cn_pollc = poll;
}
