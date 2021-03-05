/*-
 * Copyright (c) 1992-1997 Søren Schmidt
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 * $FreeBSD$
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
#include <sys/malloc.h>

#include <dev/misc/cons/cons.h>

#include <devel/dev/consio.h>
#include <devel/dev/fbio.h>
#include <devel/dev/kbio.h>
#include <devel/dev/mouse.h>
#include <devel/dev/syscons/syscons.h>

dev_type_open(scopen);
dev_type_close(scclose);
dev_type_read(scread);
dev_type_write(scwrite);
dev_type_stop(scstop);
dev_type_ioctl(scioctl);
dev_type_poll(scpoll);

const struct cdevsw syscons_cdevsw = {
		.d_open = scopen,
		.d_close = scclose,
		.d_read = scread,
		.d_write = scwrite,
		.d_ioctl = scioctl,
		.d_stop = scstop,
		.d_tty = notty,
		.d_select = noselect,
		.d_poll = scpoll,
		.d_mmap = nommap,
		.d_strategy = nostrategy,
		.d_discard = nodiscard,
		.d_type = D_TTY
};


#if !defined(SC_MAX_HISTORY_SIZE)
#define SC_MAX_HISTORY_SIZE	(1000 * MAXCONS)
#endif

#if !defined(SC_HISTORY_SIZE)
#define SC_HISTORY_SIZE		(ROW * 4)
#endif

#if (SC_HISTORY_SIZE * MAXCONS) > SC_MAX_HISTORY_SIZE
#undef SC_MAX_HISTORY_SIZE
#define SC_MAX_HISTORY_SIZE	(SC_HISTORY_SIZE * MAXCONS)
#endif

#define COLD 			0
#define WARM 			1

static  scr_stat    	main_console;
static  scr_stat    	*console[MAXCONS];
static  char        	init_done = COLD;
scr_stat    			*cur_console;
static  char        	blink_in_progress = FALSE;

static	int				mouse_level = 0;	/* sysmouse protocol level */

static int				extra_history_size = SC_MAX_HISTORY_SIZE - SC_HISTORY_SIZE * MAXCONS;

struct  tty         	*sccons[MAXCONS+2];

#define VIRTUAL_TTY(x)  &sccons[x]
#define CONSOLE_TTY 	&sccons[MAXCONS]
#define MOUSE_TTY 		&sccons[MAXCONS+1]
#define SC_MOUSE 		128
#define SC_CONSOLE		255

struct tty
*scdevtotty(dev_t dev)
{
    int unit = minor(dev);

	if (init_done == COLD)
		return (NULL);
	if (unit == SC_CONSOLE)
		return CONSOLE_TTY;
	if (unit == SC_MOUSE)
		return MOUSE_TTY;
	if (unit >= MAXCONS || unit < 0)
		return (NULL);
	return VIRTUAL_TTY(unit);
}

int
scopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct tty *tp = scdevtotty(dev);

	if (!tp) {
		return(ENXIO);
	}

	tp->t_oproc = (minor(dev) == SC_MOUSE) ? scmousestart : scstart;
	tp->t_param = scparam;
	tp->t_dev = dev;
	if (!(tp->t_state & TS_ISOPEN)) {
		ttychars(tp);
		/* Use the current setting of the <-- key as default VERASE. */
		/* If the Delete key is preferable, an stty is necessary     */
		tp->t_cc[VERASE] = key_map.key[0x0e].map[0];
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		scparam(tp, &tp->t_termios);
		ttsetwater(tp);
		(*linesw[tp->t_line].l_modem)(tp, 1);
		if (minor(dev) == SC_MOUSE) {
			mouse_level = 0; /* XXX */
		}
	} else {
		if ((tp->t_state & TS_XCLUDE) && p->p_ucred->cr_uid != 0) {
			return (EBUSY);
		}
	}
	if (minor(dev) < MAXCONS && !console[minor(dev)]) {
		console[minor(dev)] = alloc_scp();
	}
	if (minor(dev) < MAXCONS && !tp->t_winsize.ws_col && !tp->t_winsize.ws_row) {
		tp->t_winsize.ws_col = console[minor(dev)]->xsize;
		tp->t_winsize.ws_row = console[minor(dev)]->ysize;
	}
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

