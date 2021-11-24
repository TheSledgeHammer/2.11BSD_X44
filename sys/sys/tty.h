/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty.h	7.1.2 (2.11BSD GTE) 1997/4/10
 */

#ifndef _SYS_TTY_H_
#define _SYS_TTY_H_

#include <sys/termios.h>
#include <sys/select.h>
#include <sys/ttydefaults.h>

/*
 * A clist structure is the head of a linked list queue
 * of characters.  The characters are stored in blocks
 * containing a link and CBSIZE (param.h) characters. 
 * The routines in tty_subr.c manipulate these structures.
 */
struct clist {
	int						c_cc;			/* character count */
	char					*c_cf;			/* pointer to first char */
	char					*c_cl;			/* pointer to last char */

	u_char					*c_cs;			/* start of ring buffer */
	u_char					*c_ce;			/* c_ce + c_len */
	u_char					*c_cq;			/* N bits/bytes long, see tty_subr.c */
	int						c_cn;			/* total ring buffer length */

	/* clist allocation from cblock */
	int						c_cbcount;		/* Number of cblocks. */
	int						c_cbmax;		/* Max # cblocks allowed for this clist. */
	int						c_cbreserved;	/* # cblocks reserved for this clist. */
};

/*
 * Per-tty structure.
 *
 * Should be split in two, into device and tty drivers.
 * Glue could be masks of what to echo and circular buffer
 * (low, high, timeout).
 */
struct tty {
	union {
		struct {
			struct	clist 	T_rawq;											/* Device raw input queue. */
			struct	clist 	T_canq;											/* Device canonical queue. */
		} t_t;

#define	t_rawq				t_nu.t_t.T_rawq									/* raw characters or partial line */
#define	t_canq				t_nu.t_t.T_canq									/* raw characters or partial line */
		struct {
			struct	buf 	*T_bufp;
			char			*T_cp;
			int				T_inbuf;
			int				T_rec;
		} t_n;

#define	t_bufp				t_nu.t_n.T_bufp									/* buffer allocated to protocol */
#define	t_cp				t_nu.t_n.T_cp									/* pointer into the ripped off buffer */
#define	t_inbuf				t_nu.t_n.T_inbuf								/* number chars in the buffer */
#define	t_rec				t_nu.t_n.T_rec									/* have a complete record */
	} t_nu;

	struct	pgrp 			*t_pgrp;										/* Foreground process group. */
	struct	session 		*t_session;										/* Enclosing session. */
	struct	termios 		t_termios;										/* Termios state. */
	int						t_hiwat;										/* High water mark. */
	int						t_lowat;										/* Low water mark. */
	int						t_gen;											/* Generation number. */
	struct	clist 			t_outq;											/* Device output queue. */
	long					t_outcc;										/* Output queue statistics. */
	struct	proc 			*t_rsel;										/* Tty read/oob select. */
	struct	proc 			*t_wsel;										/* Tty write select. */
	caddr_t					T_LINEP;										/* ### */
	caddr_t					t_addr;											/* ??? */
	dev_t					t_dev;											/* device */
	int						t_wopen;										/* Processes waiting for open. */
	long					t_flags;										/* Tty flags. */
	long					t_state;										/* Device and driver (TS*) state. */
	char					t_delct;										/* tty */
	char					t_line;											/* Interface to device drivers. */
	char					t_col;											/* Tty output column. */
	char					t_ispeed, t_ospeed;								/* device */
	char					t_rocount, t_rocol;								/* tty */
	struct	ttychars 		t_chars;										/* tty */
	struct	winsize 		t_winsize;										/* window size */

	void					(*t_oproc) (struct tty *);						/* device *//* Start output. */
	void					(*t_stop) (struct tty *, int);					/* Stop output. */
	int						(*t_param) (struct tty *, struct termios *);	/* Set hardware state. */

