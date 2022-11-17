/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ioctl.h	1.4 (2.11BSD GTE) 1997/3/28
 */
/*
 * 2.11BSD's ioctl.h combines ttycom.h, filio.h & sockio.h into one
 */

/*
 * Ioctl definitions
 */
#ifndef	_SYS_IOCTL_H_
#define	_SYS_IOCTL_H_

#include <sys/ioccom.h>
//#include <sys/ttychars.h>

struct tchars {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};
struct ltchars {
	char	t_suspc;	/* stop process signal */
	char	t_dsuspc;	/* delayed stop process signal */
	char	t_rprntc;	/* reprint line */
	char	t_flushc;	/* flush output (toggles) */
	char	t_werasc;	/* word erase */
	char	t_lnextc;	/* literal next character */
};

/*
 * Structure for TIOCGETP and TIOCSETP ioctls.
 */

#ifndef _SGTTYB_
#define	_SGTTYB_
struct sgttyb {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	short	sg_flags;		/* mode flags */
};
#endif

/*
 * Window/terminal size structure.
 * This information is stored by the kernel
 * in order to provide a consistent interface,
 * but is not used by the kernel.
 *
 * Type must be "unsigned short" so that types.h not required.
 */
struct winsize {
	unsigned short	ws_row;			/* rows, in characters */
	unsigned short	ws_col;			/* columns, in characters */
	unsigned short	ws_xpixel;		/* horizontal size, pixels */
	unsigned short	ws_ypixel;		/* vertical size, pixels */
};

/*
 * Pun for SUN.
 */
/*
 * Pun for SunOS prior to 3.2.  SunOS 3.2 and later support TIOCGWINSZ
 * and TIOCSWINSZ (yes, even 3.2-3.5, the fact that it wasn't documented
 * non with standing).
 */
struct ttysize {
	unsigned short	ts_lines;
	unsigned short	ts_cols;
	unsigned short	ts_xxx;
	unsigned short	ts_yyy;
};
#define	TIOCGSIZE	TIOCGWINSZ
#define	TIOCSSIZE	TIOCSWINSZ

/*
 * tty ioctl commands
 */
#define	TIOCGETD			_IOR(t, 0, int)					/* get line discipline */
#define	TIOCSETD			_IOW(t, 1, int)					/* set line discipline */
#define	TIOCHPCL			_IO(t, 2)						/* hang up on last close */
#define	TIOCMODG			_IOR(t, 3, int)					/* get modem control state */
#define	TIOCMODS			_IOW(t, 4, int)					/* set modem control state */
#define	TIOCM_LE			0001							/* line enable */
#define	TIOCM_DTR			0002							/* data terminal ready */
#define	TIOCM_RTS			0004							/* request to send */
#define	TIOCM_ST			0010							/* secondary transmit */
#define	TIOCM_SR			0020							/* secondary receive */
#define	TIOCM_CTS			0040							/* clear to send */
#define	TIOCM_CAR			0100							/* carrier detect */
#define	TIOCM_CD			TIOCM_CAR
#define	TIOCM_RNG			0200							/* ring */
#define	TIOCM_RI			TIOCM_RNG
#define	TIOCM_DSR			0400							/* data set ready */
#define	TIOCGETP			_IOR(t, 8,struct sgttyb)		/* get parameters -- gtty */
#define	TIOCSETP			_IOW(t, 9,struct sgttyb)		/* set parameters -- stty */
#define	TIOCSETN			_IOW(t,10,struct sgttyb)		/* as above, but no flushtty */
#define	TIOCEXCL			_IO(t, 13)						/* set exclusive use of tty */
#define	TIOCNXCL			_IO(t, 14)						/* reset exclusive use of tty */
#define	TIOCFLUSH			_IOW(t, 16, int)				/* flush buffers */
#define	TIOCSETC			_IOW(t,17,struct tchars)		/* set special characters */
#define	TIOCGETC			_IOR(t,18,struct tchars)		/* get special characters */
#define	TIOCGETA			_IOR('t', 19, struct termios) 	/* get termios struct */
#define	TIOCSETA			_IOW('t', 20, struct termios) 	/* set termios struct */
#define	TIOCSETAW			_IOW('t', 21, struct termios) 	/* drain output, set */
#define	TIOCSETAF			_IOW('t', 22, struct termios) 	/* drn out, fls in, set */
/* 4.4BSD's TIOCGETD		_IOR('t', 26, int) 				 get line discipline */
/* 4.4BSD's TIOCSETD		_IOW('t', 27, int)				 set line discipline */
#define	TANDEM				0x00000001						/* send stopc on out q full */
#define	CBREAK				0x00000002						/* half-cooked mode */
						/* 	0x4 (old LCASE) */