int
scclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct tty *tp = scdevtotty(dev);
	struct scr_stat *scp;

	if (!tp)
		return (ENXIO);
	if (minor(dev) < MAXCONS) {
		scp = get_scr_stat(tp->t_dev);
		if (scp->status & SWITCH_WAIT_ACQ) {
			wakeup((caddr_t) &scp->smode);
		}
#if not_yet_done
		if (scp == &main_console) {
		    scp->pid = 0;
		    scp->proc = NULL;
		    scp->smode.mode = VT_AUTO;
		} else {
		    free(scp->scr_buf, M_DEVBUF);
		    if (scp->history != NULL) {
		    	free(scp->history, M_DEVBUF);
		    	if (scp->history_size / scp->xsize > imax(SC_HISTORY_SIZE, scp->ysize)) {
		    		extra_history_size += scp->history_size / scp->xsize - imax(SC_HISTORY_SIZE, scp->ysize);
		    	}
		    }
		    free(scp, M_DEVBUF);
		    console[minor(dev)] = NULL;
		}
#else
		scp->pid = 0;
		scp->proc = NULL;
		scp->smode.mode = VT_AUTO;
#endif
	}
	spltty();
	(*linesw[tp->t_line].l_close)(tp, flag);
	ttyclose(tp);
	spl0();
	return (0);
}

int
scread(dev, uio, flag)
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
scwrite(dev, uio, flag)
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


void
scintr(int unit)
{
    static struct tty *cur_tty;
    int c, len;
    u_char *cp;

	/*
	 * Loop while there is still input to get from the keyboard.
	 * I don't think this is nessesary, and it doesn't fix
	 * the Xaccel-2.1 keyboard hang, but it can't hurt.		XXX
	 */
	while ((c = scgetc(SCGETC_NONBLOCK)) != NOKEY) {

		cur_tty = VIRTUAL_TTY(get_scr_num());
		if (!(cur_tty->t_state & TS_ISOPEN))
			if (!((cur_tty = CONSOLE_TTY)->t_state & TS_ISOPEN))
				continue;

		switch (c & 0xff00) {
		case 0x0000: /* normal key */
			(*linesw[cur_tty->t_line].l_rint)(c & 0xFF, cur_tty);
			break;
		case FKEY: /* function key, return string */
			if (cp == get_fstr((u_int) c, (u_int*) &len)) {
				while (len-- > 0)
					(*linesw[cur_tty->t_line].l_rint)(*cp++ & 0xFF, cur_tty);
			}
			break;
		case MKEY: /* meta is active, prepend ESC */
			(*linesw[cur_tty->t_line].l_rint)(0x1b, cur_tty);
			(*linesw[cur_tty->t_line].l_rint)(c & 0xFF, cur_tty);
			break;
		case BKEY: /* backtab fixed sequence (esc [ Z) */
			(*linesw[cur_tty->t_line].l_rint)(0x1b, cur_tty);
			(*linesw[cur_tty->t_line].l_rint)('[', cur_tty);
			(*linesw[cur_tty->t_line].l_rint)('Z', cur_tty);
			break;
		}
	}

	if (cur_console->status & MOUSE_ENABLED) {
		cur_console->status &= ~MOUSE_VISIBLE;
		remove_mouse_image(cur_console);
	}
}

int
scstop(tp, flag)
	struct tty *tp;
	int flag;
{
	return (cnstop(tp, flag));
}

int
scpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

}

static int
scparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
    tp->t_ispeed = t->c_ispeed;
    tp->t_ospeed = t->c_ospeed;
    tp->t_cflag = t->c_cflag;

    return (0);
}

static void
scstart(struct tty *tp)
{
    struct clist *rbp;
    int s, len;
    u_char buf[PCBURST];
    scr_stat *scp = get_scr_stat(tp->t_dev);

	if ((scp->status & SLKED) || blink_in_progress) {
		return; /* XXX who repeats the call when the above flags are cleared? */
	}
	//s = spltty();
	if (!(tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))) {
		tp->t_state |= TS_BUSY;
		rbp = &tp->t_outq;
		while (rbp->c_cc) {
			len = q_to_b(rbp, buf, PCBURST);
		//	splx(s);
			ansi_put(scp, buf, len);
	//		s = spltty();
		}
		tp->t_state &= ~TS_BUSY;
		ttwwakeup(tp);
	}
