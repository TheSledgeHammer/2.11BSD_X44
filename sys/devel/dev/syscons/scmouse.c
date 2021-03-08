/*
 * scmouse.c
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
#include <sys/errno.h>

#include <dev/misc/cons/cons.h>

#include <devel/dev/consio.h>
#include <devel/dev/fbio.h>
#include <devel/dev/kbio.h>
#include <devel/dev/mouse.h>
#include <devel/dev/syscons/syscons.h>

#define MOUSE_ENABLED		0x00800
#define MOUSE_MOVED			0x01000
#define MOUSE_CUTTING		0x02000
#define MOUSE_VISIBLE		0x04000

#if !defined(SC_MOUSE_CHAR)
#define SC_MOUSE_CHAR		(0xd0)
#endif

/* for backward compatibility */
#define OLD_CONS_MOUSECTL	_IOWR('c', 10, old_mouse_info_t)

typedef struct old_mouse_data old_mouse_data_t;
typedef struct old_mouse_info old_mouse_info_t;

struct old_mouse_data {
    int x;
    int y;
    int buttons;
};

struct old_mouse_info {
	int 						operation;
	union {
		struct old_mouse_data 	data;
		struct mouse_mode 		mode;
	} u;
};

static  long       		scrn_time_stamp;
static	char 			*cut_buffer;
static	int				cut_buffer_size;
static	int				mouse_level = 0;	/* sysmouse protocol level */
static	mousestatus_t	mouse_status = { 0, 0, 0, 0, 0, 0 };
static u_short 			mouse_and_mask[16] = {
						0xc000, 0xe000, 0xf000, 0xf800,
						0xfc00, 0xfe00, 0xff00, 0xff80,
						0xfe00, 0x1e00, 0x1f00, 0x0f00,
						0x0f00, 0x0000, 0x0000, 0x0000 };
static u_short 			mouse_or_mask[16] = {
						0x0000, 0x4000, 0x6000, 0x7000,
						0x7800,	0x7c00, 0x7e00, 0x6800,
						0x0c00, 0x0c00, 0x0600, 0x0600,
						0x0000, 0x0000, 0x0000, 0x0000 };

#define MOUSE_TTY 		&sccons[MAXCONS+1]
#define SC_MOUSE 		128