#define	ECHO				0x00000008						/* echo input */
#define	CRMOD				0x00000010						/* map \r to \r\n on output */
#define	RAW					0x00000020						/* no i/o processing */
#define	ODDP				0x00000040						/* get/send odd parity */
#define	EVENP				0x00000080						/* get/send even parity */
#define	ANYP				0x000000c0						/* get any parity/send none */
						/* 	0x100 (old NLDELAY) */
						/* 	0x200 */
#define	XTABS				0x00000400	/* expand tabs on output */
						/* 	0x0800 (part of old XTABS) */
						/* 	0x1000 (old CRDELAY) */
						/* 	0x2000 */
						/* 	0x4000 (old VTDELAY) */
						/* 	0x8000 (old BSDELAY) */
#define	CRTBS				0x00010000						/* do backspacing for crt */
#define	PRTERA				0x00020000						/* \ ... / erase */
#define	CRTERA				0x00040000						/* " \b " to wipe out char */
						/* 	0x00080000 (old TILDE) */
#define	MDMBUF				0x00100000						/* start/stop output on carrier intr */
#define	LITOUT				0x00200000						/* literal output */
#define	TOSTOP				0x00400000						/* SIGSTOP on background output */
#define	FLUSHO				0x00800000						/* flush output to terminal */
#define	NOHANG				0x01000000						/* no SIGHUP on carrier drop */
#define	RTSCTS				0x02000000						/* use RTS/CTS flow control */
#define	CRTKIL				0x04000000						/* kill line with " \b " */
#define	PASS8				0x08000000
#define	CTLECH				0x10000000						/* echo control chars as ^X */
#define	PENDIN				0x20000000						/* tp->t_rawq needs reread */
#define	DECCTQ				0x40000000						/* only ^Q starts after ^S */
#define	NOFLSH				0x80000000						/* no output flush on signal */
/* locals, from 127 down */
#define	TIOCLBIS			_IOW(t, 127, int)				/* bis local mode bits */
#define	TIOCLBIC			_IOW(t, 126, int)				/* bic local mode bits */
#define	TIOCLSET			_IOW(t, 125, int)				/* set entire local mode word */
#define	TIOCLGET			_IOR(t, 124, int)				/* get local modes */
#define	LCRTBS				((int)(CRTBS>>16))
#define	LPRTERA				((int)(PRTERA>>16))
#define	LCRTERA				((int)(CRTERA>>16))
#define	LMDMBUF				((int)(MDMBUF>>16))
#define	LLITOUT				((int)(LITOUT>>16))
#define	LTOSTOP				((int)(TOSTOP>>16))
#define	LFLUSHO				((int)(FLUSHO>>16))
#define	LNOHANG				((int)(NOHANG>>16))
#define	LRTSCTS				((int)(RTSCTS>>16))
#define	LCRTKIL				((int)(CRTKIL>>16))
#define	LPASS8				((int)(PASS8>>16))
#define	LCTLECH				((int)(CTLECH>>16))
#define	LPENDIN				((int)(PENDIN>>16))
#define	LDECCTQ				((int)(DECCTQ>>16))
#define	LNOFLSH				((int)(NOFLSH>>16))
#define	TIOCSBRK			_IO(t, 123)						/* set break bit */
#define	TIOCCBRK			_IO(t, 122)						/* clear break bit */
#define	TIOCSDTR			_IO(t, 121)						/* set data terminal ready */
#define	TIOCCDTR			_IO(t, 120)						/* clear data terminal ready */
#define	TIOCGPGRP			_IOR(t, 119, int)				/* get pgrp of tty */
#define	TIOCSPGRP			_IOW(t, 118, int)				/* set pgrp of tty */
#define	TIOCSLTC			_IOW(t,117,struct ltchars)		/* set local special chars */
#define	TIOCGLTC			_IOR(t,116,struct ltchars)		/* get local special chars */
#define	TIOCOUTQ			_IOR(t, 115, int)				/* output queue size */
#define	TIOCSTI				_IOW(t, 114, char)				/* simulate terminal input */
#define	TIOCNOTTY			_IO(t, 113)						/* void tty association */
#define	TIOCPKT				_IOW(t, 112, int)				/* pty: set/clear packet mode */
#define	TIOCPKT_DATA		0x000							/* data packet */
#define	TIOCPKT_FLUSHREAD	0x001							/* flush packet */
#define	TIOCPKT_FLUSHWRITE	0x002							/* flush packet */
#define	TIOCPKT_STOP		0x004							/* stop output */
#define	TIOCPKT_START		0x008							/* start output */
#define	TIOCPKT_NOSTOP		0x010							/* no more ^S, ^Q */
#define	TIOCPKT_DOSTOP		0x020							/* now do ^S ^Q */
#define	TIOCPKT_IOCTL		0x040							/* state change of pty driver */
#define	TIOCSTOP			_IO(t, 111)						/* stop output, like ^S */
#define	TIOCSTART			_IO(t, 110)						/* start output, like ^Q */
#define	TIOCMSET			_IOW(t, 109, int)				/* set all modem bits */
#define	TIOCMBIS			_IOW(t, 108, int)				/* bis modem bits */
#define	TIOCMBIC			_IOW(t, 107, int)				/* bic modem bits */
#define	TIOCMGET			_IOR(t, 106, int)				/* get all modem bits */
#define	TIOCREMOTE			_IOW(t, 105, int)				/* remote input editing */
#define	TIOCGWINSZ			_IOR(t, 104, struct winsize)	/* get window size */
#define	TIOCSWINSZ			_IOW(t, 103, struct winsize)	/* set window size */
#define	TIOCUCNTL			_IOW(t, 102, int)				/* pty: set/clr usr cntl mode */
#define	TIOCSTAT			_IOW('t', 101, int)				/* generate status message */
#define	UIOCCMD(n)			_IO(u, n)						/* usr cntl op "n" */
#define	TIOCGSID			_IOR('t', 99, int)				/* get sid of tty */
#define	TIOCCONS			_IOW('t', 98, int)				/* become virtual console */
#define	TIOCSCTTY	 		_IO('t', 97)					/* become controlling tty */
#define	TIOCEXT				_IOW('t', 96, int)				/* pty: external processing */
#define	TIOCSIG		 		_IO('t', 95)					/* pty: generate signal */
#define	TIOCDRAIN	 		_IO('t', 94)					/* wait till output drained */
#define TIOCMSBIDIR			_IOW('t', 93, int)				/* modem: set bidir cap. */
#define TIOCMGBIDIR			_IOR('t', 92, int)				/* modem: get bidir cap. */
#define TIOCMSDTRWAIT		_IOW('t', 91, int)				/* modem: set wait on close */
#define TIOCMGDTRWAIT		_IOR('t', 90, int)				/* modem: get wait on close */
#define TIOCTIMESTAMP	 	_IOR('t', 89, struct timeval)	/* get timestamp of last interrupt for xntp. */
#define TIOCDSIMICROCODE 	_IO('t', 88)					/* Download microcode to DSI Softmodem */
#define	TIOCGFLAGS			_IOR('t', 87, int)				/* get device flags */
#define	TIOCSFLAGS			_IOW('t', 86, int)				/* set device flags */
#define	TIOCDCDTIMESTAMP 	_IOR('t', 85, struct timeval) 	/* get timestamp of last Cd rise, stamp next rise */
#define	TIOCFLAG_SOFTCAR	0x080							/* ignore hardware carrier */
#define	TIOCFLAG_CLOCAL		0x100							/* set clocal on open */
#define	TIOCFLAG_CRTSCTS	0x120							/* set crtscts on open */
#define	TIOCFLAG_MDMBUF		0x140							/* set mdmbuf on open */
#define	TIOCFLAG_PPS		0x180							/* call hardpps on carrier up */

