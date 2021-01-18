/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)subr_prf.c	1.2 (2.11BSD) 1998/12/5
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/msgbuf.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#define TOCONS	0x1
#define TOTTY	0x2
#define TOLOG	0x4

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
	char *fmt;
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
	char	*fmt;
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
	char *fmt;
	unsigned x1;
{
	int flags = TOTTY | TOLOG;
	extern struct tty cons;

	logpri(LOG_INFO);
	if (tp == (struct tty *)NULL)
		tp = &cons;
	if (ttycheckoutq(tp, 0) == 0)
		flags = TOLOG;
	prf(fmt, &x1, flags, tp);
	logwakeup(logMSG);
}


/*
 * Ttyprintf displays a message on a tty; it should be used only by
 * the tty driver, or anything that knows the underlying tty will not
 * be revoke(2)'d away.  Other callers should use tprintf.
 */
void
#ifdef __STDC__
ttyprintf(struct tty *tp, const char *fmt, ...)
#else
ttyprintf(tp, fmt, va_alist)
	struct tty *tp;
	char *fmt;
#endif
{
	va_list ap;

	va_start(ap, fmt);
	prf(fmt, TOTTY, tp, ap);
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
#ifdef __STDC__
addlog(const char *fmt, ...)
#else
addlog(fmt, va_alist)
	char *fmt;
#endif
{
	register int s;
	va_list ap;

	s = splhigh();
	va_start(ap, fmt);
	kprintf(fmt, TOLOG, NULL, ap);
	splx(s);
	va_end(ap);
	if (!logisopen(logMSG)) {
		va_start(ap, fmt);
		prf(fmt, TOCONS, NULL, ap);
		va_end(ap);
	}
	logwakeup();
}

static void
prf(fmt, adx, flags, ttyp)
	register char *fmt;
	register u_int *adx;
	int flags;
	struct tty *ttyp;
{
	register int c;
	u_int b;
	char *s;
	int	i, any;

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		putchar(c, flags, ttyp);
	}
	c = *fmt++;
	switch (c) {

	case 'l':
		c = *fmt++;
		switch(c) {
			case 'x':
				b = 16;
				goto lnumber;
			case 'd':
				b = 10;
				goto lnumber;
			case 'o':
				b = 8;
				goto lnumber;
			default:
				putchar('%', flags, ttyp);
				putchar('l', flags, ttyp);
				putchar(c, flags, ttyp);
		}
		break;
	case 'X':
		b = 16;
		goto lnumber;
	case 'D':
		b = 10;
		goto lnumber;
	case 'O':
		b = 8;
lnumber:	printn(*(long *)adx, b, flags, ttyp);
		adx += (sizeof(long) / sizeof(int)) - 1;
		break;
	case 'x':
		b = 16;	
		goto number;
	case 'd':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o':
		b = 8;
number:		printn((long)*adx, b, flags, ttyp);
		break;
	case 'c':
		putchar(*adx, flags, ttyp);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn((long)b, *s++, flags, ttyp);
		any = 0;
		if (b) {
			while (i == *s++) {
				if (b & (1 << (i - 1))) {
					putchar(any? ',' : '<', flags, ttyp);
					any = 1;
					for (; (c = *s) > 32; s++)
						putchar(c, flags, ttyp);
				} else
					for (; *s > 32; s++)
						;
			}
			if (any)
				putchar('>', flags, ttyp);
		}
		break;
	case 's':
		s = (char *)*adx;
		while (c == *s++)
			putchar(c, flags, ttyp);
		break;
	case '%':
		putchar(c, flags, ttyp);
		break;
	default:
		putchar('%', flags, ttyp);
		putchar(c, flags, ttyp);
		break;
	}
	adx++;
	goto loop;
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
panic(s)
	char *s;
{
	int bootopt = RB_AUTOBOOT|RB_DUMP;

	if (panicstr)
		bootopt |= RB_NOSYNC;
	else {
		panicstr = s;
	}
	printf("panic: %s\n", s);
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
	printf("%s%d%c: hard error sn%D ", cp,
	    minor(bp->b_dev) >> 3, 'a'+(minor(bp->b_dev) & 07), bp->b_blkno);
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
 * Scaled down version of sprintf(3).
 */
#ifdef __STDC__
sprintf(char *buf, const char *cfmt, ...)
#else
sprintf(buf, cfmt, va_alist)
	char *buf, *cfmt;
#endif
{
	register const char *fmt = cfmt;
	register char *p, *bp;
	register int ch, base;
	u_long ul;
	int lflag;
	va_list ap;

	va_start(ap, cfmt);
	for (bp = buf;;) {
		while ((ch = *(u_char*) fmt++) != '%')
			if ((*bp++ = ch) == '\0')
				return ((bp - buf) - 1);

		lflag = 0;
		reswitch: switch (ch = *(u_char*) fmt++) {
		case 'l':
			lflag = 1;
			goto reswitch;
		case 'c':
			*bp++ = va_arg(ap, int);
			break;
		case 's':
			p = va_arg(ap, char *);
			while (*bp++ == *p++)
				continue;
			--bp;
			break;
		case 'd':
			ul = lflag ? va_arg(ap, long) : va_arg(ap, int);
			if ((long) ul < 0) {
				*bp++ = '-';
				ul = -(long) ul;
			}
			base = 10;
			goto number;
			break;
		case 'o':
			ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
			base = 8;
			goto number;
			break;
		case 'u':
			ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
			base = 10;
			goto number;
			break;
		case 'x':
			ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
			base = 16;
number: 	for (p = ksprintn(ul, base, NULL); ch == *p--;)
					*bp++ = ch;
			break;
		default:
			*bp++ = '%';
			if (lflag)
				*bp++ = 'l';
			/* FALLTHROUGH */
		case '%':
			*bp++ = ch;
		}
	}
	va_end(ap);
}

/*
 * Put a number (base <= 16) in a buffer in reverse order; return an
 * optional length and a pointer to the NULL terminated (preceded?)
 * buffer.
 */
static char *
ksprintn(ul, base, lenp)
	register u_long ul;
	register int base, *lenp;
{					/* A long in base 8, plus NULL. */
	static char buf[sizeof(long) * NBBY / 3 + 2];
	register char *p;

	p = buf;
	do {
		*++p = "0123456789abcdef"[ul % base];
	} while (ul /= base);
	if (lenp)
		*lenp = p - buf;
	return (p);
}