	int						(*t_hwiflow)(struct tty *, int);				/* Set hardware flow control. */


/* be careful of tchars & co. */
#define	t_erase				t_chars.tc_erase
#define	t_kill				t_chars.tc_kill
#define	t_intrc				t_chars.tc_intrc
#define	t_quitc				t_chars.tc_quitc
#define	t_startc			t_chars.tc_startc
#define	t_stopc				t_chars.tc_stopc
#define	t_eofc				t_chars.tc_eofc
#define	t_brkc				t_chars.tc_brkc
#define	t_suspc				t_chars.tc_suspc
#define	t_dsuspc			t_chars.tc_dsuspc
#define	t_rprntc			t_chars.tc_rprntc
#define	t_flushc			t_chars.tc_flushc
#define	t_werasc			t_chars.tc_werasc
#define	t_lnextc			t_chars.tc_lnextc
};

#define	t_cc				t_termios.c_cc
#define	t_cflag				t_termios.c_cflag
#define	t_iflag				t_termios.c_iflag
#define	t_ispeed			t_termios.c_ispeed
#define	t_lflag				t_termios.c_lflag
#define	t_min				t_termios.c_min
#define	t_oflag				t_termios.c_oflag
#define	t_ospeed			t_termios.c_ospeed
#define	t_time				t_termios.c_time

#define	TTIPRI		28	/* Sleep priority for tty reads. */
#define	TTOPRI		29	/* Sleep priority for tty writes. */

/* limits */
#define	NSPEEDS		16
#define	TTMASK		15
#define	OBUFSIZ		100
#define	TTYHOG		1024

#ifdef KERNEL
short						tthiwat[NSPEEDS], ttlowat[NSPEEDS];
#define	TTHIWAT(tp)			tthiwat[(tp)->t_ospeed&TTMASK]
#define	TTMLOWAT(tp)		ttlowat[(tp)->t_ospeed&TTMASK]
#define	TTMAXHIWAT			roundup(2048, CBSIZE)
#define	TTMINHIWAT			roundup(100, CBSIZE)
#define TTMAXLOWAT 			256
#define TTMINLOWAT 			32
extern	struct ttychars 	ttydefaults;
#endif

/* internal state bits */
#define	TS_TIMEOUT	0x000001L	/* delay timeout in progress */
#define	TS_WOPEN	0x000002L	/* waiting for open to complete */
#define	TS_ISOPEN	0x000004L	/* device is open */
#define	TS_FLUSH	0x000008L	/* outq has been flushed during DMA */
#define	TS_CARR_ON	0x000010L	/* software copy of carrier-present */
#define	TS_BUSY		0x000020L	/* output in progress */
#define	TS_ASLEEP	0x000040L	/* wakeup when output done */
#define	TS_XCLUDE	0x000080L	/* exclusive-use flag against open */
#define	TS_TTSTOP	0x000100L	/* output stopped by ctl-s */
#define	TS_HUPCLS	0x000200L	/* hang up upon last close */
#define	TS_TBLOCK	0x000400L	/* tandem queue blocked */
#define	TS_RCOLL	0x000800L	/* collision in read select */
#define	TS_WCOLL	0x001000L	/* collision in write select */
#define	TS_ASYNC	0x004000L	/* tty in async i/o mode */

/* state for intra-line fancy editing work */
#define	TS_BKSL		0x008000L	/* State for lowercase \ work. */
#define	TS_ERASE	0x040000L	/* within a \.../ for PRTRUB */
#define	TS_LNCH		0x080000L	/* next character is literal */
#define	TS_TYPEN	0x100000L	/* retyping suspended input (PENDIN) */
#define	TS_CNTTB	0x200000L	/* counting tab width; leave FLUSHO alone */
#define	TS_LOCAL	(TS_BKSL|TS_ERASE|TS_LNCH|TS_TYPEN|TS_CNTTB)

/* define partab character types */
#define	ORDINARY	0
#define	CONTROL		1
#define	BACKSPACE	2
#define	NEWLINE		3
#define	TAB			4
#define	VTAB		5
#define	RETURN		6

struct speedtab {
	int sp_speed;			/* Speed. */
	int sp_code;			/* Code. */
};

/* Modem control commands (driver). */
#define	DMSET		0
#define	DMBIS		1
#define	DMBIC		2
#define	DMGET		3