#define	TTYDISC				0								/* termios tty line discipline */
#define	NTTYDISC			1								/* new tty discipline */
#define	OTTYDISC			2								/* old, v7 std tty driver */
#define	NETLDISC			3								/* line discip for berk net */
#define	TABLDISC			4								/* tablet discipline */
#define	SLIPDISC			5								/* serial IP discipline */
#define PPPDISC				6								/* PPP discipline */

/* Generic file-descriptor ioctl's. */
#define	FIOCLEX				_IO(f, 1)						/* set exclusive use on fd */
#define	FIONCLEX			_IO(f, 2)						/* remove exclusive use */
/* another local */
/* should use off_t for FIONREAD but that would require types.h */
#define	FIONREAD			_IOR(f, 127, long)				/* get # bytes to read */
#define	FIONBIO				_IOW(f, 126, int)				/* set/clear non-blocking i/o */
#define	FIOASYNC			_IOW(f, 125, int)				/* set/clear async i/o */
#define	FIOSETOWN			_IOW(f, 124, int)				/* set owner */
#define	FIOGETOWN			_IOR(f, 123, int)				/* get owner */
#define	FIODTYPE			_IOR(f, 122, int)				/* get d_flags type part */
#define	FIOGETLBA			_IOR(f, 121, int)				/* get start blk # */

