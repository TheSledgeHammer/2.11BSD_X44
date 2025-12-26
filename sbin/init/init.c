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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/ioctl.h>
//#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/reboot.h>
//#include <sys/signal.h>
#include <sys/sysctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
//#include <sys/uio.h>

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kenv.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <ttyent.h>
#include <unistd.h>
#include <util.h>
#ifdef SUPPORT_UTMP
#include <utmp.h>
#endif
#ifdef SUPPORT_UTMPX
#include <utmpx.h>
#endif

#ifdef SECURE
#include <pwd.h>
#endif

#ifdef LOGIN_CAP
#include <login_cap.h>
#endif

//#include "mntopts.h"
#include "pathnames.h"

/*
 * Sleep times; used to prevent thrashing.
 */
#define	GETTY_SPACING	5	/* N secs minimum getty spacing */
#define	GETTY_SLEEP	30	/* sleep N secs after spacing problem */
#define	GETTY_NSPACE	3	/* max. spacing count to bring reaction */
#define	WINDOW_WAIT	3	/* wait N secs after starting window */
#define	STALL_TIMEOUT	30	/* wait N secs after warning */
#define	DEATH_WATCH	10	/* wait N secs for procs to die */
#define	DEATH_SCRIPT	120	/* wait for 2min for /etc/rc.shutdown */

#define	RESOURCE_RC		"daemon"
#define	RESOURCE_WINDOW	"default"
#define	RESOURCE_GETTY	"default"

static void handle(sig_t, ...);
static void delset(sigset_t *, ...);

static void stall(const char *, ...) __sysloglike(1, 2);
static void warning(const char *, ...) __sysloglike(1, 2);
static void emergency(const char *, ...) __sysloglike(1, 2);
static void badsys(int);
static void disaster(int);
static char *strk(char *);

#define	DEATH		'd'
#define	SINGLE_USER	's'
#define	RUNCOM		'r'
#define	READ_TTYS	't'
#define	MULTI_USER	'm'
#define	CLEAN_TTYS	'T'
#define	CATATONIA	'c'

typedef enum state_enum {
	invalid_state,
	single_user,
	runcom,
	read_ttys,
	multi_user,
	clean_ttys,
	catatonia,
	death,
	run_script,
} state_enum;

typedef long (*state_func_t)(void);
typedef state_func_t (*state_t)(void);

static state_func_t f_single_user(void);
static state_func_t f_runcom(void);
static state_func_t f_read_ttys(void);
static state_func_t f_multi_user(void);
static state_func_t f_clean_ttys(void);
static state_func_t f_catatonia(void);
static state_func_t f_death(void);
static state_func_t f_run_script(void);

typedef struct state_handle {
	state_t state;
} state_handle_t;

state_handle_t state_funcs[] = {
		{	/* invalid_state */
				.state = NULL,
		},
		{	/* single_user */
				.state = f_single_user,
		},
		{	/* runcom */
				.state = f_runcom,
		},
		{	/* read_ttys */
				.state = f_read_ttys,
		},
		{	/* multi_user */
				.state = f_multi_user,
		},
		{	/* clean_ttys */
				.state = f_clean_ttys,
		},
		{	/* catatonia */
				.state = f_catatonia,
		},
		{	/* death */
				.state = f_death,
		},
		{	/* run_script */
				.state = f_run_script,
		},
};

enum { AUTOBOOT, FASTBOOT } runcom_mode = AUTOBOOT;
#define FALSE	0
#define TRUE	1

static char *init_path_argv0;

static void transition(state_t);
static state_t requested_transition = f_runcom;

static void execute_script(const char *argv[]);
static const char *get_shell(void);
static void replace_init(char *path);

typedef struct init_session {
	int		se_index;		/* index of entry in ttys file */
	pid_t		se_process;		/* controlling process */
	time_t		se_started;		/* used to avoid thrashing */
	int		se_flags;		/* status of session */
#define	SE_SHUTDOWN	0x1			/* session won't be restarted */
#define	SE_PRESENT	0x2			/* session is in /etc/ttys */
	int     	se_nspace;              /* spacing count */
	char		*se_device;		/* filename of port */
	char		*se_getty;		/* what to run on that port */
	char    	*se_getty_argv_space;   /* pre-parsed argument array space */
	char		**se_getty_argv;	/* pre-parsed argument array */
	char		*se_window;		/* window system (started only once) */
	char    	*se_window_argv_space;  /* pre-parsed argument array space */
	char		**se_window_argv;	/* pre-parsed argument array */
	char    	*se_type;               /* default terminal type */
	struct	init_session *se_prev;
	struct	init_session *se_next;
} session_t;

static void	free_session(session_t *);
static session_t *new_session(session_t *, int, struct ttyent *);
static session_t *sessions;

static char	**construct_argv(char *);
static void	start_window_system(session_t *);
static void	collect_child(pid_t, int);
static pid_t start_getty(session_t *);
static void	transition_handler(int);
static void	alrm_handler(int);
static void	setsecuritylevel(int);
static int	getsecuritylevel(void);
static int	setupargv(session_t *, struct ttyent *);
static state_func_t get_next_transition(const char *script);
static state_handle_t *state_handler(int);
static state_t get_state(int);
#ifdef LOGIN_CAP
static void	setprocresources(const char *);
#endif
static int 	clang;

