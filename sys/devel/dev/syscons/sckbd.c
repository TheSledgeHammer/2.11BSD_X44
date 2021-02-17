/*
 * sckbd.c
 *
 *  Created on: 15 Feb 2021
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
#include <sys/malloc.h>

#include <dev/misc/cons/cons.h>

#include <devel/dev/consio.h>
#include <devel/dev/fbio.h>
#include <devel/dev/kbio.h>
#include <devel/dev/mouse.h>
#include <devel/dev/syscons/syscons.h>

int
keyboard_ioctl(dev, cmd, data, flag, p)
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

	switch (cmd) {
	case KDENABIO:
		/* allow io operations */
		error = suser(p->p_ucred, &p->p_acflag);
		if (error != 0)
			return error;
		if (securelevel > 0)
			return EPERM;
#ifdef __i386__
		p->p_md.md_regs->tf_eflags |= PSL_IOPL;
#endif
		return 0;

	case KDDISABIO:
		/* disallow io operations (default) */
#ifdef __i386__
		p->p_md.md_regs->tf_eflags &= ~PSL_IOPL;
#endif
		return 0;

	case KDSKBSTATE:
		/* set keyboard state (locks) */
		if (*(int*) data & ~LOCK_MASK)
			return EINVAL;
		scp->status &= ~LOCK_MASK;
		scp->status |= *(int*) data;
		if (scp == cur_console)
			update_kbd_state(scp->status, LOCK_MASK);
		return 0;

	case KDGKBSTATE:
		/* get keyboard state (locks) */
		if (scp == cur_console)
			save_kbd_state(scp);
		*(int*) data = scp->status & LOCK_MASK;
		return 0;

	case KDGETREPEAT:
		/* get keyboard repeat & delay rates */
	case KDSETREPEAT:
		/* set keyboard repeat & delay rates (new) */
		error = kbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL)
			error = ENODEV;
		return error;

	case KDSETRAD:
		/* set keyboard repeat & delay rates (old) */
		if (*(int*) data & ~0x7f)
			return EINVAL;
		error = kbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL)
			error = ENODEV;
		return error;

	case KDSKBMODE:
		/* set keyboard mode */
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

	case KDGKBMODE:
		/* get keyboard mode */
		*(int*) data = scp->kbd_mode;
		return 0;

	case KDGKBINFO:
		error = kbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL)
			error = ENODEV;
		return error;

	case KDMKTONE:
		/* sound the bell */
		if (*(int*) data)
			do_bell(scp, (*(int*) data) & 0xffff,
					(((*(int*) data) >> 16) & 0xffff) * hz / 1000);
		else
			do_bell(scp, scp->bell_pitch, scp->bell_duration);
		return 0;

	case KIOCSOUND:
		/* make tone (*data) hz */
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

	case KDGKBTYPE:
		/* get keyboard type */
		error = kbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL) {
			/* always return something? XXX */
			*(int*) data = 0;
		}
		return 0;

	case KDSETLED:
		/* set keyboard LED status */
		if (*(int*) data & ~LED_MASK) /* FIXME: LOCK_MASK? */
			return EINVAL;
		scp->status &= ~LED_MASK;
		scp->status |= *(int*) data;
		if (scp == cur_console)
			update_kbd_leds(scp->status);
		return 0;

	case KDGETLED:
		/* get keyboard LED status */
		if (scp == cur_console)
			save_kbd_state(scp);
		*(int*) data = scp->status & LED_MASK;
		return 0;

	case CONS_SETKBD:
		/* set the new keyboard */
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

	case CONS_RELKBD:
		/* release the current keyboard */
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

	case GIO_SCRNMAP:
		/* get output translation table */
		bcopy(&scr_map, data, sizeof(scr_map));
		return 0;

	case PIO_SCRNMAP:
		/* set output translation table */
		bcopy(data, &scr_map, sizeof(scr_map));
		for (i = 0; i < sizeof(scr_map); i++)
			scr_rmap[scr_map[i]] = i;
		return 0;

	case GIO_KEYMAP:
		/* get keyboard translation table */
	case PIO_KEYMAP:
		/* set keyboard translation table */
	case GIO_DEADKEYMAP:
		/* get accent key translation table */
	case PIO_DEADKEYMAP:
		/* set accent key translation table */
	case GETFKEY:
		/* get function key string */
	case SETFKEY:
		/* set function key string */
		error = genkbd_ioctl(kbd, cmd, data);
		if (error == ENOIOCTL)
			error = ENODEV;
		return error;

	case PIO_FONT8x8:
		/* set 8x8 dot font */
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

	case GIO_FONT8x8:
		/* get 8x8 dot font */
		if (!ISFONTAVAIL(scp->adp->va_flags))
			return ENXIO;
		if (fonts_loaded & FONT_8) {
			bcopy(font_8, data, 8 * 256);
			return 0;
		} else
			return ENXIO;

	case PIO_FONT8x14:
		/* set 8x14 dot font */
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

	case GIO_FONT8x14:
		/* get 8x14 dot font */
		if (!ISFONTAVAIL(scp->adp->va_flags))
			return ENXIO;
		if (fonts_loaded & FONT_14) {
			bcopy(font_14, data, 14 * 256);
			return 0;
		} else
			return ENXIO;

	case PIO_FONT8x16:
		/* set 8x16 dot font */
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

	case GIO_FONT8x16:
		/* get 8x16 dot font */
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
