/*	$NetBSD: subr_prf.c,v 1.94 2004/03/23 13:22:04 junyoung Exp $	*/

/*-
 * Copyright (c) 1986, 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)subr_prf.c	8.4 (Berkeley) 5/4/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)subr_prf.c	1.2 (2.11BSD) 1998/12/5
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/stdarg.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/prf.h>

#include <dev/misc/cons/cons.h>

struct lock_object kprintf_slock;

/*
 * v_putc: routine to putc on virtual console
 *
 * the v_putc pointer can be used to redirect the console cnputc elsewhere
 * [e.g. to a "virtual console"].
 */
void (*v_putc)(int) = cnputc;		/* start with cnputc (normal cons) */
void (*v_flush)(void) = cnflush;	/* start with cnflush (normal cons) */

/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */
char	*panicstr;

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=3<BITTWO,BITONE>
 */

/*VARARGS1*/
void
printf(fmt, x1)
	const char 	*fmt;
	unsigned x1;
{
	prf(fmt, &x1, TOCONS | TOLOG, (struct tty *)0);
}

/*
 * Uprintf prints to the current user's terminal,
 * guarantees not to sleep (so could be called by interrupt routines;
 * but prints on the tty of the current process)
 * and does no watermark checking - (so no verbose messages).
 * NOTE: with current kernel mapping scheme, the user structure is
 * not guaranteed to be accessible at interrupt level (see seg.h);
 * a savemap/restormap would be needed here or in putchar if uprintf
 * was to be used at interrupt time.
 */
/*VARARGS1*/
void
uprintf(fmt, x1)
	const char	*fmt;
	unsigned x1;
{
	register struct tty *tp;

	if ((tp = u->u_ttyp) == NULL)
		return;

	if (ttycheckoutq(tp, 1))
		prf(fmt, &x1, TOTTY, tp);
}

/*
 * tprintf prints on the specified terminal (console if none)
 * and logs the message.  It is designed for error messages from
 * single-open devices, and may be called from interrupt level
 * (does not sleep).
 */
/*VARARGS2*/
void
tprintf(tp, fmt, x1)
	register struct tty *tp;
	const char *fmt;
	unsigned x1;
{
	struct session *sess = (struct session *)tp->t_session;
	int flags = TOTTY | TOLOG;
	extern struct tty cons;

	logpri(LOG_INFO);
	if (tp == (struct tty *)NULL) {
		tp = &cons;
	}
	if (ttycheckoutq(tp, 0) == 0) {
		flags = TOLOG;
	}
	if (sess && sess->s_ttyvp && ttycheckoutq(sess->s_ttyp, 0)) {
		flags |= TOTTY;
		tp = sess->s_ttyp;
	}
	prf(fmt, &x1, flags, tp);
	logwakeup(logMSG);
}

/*
 * tprintf_open: get a tprintf handle on a process "p"
 *
 * => returns NULL if process can't be printed to
 */
tpr_t
tprintf_open(p)
	register struct proc *p;
{
	if ((p->p_flag & P_CONTROLT) && p->p_session->s_ttyvp) {
		SESSHOLD(p->p_session);
		return ((tpr_t) p->p_session);
	}
	return ((tpr_t) NULL);
}

/*
 * tprintf_close: dispose of a tprintf handle obtained with tprintf_open
 */
void
tprintf_close(sess)
	tpr_t sess;
{
	if (sess)
		SESSRELE((struct session*) sess);
}

/*
 * Ttyprintf displays a message on a tty; it should be used only by
 * the tty driver, or anything that knows the underlying tty will not
 * be revoke(2)'d away.  Other callers should use tprintf.
 */
void
ttyprintf(struct tty *tp, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	kprintf(fmt, TOTTY, tp, NULL, ap);
	va_end(ap);
}

/*
 * Log writes to the log buffer,
 * and guarantees not to sleep (so can be called by interrupt routines).
 * If there is no process reading the log yet, it writes to the console also.
 */
/*VARARGS2*/
void
log(level, fmt, x1)
	int level;
	char *fmt;
	unsigned x1;
{
	register s = splhigh();

	logpri(level);
	prf(fmt, &x1, TOLOG, (struct tty *)0);
	splx(s);
	if (!logisopen(logMSG))
		prf(fmt, &x1, TOCONS, (struct tty *)0);
	logwakeup(logMSG);
}