static void clear_session_logs(session_t *, int);

static int start_session_db(void);
static void add_session(session_t *);
static void del_session(session_t *);
static session_t *find_session(pid_t);
static DB *session_db;

#ifdef SUPPORT_UTMPX
static struct timeval boot_time;
static state_t current_state = f_death;
static void session_utmpx(const session_t *, int);
static void make_utmpx(const char *, const char *, int, pid_t,
    const struct timeval *, int);
static char get_runlevel(const state_t);
static void utmpx_set_runlevel(char, char);
#endif

/*
 * The mother of all processes.
 */
int
main(int argc, char *argv[])
{
	state_t initial_transition = get_state(runcom);
	char kenv_value[PATH_MAX];
	int c;
	struct sigaction sa;
	sigset_t mask;

#ifdef SUPPORT_UTMPX
	(void)gettimeofday(&boot_time, NULL);
#endif /* SUPPORT_UTMPX */

	/* Dispose of random users. */
	if (getuid() != 0)
		errx(1, "%s", strerror(EPERM));

	/* System V users like to reexec init. */
	if (getpid() != 1) {
		errx(1, "already running");
	}

	init_path_argv0 = strdup(argv[0]);
	if (init_path_argv0 == NULL) {
		err(1, "strdup");
	}

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
	while ((c = getopt(argc, argv, "sf")) != -1)
		switch (c) {
		case 's':
			initial_transition = get_state(single_user);
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
	handle(badsys, SIGSYS, 0);
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
	(void)sigaction(SIGTTIN, &sa, NULL);
	(void)sigaction(SIGTTOU, &sa, NULL);

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

	if (kenv(KENV_GET, "init_script", kenv_value, sizeof(kenv_value)) > 0) {
		state_func_t next_transition;

		if ((next_transition = get_next_transition(kenv_value)) != NULL)
			initial_transition = (state_t)next_transition;
	}

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
 * Catch a SIGSYS signal.
 *
 * These may arise if a system does not support sysctl.
 * We tolerate up to 25 of these, then throw in the towel.
 */
static void
badsys(int sig)
{
	static int badcount = 0;

	if (badcount++ < 25)
		return;
	disaster(sig);
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
 * Get the security level of the kernel.
 */
static int
getsecuritylevel(void)
{
#ifdef KERN_SECURELVL
	int name[2], curlevel;
	size_t len;

	name[0] = CTL_KERN;
	name[1] = KERN_SECURELVL;
	len = sizeof curlevel;
	if (sysctl(name, 2, &curlevel, &len, NULL, 0) == -1) {
		emergency("cannot get kernel security level: %s", strerror(errno));
		return (-1);
	}
	return (curlevel);
#else
	return (-1);
#endif
}

/*
 * Set the security level of the kernel.
 */
static void
setsecuritylevel(int newlevel)
{
#ifdef KERN_SECURELVL
	int name[2], curlevel;

	curlevel = getsecuritylevel();
	if (newlevel == curlevel)
		return;
	name[0] = CTL_KERN;
	name[1] = KERN_SECURELVL;
	if (sysctl(name, 2, NULL, NULL, &newlevel, sizeof newlevel) == -1) {
		emergency("cannot change kernel security level from %d to %d: %s",
				curlevel, newlevel, strerror(errno));
		return;
	}
#ifdef SECURE
	warning("kernel security level changed from %d to %d",
	    curlevel, newlevel);
#endif
#endif
}

/*
 * Change states in the finite state machine.
 * The initial state is passed as an argument.
 */
static void
transition(state_t s)
{
	for (;;) {
#ifdef SUPPORT_UTMPX
		utmpx_set_runlevel(get_runlevel(current_state), get_runlevel(s));
		current_state = s;
#endif
		s = (state_t)(*s)();
	}
}

/*
 * Close out the accounting files for a login session.
 * NB: should send a message to the session logger to avoid blocking.
 */
static void
clear_session_logs(session_t *sp, int status)
{
#if defined(SUPPORT_UTMP) || defined(SUPPORT_UTMPX)
	char *line = sp->se_device + sizeof(_PATH_DEV) - 1;
#endif

#ifdef SUPPORT_UTMPX
	if (logoutx(line, status, DEAD_PROCESS))
		logwtmpx(line, "", "", status, DEAD_PROCESS);
#endif
#ifdef SUPPORT_UTMP
	if (logout(line))
		logwtmp(line, "", "");
#endif
}

static const char *
get_shell(void)
{
	static char kenv_value[PATH_MAX];

	if (kenv(KENV_GET, "init_shell", kenv_value, sizeof(kenv_value)) > 0) {
		return kenv_value;
	} else {
		return _PATH_BSHELL;
	}
}

/*
 * Start a session and allocate a controlling terminal.
 * Only called by children of init after forking.
 */
static void
setctty(const char *name)
{
	int fd;

	(void)revoke(name);
	if ((fd = open(name, O_RDWR)) == -1) {
		stall("can't open %s: %m", name);
		_exit(1);
	}
	if (login_tty(fd) == -1) {
		stall("can't get %s for controlling terminal: %m", name);
		_exit(1);
	}
}

/*
 * Bring the system up single user.
 */
static state_func_t
f_single_user(void)
{
	pid_t pid, wpid;
	int status;
	sigset_t mask;
	const char *shell;
	const char *argv[2];
#ifdef SECURE
	struct ttyent *typ;
	struct passwd *pp;
	static const char banner[] =
		"Enter root password, or ^D to go multi-user\n";
	char *clear, *password;
#endif
#ifdef DEBUGSHELL
	char altshell[128];
#endif

#ifdef noreboot
	if (Reboot) {
		/* Instead of going single user, let's reboot the machine */
		sync();
		if (reboot(howto) == -1) {
			emergency("reboot(%#x) failed, %s", howto,
			    strerror(errno));
			_exit(1); /* panic and reboot */
		}
		warning("reboot(%#x) returned", howto);
		_exit(0); /* panic as well */
	}
#endif

	/*
	 * If the kernel is in secure mode, downgrade it to insecure mode.
	 */
	if (getsecuritylevel() > 0) {
		setsecuritylevel(0);
	}

	shell = get_shell();

	if ((pid = fork()) == 0) {
		/*
		 * Start the single user session.
		 */
		setctty(_PATH_CONSOLE);

#ifdef SECURE
		/*
		 * Check the root password.
		 * We don't care if the console is 'on' by default;
		 * it's the only tty that can be 'off' and 'secure'.
		 */
		typ = getttynam("console");
		pp = getpwnam("root");
		if (typ && (typ->ty_status & TTY_SECURE) == 0 && pp && *pp->pw_passwd) {
			write(2, banner, sizeof banner - 1);
			for (;;) {
				clear = getpass("Password:");
				if (clear == NULL || *clear == '\0')
					_exit(0);
				password = crypt(clear, pp->pw_passwd);
				bzero(clear, _PASSWORD_LEN);
				if (password != NULL && strcmp(password, pp->pw_passwd) == 0)
					break;
				warning("single-user login failed\n");
			}
		}
		endttyent();
		endpwent();
#endif /* SECURE */

#ifdef DEBUGSHELL
		{
			char altshell[128], *cp = altshell;
			int num;

#define	SHREQUEST \
	"Enter pathname of shell or RETURN for sh: "
			(void)write(STDERR_FILENO,
			    SHREQUEST, sizeof(SHREQUEST) - 1);
			while ((num = read(STDIN_FILENO, cp, 1)) != -1 &&
			    num != 0 && *cp != '\n' && cp < &altshell[127])
					cp++;
			*cp = '\0';
			if (altshell[0] != '\0')
				shell = altshell;
		}
#endif /* DEBUGSHELL */

		/*
		 * Unblock signals.
		 * We catch all the interesting ones,
		 * and those are reset to SIG_DFL on exec.
		 */
		sigemptyset(&mask);
		sigprocmask(SIG_SETMASK, &mask, NULL);

		/*
		 * Fire off a shell.
		 * If the default one doesn't work, try the Bourne shell.
		 */
		char name[] = "-sh";

		argv[0] = name;
		argv[1] = NULL;
		execv(shell, __UNCONST(argv));
		emergency("can't exec %s for single user: %m", shell);
		execv(_PATH_BSHELL, __UNCONST(argv));
		emergency("can't exec %s for single user: %m", _PATH_BSHELL);
		sleep(STALL_TIMEOUT);
		_exit(1);
	}

	if (pid == -1) {
		/*
		 * We are seriously hosed.  Do our best.
		 */
		emergency("can't fork single-user shell, trying again");
		while (waitpid(-1, NULL, WNOHANG) > 0)
			continue;
		return (state_func_t) get_state(single_user);
	}

	requested_transition = 0;
	do {
		if ((wpid = waitpid(-1, &status, WUNTRACED)) != -1)
			collect_child(wpid, status);
		if (wpid == -1) {
			if (errno == EINTR)
				continue;
			warning("wait for single-user shell failed: %m; restarting");
			return (state_func_t) get_state(single_user);
		}
		if (wpid == pid && WIFSTOPPED(status)) {
			warning("init: shell stopped, restarting\n");
			kill(pid, SIGCONT);
			wpid = -1;
		}
	} while (wpid != pid && !requested_transition);

	if (requested_transition)
		return (state_func_t) requested_transition;

	if (!WIFEXITED(status)) {
		if (WTERMSIG(status) == SIGKILL) {
			/*
			 *  reboot(8) killed shell?
			 */
			warning("single user shell terminated.");
			sleep(STALL_TIMEOUT);
			_exit(0);
		} else {
			warning("single user shell terminated, restarting");
			return (state_func_t) get_state(single_user);
		}
	}

	runcom_mode = FASTBOOT;
	return (state_func_t) get_state(runcom);
}

/*
 * Run the system startup script.
 */
static state_func_t
f_runcom(void)
{
	state_func_t next_transition;

	next_transition = (state_func_t)get_state(run_script);
	if (next_transition != NULL) {
		return next_transition;
	}

	runcom_mode = AUTOBOOT;		/* the default */
	/* NB: should send a message to the session logger to avoid blocking. */
	logwtmp("~", "reboot", "");
	return (state_func_t) get_state(read_ttys);
}

static void
execute_script(const char *argv[])
{
	struct sigaction sa;
	const char *shell, *script;
	char sh[] = "sh";
	int error;

	bzero(&sa, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	(void)sigaction(SIGTSTP, &sa, NULL);
	(void)sigaction(SIGHUP, &sa, NULL);

	setctty(_PATH_CONSOLE);

	argv[0] = sh;
	argv[1] = _PATH_RUNCOM;
	argv[2] = (runcom_mode == AUTOBOOT ? "autoboot" : 0);
	argv[3] = NULL;

	sigprocmask(SIG_SETMASK, &sa.sa_mask, NULL);
#ifdef LOGIN_CAP
	setprocresources(RESOURCE_RC);
#endif

	script = argv[0];
	error = access(script, X_OK);
	if (error == 0) {
		execv(script, __UNCONST(argv));
		warning("can't directly exec %s: %m", script);
	} else if (errno != EACCES) {
		warning("can't access %s: %m", script);
	}
	shell = get_shell();
	execv(shell, __UNCONST(argv));
	stall("can't exec %s for %s: %m", shell, script);
}

/*
 * Execute binary, replacing init(8) as PID 1.
 */
static void
replace_init(char *path)
{
	const char *argv[4];
	char sh[] = "sh";

	argv[0] = sh;
	argv[1] = path;
	argv[2] = (runcom_mode == AUTOBOOT ? "autoboot" : 0);
	argv[3] = NULL;

	execute_script(argv);
}

/*
 * Run a shell script.
 * Returns 0 on success, otherwise the next transition to enter:
 *  - single_user if fork/execv/waitpid failed, or if the script
 *    terminated with a signal or exit code != 0.
 *  - death_single if a SIGTERM was delivered to init(8).
 */

static state_func_t
f_run_script(void)
{
	return (get_next_transition(_PATH_RUNCOM));
}

static state_func_t
get_next_transition(const char *script)
{
	pid_t pid, wpid;
	int status;
	const char *argv[4];
	const char *shell;
	shell = get_shell();

	if ((pid = fork()) == 0) {
		execute_script(argv);
		sleep(STALL_TIMEOUT);
		_exit(1); /* force single user mode */
	}

	if (pid == -1) {
		emergency("can't fork for %s on %s: %m", shell, script);
		while (waitpid(-1, (int*) 0, WNOHANG) > 0)
			continue;
		sleep(STALL_TIMEOUT);
		return (state_func_t) get_state(single_user);
	}

	/*
	 * Copied from single_user().  This is a bit paranoid.
	 */
	requested_transition = 0;
	do {
		if ((wpid = waitpid(-1, &status, WUNTRACED)) != -1)
			collect_child(wpid, status);
		if (wpid == -1) {
			if (requested_transition == get_state(death))
				return (state_func_t) get_state(death);
			if (errno == EINTR)
				continue;
			warning("wait for %s on %s failed: %m; going to single user mode", shell, script);
			return (state_func_t) get_state(single_user);
		}
		if (wpid == pid && WIFSTOPPED(status)) {
			warning("init: %s on %s stopped, restarting\n", shell, script);
			kill(pid, SIGCONT);
			wpid = -1;
		}
	} while (wpid != pid);

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM && requested_transition == get_state(catatonia)) {
		/* /etc/rc executed /sbin/reboot; wait for the end quietly */
		sigset_t s;

		sigfillset(&s);
		for (;;)
			sigsuspend(&s);
	}

	if (!WIFEXITED(status)) {
		warning("%s on %s terminated abnormally, going to single user mode", shell, script);
		return (state_func_t) get_state(single_user);
	}

	if (WEXITSTATUS(status))
		return (state_func_t) get_state(single_user);

	return (state_func_t) 0;
}

/*
 * Open the session database.
 *
 * NB: We could pass in the size here; is it necessary?
 */
static int
start_session_db(void)
{
	if (session_db && (*session_db->close)(session_db))
		emergency("session database close: %s", strerror(errno));
	if ((session_db = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL)) == NULL) {
		emergency("session database open: %s", strerror(errno));
		return (1);
	}
	return (0);
}

/*
 * Add a new login session.
 */
static void
add_session(session_t *sp)
{
	DBT key;
	DBT data;

	key.data = &sp->se_process;
	key.size = sizeof sp->se_process;
	data.data = &sp;
	data.size = sizeof sp;

	if ((*session_db->put)(session_db, &key, &data, 0))
		emergency("insert %d: %s", sp->se_process, strerror(errno));
}

/*
 * Delete an old login session.
 */
static void
del_session(session_t *sp)
{
	DBT key;

	key.data = &sp->se_process;
	key.size = sizeof sp->se_process;

	if ((*session_db->del)(session_db, &key, 0))
		emergency("delete %d: %s", sp->se_process, strerror(errno));
}

/*
 * Look up a login session by pid.
 */
static session_t *
find_session(pid_t pid)
{
	DBT key;
	DBT data;
	session_t *ret;

	key.data = &pid;
	key.size = sizeof(pid);
	if ((*session_db->get)(session_db, &key, &data, 0) != 0)
		return 0;
	bcopy(data.data, (char*) &ret, sizeof(ret));
	return ret;
}

/*
 * Construct an argument vector from a command line.
 */
static char **
construct_argv(char *command)
{
	int argc = 0;
	char **argv = malloc(((strlen(command) + 1) / 2 + 1)
						* sizeof (char *));

	if ((argv[argc++] = strk(command)) == NULL) {
		free(argv);
		return (NULL);
	}
	while ((argv[argc++] = strk(NULL)) != NULL)
		continue;
	return argv;
}

/*
 * Deallocate a session descriptor.
 */
static void
free_session(session_t *sp)
{
	free(sp->se_device);
	if (sp->se_getty) {
		free(sp->se_getty);
		free(sp->se_getty_argv_space);
		free(sp->se_getty_argv);
	}
	if (sp->se_window) {
		free(sp->se_window);
		free(sp->se_window_argv_space);
		free(sp->se_window_argv);
	}
	if (sp->se_type)
		free(sp->se_type);
	free(sp);
}

/*
 * Allocate a new session descriptor.
 * Mark it SE_PRESENT.
 */
static session_t *
new_session(session_t *sprev, int session_index, struct ttyent *typ)
{
	session_t *sp;

	if ((typ->ty_status & TTY_ON) == 0 ||
			typ->ty_name == NULL || typ->ty_getty == NULL) {
		return NULL;
	}

	sp = (session_t *)malloc(sizeof (session_t));
	if (sp == NULL) {
		return NULL;
	}
	memset(sp, 0, sizeof *sp);

	sp->se_index = session_index;
	sp->se_flags |= SE_PRESENT;

	asprintf(&sp->se_device, "%s%s", _PATH_DEV, typ->ty_name);
	if (!sp->se_device) {
		free_session(sp);
		return NULL;
	}

	if (setupargv(sp, typ) == 0) {
		free_session(sp);
		return (0);
	}

	sp->se_next = NULL;
	if (sprev == NULL) {
		sessions = sp;
		sp->se_prev = NULL;
	} else {
		sprev->se_next = sp;
		sp->se_prev = sprev;
	}

	return sp;
}

/*
 * Calculate getty and if useful window argv vectors.
 */
static int
setupargv(session_t *sp, struct ttyent *typ)
{
	if (sp->se_getty) {
		free(sp->se_getty);
		free(sp->se_getty_argv_space);
		free(sp->se_getty_argv);
	}
	sp->se_getty = malloc(strlen(typ->ty_getty) + strlen(typ->ty_name) + 2);
	sprintf(sp->se_getty, "%s %s", typ->ty_getty, typ->ty_name);
	sp->se_getty_argv_space = strdup(sp->se_getty);
	sp->se_getty_argv = construct_argv(sp->se_getty_argv_space);
	if (sp->se_getty_argv == NULL) {
		warning("can't parse getty for port %s", sp->se_device);
		free(sp->se_getty);
		free(sp->se_getty_argv_space);
		sp->se_getty = sp->se_getty_argv_space = NULL;
		return (0);
	}
	if (sp->se_window) {
		free(sp->se_window);
		free(sp->se_window_argv_space);
		free(sp->se_window_argv);
	}
	sp->se_window = sp->se_window_argv_space = NULL;
	sp->se_window_argv = NULL;
	if (typ->ty_window) {
		sp->se_window = strdup(typ->ty_window);
		sp->se_window_argv_space = strdup(sp->se_window);
		sp->se_window_argv = construct_argv(sp->se_window_argv_space);
		if (sp->se_window_argv == NULL) {
			warning("can't parse window for port %s", sp->se_device);
			free(sp->se_window_argv_space);
			free(sp->se_window);
			sp->se_window = sp->se_window_argv_space = NULL;
			return (0);
		}
	}
	if (sp->se_type)
		free(sp->se_type);
	sp->se_type = typ->ty_type ? strdup(typ->ty_type) : 0;
	return (1);
}

/*
 * Walk the list of ttys and create sessions for each active line.
 */
static state_func_t
f_read_ttys(void)
{
	int session_index = 0;
	session_t *sp, *snext;
	struct ttyent *typ;

#ifdef SUPPORT_UTMPX
	if (sessions == NULL) {
		struct stat st;

		make_utmpx("", BOOT_MSG, BOOT_TIME, 0, &boot_time, 0);

		/*
		 * If wtmpx is not empty, pick the down time from there
		 */
		if (stat(_PATH_WTMPX, &st) != -1 && st.st_size != 0) {
			struct timeval down_time;

			TIMESPEC_TO_TIMEVAL(&down_time,
			    st.st_atime > st.st_mtime ?
			    &st.st_atimespec : &st.st_mtimespec);
			make_utmpx("", DOWN_MSG, DOWN_TIME, 0, &down_time, 0);
		}
	}
#endif

	/*
	 * Destroy any previous session state.
	 * There shouldn't be any, but just in case...
	 */
	for (sp = sessions; sp; sp = snext) {
		if (sp->se_process)
			clear_session_logs(sp, 0);
		snext = sp->se_next;
		free_session(sp);
	}
	sessions = 0;
	if (start_session_db())
		return (state_func_t) get_state(single_user);

	/*
	 * Allocate a session entry for each active port.
	 * Note that sp starts at 0.
	 */
	while ((typ = getttyent()) != NULL)
		if ((snext = new_session(sp, ++session_index, typ)) != NULL)
			sp = snext;

	endttyent();

	return (state_func_t) get_state(multi_user);
}

/*
 * Start a window system running.
 */
static void
start_window_system(session_t *sp)
{
	pid_t pid;
	sigset_t mask;
	char term[64], *env[2];
	int status;

	if ((pid = fork()) == -1) {
		emergency("can't fork for window system on port %s: %m",
		    sp->se_device);
		/* hope that getty fails and we can try again */
		return;
	}
	if (pid) {
		waitpid(-1, &status, 0);
		return;
	}

	/* reparent window process to the init to not make a zombie on exit */
	if ((pid = fork()) == -1) {
		emergency("can't fork for window system on port %s: %m",
		    sp->se_device);
		_exit(1);
	}
	if (pid)
		_exit(0);

	sigemptyset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

	if (setsid() < 0)
		emergency("setsid failed (window) %m");

#ifdef LOGIN_CAP
	setprocresources(RESOURCE_WINDOW);
#endif
	if (sp->se_type) {
		/* Don't use malloc after fork */
		strcpy(term, "TERM=");
		strlcat(term, sp->se_type, sizeof(term));
		env[0] = term;
		env[1] = NULL;
	} else {
		env[0] = NULL;
	}
	execve(sp->se_window_argv[0], sp->se_window_argv, env);
	stall("can't exec window system '%s' for port %s: %m",
			sp->se_window_argv[0], sp->se_device);
	_exit(1);
}

/*
 * Start a login session running.
 */
static pid_t
start_getty(session_t *sp)
{
	pid_t pid;
	sigset_t mask;
	time_t current_time = time((time_t *) 0);
	int too_quick = 0;
	char term[64], *env[2];

	if (current_time >= sp->se_started &&
	current_time - sp->se_started < GETTY_SPACING) {
		if (++sp->se_nspace > GETTY_NSPACE) {
			sp->se_nspace = 0;
			too_quick = 1;
		}
	} else
		sp->se_nspace = 0;

	/*
	 * fork(), not vfork() -- we can't afford to block.
	 */
	if ((pid = fork()) == -1) {
		emergency("can't fork for getty on port %s: %m", sp->se_device);
		return -1;
	}

	if (pid)
		return pid;

	if (too_quick) {
		warning("getty repeating too quickly on port %s, sleeping %d secs",
				sp->se_device, GETTY_SLEEP);
		sleep((unsigned) GETTY_SLEEP);
	}

	if (sp->se_window) {
		start_window_system(sp);
		sleep(WINDOW_WAIT);
	}

	sigemptyset(&mask);
	sigprocmask(SIG_SETMASK, &mask, NULL);

#ifdef LOGIN_CAP
	setprocresources(RESOURCE_GETTY);
#endif
	if (sp->se_type) {
		/* Don't use malloc after fork */
		strcpy(term, "TERM=");
		strlcat(term, sp->se_type, sizeof(term));
		env[0] = term;
		env[1] = NULL;
	} else {
		env[0] = NULL;
	}
	execve(sp->se_getty_argv[0], sp->se_getty_argv, env);
	stall("can't exec getty '%s' for port %s: %m", sp->se_getty_argv[0],
			sp->se_device);
	_exit(1);
}

#ifdef SUPPORT_UTMPX

static void
session_utmpx(const session_t *sp, int add)
{
    struct timeval session_time;

	const char *name = sp->se_getty ? sp->se_getty :
	    (sp->se_window ? sp->se_window : "");
	const char *line = sp->se_device + sizeof(_PATH_DEV) - 1;

    (void)gettimeofday(&session_time, NULL);
    if (session_time.tv_sec != sp->se_started) {
        session_time.tv_sec = sp->se_started
    }
	make_utmpx(name, line, add ? LOGIN_PROCESS : DEAD_PROCESS,
	    sp->se_process, &session_time, sp->se_index);
}

static void
make_utmpx(const char *name, const char *line, int type, pid_t pid,
    const struct timeval *tv, int session)
{
	struct utmpx ut;
	const char *eline;

	(void)memset(&ut, 0, sizeof(ut));
	(void)strlcpy(ut.ut_name, name, sizeof(ut.ut_name));
	ut.ut_type = type;
	(void)strlcpy(ut.ut_line, line, sizeof(ut.ut_line));
	ut.ut_pid = pid;
	if (tv)
		ut.ut_tv = *tv;
	else
		(void)gettimeofday(&ut.ut_tv, NULL);
	ut.ut_session = session;

	eline = line + strlen(line);
	if ((size_t)(eline - line) >= sizeof(ut.ut_id))
		line = eline - sizeof(ut.ut_id);
	(void)strncpy(ut.ut_id, line, sizeof(ut.ut_id));

	if (pututxline(&ut) == NULL)
		warning("can't add utmpx record for `%s': %m", ut.ut_line);
	endutxent();
}

static char
get_runlevel(const state_t s)
{
	if (s == (state_t)get_state(single_user))
		return SINGLE_USER;
	if (s == (state_t)get_state(runcom))
		return RUNCOM;
	if (s == (state_t)get_state(read_ttys))
		return READ_TTYS;
	if (s == (state_t)get_state(multi_user))
		return MULTI_USER;
	if (s == (state_t)get_state(clean_ttys))
		return CLEAN_TTYS;
	if (s == (state_t)get_state(catatonia))
		return CATATONIA;
	return DEATH;
}

static void
utmpx_set_runlevel(char old, char new)
{
	struct utmpx ut;

	/*
	 * Don't record any transitions until we did the first transition
	 * to read ttys, which is when we are guaranteed to have a read-write
	 * /var. Perhaps use a different variable for this?
	 */
	if (sessions == NULL)
		return;

	(void)memset(&ut, 0, sizeof(ut));
	(void)snprintf(ut.ut_line, sizeof(ut.ut_line), RUNLVL_MSG, new);
	ut.ut_type = RUN_LVL;
	(void)gettimeofday(&ut.ut_tv, NULL);
	ut.ut_exit.e_exit = old;
	ut.ut_exit.e_termination = new;
	if (pututxline(&ut) == NULL)
		warning("can't add utmpx record for `runlevel': %m");
	endutxent();
}

#endif

/*
 * Collect exit status for a child.
 * If an exiting login, start a new login running.
 */
static void
collect_child(pid_t pid, int status)
{
	session_t *sp, *sprev, *snext;

	if (!sessions)
		return;

	if (!(sp = find_session(pid)))
		return;

	clear_session_logs(sp, status);
	del_session(sp);
	sp->se_process = 0;

	if ((sp->se_flags & SE_SHUTDOWN)) {
		if ((sprev = sp->se_prev) != NULL)
			sprev->se_next = sp->se_next;
		else
			sessions = sp->se_next;
		if ((snext = sp->se_next) != NULL)
			snext->se_prev = sp->se_prev;
		free_session(sp);
		return;
	}

	if ((pid = start_getty(sp)) == -1) {
		/* serious trouble */
		requested_transition = get_state(clean_ttys);
		return;
	}

	sp->se_process = pid;
	sp->se_started = time((time_t *) 0);
	add_session(sp);
}

/* Return state_handle_t from state enumerator */
static state_handle_t *
state_handler(int num)
{
	state_handle_t *handle;
	int i;

#define nelems(x) (sizeof(x) / sizeof((x)[0]))

	handle = &state_funcs[0];
	for (i = 0; i < (int)nelems(state_funcs); i++) {
		handle = &state_funcs[i];
		if ((handle != NULL) && (num == i)) {
			return (handle);
		}
	}
	return (NULL);
}

static state_t
get_state(int num)
{
	state_handle_t *state;

	state = state_handler(num);
	if (state != NULL) {
		return (state->state);
	}
	return (NULL);
}

/*
 * Catch a signal and request a state transition.
 */
static void
transition_handler(int sig)
{
	switch (sig) {
	case SIGHUP:
		requested_transition = get_state(clean_ttys);
		break;
	case SIGTERM:
		requested_transition = get_state(death);
		break;
	case SIGTSTP:
		requested_transition = get_state(catatonia);
		break;
	default:
		requested_transition = 0;
		break;
	}
}

/*
 * Take the system multiuser.
 */
static state_func_t
f_multi_user(void)
{
	pid_t pid;
	int status;
	session_t *sp;

	requested_transition = 0;

	/*
	 * If the administrator has not set the security level to -1
	 * to indicate that the kernel should not run multiuser in secure
	 * mode, and the run script has not set a higher level of security
	 * than level 1, then put the kernel into secure mode.
	 */
	if (getsecuritylevel() == 0)
		setsecuritylevel(1);

	for (sp = sessions; sp; sp = sp->se_next) {
		if (sp->se_process)
			continue;
		if ((pid = start_getty(sp)) == -1) {
			/* serious trouble */
			requested_transition = get_state(clean_ttys);
			break;
		}
		sp->se_process = pid;
		sp->se_started = time((time_t *) 0);
		add_session(sp);
	}

	while (!requested_transition)
		if ((pid = waitpid(-1, &status, 0)) != -1)
			collect_child(pid, status);

	return (state_func_t) requested_transition;
}

/*
 * This is an (n*2)+(n^2) algorithm.  We hope it isn't run often...
 */
static state_func_t
f_clean_ttys(void)
{
	session_t *sp, *sprev;
	struct ttyent *typ;
	int session_index = 0;
	int devlen;
	char *old_getty, *old_window, *old_type;

	if (!sessions)
		return (state_func_t) get_state(multi_user);

	/*
	 * mark all sessions for death, (!SE_PRESENT)
	 * as we find or create new ones they'll be marked as keepers,
	 * we'll later nuke all the ones not found in /etc/ttys
	 */
	for (sp = sessions; sp != NULL; sp = sp->se_next)
		sp->se_flags &= ~SE_PRESENT;

	devlen = sizeof(_PATH_DEV) - 1;
	while ((typ = getttyent()) != NULL) {
		++session_index;

		for (sprev = NULL, sp = sessions; sp; sprev = sp, sp = sp->se_next)
			if (strcmp(typ->ty_name, sp->se_device + devlen) == 0)
				break;

		if (sp) {
			/* we want this one to live */
			sp->se_flags |= SE_PRESENT;
			if (sp->se_index != session_index) {
				warning("port %s changed utmpx index from %d to %d",
						sp->se_device, sp->se_index, session_index);
				sp->se_index = session_index;
			}
			if ((typ->ty_status & TTY_ON) == 0 || typ->ty_getty == 0) {
				sp->se_flags |= SE_SHUTDOWN;
				kill(sp->se_process, SIGHUP);
				continue;
			}
			sp->se_flags &= ~SE_SHUTDOWN;
			old_getty = sp->se_getty ? strdup(sp->se_getty) : 0;
			old_window = sp->se_window ? strdup(sp->se_window) : 0;
			old_type = sp->se_type ? strdup(sp->se_type) : 0;
			if (setupargv(sp, typ) == 0) {
				warning("can't parse getty for port %s", sp->se_device);
				sp->se_flags |= SE_SHUTDOWN;
				kill(sp->se_process, SIGHUP);
			} else if (!old_getty || (!old_type && sp->se_type)
					|| (old_type && !sp->se_type)
					|| (!old_window && sp->se_window)
					|| (old_window && !sp->se_window)
					|| (strcmp(old_getty, sp->se_getty) != 0)
					|| (old_window && strcmp(old_window, sp->se_window) != 0)
					|| (old_type && strcmp(old_type, sp->se_type) != 0)) {
				/* Don't set SE_SHUTDOWN here */
				sp->se_nspace = 0;
				sp->se_started = 0;
				kill(sp->se_process, SIGHUP);
			}
			if (old_getty)
				free(old_getty);
			if (old_window)
				free(old_window);
			if (old_type)
				free(old_type);
			continue;
		}

		new_session(sprev, session_index, typ);
	}

	endttyent();

	/*
	 * sweep through and kill all deleted sessions
	 * ones who's /etc/ttys line was deleted (SE_PRESENT unset)
	 */
	for (sp = sessions; sp != NULL; sp = sp->se_next) {
		if ((sp->se_flags & SE_PRESENT) == 0) {
			sp->se_flags |= SE_SHUTDOWN;
			kill(sp->se_process, SIGHUP);
		}
	}

	return (state_func_t) get_state(multi_user);
}

/*
 * Block further logins.
 */
static state_func_t
f_catatonia(void)
{
	session_t *sp;

	for (sp = sessions; sp; sp = sp->se_next)
		sp->se_flags |= SE_SHUTDOWN;

	return (state_func_t) get_state(multi_user);
}

/*
 * Note SIGALRM.
 */
static void
alrm_handler(int sig)
{
	clang = 1;
}

/*
 * Bring the system down to single user.
 */
static state_func_t
f_death(void)
{
	session_t *sp;
	int i, status;
	pid_t pid;
	static const int death_sigs[3] = { SIGHUP, SIGTERM, SIGKILL };

	for (sp = sessions; sp; sp = sp->se_next)
			sp->se_flags |= SE_SHUTDOWN;

	/* NB: should send a message to the session logger to avoid blocking. */
	logwtmp("~", "shutdown", "");

	for (i = 0; i < 3; ++i) {
		if (kill(-1, death_sigs[i]) == -1 && errno == ESRCH) {
			return (state_func_t) get_state(single_user);
		}
		clang = 0;
		alarm(DEATH_WATCH);
		do
			if ((pid = waitpid(-1, &status, 0)) != -1)
				collect_child(pid, status);
		while (clang == 0 && errno != ECHILD);

		if (errno == ECHILD)
			return (state_func_t) get_state(single_user);
	}

	warning("some processes would not die; ps axl advised");

	return (state_func_t) get_state(single_user);
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

#ifdef LOGIN_CAP
static void
setprocresources(const char *cname)
{
	login_cap_t *lc;
	if ((lc = login_getclassbyname(cname, NULL)) != NULL) {
		setusercontext(lc, (struct passwd*) NULL, 0,
				LOGIN_SETENV | LOGIN_SETPRIORITY | LOGIN_SETRESOURCES
						| LOGIN_SETLOGINCLASS | LOGIN_SETCPUMASK);
		login_close(lc);
	}
}
#endif
