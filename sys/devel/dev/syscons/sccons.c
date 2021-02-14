/*
 * sccons.c
 *
 *  Created on: 14 Feb 2021
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/vnode.h>
#include <sys/poll.h>
#include <dev/cons.h>

#include <devel/dev/kbio.h>
#include <devel/dev/syscons/syscons.h>

#include <devel/dev/kbd/kbdreg.h>

/*
static cn_init_t	sccninit_fini;
static cn_checkc_t	sccncheckc;
static cn_dbctl_t	sccndbctl;
static cn_term_t	sccnterm;
*/

static	int			sc_console_unit = -1;
static  scr_stat    *sc_console;

/*
dev_type_cnprobe(sccnprobe);
dev_type_cninit(sccninit);
dev_type_cngetc(sccngetc);
dev_type_cnputc(sccnputc);
dev_type_cnpollc(sccnpollc);

struct consdev sccons = {
		.cn_probe = sccnprobe,
		.cn_init = sccninit,
		.cn_getc = sccngetc,
		.cn_putc = sccnputc,
		.cn_pollc = sccnpollc,
		.cn_dev = 0,
		.cn_pri= 0,
};
*/

dev_type_open(sccnopen);
dev_type_close(sccnclose);
dev_type_read(sccnread);
dev_type_write(sccnwrite);
dev_type_stop(sccnstop);
dev_type_ioctl(sccnioctl);
dev_type_poll(sccnpoll);

const struct cdevsw sccons_cdevsw = {
		.d_open = sccnopen,
		.d_close = sccnclose,
		.d_read = sccnread,
		.d_write = sccnwrite,
		.d_ioctl = sccnioctl,
		.d_stop = sccnstop,
		.d_tty = notty,
		.d_select = noselect,
		.d_poll = sccnpoll,
		.d_mmap = nommap,
		.d_strategy = nostrategy,
		.d_discard = nodiscard,
		.d_type = D_TTY
};

static void
sccnprobe(struct consdev *cp)
{
    int unit;
    int flags;

    cp->cn_pri = sc_get_cons_priority(&unit, &flags);

    /* a video card is always required */
    if (probe_efi_fb(1) != 0 && !scvidprobe(unit, flags, TRUE)) {
    	cp->cn_pri = CN_DEAD;
    }

    /* syscons will become console even when there is no keyboard */
    sckbdprobe(unit, flags, TRUE);

    if (cp->cn_pri == CN_DEAD) {
    	return;
    }
}

static void
sccninit(struct consdev *cp)
{
    int unit;
    int flags;

    sc_get_cons_priority(&unit, &flags);
    scinit(unit, flags | SC_KERNEL_CONSOLE);
    sc_console_unit = unit;
    sc_console = sc_get_softc(unit, SC_KERNEL_CONSOLE)->console_scp;
}

int
sccnopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{

}

int
sccnclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{

}

int
sccnread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct tty *tp = scdevtotty(dev);

	if (!tp) {
		return (ENXIO);
	}
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

int
sccnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct tty *tp = scdevtotty(dev);

	if (!tp) {
		return (ENXIO);
	}
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

int
sccnstop(tp, flag)
	struct tty *tp;
	int flag;
{
	return (cdev_stop(tp, flag));
}

int
sccnioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{

}

int
sccnpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

}

int
sccngetc()
{
	return (sccngetch(0));
}

/*
 * Console path - cannot block!
 */
void
sccnputc(c)
	register int c;
{
	u_char buf[1];
	scr_stat *scp = sc_console;
	void *save;
#ifndef SC_NO_HISTORY
#if 0
	    struct tty *tp;
#endif
#endif /* !SC_NO_HISTORY */

	/* assert(sc_console != NULL) */

	syscons_lock();
#ifndef SC_NO_HISTORY
	if (scp == scp->sc->cur_scp && (scp->status & SLKED)) {
		scp->status &= ~SLKED;
#if 0
		/* This can block, illegal in the console path */
		update_kbd_state(scp, scp->status, SLKED, TRUE);
#endif
		if (scp->status & BUFFER_SAVED) {
			if (!sc_hist_restore(scp))
				sc_remove_cutmarking(scp);
			scp->status &= ~BUFFER_SAVED;
			scp->status |= CURSOR_ENABLED;
			sc_draw_cursor_image(scp);
		}
#if 0
		tp = VIRTUAL_TTY(scp->sc, scp->index);
		/* This can block, illegal in the console path */
		if (ISTTYOPEN(tp)) {
		    scstart(tp);
		}
#endif
	}
#endif /* !SC_NO_HISTORY */

	save = scp->ts;
	if (kernel_console_ts != NULL)
		scp->ts = kernel_console_ts;
	buf[0] = c;
	sc_puts(scp, buf, 1);
	scp->ts = save;

	sccnupdate(scp);
	syscons_unlock();
}

/*
 * Set polling mode (also disables the tty feed), use when
 * polling for console key input.
 */
static void
sccnpollc(int on)
{
    scr_stat *scp;

    syscons_lock();
    sc_touch_scrn_saver();
    scp = sc_console->sc->cur_scp;	/* XXX */
    syscons_unlock();
    if (scp->sc->kbd) {
	    genkbd_poll(scp->sc->kbd, on);
    }
}

/*
 * Console path - cannot block!
 */
static int
sccngetch(flags)
	int flags;
{
	static struct fkeytab fkey;
	static int fkeycp;
	scr_stat *scp;
	u_char *p;
	int cur_mode;
	int c;

	syscons_lock();
	/* assert(sc_console != NULL) */

	/*
	 * Stop the screen saver and update the screen if necessary.
	 * What if we have been running in the screen saver code... XXX
	 */
	sc_touch_scrn_saver();
	scp = sc_console->sc->cur_scp; /* XXX */
	sccnupdate(scp);
	syscons_unlock();

	if (fkeycp < fkey.len) {
		return fkey.str[fkeycp++];
	}

	if (scp->sc->kbd == NULL) {
		return -1;
	}

	/*
	 * Make sure the keyboard is accessible even when the kbd device
	 * driver is disabled.
	 */
	genkbd_enable(scp->sc->kbd);

	/* we shall always use the keyboard in the XLATE mode here */
	cur_mode = scp->kbd_mode;
	scp->kbd_mode = K_XLATE;
	genkbd_ioctl(scp->sc->kbd, KDSKBMODE, (caddr_t) &scp->kbd_mode);

	c = scgetc(scp->sc, SCGETC_CN | flags);

	scp->kbd_mode = cur_mode;
	genkbd_ioctl(scp->sc->kbd, KDSKBMODE, (caddr_t) &scp->kbd_mode);
	genkbd_disable(scp->sc->kbd);

	switch (KEYFLAGS(c)) {
	case 0: /* normal char */
		return KEYCHAR(c);
	case FKEY: /* function key */
		p = genkbd_get_fkeystr(scp->sc->kbd, KEYCHAR(c), (size_t*) &fkeycp);
		fkey.len = fkeycp;
		if ((p != NULL) && (fkey.len > 0)) {
			bcopy(p, fkey.str, fkey.len);
			fkeycp = 1;
			return fkey.str[0];
		}
		return c; /* XXX */
	case NOKEY:
		if (flags & SCGETC_NONBLOCK)
			return (c);
		/* fall through */
	case ERRKEY:
	default:
		return -1;
	}
	/* NOT REACHED */
}
