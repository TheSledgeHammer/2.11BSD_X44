/*
 * Steven Schultz - sms@moe.2bsd.com
 *
 *	@(#)ctimed.c	1.0 (2.11BSD) 1996/6/25
 *
 * ctimed - the daemon that supports the ctime() and getpw*() stubs
 * 	    in 'libcstubs.a'.
*/

#include	<sys/cdefs.h>

#include	<sys/types.h>
#include	<sys/ioctl.h>
#include   	<sys/file.h>

#include	<pwd.h>
#include	<setjmp.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>
#include	<utmp.h>
#include	<unistd.h>

#include	"ctimed.h"

/* Unused */
struct cmd_ops ctimed_ops = {
		.cmd_ctime = ctime,
		.cmd_asctime = asctime,
		.cmd_tzset = tzset,
		.cmd_localtime = localtime,
		.cmd_gmtime = gmtime,
		.cmd_offtime = offtime,
		.cmd_getpwent = getpwent,
		.cmd_getpwnam = getpwnam,
		.cmd_getpwuid = getpwuid,
		.cmd_setpwent = setpwent,
		.cmd_setpassent = setpassent,
		.cmd_endpwent = endpwent,
};

struct cmds cmd_table[] = {
		{ .arg = CTIME_ARG, 		.val = CTIME_VAL },
		{ .arg = ASCTIME_ARG, 		.val = ASCTIME_VAL },
		{ .arg = TZSET_ARG, 		.val = TZSET_VAL },
		{ .arg = LOCALTIME_ARG, 	.val = LOCALTIME_VAL },
		{ .arg = GMTIME_ARG, 		.val = GMTIME_VAL },
		{ .arg = OFFTIME_ARG, 		.val = OFFTIME_VAL },
		{ .arg = GETPWENT_ARG, 		.val = GETPWENT_VAL },
		{ .arg = GETPWNAM_ARG, 		.val = GETPWNAM_VAL },
		{ .arg = GETPWUID_ARG, 		.val = GETPWUID_VAL },
		{ .arg = SETPASSENT_ARG, 	.val = SETPASSENT_VAL },
		{ .arg = ENDPWENT_ARG, 		.val = ENDPWENT_VAL },
};

static void prog_switch(int, struct cmd_ops *, time_t *, struct tm *, struct tm *, struct passwd *);
static void getb(int, void *, size_t);
static void ctimeout(int);
static void checkppid(int);
static void do_pw(struct passwd *);
static int packpwtobuf(struct passwd *, char *);
static void usage(void);

jmp_buf env;
char junk[256 + sizeof(struct passwd) + 4];
long off;

int
main(int argc, char **argv)
{
	register int i;
	struct passwd pw;
	struct itimerval it;
	int ch, error;
	time_t l;
	struct tm tmtmp, tp;

	signal(SIGPIPE, SIG_DFL);
	for (i = getdtablesize(); --i > 2;) {
		close(i);
	}

	/*
	 * Need a timer running while we disassociate from the control terminal
	 * in case of a modem line which has lost carrier.
	 */
	timerclear(&it.it_interval);
	it.it_value.tv_sec = 5;
	it.it_value.tv_usec = 0;
	signal(SIGALRM, ctimeout);
	setitimer(ITIMER_REAL, &it, (struct itimerval*) NULL);
	if (setjmp(env) == 0) {
		i = open("/dev/tty", 0);
		if (i >= 0) {
			ioctl(i, TIOCNOTTY, NULL);
			close(i);
		}
	}

	/*
	 * Now start a timer with one minute refresh.  In the signal service
	 * routine, check the parent process id to see if this process has
	 * been orphaned and if so exit.  This is primarily aimed at removing
	 * the 'ctimed' process left behind by 'sendmail's multi-fork startup
	 * but may prove useful in preventing accumulation of 'ctimed' processes
	 * in other circumstances as well.  Normally this process is short
	 * lived.
	 */
	it.it_interval.tv_sec = 60;
	it.it_interval.tv_usec = 0;
	it.it_value.tv_sec = 60;
	it.it_value.tv_usec = 0;
	signal(SIGALRM, checkppid);
	setitimer(ITIMER_REAL, &it, (struct itimerval*) NULL);

/*
	ctimed_cmds_setup(cmd_table, &ctimed_ops);
	while ((ch = getopt(argc, argv, "-catlgoenupf")) != EOF) {
		prog_switch(ch, &ctimed_ops, &l, &tmtmp, &tp, &pw);
	}
*/
	/* setup ctimed command table and operations */
	ctimed_cmds_setup(cmd_table, &stub_ops);
	error = read(fileno(stdin), &ch, 1);
	while (error == 1) {
		prog_switch(ch, &stub_ops, &l, &tmtmp, &tp, &pw);
	}
}