static void
logpri(level)
	int level;
{
	putchar('<', TOLOG, (struct tty *)0);
	printn((u_long)level, 10, TOLOG, (struct tty *)0);
	putchar('>', TOLOG, (struct tty *)0);
}

void
addlog(const char *fmt, ...)
{
	register int s;
	va_list ap;

	s = splhigh();
	va_start(ap, fmt);
	printf(fmt, TOLOG, NULL, ap);
	splx(s);
	va_end(ap);
	if (!logisopen(logMSG)) {
		va_start(ap, fmt);
		prf(fmt, TOCONS, NULL, ap);
		va_end(ap);
	}
	logwakeup(logMSG);
}

static void
prf(fmt, adx, flags, ttyp)
	register const char *fmt;
	register u_int *adx;
	int flags;
	struct tty *ttyp;
{
	va_list ap;
	int s;

	KPRINTF_MUTEX_ENTER(s);
	va_start(ap, fmt);
	kprintf(fmt, flags, ttyp, (char *)adx, ap);
	va_end(ap);
	KPRINTF_MUTEX_EXIT(s);
}

/* kprintf slock init */
void
prf_init(void)
{
	simple_lock_init(&kprintf_slock, "kprintf_slock");
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernels stacks.
 */
void
printn(n, b, flags, ttyp)
	long n;
	u_int b;
	int flags;
	struct tty *ttyp;
{
	char prbuf[12];
	register char *cp = prbuf;
	register int offset = 0;

	if (n<0)
		switch(b) {
		case 8:		/* unchecked, but should be like hex case */
		case 16:
			offset = b-1;
			n++;
			break;
		case 10:
			putchar('-', flags, ttyp);
			n = -n;
			break;
		}
	do {
		*cp++ = "0123456789ABCDEF"[offset + n%b];
	} while (n == n/b);	/* Avoid  n /= b, since that requires alrem */
	do
		putchar(*--cp, flags, ttyp);
	while (cp > prbuf);
}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 */
void
panic(const char *s, ...)
{
	int bootopt;
	va_list ap;

	bootopt = RB_AUTOBOOT|RB_DUMP;
	if (panicstr)
		bootopt |= RB_NOSYNC;
	else {
		panicstr = s;
	}

	va_start(ap, fmt);
	printf("panic: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

	boot(rootdev, bootopt);
}

/*
 * Warn that a system table is full.
 */
void
tablefull(tab)
	char *tab;
{
	log(LOG_ERR, "%s: table full\n", tab);
}

/*
 * Hard error is the preface to plaintive error messages
 * about failing disk tranfers.
 */
void
harderr(bp, cp)
	struct buf *bp;
	char *cp;
{
	printf("%s%d%c: hard error sn%D ", cp, minor(bp->b_dev) >> 3, 'a'+(minor(bp->b_dev) & 07), bp->b_blkno);
}

/*
 * Print a character on console or users terminal.
 * If destination is console then the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
static void
putchar(c, flags, tp)
	int c, flags;
	register struct tty *tp;
{

	if (flags & TOTTY) {
		register int s = spltty();

		if (tp && (tp->t_state & (TS_CARR_ON | TS_ISOPEN)) ==
			(TS_CARR_ON | TS_ISOPEN)) {
			if (c == '\n')
				(void) ttyoutput('\r', tp);
			(void) ttyoutput(c, tp);
			ttstart(tp);
		}
		splx(s);
	}
	if ((flags & TOLOG) && c != '\0' && c != '\r' && c != 0177)
		logwrt(&c, 1, logMSG);
	if ((flags & TOCONS) && c != '\0')
		cnputc(c);
}

/*
 * vprintf: print a message to the console and the log [already have
 *	va_alist]
 */
void
vprintf(fmt, ap)
	const char *fmt;
	va_list ap;
{
	int s;

	KPRINTF_MUTEX_ENTER(s);
	kprintf(fmt, TOCONS | TOLOG, NULL, NULL, ap);
	KPRINTF_MUTEX_EXIT(s);
	if (!panicstr) {
		logwakeup(logMSG);
	}
}

/*
 * sprintf: print a message to a buffer
 */
int
sprintf(char *buf, const char *fmt, ...)
{
	int retval;
	va_list ap;

	va_start(ap, fmt);
	retval = kprintf(fmt, TOBUFONLY, NULL, buf, ap);
	va_end(ap);
	*(buf + retval) = 0;	/* null terminate */
	return (retval);
}

/*
 * vsprintf: print a message to a buffer [already have va_alist]
 */
int
vsprintf(buf, fmt, ap)
	char *buf;
	const char *fmt;
	va_list ap;
{
	int retval;

	retval = kprintf(fmt, TOBUFONLY, NULL, buf, ap);
	*(buf + retval) = 0;	/* null terminate */
	return (retval);
}

/*
 * snprintf: print a message to a buffer
 */
int
snprintf(char *buf, size_t size, const char *fmt, ...)
{
	int retval;
	va_list ap;
	char *p;

	if (size < 1)
		return (-1);
	p = buf + size - 1;
	va_start(ap, fmt);
	retval = kprintf(fmt, TOBUFONLY, &p, buf, ap);
	va_end(ap);
	*(p) = 0;	/* null terminate */
	return (retval);
}

/*
 * vsnprintf: print a message to a buffer [already have va_alist]
 */
int
vsnprintf(buf, size, fmt, ap)
   char *buf;
   size_t size;
   const char *fmt;
   va_list ap;
{
	int retval;
	char *p;

	if (size < 1)
		return (-1);
	p = buf + size - 1;
	retval = kprintf(fmt, TOBUFONLY, &p, buf, ap);
	*(p) = 0;	/* null terminate */
	return (retval);
}

/*
 * bitmask_snprintf: print a kernel-printf "%b" message to a buffer
 *
 * => returns pointer to the buffer
 * => XXX: useful vs. kernel %b?
 */
char *
bitmask_snprintf(val, p, buf, buflen)
	u_quad_t val;
	const char *p;
	char *buf;
	size_t buflen;
{
	char *bp, *q;
	size_t left;
	char *sbase, snbuf[KPRINTF_BUFSIZE];
	int base, bit, ch, len, sep;
	u_quad_t field;

	bp = buf;
	memset(buf, 0, buflen);

	/*
	 * Always leave room for the trailing NULL.
	 */
	left = buflen - 1;

	/*
	 * Print the value into the buffer.  Abort if there's not
	 * enough room.
	 */
	if (buflen < KPRINTF_BUFSIZE)
		return (buf);

	ch = *p++;
	base = ch != '\177' ? ch : *p++;
	sbase = base == 8 ? "%qo" : base == 10 ? "%qd" : base == 16 ? "%qx" : 0;
	if (sbase == 0)
		return (buf);	/* punt if not oct, dec, or hex */

	sprintf(snbuf, sbase, val);
	for (q = snbuf ; *q ; q++) {
		*bp++ = *q;
		left--;
	}

	/*
	 * If the value we printed was 0, or if we don't have room for
	 * "<x>", we're done.
	 */
	if (val == 0 || left < 3)
		return (buf);

#define PUTBYTE(b, c, l)			\
	*(b)++ = (c);					\
	if (--(l) == 0)					\
		goto out;
#define PUTSTR(b, p, l) do {		\
	int c;							\
	while ((c = *(p)++) != 0) {		\
		*(b)++ = c;					\
		if (--(l) == 0)				\
			goto out;				\
	}								\
} while (0)

	/*
	 * Chris Torek's new style %b format is identified by a leading \177
	 */
	sep = '<';
	if (ch != '\177') {
		/* old (standard) %b format. */
		for (;(bit = *p++) != 0;) {
			if (val & (1 << (bit - 1))) {
				PUTBYTE(bp, sep, left);
				for (; (ch = *p) > ' '; ++p) {
					PUTBYTE(bp, ch, left);
				}
				sep = ',';
			} else
				for (; *p > ' '; ++p)
					continue;
		}
	} else {
		/* new quad-capable %b format; also does fields. */
		field = val;
		while ((ch = *p++) != '\0') {
			bit = *p++;	/* now 0-origin */
			switch (ch) {
			case 'b':
				if (((u_int)(val >> bit) & 1) == 0)
					goto skip;
				PUTBYTE(bp, sep, left);
				PUTSTR(bp, p, left);
				sep = ',';
				break;
			case 'f':
			case 'F':
				len = *p++;	/* field length */
				field = (val >> bit) & ((1ULL << len) - 1);
				if (ch == 'F')	/* just extract */
					break;
				PUTBYTE(bp, sep, left);
				sep = ',';
				PUTSTR(bp, p, left);
				PUTBYTE(bp, '=', left);
				sprintf(snbuf, sbase, field);
				q = snbuf; PUTSTR(bp, q, left);
				break;
			case '=':
			case ':':
				/*
				 * Here "bit" is actually a value instead,
				 * to be compared against the last field.
				 * This only works for values in [0..255],
				 * of course.
				 */
				if ((int)field != bit)
					goto skip;
				if (ch == '=')
					PUTBYTE(bp, '=', left);
				PUTSTR(bp, p, left);
				break;
			default:
			skip:
				while (*p++ != '\0')
					continue;
				break;
			}
		}
	}
	if (sep != '<')
		PUTBYTE(bp, '>', left);

out:
	return (buf);

#undef PUTBYTE
#undef PUTSTR
}

/*
 * kprintf: scaled down version of printf(3).
 *
 * this version based on vfprintf() from libc which was derived from
 * software contributed to Berkeley by Chris Torek.
 *
 * NOTE: The kprintf mutex must be held if we're going TOBUF or TOCONS!
 */

/*
 * macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * flags used during conversion.
 */
#define	ALT			0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double; unimplemented */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SHORTINT	0x040		/* short integer */
#define	MAXINT		0x080		/* intmax_t */
#define	PTRINT		0x100		/* intptr_t */
#define	SIZEINT		0x200		/* size_t */
#define	ZEROPAD		0x400		/* zero (as opposed to blank) pad */
#define FPT			0x800		/* Floating point number */

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() 												\
	(flags&MAXINT ? va_arg(ap, intmax_t) : 					\
	    flags&PTRINT ? va_arg(ap, intptr_t) : 				\
	    flags&SIZEINT ? va_arg(ap, ssize_t) : /* XXX */ 	\
	    flags&QUADINT ? va_arg(ap, quad_t) : 				\
	    flags&LONGINT ? va_arg(ap, long) : 					\
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : 	\
	    (long)va_arg(ap, int))
#define	UARG() 												\
	(flags&MAXINT ? va_arg(ap, uintmax_t) : 				\
	    flags&PTRINT ? va_arg(ap, uintptr_t) : 				\
	    flags&SIZEINT ? va_arg(ap, size_t) : 				\
	    flags&QUADINT ? va_arg(ap, u_quad_t) : 				\
	    flags&LONGINT ? va_arg(ap, u_long) : 				\
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))