/* Flags on a character passed to ttyinput. */
#define	TTY_CHARMASK	0x000000ff	/* Character mask */
#define	TTY_QUOTE		0x00000100	/* Character quoted */
#define	TTY_ERRORMASK	0xff000000	/* Error mask */
#define	TTY_FE			0x01000000	/* Framing error or BREAK condition */
#define	TTY_PE			0x02000000	/* Parity error */

/* Is tp controlling terminal for p? */
#define	isctty(p, tp)							\
	((p)->p_session == (tp)->t_session && (p)->p_flag & P_CONTROLT)

/* Is p in background of tp? */
#define	isbackground(p, tp)						\
	(isctty((p), (tp)) && (p)->p_pgrp != (tp)->t_pgrp)

#ifdef KERNEL
extern	struct ttychars ttydefaults;

/* Symbolic sleep message strings. */
extern	 char ttyin[], ttyout[], ttopen[], ttclos[], ttybg[], ttybuf[];

extern void	ctty_init(struct devswtable *);
extern void pty_init(struct devswtable *);
extern void tty_conf_init(struct devswtable *);

void clist_alloc_cblocks (struct clist *q, int ccmax, int ccres);
void clist_free_cblocks (struct clist *q);

int	 b_to_q (char *cp, int cc, struct clist *q);
void catq (struct clist *from, struct clist *to);
int	 getc (struct clist *q);
void ndflush (struct clist *q, int cc);
int	 ndqb (struct clist *q, int flag);
char *nextc (struct clist *q, char *cp, int *c);
int	 putc (int c, struct clist *q);
int	 q_to_b (struct clist *q, char *cp, int cc);
int	 unputc (struct clist *q);

int	 nullmodem (struct tty *tp, int flag));
int	 tputchar (int c, struct tty *tp));
int	 ttioctl (struct tty *tp, int com, void *data, int flag));
int	 ttread (struct tty *tp, struct uio *uio, int flag));
void ttrstrt (void *tp));
int	 ttselect (dev_t device, int rw, struct proc *p));
void ttsetwater (struct tty *tp));
int	 ttspeedtab (int speed, struct speedtab *table));
int	 ttstart (struct tty *tp));
void ttwakeup (struct tty *tp));
void ttwwakeup (struct tty *tp));
int	 ttwrite (struct tty *tp, struct uio *uio, int flag));
void ttychars (struct tty *tp));
int	 ttycheckoutq (struct tty *tp, int wait));
int	 ttyclose (struct tty *tp));
void ttyflush (struct tty *tp, int rw));
void ttyinfo (struct tty *tp));
int	 ttyinput (int c, struct tty *tp));
int	 ttylclose (struct tty *tp, int flag));
int	 ttymodem (struct tty *tp, int flag));
int	 ttyopen (dev_t device, struct tty *tp));
int	 ttyoutput (int c, struct tty *tp));
void ttypend (struct tty *tp));
void ttyretype (struct tty *tp));
void ttyrub (int c, struct tty *tp));
int	 ttysleep (struct tty *tp, void *chan, int pri, char *wmesg, int timeout);
int	 ttywait (struct tty *tp);
int	 ttywflush (struct tty *tp);
struct tty *ttymalloc (void);
void ttyfree (struct tty *tp);
void tty_init_console(struct tty *tp, speed_t speed);

/* From tty_ctty.c. */
int	cttyioctl (dev_t dev, int cmd, caddr_t addr, int flag, struct proc *p);
int	cttyopen (dev_t dev, int flag, int mode, struct proc *p);
int	cttyread (dev_t dev, struct uio *uio, int flag);
int cttywrite(dev_t dev, struct uio *uio, int flag);
int cttypoll(dev_t dev, int events, struct proc *p);
int	cttykqfilter(dev_t dev, struct knote *kn);
int	cttyselect (dev_t dev, int flag, struct proc *p);

/* From tty_tty.c. */
int	syopen(dev_t dev, int flag, int type);
int	syread(dev_t dev, struct uio *uio, int flag);
int	sywrite(dev_t dev, struct uio *uio, int flag);
int syioctl(dev_t dev, u_int cmd, caddr_t addr, int flag);
int	sypoll(dev_t dev, int events);
int	sykqfilter(dev_t dev, struct knote *kn);
int	syselect(dev_t dev, int flag);

#endif
#endif /* !_SYS_TTY_H_ */
