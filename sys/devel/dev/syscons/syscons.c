/*
 * syscons.c
 *
 *  Created on: 31 Dec 2020
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

#include <dev/cons.h>

#include <devel/dev/syscons/syscons.h>
/*
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
*/

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

int
scstop(tp, flag)
	struct tty *tp;
	int flag;
{

}

int
scioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{

}

int
scpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

	return (cnpoll(dev, events, p));
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
