/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_tb.c	1.2 (2.11BSD GTE) 11/29/94
 */

#include <sys/cdefs.h>

#include "tb.h"

/*
 * Line discipline for RS232 tablets;
 * supplies binary coordinate data.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/tablet.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/uio.h>

/*
 * Tablet configuration table.
 */
struct	tbconf {
	short	tbc_recsize;		/* input record size in bytes */
	short	tbc_uiosize;		/* size of data record returned user */
	int		tbc_sync;			/* mask for finding sync byte/bit */
	void	(*tbc_decode)();	/* decoding routine */
	char	*tbc_run;			/* enter run mode sequence */
	char	*tbc_point;			/* enter point mode sequence */
	char	*tbc_stop;			/* stop sequence */
	char	*tbc_start;			/* start/restart sequence */
	int		tbc_flags;
#define	TBF_POL	0x1				/* polhemus hack */
};

static void gtcodecode(char *, struct gtcopos *);
static void tbdecode(char *, struct tbpos *);
static void tblresdecode(char *, struct tbpos *);
static void	tbhresdecode(char *, struct tbpos *);
static void poldecode(char *, struct polpos *);

struct	tbconf tbconf[TBTYPE] = {
		{ 0 },
		{ 5, sizeof (struct tbpos), 0200, tbdecode, "6", "4" },
		{ 5, sizeof (struct tbpos), 0200, tbdecode, "\1CN", "\1RT", "\2", "\4" },
		{ 8, sizeof (struct gtcopos), 0200, gtcodecode },
		{17, sizeof (struct polpos), 0200, poldecode, 0, 0, "\21", "\5\22\2\23", TBF_POL },
		{ 5, sizeof (struct tbpos), 0100, tblresdecode, "\1CN", "\1PT", "\2", "\4"},
		{ 6, sizeof (struct tbpos), 0200, tbhresdecode, "\1CN", "\1PT", "\2", "\4"},
};

/*
 * Tablet state
 */
struct tb {
	int					tbflags;		/* mode & type bits */
#define	TBMAXREC		17				/* max input record size */
	char				cbuf[TBMAXREC];	/* input buffer */
	union {
		struct	tbpos 	tbpos;
		struct	gtcopos gtcopos;
		struct	polpos 	polpos;
	} rets;								/* processed state */
#define NTBS 			16
} tb[NTBS];

/*
 * Open as tablet discipline; called on discipline change.
 */
/*ARGSUSED*/
int
tbopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct tb *tbp;

	if (tp->t_line == TABLDISC) {
		return (ENODEV);
	}
	ttywflush(tp);
	for (tbp = tb; tbp < &tb[NTBS]; tbp++) {
		if (tbp->tbflags == 0) {
			break;
		}
	}
	if (tbp >= &tb[NTBS]) {
		return (EBUSY);
	}
	tbp->tbflags = TBTIGER|TBPOINT;		/* default */
	tp->t_cp = tbp->cbuf;
	tp->t_inbuf = 0;
	bzero((caddr_t)&tbp->rets, sizeof(tbp->rets));
	tp->T_LINEP = (caddr_t)tbp;
	tp->t_flags |= LITOUT;
	return (0);
}

/*
 * Line discipline change or last device close.
 */
int
tbclose(tp, flag)
	register struct tty *tp;
	int flag;
{
	int modebits = TBPOINT|TBSTOP;

	tbioctl(tp, BIOSMODE, (caddr_t) &modebits, 0, curproc);
	((struct tb *)tp->T_LINEP)->tbflags = 0;
	tp->t_cp = 0;
	tp->t_inbuf = 0;
	tp->t_rawq.c_cc = 0;		/* clear queues -- paranoid */
	tp->t_canq.c_cc = 0;
	tp->t_line = 0;				/* paranoid: avoid races */
	return (0);
}

/*
 * Read from a tablet line.
 * Characters have been buffered in a buffer and decoded.
 */
int
tbread(tp, uio, flag)
	register struct tty *tp;
	struct uio *uio;
	int flag;
{
	register struct tb *tbp = (struct tb *)tp->T_LINEP;
	register struct tbconf *tc = &tbconf[tbp->tbflags & TBTYPE];
	int ret;

	if ((tp->t_state&TS_CARR_ON) == 0) {
		return (EIO);
	}
	ret = uiomove(&tbp->rets, tc->tbc_uiosize, uio);
	if (tc->tbc_flags&TBF_POL) {
		tbp->rets.polpos.p_key = ' ';
	}
	return (ret);
}

/*
 * Low level character input routine.
 * Stuff the character in the buffer, and decode
 * if all the chars are there.
 *
 * This routine could be expanded in-line in the receiver
 * interrupt routine to make it run as fast as possible.
 */
int
tbinput(c, tp)
	register int c;
	register struct tty *tp;
{
	register struct tb *tbp = (struct tb *)tp->T_LINEP;
	register struct tbconf *tc = &tbconf[tbp->tbflags & TBTYPE];

	if (tc->tbc_recsize == 0 || tc->tbc_decode == 0) {	/* paranoid? */
		return (EIO);
	}
	/*
	 * Locate sync bit/byte or reset input buffer.
	 */
	if ((c&tc->tbc_sync) || tp->t_inbuf == tc->tbc_recsize) {
		tp->t_cp = tbp->cbuf;
		tp->t_inbuf = 0;
	}
	*tp->t_cp++ = c&0177;
	/*
	 * Call decode routine only if a full record has been collected.
	 */
	if (++tp->t_inbuf == tc->tbc_recsize) {
		(*tc->tbc_decode)(tbp->cbuf, &tbp->rets);
	}
	return (0);
}

