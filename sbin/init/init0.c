/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Donn Seeley at Berkeley Software Design, Inc.
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
 * @(#) Copyright (c) 1991, 1993 The Regents of the University of California.  All rights reserved.
 * @(#)init.c	8.1 (Berkeley) 7/15/93
 * $FreeBSD: src/sbin/init/init.c,v 1.38.2.8 2001/10/22 11:27:32 des Exp $
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/uio.h>

#include <db.h>
#include <errno.h>
#include <fcntl.h>
#include <kenv.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <ttyent.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <err.h>

#include <stdarg.h>

#include "mntopts.h"
#include "pathnames.h"

/*
 * Sleep times; used to prevent thrashing.
 */
#define	GETTY_SPACING		5	/* N secs minimum getty spacing */
#define	GETTY_SLEEP			30	/* sleep N secs after spacing problem */
#define	GETTY_NSPACE		3	/* max. spacing count to bring reaction */
#define	WINDOW_WAIT			3	/* wait N secs after starting window */
#define	STALL_TIMEOUT		30	/* wait N secs after warning */
#define	DEATH_WATCH			10	/* wait N secs for procs to die */
#define	DEATH_SCRIPT		120	/* wait for 2min for /etc/rc.shutdown */

/*
 * User-based resource limits.
 */
#define RESOURCE_RC			"daemon"
#define RESOURCE_WINDOW		"default"
#define RESOURCE_GETTY		"default"

#ifndef DEFAULT_STATE
#define DEFAULT_STATE		runcom
#endif

static void	 handle(sig_t, ...);
static void	 delset(sigset_t *, ...);

static void	 stall(const char *, ...) __printflike(1, 2);
static void	 warning(const char *, ...) __printflike(1, 2);
static void	 emergency(const char *, ...) __printflike(1, 2);
static void	 disaster(int);
static int	 runshutdown(void);
static char	*strk(char *);

#define	DEATH		'd'
#define	SINGLE_USER	's'
#define	RUNCOM		'r'
#define	READ_TTYS	't'
#define	MULTI_USER	'm'
#define	CLEAN_TTYS	'T'
#define	CATATONIA	'c'

typedef enum {
	invalid_state,
	single_user,
	runcom,
	read_ttys,
	multi_user,
	clean_ttys,
	catatonia,
	death,
} state_t;
typedef state_t (*state_func_t)(void);

static state_t f_single_user(void);
static state_t f_runcom(void);
static state_t f_read_ttys(void);
static state_t f_multi_user(void);
static state_t f_clean_ttys(void);
static state_t f_catatonia(void);
static state_t f_death(void);

//static state_t f_run_script(const char *);

state_func_t state_funcs[] = {
	NULL,
	f_single_user,
	f_runcom,
	f_read_ttys,
	f_multi_user,
	f_clean_ttys,
	f_catatonia,
	f_death
};

enum { AUTOBOOT, FASTBOOT } runcom_mode = AUTOBOOT;
#define FALSE	0
#define TRUE	1

static struct timeval boot_time;
state_t current_state = death;

static int Reboot = FALSE;
static int howto = RB_AUTOBOOT;

static char *init_path_argv0;

static void transition(state_t);
static state_t requested_transition = DEFAULT_STATE;

typedef struct init_session {
	int						se_index;				/* index of entry in ttys file */
	pid_t					se_process;				/* controlling process */
	struct timeval			se_started;				/* used to avoid thrashing */
	int						se_flags;				/* status of session */
#define	SE_SHUTDOWN			0x1						/* session won't be restarted */
#define	SE_PRESENT			0x2						/* session is in /etc/ttys */
	int     				se_nspace;              /* spacing count */
	char					*se_device;				/* filename of port */
	char					*se_getty;				/* what to run on that port */
	char    				*se_getty_argv_space;   /* pre-parsed argument array space */
	char					**se_getty_argv;		/* pre-parsed argument array */
	char					*se_window;				/* window system (started only once) */
	char    				*se_window_argv_space;  /* pre-parsed argument array space */
	char					**se_window_argv;		/* pre-parsed argument array */
	char    				*se_type;               /* default terminal type */
	struct	init_session 	*se_prev;
	struct	init_session 	*se_next;
} session_t;

static void		free_session(session_t *);
static session_t *new_session(session_t *, int, struct ttyent *);
static session_t *sessions;

static char		**construct_argv(char *);
static void		start_window_system(session_t *);
static void		collect_child(pid_t, int);
static pid_t 	start_getty(session_t *);
static void		transition_handler(int);
static void		alrm_handler(int);
static void		setsecuritylevel(int);
static int		getsecuritylevel(void);
static int		setupargv(session_t *, struct ttyent *);

/*
 * The mother of all processes.
 */
int
main(int argc, char *argv[])
{
	state_t initial_transition = runcom;
	char kenv_value[PATH_MAX];
	int c, error;
	struct sigaction sa;
	sigset_t mask;

	/* Dispose of random users. */
	if (getuid() != 0)
		errx(1, "%s", strerror(EPERM));

	/* System V users like to reexec init. */
	if (getpid() != 1) {
#ifdef COMPAT_SYSV_INIT
		/* So give them what they want */
		if (argc > 1) {
			if (strlen(argv[1]) == 1) {
				char runlevel = *argv[1];
				int sig;

				switch (runlevel) {
				case '0': /* halt + poweroff */
					sig = SIGUSR2;
					break;
				case '1': /* single-user */
					sig = SIGTERM;
					break;
				case '6': /* reboot */
					sig = SIGINT;
					break;
				case 'c': /* block further logins */
					sig = SIGTSTP;
					break;
				case 'q': /* rescan /etc/ttys */
					sig = SIGHUP;
					break;
				case 'r': /* remount root */
					sig = SIGEMT;
					break;
				default:
					goto invalid;
				}
				kill(1, sig);
				_exit(0);
			} else
				invalid: errx(1, "invalid run-level ``%s''", argv[1]);
		} else
#endif
			errx(1, "already running");
	}

	init_path_argv0 = strdup(argv[0]);
	if (init_path_argv0 == NULL)
		err(1, "strdup");

	/*
	 * Note that this does NOT open a file...
	 * Does 'init' deserve its own facility number?
	 */
	openlog("init", LOG_CONS, LOG_AUTH);

	/*
	 * Create an initial session.
	 */
	if (setsid() < 0 && (errno != EPERM || getsid(0) != 1))
		warning("initial setsid() failed: %m");

	/*
	 * Establish an initial user so that programs running
	 * single user do not freak out and die (like passwd).
	 */
	if (setlogin("root") < 0)
		warning("setlogin() failed: %m");

	/*
	 * This code assumes that we always get arguments through flags,
	 * never through bits set in some random machine register.
	 */
	while ((c = getopt(argc, argv, "dsf")) != -1)
		switch (c) {
		case 'd':
			/* We don't support DEVFS. */
			break;
		case 's':
			initial_transition = single_user;
			break;
		case 'f':
			runcom_mode = FASTBOOT;
			break;
		default:
			warning("unrecognized flag '-%c'", c);
			break;
		}

	if (optind != argc)
		warning("ignoring excess arguments");

	/*
	 * We catch or block signals rather than ignore them,
	 * so that they get reset on exec.
	 */
	handle(disaster, SIGABRT, SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGSYS,
			SIGXCPU, SIGXFSZ, 0);
	handle(transition_handler, SIGHUP, SIGINT, SIGEMT, SIGTERM, SIGTSTP,
			SIGUSR1, SIGUSR2, SIGWINCH, 0);
	handle(alrm_handler, SIGALRM, 0);
	sigfillset(&mask);
	delset(&mask, SIGABRT, SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGSYS,
			SIGXCPU, SIGXFSZ, SIGHUP, SIGINT, SIGEMT, SIGTERM, SIGTSTP,
			SIGALRM, SIGUSR1, SIGUSR2, SIGWINCH, 0);
	sigprocmask(SIG_SETMASK, &mask, NULL);
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGTTIN, &sa, NULL);
	sigaction(SIGTTOU, &sa, NULL);

	/*
	 * Paranoia.
	 */
	close(0);
	close(1);
	close(2);

	if (kenv(KENV_GET, "init_exec", kenv_value, sizeof(kenv_value)) > 0) {
		replace_init(kenv_value);
		_exit(0); /* reboot */
	}

	/*
	if (kenv(KENV_GET, "init_script", kenv_value, sizeof(kenv_value)) > 0) {
		state_func_t next_transition;

		if ((next_transition = f_run_script(kenv_value)) != NULL)
			initial_transition = (state_t) next_transition;
	}
	*/

	if (kenv(KENV_GET, "init_chroot", kenv_value, sizeof(kenv_value)) > 0) {
		if (chdir(kenv_value) != 0 || chroot(".") != 0)
			warning("Can't chroot to %s: %m", kenv_value);
	}

	/*
	 * Start the state machine.
	 */
	transition(initial_transition);

	/*
	 * Should never reach here.
	 */
	return 1;
}

/*
 * Associate a function with a signal handler.
 */
static void
handle(sig_t handler, ...)
{
	int sig;
	struct sigaction sa;
	sigset_t mask_everything;
	va_list ap;

	va_start(ap, handler);

	sa.sa_handler = handler;
	sigfillset(&mask_everything);

	while ((sig = va_arg(ap, int)) != 0) {
		sa.sa_mask = mask_everything;
		/* XXX SA_RESTART? */
		sa.sa_flags = sig == SIGCHLD ? SA_NOCLDSTOP : 0;
		sigaction(sig, &sa, NULL);
	}
	va_end(ap);
}

/*
 * Delete a set of signals from a mask.
 */
static void
delset(sigset_t *maskp, ...)
{
	int sig;
	va_list ap;

	va_start(ap, maskp);

	while ((sig = va_arg(ap, int)) != 0)
		sigdelset(maskp, sig);
	va_end(ap);
}

/*
 * Log a message and sleep for a while (to give someone an opportunity
 * to read it and to save log or hardcopy output if the problem is chronic).
 */
static void
stall(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);

	vsyslog(LOG_ALERT, message, ap);
	va_end(ap);
	sleep(STALL_TIMEOUT);
}