int
mouse_ioctl(dev, cmd, data, flag, p)
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

	switch (cmd) { /* process console hardware related ioctl's */
	case CONS_MOUSECTL:
		/* control mouse arrow */
	case OLD_CONS_MOUSECTL: {
		/* MOUSE_BUTTON?DOWN -> MOUSE_MSC_BUTTON?UP */
		static butmap[8] = { MOUSE_MSC_BUTTON1UP | MOUSE_MSC_BUTTON2UP
				| MOUSE_MSC_BUTTON3UP, MOUSE_MSC_BUTTON2UP
				| MOUSE_MSC_BUTTON3UP, MOUSE_MSC_BUTTON1UP
				| MOUSE_MSC_BUTTON3UP, MOUSE_MSC_BUTTON3UP, MOUSE_MSC_BUTTON1UP
				| MOUSE_MSC_BUTTON2UP, MOUSE_MSC_BUTTON2UP, MOUSE_MSC_BUTTON1UP,
				0, };
		mouse_info_t *mouse = (mouse_info_t*) data;
		mouse_info_t buf;
		int f;

		/* FIXME: */
		if (!ISMOUSEAVAIL(scp->adp->va_flags))
			return ENODEV;

		if (cmd == OLD_CONS_MOUSECTL) {
			static unsigned char swapb[] = { 0, 4, 2, 6, 1, 5, 3, 7 };
			old_mouse_info_t *old_mouse = (old_mouse_info_t*) data;

			mouse = &buf;
			mouse->operation = old_mouse->operation;
			switch (mouse->operation) {
			case MOUSE_MODE:
				mouse->u.mode = old_mouse->u.mode;
				break;
			case MOUSE_SHOW:
			case MOUSE_HIDE:
				break;
			case MOUSE_MOVEABS:
			case MOUSE_MOVEREL:
			case MOUSE_ACTION:
				mouse->u.data.x = old_mouse->u.data.x;
				mouse->u.data.y = old_mouse->u.data.y;
				mouse->u.data.z = 0;
				mouse->u.data.buttons = swapb[old_mouse->u.data.buttons & 0x7];
				break;
			case MOUSE_GETINFO:
				old_mouse->u.data.x = scp->mouse_xpos;
				old_mouse->u.data.y = scp->mouse_ypos;
				old_mouse->u.data.buttons = swapb[scp->mouse_buttons & 0x7];
				break;
			default:
				return EINVAL;
			}
		}

		switch (mouse->operation) {
		case MOUSE_MODE:
			if (ISSIGVALID(mouse->u.mode.signal)) {
				scp->mouse_signal = mouse->u.mode.signal;
				scp->mouse_proc = p;
				scp->mouse_pid = p->p_pid;
			} else {
				scp->mouse_signal = 0;
				scp->mouse_proc = NULL;
				scp->mouse_pid = 0;
			}
			return 0;

		case MOUSE_SHOW:
			if (ISTEXTSC(scp) && !(scp->status & MOUSE_ENABLED)) {
				scp->status |= (MOUSE_ENABLED | MOUSE_VISIBLE);
				scp->mouse_oldpos = scp->mouse_pos;
				mark_all(scp);
				return 0;
			} else
				return EINVAL;
			break;

		case MOUSE_HIDE:
			if (ISTEXTSC(scp) && (scp->status & MOUSE_ENABLED)) {
				scp->status &= ~(MOUSE_ENABLED | MOUSE_VISIBLE);
				mark_all(scp);
				return 0;
			} else
				return EINVAL;
			break;

		case MOUSE_MOVEABS:
			scp->mouse_xpos = mouse->u.data.x;
			scp->mouse_ypos = mouse->u.data.y;
			set_mouse_pos(scp);
			break;

		case MOUSE_MOVEREL:
			scp->mouse_xpos += mouse->u.data.x;
			scp->mouse_ypos += mouse->u.data.y;
			set_mouse_pos(scp);
			break;

		case MOUSE_GETINFO:
			mouse->u.data.x = scp->mouse_xpos;
			mouse->u.data.y = scp->mouse_ypos;
			mouse->u.data.z = 0;
			mouse->u.data.buttons = scp->mouse_buttons;
			break;

		case MOUSE_ACTION:
		case MOUSE_MOTION_EVENT:
			/* this should maybe only be settable from /dev/consolectl SOS */
			/* send out mouse event on /dev/sysmouse */

			s = spltty();
			if (mouse->u.data.x != 0 || mouse->u.data.y != 0) {
				cur_console->mouse_xpos += mouse->u.data.x;
				cur_console->mouse_ypos += mouse->u.data.y;
				set_mouse_pos(cur_console);
			}
			f = 0;
			if (mouse->operation == MOUSE_ACTION) {
				f = cur_console->mouse_buttons ^ mouse->u.data.buttons;
				cur_console->mouse_buttons = mouse->u.data.buttons;
			}
			splx(s);

			mouse_status.dx += mouse->u.data.x;
			mouse_status.dy += mouse->u.data.y;
			mouse_status.dz += mouse->u.data.z;
			if (mouse->operation == MOUSE_ACTION)
				mouse_status.button = mouse->u.data.buttons;
			mouse_status.flags |= (
					(mouse->u.data.x || mouse->u.data.y || mouse->u.data.z) ? MOUSE_POSCHANGED : 0)
					| (mouse_status.obutton ^ mouse_status.button);

			if (mouse_status.flags == 0)
				return 0;

			if (ISTEXTSC(cur_console) && (cur_console->status & MOUSE_ENABLED))
				cur_console->status |= MOUSE_VISIBLE;

			if ((MOUSE_TTY)->t_state & TS_ISOPEN) {
				u_char buf[MOUSE_SYS_PACKETSIZE];
				int j;

				/* the first five bytes are compatible with MouseSystems' */
				buf[0] = MOUSE_MSC_SYNC
						| butmap[mouse_status.button & MOUSE_STDBUTTONS];
				j = imax(imin(mouse->u.data.x, 255), -256);
				buf[1] = j >> 1;
				buf[3] = j - buf[1];
				j = -imax(imin(mouse->u.data.y, 255), -256);
				buf[2] = j >> 1;
				buf[4] = j - buf[2];
				for (j = 0; j < MOUSE_MSC_PACKETSIZE; j++)
					(*linesw[MOUSE_TTY->t_line].l_rint)(buf[j], MOUSE_TTY);
				if (mouse_level >= 1) { /* extended part */
					j = imax(imin(mouse->u.data.z, 127), -128);
					buf[5] = (j >> 1) & 0x7f;
					buf[6] = (j - (j >> 1)) & 0x7f;
					/* buttons 4-10 */
					buf[7] = (~mouse_status.button >> 3) & 0x7f;
					for (j = MOUSE_MSC_PACKETSIZE; j < MOUSE_SYS_PACKETSIZE;
							j++)
						(*linesw[MOUSE_TTY->t_line].l_rint)(buf[j],
						MOUSE_TTY);
				}
			}

			if (cur_console->mouse_signal) {
				cur_console->mouse_buttons = mouse->u.data.buttons;
				/* has controlling process died? */
				if (cur_console->mouse_proc
						&& (cur_console->mouse_proc
								!= pfind(cur_console->mouse_pid))) {
					cur_console->mouse_signal = 0;
					cur_console->mouse_proc = NULL;
					cur_console->mouse_pid = 0;
				} else
					psignal(cur_console->mouse_proc, cur_console->mouse_signal);
			} else if ((mouse->operation == MOUSE_ACTION) && f) {
				/* process button presses */
				if ((cur_console->mouse_buttons ^ mouse->u.data.buttons)
						&& !(cur_console->status & UNKNOWN_MODE)) {
					cur_console->mouse_buttons = mouse->u.data.buttons;
					if (cur_console->mouse_buttons & MOUSE_BUTTON1DOWN)
						mouse_cut_start(cur_console);
					else
						mouse_cut_end(cur_console);
					if ((cur_console->mouse_buttons & MOUSE_BUTTON2DOWN)
							|| (cur_console->mouse_buttons & MOUSE_BUTTON3DOWN))
						mouse_paste(cur_console);
				}
			}

			break;

		case MOUSE_BUTTON_EVENT:
			if ((mouse->u.event.id & MOUSE_BUTTONS) == 0)
				return EINVAL;
			if (mouse->u.event.value < 0)
				return EINVAL;

			if (mouse->u.event.value > 0) {
				cur_console->mouse_buttons |= mouse->u.event.id;
				mouse_status.button |= mouse->u.event.id;
			} else {
				cur_console->mouse_buttons &= ~mouse->u.event.id;
				mouse_status.button &= ~mouse->u.event.id;
			}
			mouse_status.flags |= mouse_status.obutton ^ mouse_status.button;
			if (mouse_status.flags == 0)
				return 0;

			if (ISTEXTSC(cur_console) && (cur_console->status & MOUSE_ENABLED))
				cur_console->status |= MOUSE_VISIBLE;

			if ((MOUSE_TTY)->t_state & TS_ISOPEN) {
				u_char buf[8];
				int i;

				buf[0] = MOUSE_MSC_SYNC
						| butmap[mouse_status.button & MOUSE_STDBUTTONS];
				buf[7] = (~mouse_status.button >> 3) & 0x7f;
				buf[1] = buf[2] = buf[3] = buf[4] = buf[5] = buf[6] = 0;
				for (i = 0;
						i
								< ((mouse_level >= 1) ?
										MOUSE_SYS_PACKETSIZE :
										MOUSE_MSC_PACKETSIZE); i++)
					(*linesw[(MOUSE_TTY)->t_line].l_rint)(buf[i], MOUSE_TTY);
			}

			if (cur_console->mouse_signal) {
				if (cur_console->mouse_proc
						&& (cur_console->mouse_proc
								!= pfind(cur_console->mouse_pid))) {
					cur_console->mouse_signal = 0;
					cur_console->mouse_proc = NULL;
					cur_console->mouse_pid = 0;
				} else
					psignal(cur_console->mouse_proc, cur_console->mouse_signal);
				break;
			}

			if (!ISTEXTSC(cur_console) || (cut_buffer == NULL))
				break;

			switch (mouse->u.event.id) {
			case MOUSE_BUTTON1DOWN:
				switch (mouse->u.event.value % 4) {
				case 0: /* up */
					mouse_cut_end(cur_console);
					break;
				case 1:
					mouse_cut_start(cur_console);
					break;
				case 2:
					mouse_cut_word(cur_console);
					break;
				case 3:
					mouse_cut_line(cur_console);
					break;
				}
				break;
			case MOUSE_BUTTON2DOWN:
				switch (mouse->u.event.value) {
				case 0: /* up */
					break;
				default:
					mouse_paste(cur_console);
					break;
				}
				break;
			case MOUSE_BUTTON3DOWN:
				switch (mouse->u.event.value) {
				case 0: /* up */
					if (!(cur_console->mouse_buttons & MOUSE_BUTTON1DOWN))
						mouse_cut_end(cur_console);
					break;
				default:
					mouse_cut_extend(cur_console);
					break;
				}
				break;
			}
			break;

		default:
			return EINVAL;
		}
		/* make screensaver happy */
		scrn_time_stamp = mono_time.tv_sec;
		return 0;
	}

		/* MOUSE_XXX: /dev/sysmouse ioctls */
	case MOUSE_GETHWINFO:
		/* get device information */
	{
		mousehw_t *hw = (mousehw_t*) data;

		if (tp != MOUSE_TTY)
			return ENOTTY;
		hw->buttons = 10; /* XXX unknown */
		hw->iftype = MOUSE_IF_SYSMOUSE;
		hw->type = MOUSE_MOUSE;
		hw->model = MOUSE_MODEL_GENERIC;
		hw->hwid = 0;
		return 0;
	}

	case MOUSE_GETMODE:
		/* get protocol/mode */
	{
		mousemode_t *mode = (mousemode_t*) data;

		if (tp != MOUSE_TTY)
			return ENOTTY;
		mode->level = mouse_level;
		switch (mode->level) {
		case 0:
			/* at this level, sysmouse emulates MouseSystems protocol */
			mode->protocol = MOUSE_PROTO_MSC;
			mode->rate = -1; /* unknown */
			mode->resolution = -1; /* unknown */
			mode->accelfactor = 0; /* disabled */
			mode->packetsize = MOUSE_MSC_PACKETSIZE;
			mode->syncmask[0] = MOUSE_MSC_SYNCMASK;
			mode->syncmask[1] = MOUSE_MSC_SYNC;
			break;

		case 1:
			/* at this level, sysmouse uses its own protocol */
			mode->protocol = MOUSE_PROTO_SYSMOUSE;
			mode->rate = -1;
			mode->resolution = -1;
			mode->accelfactor = 0;
			mode->packetsize = MOUSE_SYS_PACKETSIZE;
			mode->syncmask[0] = MOUSE_SYS_SYNCMASK;
			mode->syncmask[1] = MOUSE_SYS_SYNC;
			break;
		}
		return 0;
	}

	case MOUSE_SETMODE:
		/* set protocol/mode */
	{
		mousemode_t *mode = (mousemode_t*) data;

		if (tp != MOUSE_TTY)
			return ENOTTY;
		if ((mode->level < 0) || (mode->level > 1))
			return EINVAL;
		mouse_level = mode->level;
		return 0;
	}

	case MOUSE_GETLEVEL:
		/* get operation level */
		if (tp != MOUSE_TTY)
			return ENOTTY;
		*(int*) data = mouse_level;
		return 0;

	case MOUSE_SETLEVEL:
		/* set operation level */
		if (tp != MOUSE_TTY)
			return ENOTTY;
		if ((*(int*) data < 0) || (*(int*) data > 1))
			return EINVAL;
		mouse_level = *(int*) data;
		return 0;

	case MOUSE_GETSTATUS:
		/* get accumulated mouse events */
		if (tp != MOUSE_TTY)
			return ENOTTY;
		s = spltty();
		*(mousestatus_t*) data = mouse_status;
		mouse_status.flags = 0;
		mouse_status.obutton = mouse_status.button;
		mouse_status.dx = 0;
		mouse_status.dy = 0;
		mouse_status.dz = 0;
		splx(s);
		return 0;

#if notyet
	case MOUSE_GETVARS: /* get internal mouse variables */
	case MOUSE_SETVARS: /* set internal mouse variables */
		if (tp != MOUSE_TTY)
			return ENOTTY;
		return ENODEV;
#endif

	case MOUSE_READSTATE:
		/* read status from the device */
	case MOUSE_READDATA:
		/* read data from the device */
		if (tp != MOUSE_TTY)
			return ENOTTY;
		return ENODEV;
	}
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error != ENOIOCTL)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error != ENOIOCTL)
		return (error);
	return (ENOTTY);
}