/*
 * Decode GTCO 8 byte format (high res, tilt, and pressure).
 */
static void
gtcodecode(cp, tbpos)
	register char *cp;
	register struct gtcopos *tbpos;
{
	tbpos->pressure = *cp >> 2;
	tbpos->status = (tbpos->pressure > 16) | TBINPROX; /* half way down */
	tbpos->xpos = (*cp++ & 03) << 14;
	tbpos->xpos |= *cp++ << 7;
	tbpos->xpos |= *cp++;
	tbpos->ypos = (*cp++ & 03) << 14;
	tbpos->ypos |= *cp++ << 7;
	tbpos->ypos |= *cp++;
	tbpos->xtilt = *cp++;
	tbpos->ytilt = *cp++;
	tbpos->scount++;
}

/*
 * Decode old Hitachi 5 byte format (low res).
 */
static void
tbdecode(cp, tbpos)
	register char *cp;
	register struct tbpos *tbpos;
{
	register char byte;

	byte = *cp++;
	tbpos->status = (byte&0100) ? TBINPROX : 0;
	byte &= ~0100;
	if (byte > 036) {
		tbpos->status |= 1 << ((byte-040)/2);
	}
	tbpos->xpos = *cp++ << 7;
	tbpos->xpos |= *cp++;
	if (tbpos->xpos < 256) {			/* tablet wraps around at 256 */
		tbpos->status &= ~TBINPROX;		/* make it out of proximity */
	}
	tbpos->ypos = *cp++ << 7;
	tbpos->ypos |= *cp++;
	tbpos->scount++;
}

/*
 * Decode new Hitach 5-byte format (low res).
 */
static void
tblresdecode(cp, tbpos)
	register char *cp;
	register struct tbpos *tbpos;
{
	*cp &= ~0100;		/* mask sync bit */
	tbpos->status = (*cp++ >> 2) | TBINPROX;
	tbpos->xpos = *cp++;
	tbpos->xpos |= *cp++ << 6;
	tbpos->ypos = *cp++;
	tbpos->ypos |= *cp++ << 6;
	tbpos->scount++;
}

/*
 * Decode new Hitach 6-byte format (high res).
 */
static void
tbhresdecode(cp, tbpos)
	register char *cp;
	register struct tbpos *tbpos;
{
	char byte;

	byte = *cp++;
	tbpos->xpos = (byte & 03) << 14;
	tbpos->xpos |= *cp++ << 7;
	tbpos->xpos |= *cp++;
	tbpos->ypos = *cp++ << 14;
	tbpos->ypos |= *cp++ << 7;
	tbpos->ypos |= *cp++;
	tbpos->status = (byte >> 2) | TBINPROX;
	tbpos->scount++;
}

/*
 * Polhemus decode.
 */
static void
poldecode(cp, polpos)
	register char *cp;
	register struct polpos *polpos;
{

	polpos->p_x = cp[4] | cp[3]<<7 | (cp[9] & 0x03) << 14;
	polpos->p_y = cp[6] | cp[5]<<7 | (cp[9] & 0x0c) << 12;
	polpos->p_z = cp[8] | cp[7]<<7 | (cp[9] & 0x30) << 10;
	polpos->p_azi = cp[11] | cp[10]<<7 | (cp[16] & 0x03) << 14;
	polpos->p_pit = cp[13] | cp[12]<<7 | (cp[16] & 0x0c) << 12;
	polpos->p_rol = cp[15] | cp[14]<<7 | (cp[16] & 0x30) << 10;
	polpos->p_stat = cp[1] | cp[0]<<7;
	if (cp[2] != ' ') {
		polpos->p_key = cp[2];
	}
}

/*ARGSUSED*/
int
tbioctl(tp, cmd, data, flag, p)
	struct tty *tp;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct tb *tbp = (struct tb *)tp->T_LINEP;

	switch (cmd) {

	case BIOGMODE:
		*(int *)data = tbp->tbflags & TBMODE;
		break;

	case BIOSTYPE:
		if (tbconf[*(int *)data & TBTYPE].tbc_recsize == 0 ||
		    tbconf[*(int *)data & TBTYPE].tbc_decode == 0) {
			return (EINVAL);
		}
		tbp->tbflags &= ~TBTYPE;
		tbp->tbflags |= *(int *)data & TBTYPE;
		break;
		/* fall thru... to set mode bits */

	case BIOSMODE: {
		register struct tbconf *tc;

		tbp->tbflags &= ~TBMODE;
		tbp->tbflags |= *(int *)data & TBMODE;
		tc = &tbconf[tbp->tbflags & TBTYPE];
		if (tbp->tbflags & TBSTOP) {
			if (tc->tbc_stop) {
				ttyoutput((int)tc->tbc_stop, tp);
			}
		} else if (tc->tbc_start) {
			ttyoutput((int)tc->tbc_start, tp);
		}
		if (tbp->tbflags & TBPOINT) {
			if (tc->tbc_point) {
				ttyoutput((int)tc->tbc_point, tp);
			}
		} else if (tc->tbc_run) {
			ttyoutput((int)tc->tbc_run, tp);
		}
		ttstart(tp);
		break;
	}

	case BIOGTYPE:
		*(int *)data = tbp->tbflags & TBTYPE;
		break;

	case TIOCSETD:
	case TIOCGETD:
	case TIOCGETP:
	case TIOCGETC:
		return (-1);		/* pass thru... */

	default:
		return (ENOTTY);
	}
	return (0);
}

void
tbattach(num)
       int num;
{
    /* stub to handle side effect of new config */
}