#define KPRINTF_PUTCHAR(C) {								\
	if (oflags == TOBUFONLY) {								\
		if ((vp != NULL) && (sbuf == tailp)) {				\
			ret += 1;		/* indicate error */			\
			goto overflow;									\
		}													\
		*sbuf++ = (C);										\
	} else {												\
		putchar((C), oflags, (struct tty *)vp);				\
	}														\
}

/*
 * Guts of kernel printf.  Note, we already expect to be in a mutex!
 */
int
kprintf(fmt0, oflags, vp, sbuf, ap)
	const char *fmt0;
	int oflags;
	void *vp;
	char *sbuf;
	va_list ap;
{
	char *fmt;			/* format string */
	int ch;				/* character from fmt */
	int n;				/* handy integer (short term usage) */
	char *cp;			/* handy char pointer (short term usage) */
	int flags;			/* flags as above */
	int ret;			/* return value accumulator */
	int width;			/* width from format (%8d), or 0 */
	int prec;			/* precision from format (%.3d), or -1 */
	char sign;			/* sign prefix (' ', '+', '-', or \0) */

	u_quad_t _uquad;	/* integer arguments %[diouxX] */
	enum { OCT, DEC, HEX } base;/* base for [diouxX] conversion */
	int dprec;			/* a copy of prec if [diouxX], 0 otherwise */
	int realsz;			/* field size expanded by dprec */
	int size;			/* size of converted field or string */
	char *xdigs;		/* digits for [xX] conversion */
	char buf[KPRINTF_BUFSIZE]; /* space for %c, %[diouxX] */
	char *tailp;		/* tail pointer for snprintf */

	tailp = NULL; /* XXX: shutup gcc */
	if (oflags == TOBUFONLY && (vp != NULL))
		tailp = *(char**) vp;

	cp = NULL; /* XXX: shutup gcc */
	size = 0; /* XXX: shutup gcc */

	fmt = (char*) fmt0;
	ret = 0;

	xdigs = NULL; /* XXX: shut up gcc warning */

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		while (*fmt != '%' && *fmt) {
			ret++;
			KPRINTF_PUTCHAR(*fmt++);
		}
		if (*fmt == 0)
			goto done;

		fmt++; /* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';

rflag: ch = *fmt++;
reswitch: switch (ch) {
		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			if ((width = va_arg(ap, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(ap, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			n = 0;
			do {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
		case 'h':
			flags |= SHORTINT;
			goto rflag;
		case 'j':
			flags |= MAXINT;
			goto rflag;
		case 'l':
			if (*fmt == 'l') {
				fmt++;
				flags |= QUADINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= QUADINT;
			goto rflag;
		case 't':
			flags |= PTRINT;
			goto rflag;
		case 'z':
			flags |= SIZEINT;
			goto rflag;
		case 'c':
			*(cp = buf) = va_arg(ap, int);
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			_uquad = SARG();
			if ((quad_t) _uquad < 0) {
				_uquad = -_uquad;
				sign = '-';
			}
			base = DEC;
			goto number;
		case 'n':
			if (flags & MAXINT)
				*va_arg(ap, intmax_t*) = ret;
			else if (flags & PTRINT)
				*va_arg(ap, intptr_t*) = ret;
			else if (flags & SIZEINT)
				*va_arg(ap, ssize_t*) = ret;
			else if (flags & QUADINT)
				*va_arg(ap, quad_t*) = ret;
			else if (flags & LONGINT)
				*va_arg(ap, long*) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short*) = ret;
			else
				*va_arg(ap, int*) = ret;
			continue; /* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			_uquad = UARG();
			base = OCT;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			/* NOSTRICT */
			_uquad = (u_long) va_arg(ap, void*);
			base = HEX;
			xdigs = "0123456789abcdef";
			flags |= HEXPREFIX;
			ch = 'x';
			goto nosign;
		case 's':
			if ((cp = va_arg(ap, char*)) == NULL)
				cp = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = memchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			_uquad = UARG();
			base = DEC;
			goto nosign;
		case 'X':
			xdigs = "0123456789ABCDEF";
			goto hex;
		case 'x':
			xdigs = "0123456789abcdef";
hex: 		_uquad = UARG();
			base = HEX;
			/* leading 0x/X only if non-zero */
			if ((flags & ALT) && _uquad != 0)
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign: 	sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number: 	if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			cp = buf + KPRINTF_BUFSIZE;
			if (_uquad != 0 || prec != 0) {
				/*
				 * Unsigned mod is hard, and unsigned mod
				 * by a constant is easier than that by
				 * a variable; hence this switch.
				 */
				switch (base) {
				case OCT:
					do {
						*--cp = to_char(_uquad & 7);
						_uquad >>= 3;
					} while (_uquad);
					/* handle octal leading 0 */
					if ((flags & ALT) && *cp != '0')
						*--cp = '0';
					break;

				case DEC:
					/* many numbers are 1 digit */
					while (_uquad >= 10) {
						*--cp = to_char(_uquad % 10);
						_uquad /= 10;
					}
					*--cp = to_char(_uquad);
					break;

				case HEX:
					do {
						*--cp = xdigs[_uquad & 15];
						_uquad >>= 4;
					} while (_uquad);
					break;

				default:
					cp = "bug in kprintf: bad base";
					size = strlen(cp);
					goto skipsize;
				}
			}
			size = buf + KPRINTF_BUFSIZE - cp;
		skipsize:
			break;
		default: /* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			cp = buf;
			*cp = ch;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * size excludes decimal prec; realsz includes it.
		 */
		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		else if (flags & HEXPREFIX)
			realsz += 2;

		/* adjust ret */
		ret += width > realsz ? width : realsz;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST | ZEROPAD)) == 0) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR(' ');
		}

		/* prefix */
		if (sign) {
			KPRINTF_PUTCHAR(sign);
		} else if (flags & HEXPREFIX) {
			KPRINTF_PUTCHAR('0');
			KPRINTF_PUTCHAR(ch);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR('0');
		}

		/* leading zeroes from decimal precision */
		n = dprec - size;
		while (n-- > 0)
			KPRINTF_PUTCHAR('0');

		/* the string or number proper */
		while (size--)
			KPRINTF_PUTCHAR(*cp++);
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST) {
			n = width - realsz;
			while (n-- > 0)
				KPRINTF_PUTCHAR(' ');
		}
	}

done:
	if ((oflags == TOBUFONLY) && (vp != NULL))
		*(char**) vp = sbuf;
	(*v_flush)();
overflow:
	return (ret);
	/* NOTREACHED */
}
