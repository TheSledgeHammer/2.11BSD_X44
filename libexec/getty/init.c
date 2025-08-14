/*	$NetBSD: init.c,v 1.18 2013/08/11 05:42:41 dholland Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if	!defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#)init.c	5.2.2 (2.11BSD GTE) 1997/3/28";
#endif

/*
 * Getty table initializations.
 *
 * Melbourne getty.
 */
//#include <sgtty.h>
#include <termios.h>

#include "gettytab.h"
#include "pathnames.h"
#include "extern.h"

/*
extern	struct sgttyb tmode;
extern	struct tchars tc;
extern	struct ltchars ltc;
*/
extern	struct termios tmode;
extern	char hostname[];

struct	gettystrs gettystrs[] = {
	{ "nx" },						    /* next table */
	{ "cl" },						    /* screen clear characters */
	{ "im" },						    /* initial message */
	{ "lm", "login: " },			    /* login message */
	{ "er", (char *)&tmode.c_verase },	/* erase character */
	{ "kl", (char *)&tmode.c_vkill },	/* kill character */
	{ "et", (char *)&tmode.c_veof },	/* eof chatacter (eot) */
	{ "pc", "" },					    /* pad character */
	{ "tt" },						    /* terminal type */
	{ "ev" },						    /* enviroment */
	{ "lo", _PATH_LOGIN },			    /* login program */
	{ "hn", hostname },				    /* host name */
	{ "he" },						    /* host name edit */
	{ "in", (char *)&tmode.c_vintr },	/* interrupt char */
	{ "qu", (char *)&tmode.c_vquit },	/* quit char */
	{ "xn", (char *)&tmode.c_vstart },	/* XON (start) char */
	{ "xf", (char *)&tmode.c_vstop },	/* XOFF (stop) char */
	{ "bk", (char *)&tmode.c_veol },	/* brk char (alt \n) */
	{ "su", (char *)&tmode.c_vsusp },	/* suspend char */
	{ "ds", (char *)&tmode.c_vdsusp },	/* delayed suspend */
	{ "rp", (char *)&tmode.c_vreprint },/* reprint char */
	{ "fl", (char *)&tmode.c_vdiscard },/* flush output */
	{ "we", (char *)&tmode.c_vwerase },	/* word erase */
	{ "ln", (char *)&tmode.c_vlnext },	/* literal next */
	{ "st", (char *)&tmode.c_vstatus },	/* status */
	{ "b2", (char *)&tmode.c_veol2 },	/* alt brk char */
	{ "pp", },						    /* ppp login program */
	{ "if", },						    /* sysv-like 'issue' filename */
	{ "al", },						    /* user to auto-login */
	{ 0 }
};

struct	gettynums gettynums[] = {
	{ "is" },			/* input speed */
	{ "os" },			/* output speed */
	{ "sp" },			/* both speeds */
	{ "nd" },			/* newline delay */
	{ "cd" },			/* carriage-return delay */
	{ "td" },			/* tab delay */
	{ "fd" },			/* form-feed delay */
	{ "bd" },			/* backspace delay */
	{ "to" },			/* timeout */
	{ "f0" },			/* output flags */
	{ "f1" },			/* input flags */
	{ "f2" },			/* user mode flags */
	{ "pf" },			/* delay before flush at 1st prompt */
	{ "c0" },			/* output c_flags */
	{ "c1" },			/* input c_flags */
	{ "c2" },			/* user mode c_flags */
	{ "i0" },			/* output i_flags */
	{ "i1" },			/* input i_flags */
	{ "i2" },			/* user mode i_flags */
	{ "l0" },			/* output l_flags */
	{ "l1" },			/* input l_flags */
	{ "l2" },			/* user mode l_flags */
	{ "o0" },			/* output o_flags */
	{ "o1" },			/* input o_flags */
	{ "o2" },			/* user mode o_flags */
	{ 0 }
};

struct	gettyflags gettyflags[] = {
	{ "ht",	0 },			/* has tabs */
	{ "nl",	1 },			/* has newline char */
	{ "ep",	0 },			/* even parity */
	{ "op",	0 },			/* odd parity */
	{ "ap",	0 },			/* any parity */
	{ "ec",	1 },			/* no echo */
	{ "co",	0 },			/* console special */
	{ "cb",	0 },			/* crt backspace */
	{ "ck",	0 },			/* crt kill */
	{ "ce",	0 },			/* crt erase */
	{ "pe",	0 },			/* printer erase */
	{ "rw",	1 },			/* don't use raw */
	{ "xc",	1 },			/* don't ^X ctl chars */
	{ "lc",	0 },			/* terminal las lower case */
	{ "uc",	0 },			/* terminal has no lower case */
	{ "ig",	0 },			/* ignore garbage */
	{ "ps",	0 },			/* do port selector speed select */
	{ "hc",	1 },			/* don't set hangup on close */
	{ "ub", 0 },			/* unbuffered output */
	{ "ab", 0 },			/* auto-baud detect with '\r' */
	{ "dx", 0 },			/* set decctlq */
	{ "np", 0 },			/* no parity at all (8bit chars) */
	{ "mb", 0 },			/* do MDMBUF flow control */
	{ "cs", 0 },			/* clear screen based on term type */
	{ "nn", 0 },			/* don't prompt for login name */
	{ 0 }
};