void
prog_switch(int ch, struct cmd_ops *cmdops, time_t *timep, struct tm *tmp, struct tm *tp, struct passwd *pw)
{
	char *cp;
	u_char xxx;
	int len, tosslen;
	uid_t uid;

	switch (ch) {
	case 'c': /* CTIME */
		timep = 0L;
		getb(fileno(stdin), timep, sizeof(*timep));
		cp = cmdop_ctime(cmdops, timep);
		write(fileno(stdout), cp, 26);
		break;
	case 'a': /* ASCTIME */
		getb(fileno(stdin), tmp, sizeof(*tmp));
		cp = cmdop_asctime(cmdops, tmp);
		write(fileno(stdout), cp, 26);
		break;
	case 't': /* TZSET */
		(void)cmdop_tzset(cmdops);
		break;
	case 'l': /* LOCALTIME */
		timep = 0L;
		getb(fileno(stdin), timep, sizeof(*timep));
		tp = cmdop_localtime(cmdops, timep);
		write(fileno(stdout), tp, sizeof(*tp));
		strcpy(junk, tp->tm_zone);
		junk[24] = '\0';
		write(fileno(stdout), junk, 24);
		break;
	case 'g': /* GMTIME */
		timep = 0L;
		getb(fileno(stdin), timep, sizeof(*timep));
		tp = cmdop_gmtime(cmdops, timep);
		write(fileno(stdout), tp, sizeof(*tp));
		strcpy(junk, tp->tm_zone);
		junk[24] = '\0';
		write(fileno(stdout), junk, 24);
		break;
	case 'o': /* OFFTIME */
		getb(fileno(stdin), timep, sizeof(*timep));
		getb(fileno(stdin), &off, sizeof(off));
#ifdef	__bsdi__
		timep += off;
		tp = cmdop_localtime(cmdops, timep);
#else
		tp = cmdop_offtime(cmdops, timep, off);
#endif
		write(fileno(stdout), tp, sizeof(*tp));
		break;
	case 'e': /* GETPWENT */
		pw = cmdop_getpwent(cmdops);
		do_pw(pw);
		break;
	case 'n': /* GETPWNAM */
		getb(fileno(stdin), &len, sizeof(int));
		if (len > UT_NAMESIZE) {
			tosslen = len - UT_NAMESIZE;
			len = UT_NAMESIZE;
		} else {
			tosslen = 0;
		}
		getb(fileno(stdin), junk, len);
		for (; tosslen; tosslen--) {
			getb(fileno(stdin), &xxx, 1);
		}
		junk[len] = '\0';
		pw = cmdop_getpwnam(cmdops, junk);
		do_pw(pw);
		break;
	case 'u': /* GETPWUID */
		getb(fileno(stdin), &uid, sizeof(uid_t));
		pw = cmdop_getpwuid(cmdops, uid);
		do_pw(pw);
		break;
	case 'p': /* SETPASSENT */
		getb(fileno(stdin), &len, sizeof(int));
		if (cmdop_setpassent(cmdops, len)) {
			len = 1;
		} else {
			len = 0;
		}
		write(fileno(stdout), &len, sizeof(int));
		break;
	case 'f': /* ENDPWENT */
		cmdop_endpwent(cmdops);
		break;
	default:
		usage();
	}
}

static void
getb(int f, void *p, size_t n)
{
	int i;
	char *b;

	b = (char *)p;
	while (n) {
		i = read(f, b, n);
		if (i <= 0) {
			return;
		}
		b += i;
		n -= (size_t)i;
	}
}

static void
ctimeout(int signo)
{
	longjmp(env, 1);
}

static void
checkppid(int signo)
{
	if (getppid() == 1) {
		exit(0);
	}
}

static void
do_pw(struct passwd *pw)
{
	int	len;

	if (!pw) {
		len = 0;
		write(fileno(stdout), &len, sizeof(int));
		return;
	}
	len = packpwtobuf(pw, junk);
	write(fileno(stdout), &len, sizeof(int));
	write(fileno(stdout), pw, sizeof(*pw));
	write(fileno(stdout), junk, len);
	return;
}

static int
packpwtobuf(struct passwd *pw, char *buf)
{
	register char *cp = buf;
	register char *dp;

	dp = pw->pw_name;
	pw->pw_name = (char *)0;
	while ((*cp++ = *dp++))
		;
	dp = pw->pw_passwd;
	pw->pw_passwd = (char*) (cp - buf);
	while ((*cp++ = *dp++))
		;
	dp = pw->pw_class;
	pw->pw_class = (char*) (cp - buf);
	while ((*cp++ = *dp++))
		;
	dp = pw->pw_gecos;
	pw->pw_gecos = (char*) (cp - buf);
	while ((*cp++ = *dp++))
		;
	dp = pw->pw_dir;
	pw->pw_dir = (char*) (cp - buf);
	while ((*cp++ = *dp++))
		;
	dp = pw->pw_shell;
	pw->pw_shell = (char*) (cp - buf);
	while ((*cp++ = *dp++))
		;
	return (cp - buf);
}

static void
usage(void)
{
	(void) fprintf(stderr,
			"usage: ctimed [-c ctime] [-a asctime] [-t tzet] [-l localtime] [-g gmtime] [-o offtime] [-e getpwent] [-n getpwnam] [-u getpwuid] [-p setpassent ] [-f endpwent]\n");
	exit(1);
}