/*
 * Like stall(), but doesn't sleep.
 * If cpp had variadic macros, the two functions could be #defines for another.
 */
static void
warning(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);

	vsyslog(LOG_ALERT, message, ap);
	va_end(ap);
}

/*
 * Log an emergency message.
 */
static void
emergency(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);

	vsyslog(LOG_EMERG, message, ap);
	va_end(ap);
}

/*
 * Catch an unexpected signal.
 */
static void
disaster(int sig)
{
	emergency("fatal signal: %s",
			(unsigned) sig < NSIG ? sys_siglist[sig] : "unknown signal");

	sleep(STALL_TIMEOUT);
	_exit(sig); /* reboot */
}

/*
 * Change states in the finite state machine.
 * The initial state is passed as an argument.
 */
static void
transition(state_t s)
{

	current_state = s;
	for (;;)
		current_state = (state_t) (*current_state)();
}

/*
 * Block further logins.
 */
static state_t
f_catatonia(void)
{
	session_t *sp;

	for (sp = sessions; sp; sp = sp->se_next)
		sp->se_flags |= SE_SHUTDOWN;

	return multi_user;
}

static char *
strk(char *p)
{
	static char *t;
	char *q;
	int c;

	if (p)
		t = p;
	if (!t)
		return 0;

	c = *t;
	while (c == ' ' || c == '\t')
		c = *++t;
	if (!c) {
		t = 0;
		return 0;
	}
	q = t;
	if (c == '\'') {
		c = *++t;
		q = t;
		while (c && c != '\'')
			c = *++t;
		if (!c) /* unterminated string */
			q = t = 0;
		else
			*t++ = 0;
	} else {
		while (c && c != ' ' && c != '\t')
			c = *++t;
		*t++ = 0;
		if (!c)
			t = 0;
	}
	return q;
}
