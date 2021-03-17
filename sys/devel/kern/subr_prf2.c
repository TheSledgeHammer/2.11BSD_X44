/*
 * subr_prf2.c
 *
 *  Created on: 17 Mar 2021
 *      Author: marti
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

/*
 * defines
 */

/* flags for kprintf */
#define TOCONS				0x01	/* to the console */
#define TOTTY				0x02	/* to the process' tty */
#define TOLOG				0x04	/* to the kernel message buffer */
#define TOBUFONLY			0x08	/* to the buffer (only) [for sprintf] */
#define TODDB				0x10	/* to ddb console */

/* max size buffer kprintf needs to print quad_t [size in base 8 + \0] */
#define KPRINTF_BUFSIZE		(sizeof(quad_t) * NBBY / 3 + 2)

struct putchar_arg {
	int			flags;
	int			pri;
	struct	tty *tty;
};

int msgbuftrigger;

static int	kprintf_logging = TOLOG | TOCONS;

/*
 * local prototypes
 */

static void  kputchar (int ch, void *arg);
static char *ksprintn (char *nbuf, uintmax_t num, int base, int *lenp, int upper);

/*
 * globals
 */

struct	tty *constty;	/* pointer to console "window" tty */
int	consintr = 1;		/* ok to handle console interrupts? */
extern	int log_open;	/* subr_log: is /dev/klog open? */

/*
void
#ifdef __STDC__
panic(const char *fmt, ...)
#else
panic(fmt, va_alist)
	char *fmt;
	va_dcl
#endif
{
	va_list ap;

	panic(fmt);

	va_start(ap, fmt);
	printf("panic: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

	cpu_reboot(bootopt, NULL);
}
*/

/*
 * Output to the console.
 */
int
kprintf(const char *fmt, ...)
{
	va_list ap;
	int savintr;
	struct putchar_arg pca;
	int retval;

	savintr = consintr;		/* disable interrupts */
	consintr = 0;
	__va_start(ap, fmt);
	pca.tty = NULL;
	pca.flags = kprintf_logging & ~TOTTY;
	pca.pri = -1;
	retval = kvcprintf(fmt, kputchar, &pca, ap);
	__va_end(ap);
	if (!panicstr)
		msgbuftrigger = 1;
	consintr = savintr;		/* reenable interrupts */
	return (retval);
}

int
kvprintf(const char *fmt, va_list ap)
{
	int savintr;
	struct putchar_arg pca;
	int retval;

	savintr = consintr; /* disable interrupts */
	consintr = 0;
	pca.tty = NULL;
	pca.flags = kprintf_logging & ~TOTTY;
	pca.pri = -1;
	retval = kvcprintf(fmt, putchar, &pca, ap);
	if (!panicstr)
		msgbuftrigger = 1;
	consintr = savintr; /* reenable interrupts */
	return (retval);
}

/*
 * tprintf functions: used to send messages to a specific process
 *
 * usage:
 *   get a tpr_t handle on a process "p" by using "tprintf_open(p)"
 *   use the handle when calling "tprintf"
 *   when done, do a "tprintf_close" to drop the handle
 */

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
		SESSRELE((struct session *) sess);
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
	int s, savintr;

	savintr = consintr;		/* disable interrupts */
	consintr = 0;
	kprintf(fmt, TOCONS | TOLOG, NULL, NULL, ap);

	if (!panicstr)
		logwakeup(logMSG);
	consintr = savintr;		/* reenable interrupts */
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
#ifdef __STDC__
snprintf(char *buf, size_t size, const char *fmt, ...)
#else
snprintf(buf, size, fmt, va_alist)
        char *buf;
        size_t size;
        const char *cfmt;
        va_dcl
#endif
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
	return(retval);
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
	return(retval);
}