//	splx(s);
}


int
scioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	u_int delta_ehs;
	int error;
	int i;
	struct tty *tp;
	scr_stat *scp;
	int s;

	tp = scdevtotty(dev);
	if (!tp)
		return ENXIO;
	scp = sc_get_scr_stat(tp->t_dev);

	/* If there is a user_ioctl function call that first */
	if (sc_user_ioctl) {
		error = (*sc_user_ioctl)(dev, cmd, data, flag, p);
		if (error != ENOIOCTL)
			return error;
	}

	error = sc_vid_ioctl(tp, cmd, data, flag, p);
	if (error != ENOIOCTL)
		return error;

	switch (cmd) { /* process console hardware related ioctl's */

	case GIO_ATTR: /* get current attributes */
		*(int*) data = (scp->term.cur_attr >> 8) & 0xFF;
		return 0;

	case GIO_COLOR: /* is this a color console ? */
		*(int*) data = (scp->adp->va_flags & V_ADP_COLOR) ? 1 : 0;
		return 0;

	case CONS_BLANKTIME: /* set screen saver timeout (0 = no saver) */
		if (*(int*) data < 0 || *(int*) data > MAX_BLANKTIME)
			return EINVAL;
		s = spltty();
		scrn_blank_time = *(int*) data;
		run_scrn_saver = (scrn_blank_time != 0);
		splx(s);
		return 0;

	case CONS_CURSORTYPE: /* set cursor type blink/noblink */
		if ((*(int*) data) & 0x01)
			sc_flags |= BLINK_CURSOR;
		else
			sc_flags &= ~BLINK_CURSOR;
		if ((*(int*) data) & 0x02) {
			if (!ISFONTAVAIL(scp->adp->va_flags))
				return ENXIO;
			sc_flags |= CHAR_CURSOR;
		} else
			sc_flags &= ~CHAR_CURSOR;
		/*
		 * The cursor shape is global property; all virtual consoles
		 * are affected. Update the cursor in the current console...
		 */
		if (!ISGRAPHSC(cur_console)) {
			s = spltty();
			remove_cursor_image(cur_console);
			if (sc_flags & CHAR_CURSOR)
				set_destructive_cursor(cur_console);
			draw_cursor_image(cur_console);
			splx(s);
		}
		return 0;

	case CONS_BELLTYPE: /* set bell type sound/visual */
		if ((*(int*) data) & 0x01)
			sc_flags |= VISUAL_BELL;
		else
			sc_flags &= ~VISUAL_BELL;
		if ((*(int*) data) & 0x02)
			sc_flags |= QUIET_BELL;
		else
			sc_flags &= ~QUIET_BELL;
		return 0;

	case CONS_HISTORY: /* set history size */
		if (*(int*) data > 0) {
			int lines; /* buffer size to allocate */
			int lines0; /* current buffer size */

			lines = imax(*(int*) data, scp->ysize);
			lines0 =
					(scp->history != NULL) ?
							scp->history_size / scp->xsize : scp->ysize;
			if (lines0 > imax(sc_history_size, scp->ysize))
				delta_ehs = lines0 - imax(sc_history_size, scp->ysize);
			else
				delta_ehs = 0;
			/*
			 * syscons unconditionally allocates buffers upto SC_HISTORY_SIZE
			 * lines or scp->ysize lines, whichever is larger. A value
			 * greater than that is allowed, subject to extra_history_size.
			 */
			if (lines > imax(sc_history_size, scp->ysize))
				if (lines - imax(sc_history_size, scp->ysize)
						> extra_history_size + delta_ehs)
					return EINVAL;
			if (cur_console->status & BUFFER_SAVED)
				return EBUSY;
			sc_alloc_history_buffer(scp, lines, delta_ehs, TRUE);
			return 0;
		} else
			return EINVAL;


	case CONS_GETVERS: /* get version number */
		*(int*) data = 0x200; /* version 2.0 */
		return 0;

	case CONS_IDLE: /* see if the screen has been idle */
		/*
		 * When the screen is in the GRAPHICS_MODE or UNKNOWN_MODE,
		 * the user process may have been writing something on the
		 * screen and syscons is not aware of it. Declare the screen
		 * is NOT idle if it is in one of these modes. But there is
		 * an exception to it; if a screen saver is running in the
		 * graphics mode in the current screen, we should say that the
		 * screen has been idle.
		 */
		*(int*) data = scrn_idle
				&& (!ISGRAPHSC(cur_console)
						|| (cur_console->status & SAVER_RUNNING));
		return 0;

	case CONS_SAVERMODE: /* set saver mode */
		switch (*(int*) data) {
		case CONS_USR_SAVER:
			/* if a LKM screen saver is running, stop it first. */
			scsplash_stick(FALSE);
			saver_mode = *(int*) data;
			s = spltty();
			if ((error = wait_scrn_saver_stop())) {
				splx(s);
				return error;
			}
			scp->status |= SAVER_RUNNING;
			scsplash_stick(TRUE);
			splx(s);
			break;
		case CONS_LKM_SAVER:
			s = spltty();
			if ((saver_mode == CONS_USR_SAVER) && (scp->status & SAVER_RUNNING))
				scp->status &= ~SAVER_RUNNING;
			saver_mode = *(int*) data;
			splx(s);
			break;
		default:
			return EINVAL;
		}
		return 0;

	case CONS_SAVERSTART: /* immediately start/stop the screen saver */
		/*
		 * Note that this ioctl does not guarantee the screen saver
		 * actually starts or stops. It merely attempts to do so...
		 */
		s = spltty();
		run_scrn_saver = (*(int*) data != 0);
		if (run_scrn_saver)
			scrn_time_stamp -= scrn_blank_time;
		splx(s);
		return 0;

	case VT_SETMODE: /* set screen switcher mode */
	{
		struct vt_mode *mode;

		mode = (struct vt_mode*) data;
		if (scp->smode.mode == VT_PROCESS) {
			if (scp->proc == pfind(scp->pid) && scp->proc != p) {
				return EPERM;
			}
		}
		s = spltty();
		if (mode->mode == VT_AUTO) {
			scp->smode.mode = VT_AUTO;
			scp->proc = NULL;
			scp->pid = 0;
			/* were we in the middle of the vty switching process? */
			if (scp->status & SWITCH_WAIT_REL) {
				/* assert(scp == cur_console) */
				scp->status &= ~SWITCH_WAIT_REL;
				s = do_switch_scr(s);
			}
			if (scp->status & SWITCH_WAIT_ACQ) {
				/* assert(scp == cur_console) */
				scp->status &= ~SWITCH_WAIT_ACQ;
				switch_in_progress = 0;
			}
		} else {
			if (!ISSIGVALID(mode->relsig) || !ISSIGVALID(mode->acqsig)
			|| !ISSIGVALID(mode->frsig)) {
				splx(s);
				return EINVAL;
			}
			bcopy(data, &scp->smode, sizeof(struct vt_mode));
			scp->proc = p;
			scp->pid = scp->proc->p_pid;
		}
		splx(s);
		return 0;
	}

	case VT_GETMODE: /* get screen switcher mode */
		bcopy(&scp->smode, data, sizeof(struct vt_mode));
		return 0;

	case VT_RELDISP: /* screen switcher ioctl */
		s = spltty();
		/*
		 * This must be the current vty which is in the VT_PROCESS
		 * switching mode...
		 */
		if ((scp != cur_console) || (scp->smode.mode != VT_PROCESS)) {
			splx(s);
			return EINVAL;
		}
		/* ...and this process is controlling it. */
		if (scp->proc != p) {
			splx(s);
			return EPERM;
		}
		error = EINVAL;
		switch (*(int*) data) {
		case VT_FALSE: /* user refuses to release screen, abort */
			if (scp == old_scp && (scp->status & SWITCH_WAIT_REL)) {
				old_scp->status &= ~SWITCH_WAIT_REL;
				switch_in_progress = 0;
				error = 0;
			}
			break;

		case VT_TRUE: /* user has released screen, go on */
			if (scp == old_scp && (scp->status & SWITCH_WAIT_REL)) {
				scp->status &= ~SWITCH_WAIT_REL;
				s = do_switch_scr(s);
				error = 0;
			}
			break;

		case VT_ACKACQ: /* acquire acknowledged, switch completed */
			if (scp == new_scp && (scp->status & SWITCH_WAIT_ACQ)) {
				scp->status &= ~SWITCH_WAIT_ACQ;
				switch_in_progress = 0;
				error = 0;
			}
			break;

		default:
			break;
		}
		splx(s);
		return error;

	case VT_OPENQRY: /* return free virtual console */
		for (i = 0; i < MAXCONS; i++) {
			tp = VIRTUAL_TTY(i);
			if (!(tp->t_state & TS_ISOPEN)) {
				*(int*) data = i + 1;
				return 0;
			}
		}
		return EINVAL;

	case VT_ACTIVATE: /* switch to screen *data */
		s = spltty();
		sc_clean_up(cur_console);
		splx(s);
		return switch_scr(scp, *(int*) data - 1);

	case VT_WAITACTIVE: /* wait for switch to occur */
		if (*(int*) data > MAXCONS || *(int*) data < 0)
			return EINVAL;
		s = spltty();
		error = sc_clean_up(cur_console);
		splx(s);
		if (error)
			return error;
		if (*(int*) data != 0)
			scp = console[*(int*) data - 1];
		if (scp == cur_console)
			return 0;
		while ((error = tsleep((caddr_t) &scp->smode, PZERO | PCATCH, "waitvt",
				0)) == ERESTART)
			;
		return error;

	case VT_GETACTIVE:
		*(int*) data = get_scr_num() + 1;
		return 0;

	case KDENABIO: /* allow io operations */
		error = suser(p->p_ucred, &p->p_acflag);
		if (error != 0)
			return error;
		if (securelevel > 0)
			return EPERM;
#ifdef __i386__
			p->p_md.md_regs->tf_eflags |= PSL_IOPL;
#endif
		return 0;

	case KDDISABIO: /* disallow io operations (default) */
#ifdef __i386__
			p->p_md.md_regs->tf_eflags &= ~PSL_IOPL;
#endif
		return 0;

	case KDSKBSTATE: /* set keyboard state (locks) */
		if (*(int*) data & ~LOCK_MASK)
			return EINVAL;
		scp->status &= ~LOCK_MASK;
		scp->status |= *(int*) data;
		if (scp == cur_console)
			update_kbd_state(scp->status, LOCK_MASK);
		return 0;

	case KDGKBSTATE: /* get keyboard state (locks) */
		if (scp == cur_console)
			save_kbd_state(scp);
		*(int*) data = scp->status & LOCK_MASK;
		return 0;

	case KDGETREPEAT: /* get keyboard repeat & delay rates */
	case KDSETREPEAT: /* set keyboard repeat & delay rates (new) */
		error = kbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL)
			error = ENODEV;
		return error;

	case KDSETRAD: /* set keyboard repeat & delay rates (old) */
		if (*(int*) data & ~0x7f)
			return EINVAL;
		error = kbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL)
			error = ENODEV;
		return error;

	case KDSKBMODE: /* set keyboard mode */
		switch (*(int*) data) {
		case K_XLATE: /* switch to XLT ascii mode */
		case K_RAW: /* switch to RAW scancode mode */
		case K_CODE: /* switch to CODE mode */
			scp->kbd_mode = *(int*) data;
			if (scp == cur_console)
				kbd_ioctl(kbd, cmd, data);
			return 0;
		default:
			return EINVAL;
		}
		/* NOT REACHED */

	case KDGKBMODE: /* get keyboard mode */
		*(int*) data = scp->kbd_mode;
		return 0;

	case KDGKBINFO:
		error = kbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL)
			error = ENODEV;
		return error;

	case KDMKTONE: /* sound the bell */
		if (*(int*) data)
			do_bell(scp, (*(int*) data) & 0xffff,
					(((*(int*) data) >> 16) & 0xffff) * hz / 1000);
		else
			do_bell(scp, scp->bell_pitch, scp->bell_duration);
		return 0;

	case KIOCSOUND: /* make tone (*data) hz */
