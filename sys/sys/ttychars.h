/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ttychars.h	7.2 (2.11BSD) 1997/4/15
 */

/*
 * User visible structures and constants
 * related to terminal handling.
 */
#ifndef _SYS_TTYCHARS_H_
#define	_SYS_TTYCHARS_H_

struct ttychars {
	char	tc_erase;	/* erase last character */
	char	tc_kill;	/* erase entire line */
	char	tc_intrc;	/* interrupt */
	char	tc_quitc;	/* quit */
	char	tc_startc;	/* start output */
	char	tc_stopc;	/* stop output */
	char	tc_eofc;	/* end-of-file */
	char	tc_brkc;	/* input delimiter (like nl) */
	char	tc_suspc;	/* stop process signal */
	char	tc_dsuspc;	/* delayed stop process signal */
	char	tc_rprntc;	/* reprint line */
	char	tc_flushc;	/* flush output (toggles) */
	char	tc_werasc;	/* word erase */
	char	tc_lnextc;	/* literal next character */
	char	tc_min;
	char 	tc_ctime;
};

/* Control Character Defaults */
#define	CTRL(c)			('c'&037)
#define	_POSIX_VDISABLE	((unsigned char)'\377')
#define	CCEQ(val,c)		(c == val ? val != _POSIX_VDISABLE : 0)

/* default special characters */
#define	CERASE		0177
#define	CKILL		CTRL(u)
#define	CINTR		CTRL(c)
#define	CQUIT		034		/* FS, ^\ */
#define	CSTART		CTRL(q)
#define	CSTOP		CTRL(s)
#define	CEOF		CTRL(d)
#define	CEOT		CEOF
#define	CBRK		_POSIX_VDISABLE
#define	CSUSP		CTRL(z)
#define	CDSUSP		CTRL(y)
#define	CRPRNT		CTRL(r)
#define	CFLUSH		CTRL(o)
#define	CWERASE		CTRL(w)
#define	CLNEXT		CTRL(v)
#define	CMIN		1
#define	CTIME		0
/* compat */
#define	CEOL		CBRK
#define	CSTATUS		CBRK
#define	CDISCARD	CFLUSH
#define	CREPRINT	CRPRNT

#endif /* !_SYS_TTYCHARS_H_ */