/* Socket ioctl's. */
#define	SIOCSHIWAT			_IOW(s,  0, int)				/* set high watermark */
#define	SIOCGHIWAT			_IOR(s,  1, int)				/* get high watermark */
#define	SIOCSLOWAT			_IOW(s,  2, int)				/* set low watermark */
#define	SIOCGLOWAT			_IOR(s,  3, int)				/* get low watermark */
#define	SIOCATMARK			_IOR(s,  7, int)				/* at oob mark? */
#define	SIOCSPGRP			_IOW(s,  8, int)				/* set process group */
#define	SIOCGPGRP			_IOR(s,  9, int)				/* get process group */

#define	SIOCADDRT			_IOW(r, 10, struct rtentry)		/* add route */
#define	SIOCDELRT			_IOW(r, 11, struct rtentry)		/* delete route */

#define	SIOCSIFADDR			_IOW(i, 12, struct ifreq)		/* set ifnet address */
#define	SIOCGIFADDR			_IOWR(i,13, struct ifreq)		/* get ifnet address */
#define	SIOCSIFDSTADDR		_IOW(i, 14, struct ifreq)		/* set p-p address */
#define	SIOCGIFDSTADDR		_IOWR(i,15, struct ifreq)		/* get p-p address */
#define	SIOCSIFFLAGS		_IOW(i, 16, struct ifreq)		/* set ifnet flags */
#define	SIOCGIFFLAGS		_IOWR(i,17, struct ifreq)		/* get ifnet flags */
#define	SIOCGIFBRDADDR		_IOWR(i,18, struct ifreq)		/* get broadcast addr */
#define	SIOCSIFBRDADDR		_IOW(i,19, struct ifreq)		/* set broadcast addr */
#define	SIOCGIFCONF			_IOWR(i,20, struct ifconf)		/* get ifnet list */
#define	SIOCGIFNETMASK		_IOWR(i,21, struct ifreq)		/* get net addr mask */
#define	SIOCSIFNETMASK		_IOW(i,22, struct ifreq)		/* set net addr mask */
#define	SIOCGIFMETRIC		_IOWR(i,23, struct ifreq)		/* get IF metric */
#define	SIOCSIFMETRIC		_IOW(i,24, struct ifreq)		/* set IF metric */
#define	SIOCDIFADDR	 		_IOW('i', 25, struct ifreq)		/* delete IF addr */
#define	SIOCAIFADDR	 		_IOW('i', 26, struct ifaliasreq)/* add/chg IF alias */

#define	SIOCSARP			_IOW(i, 30, struct arpreq)		/* set arp entry */
#define	SIOCGARP			_IOWR(i,31, struct arpreq)		/* get arp entry */
#define	SIOCDARP			_IOW(i, 32, struct arpreq)		/* delete arp entry */

#define	SIOCADDMULTI		_IOW('i', 49, struct ifreq)		/* add m'cast addr */
#define	SIOCDELMULTI	 	_IOW('i', 50, struct ifreq)		/* del m'cast addr */

#ifndef _KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
int	ioctl(/*int, unsigned long, ...*/);
__END_DECLS
#endif /* !KERNEL */
#endif /* !_SYS_IOCTL_H_ */

/*
 * Keep outside _SYS_IOCTL_H_
 * Compatability with old terminal driver
 *
 * Source level -> #define USE_OLD_TTY
 * Kernel level -> options COMPAT_43 or COMPAT_SUNOS
 */
#if defined(USE_OLD_TTY) || defined(COMPAT_43) || defined(COMPAT_SUNOS)
#include <sys/ioctl_compat.h>
#endif