#ifdef __i386__
		if (scp == cur_console) {
			if (*(int*) data) {
				int pitch = timer_freq / *(int*) data;

				/* set command for counter 2, 2 byte write */
				if (acquire_timer2(TIMER_16BIT | TIMER_SQWAVE))
					return EBUSY;

				/* set pitch */
				outb(TIMER_CNTR2, pitch);
				outb(TIMER_CNTR2, (pitch >> 8));

				/* enable counter 2 output to speaker */
				outb(IO_PPI, inb(IO_PPI) | 3);
			} else {
				/* disable counter 2 output to speaker */
				outb(IO_PPI, inb(IO_PPI) & 0xFC);
				release_timer2();
			}
		}
#endif /* __i386__ */
		return 0;

	case KDGKBTYPE: /* get keyboard type */
		error = kbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL) {
			/* always return something? XXX */
			*(int*) data = 0;
		}
		return 0;

	case KDSETLED: /* set keyboard LED status */
		if (*(int*) data & ~LED_MASK) /* FIXME: LOCK_MASK? */
			return EINVAL;
		scp->status &= ~LED_MASK;
		scp->status |= *(int*) data;
		if (scp == cur_console)
			update_kbd_leds(scp->status);
		return 0;

	case KDGETLED: /* get keyboard LED status */
		if (scp == cur_console)
			save_kbd_state(scp);
		*(int*) data = scp->status & LED_MASK;
		return 0;

	case CONS_SETKBD: /* set the new keyboard */
	{
		keyboard_t *newkbd;

		s = spltty();
		newkbd = kbd_get_keyboard(*(int*) data);
		if (newkbd == NULL) {
			splx(s);
			return EINVAL;
		}
		error = 0;
		if (kbd != newkbd) {
			i = kbd_allocate(newkbd->kb_name, newkbd->kb_unit,
					(void*) &keyboard, sckbdevent, NULL);
			/* i == newkbd->kb_index */
			if (i >= 0) {
				if (kbd != NULL) {
					save_kbd_state(cur_console);
					kbd_release(kbd, (void*) &keyboard);
				}
				kbd = kbd_get_keyboard(i); /* kbd == newkbd */
				keyboard = i;
				kbd_ioctl(kbd, KDSKBMODE, (caddr_t) &cur_console->kbd_mode);
				update_kbd_state(cur_console->status, LOCK_MASK);
			} else {
				error = EPERM; /* XXX */
			}
		}
		splx(s);
		return error;
	}

	case CONS_RELKBD: /* release the current keyboard */
		s = spltty();
		error = 0;
		if (kbd != NULL) {
			save_kbd_state(cur_console);
			error = kbd_release(kbd, (void*) &keyboard);
			if (error == 0) {
				kbd = NULL;
				keyboard = -1;
			}
		}
		splx(s);
		return error;

	case GIO_SCRNMAP: /* get output translation table */
		bcopy(&scr_map, data, sizeof(scr_map));
		return 0;

	case PIO_SCRNMAP: /* set output translation table */
		bcopy(data, &scr_map, sizeof(scr_map));
		for (i = 0; i < sizeof(scr_map); i++)
			scr_rmap[scr_map[i]] = i;
		return 0;

	case GIO_KEYMAP: /* get keyboard translation table */
	case PIO_KEYMAP: /* set keyboard translation table */
	case GIO_DEADKEYMAP: /* get accent key translation table */
	case PIO_DEADKEYMAP: /* set accent key translation table */
	case GETFKEY: /* get function key string */
	case SETFKEY: /* set function key string */
		error = genkbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL)
			error = ENODEV;
		return error;

	case PIO_FONT8x8: /* set 8x8 dot font */
		if (!ISFONTAVAIL(scp->adp->va_flags))
			return ENXIO;
		bcopy(data, font_8, 8 * 256);
		fonts_loaded |= FONT_8;
		/*
		 * FONT KLUDGE
		 * Always use the font page #0. XXX
		 * Don't load if the current font size is not 8x8.
		 */
		if (ISTEXTSC(cur_console) && (cur_console->font_size < 14))
			copy_font(cur_console, LOAD, 8, font_8);
		return 0;

	case GIO_FONT8x8: /* get 8x8 dot font */
		if (!ISFONTAVAIL(scp->adp->va_flags))
			return ENXIO;
		if (fonts_loaded & FONT_8) {
			bcopy(font_8, data, 8 * 256);
			return 0;
		} else
			return ENXIO;

	case PIO_FONT8x14: /* set 8x14 dot font */
		if (!ISFONTAVAIL(scp->adp->va_flags))
			return ENXIO;
		bcopy(data, font_14, 14 * 256);
		fonts_loaded |= FONT_14;
		/*
		 * FONT KLUDGE
		 * Always use the font page #0. XXX
		 * Don't load if the current font size is not 8x14.
		 */
		if (ISTEXTSC(cur_console) && (cur_console->font_size >= 14)
				&& (cur_console->font_size < 16))
			copy_font(cur_console, LOAD, 14, font_14);
		return 0;

	case GIO_FONT8x14: /* get 8x14 dot font */
		if (!ISFONTAVAIL(scp->adp->va_flags))
			return ENXIO;
		if (fonts_loaded & FONT_14) {
			bcopy(font_14, data, 14 * 256);
			return 0;
		} else
			return ENXIO;

	case PIO_FONT8x16: /* set 8x16 dot font */
		if (!ISFONTAVAIL(scp->adp->va_flags))
			return ENXIO;
		bcopy(data, font_16, 16 * 256);
		fonts_loaded |= FONT_16;
		/*
		 * FONT KLUDGE
		 * Always use the font page #0. XXX
		 * Don't load if the current font size is not 8x16.
		 */
		if (ISTEXTSC(cur_console) && (cur_console->font_size >= 16))
			copy_font(cur_console, LOAD, 16, font_16);
		return 0;

	case GIO_FONT8x16: /* get 8x16 dot font */
		if (!ISFONTAVAIL(scp->adp->va_flags))
			return ENXIO;
		if (fonts_loaded & FONT_16) {
			bcopy(font_16, data, 16 * 256);
			return 0;
		} else
			return ENXIO;
	default:
		break;
	}

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error != ENOIOCTL)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error != ENOIOCTL)
		return (error);
	return (ENOTTY);
}

static void
scmousestart(struct tty *tp)
{
    struct clist *rbp;
    int s;
    u_char buf[PCBURST];

	s = spltty();
	if (!(tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))) {
		tp->t_state |= TS_BUSY;
		rbp = &tp->t_outq;
		while (rbp->c_cc) {
			q_to_b(rbp, buf, PCBURST);
		}
		tp->t_state &= ~TS_BUSY;
		ttwwakeup(tp);
	}
	splx(s);
}

void
sccnputc(dev_t dev, int c)
{
    u_char buf[1];
    int s;
    scr_stat *scp = console[0];
    term_stat save = scp->term;

    scp->term = kernel_console;
    current_default = &kernel_default;
    if (scp == cur_console && !(scp->status & UNKNOWN_MODE))
	remove_cursor_image(scp);
    buf[0] = c;
    ansi_put(scp, buf, 1);
    kernel_console = scp->term;
    current_default = &user_default;
    scp->term = save;

    s = spltty();	/* block scintr and scrn_timer */
    sccnupdate(scp);
    splx(s);
}


int
sccngetc(dev_t dev)
{
    int s = spltty();	/* block scintr and scrn_timer while we poll */
    int c;

	/*
	 * Stop the screen saver and update the screen if necessary.
	 * What if we have been running in the screen saver code... XXX
	 */
	sccnupdate(cur_console);

	c = scgetc(SCGETC_CN);
	splx(s);
	return (c);
}

int
sccncheckc(dev_t dev)
{
    int s = spltty();	/* block scintr and scrn_timer while we poll */
    int c;

	sccnupdate(cur_console);
	c = scgetc(SCGETC_CN | SCGETC_NONBLOCK);
	splx(s);
	return (c == NOKEY ? -1 : c); /* c == -1 can't happen */
}

static void
sccnupdate(scr_stat *scp)
{
	if (scp == cur_console && !font_loading_in_progress) {
		if (scrn_blanked > 0)
			stop_scrn_saver(current_saver);
		if (!(scp->status & UNKNOWN_MODE) && scrn_blanked <= 0
				&& !blink_in_progress && !switch_in_progress)
			scrn_update(scp, TRUE);
	}
}

static scr_stat
*get_scr_stat(dev_t dev)
{
    int unit = minor(dev);

	if (unit == SC_CONSOLE) {
		return console[0];
	}
	if (unit >= MAXCONS || unit < 0) {
		return (NULL);
	}
	return (console[unit]);
}

static int
get_scr_num()
{
	int i = 0;

	while ((i < MAXCONS) && (cur_console != console[i])) {
		i++;
	}
	return (i < MAXCONS ? i : 0);
}
