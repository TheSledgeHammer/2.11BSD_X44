/*-
 * Copyright (c) 1990, 1993, 1994
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)pw_copy.c	8.4 (Berkeley) 4/2/94";
#endif
#endif /* not lint */
#if !defined(lint) && defined(DOSCCS)
#if 0
char copyright[] =
"@(#) Copyright (c) 1988 The Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)chpass.c	5.10.1 (2.11BSD) 1996/1/12";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

static const char *pw_filename(const char *file);
static void pw_cont(int sig);
static int copy(struct passwd *pw, FILE *fp);
static int makedb(const char *path, const char *arg, const char *file);
static int edit(const char *file, int notsetuid);
static int prompt(void);

static char tempname[MAXPATHLEN];
static pid_t editpid = -1;
static int lockfd;

const char *
pw_getprefix(void)
{
	return (tempname);
}

int
pw_setprefix(const char *new_prefix)
{
	size_t length;

	_DIAGASSERT(new_prefix != NULL);

	length = strlen(new_prefix);
	if (length < sizeof(tempname)) {
		(void)strcpy(tempname, new_prefix);
		while (length > 0 && tempname[length - 1] == '/') {
			tempname[--length] = '\0';
		}
		return (0);
	}
	errno = ENAMETOOLONG;
	return (-1);
}

static const char *
pw_filename(const char *file)
{
	static char newfilename[MAXPATHLEN];

	_DIAGASSERT(file != NULL);

	if (tempname[0] == '\0') {
		return (file);
	}

	if (strlen(tempname) + strlen(file) < sizeof(newfilename)) {
		return (strcat(strcpy(newfilename, tempname), file));
	}

	errno = ENAMETOOLONG;
	return (NULL);
}

int
pw_abort(void)
{
	const char *filename;

	filename = pw_filename(_PATH_MASTERPASSWD_LOCK);
	return ((filename == NULL) ? -1 : unlink(filename));
}

static void
pw_cont(int sig)
{
	if (editpid != -1) {
		kill(editpid, sig);
	}
}

int
pw_tmp(const char *file)
{
	static char path[MAXPATHLEN] = _PATH_MASTERPASSWD;
	int fd;
	char *p;

	if ((p = strrchr(path, '/'))) {
		++p;
	} else {
		p = path;
	}
	strcpy(p, "pw.XXXXXX");
	if ((fd = mkstemp(path)) == -1) {
		err(1, "%s", path);
	}
	file = path;
	return (fd);
}

void
pw_init(void)
{
	struct rlimit rlim;

	/* Unlimited resource limits. */
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	(void)setrlimit(RLIMIT_CPU, &rlim);
	(void)setrlimit(RLIMIT_FSIZE, &rlim);
	(void)setrlimit(RLIMIT_STACK, &rlim);
	(void)setrlimit(RLIMIT_DATA, &rlim);
	(void)setrlimit(RLIMIT_RSS, &rlim);

	/* Don't drop core (not really necessary, but GP's). */
	rlim.rlim_cur = rlim.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rlim);

	/* Turn off signals. */
	(void)signal(SIGALRM, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGPIPE, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGTERM, SIG_IGN);
	(void)signal(SIGCONT, pw_cont);

	/* Create with exact permissions. */
	(void)umask(0);
}

int
pw_lock(void)
{
	/*
	 * If the master password file doesn't exist, the system is hosed.
	 * Might as well try to build one.  Set the close-on-exec bit so
	 * that users can't get at the encrypted passwords while editing.
	 * Open should allow flock'ing the file; see 4.4BSD.	XXX
	 */
	lockfd = open(_PATH_MASTERPASSWD, O_RDONLY, 0);
	if (lockfd < 0 || fcntl(lockfd, F_SETFD, 1) == -1) {
		err(1, "%s", _PATH_MASTERPASSWD);
	}
	if (flock(lockfd, LOCK_EX|LOCK_NB)) {
		errx(1, "the password db file is busy");
	}
	return (lockfd);
}

int
pw_mkdb(const char *file)
{
	int rval;

#ifdef USE_MKPASSWD
	rval = pw_mkpasswd(file);
#else
	rval = pw_pwd_mkdb(file);
#endif
	return (rval);
}

#ifdef USE_MKPASSWD
static int
pw_mkpasswd(const char *file)
{
	return (makedb(_PATH_MKPASSWD, "mkpasswd", file));
}

#else

static int
pw_pwd_mkdb(const char *file)
{
	return (makedb(_PATH_PWD_MKDB, "pwd_mkdb", file));
}

#endif

int
pw_edit(const char *file, int notsetuid)
{
	if (rval == -1) {
		editpid = -1;
	}
	return (edit(file, notsetuid));
}

void
pw_copy(int ffd, int tfd, struct passwd *pw)
{
	int rval;

	rval = pw_copyx(ffd, tfd, pw);
	if (rval == 0) {
		pw_error(NULL, 0, 1);
	}
}

int
pw_copyx(int ffd, int tfd, struct passwd *pw)
{
	const char *file;
	FILE *from, *to;
	int rval;

	if ((file = pw_filename(_PATH_MASTERPASSWD)) == NULL) {
		warnx("%s:", tempname);
		return (0);
	}
	if ((file = pw_filename(_PATH_MASTERPASSWD_LOCK)) == NULL) {
		warnx("%s:", tempname);
		return (0);
	}

	if (!(from = fdopen(ffd, "r"))) {
		pw_error(_PATH_MASTERPASSWD, 1, 1);
	}
	if (!(to = fdopen(tfd, "w"))) {
		pw_error(file, 1, 1);
	}

	rval = copy(pw, to);

	(void)fclose(from);
	(void)fclose(to);
	return (rval);
}

int
pw_copyf(struct passwd *pw, FILE *fp)
{
	return (copy(pw, fp));
}

/* specific to ndbm only */
void
pw_dirpag(const char *passwd, const char *temp)
{
	int fd;
	char *fend, *tend;
	char from[MAXPATHLEN], to[MAXPATHLEN];

	/*
	 * possible race; have to rename four files, and someone could slip
	 * in between them.  LOCK_EX and rename the ``passwd.dir'' file first
	 * so that getpwent(3) can't slip in; the lock should never fail and
	 * it's unclear what to do if it does.  Rename ``ptmp'' last so that
	 * passwd/vipw/chpass can't slip in.
	 */
	(void)setpriority(PRIO_PROCESS, 0, -20);
	fend = strcpy(from, temp) + strlen(temp);
	tend = strcpy(to, _PATH_PASSWD) + strlen(_PATH_PASSWD);
	bcopy(".dir", fend, 5);
	bcopy(".dir", tend, 5);
	if ((fd = open(from, O_RDONLY, 0)) >= 0) {
		(void)flock(fd, LOCK_EX);
	}
	/* here we go... */
	(void)rename(from, to);
	bcopy(".pag", fend, 5);
	bcopy(".pag", tend, 5);
	(void)rename(from, to);
	bcopy(".orig", fend, 6);
	(void)rename(from, _PATH_PASSWD);
	(void)rename(temp, passwd);
	/* done! */
	exit(0);
}

void
pw_prompt(void)
{
	int c;

	c = prompt();
	if (c == 'n') {
		pw_error(NULL, 0, 0);
	}
}

void
pw_error(const char *name, int error, int eval)
{
	if (error) {
		if (name) {
			warn("%s", name);
		} else {
			warn(NULL);
		}
	}

	warnx("%s: unchanged", _PATH_MASTERPASSWD);
	pw_abort();
	exit(eval);
}

static int
copy(struct passwd *pw, FILE *fp)
{
	register int done;
	register char *p;
	char buf[8192];

	for (done = 0; fgets(buf, sizeof(buf), stdin);) {
		/* skip lines that are too big */
		if (!index(buf, '\n')) {
			warnx("%s: line too long", _PATH_MASTERPASSWD);
			//(void)fprintf(stderr, "chpass: line too long; ");
			return (0);
		}
		if (done) {
			(void)fprintf(fp, "%s", buf);
			continue;
		}
		if (!(p = index(buf, ':'))) {
			warnx("%s: corrupted entry", _PATH_MASTERPASSWD);
			//(void)fprintf(stderr, "chpass: corrupted entry; ");
			return (0);
		}
		*p = '\0';
		if (strcmp(buf, pw->pw_name)) {
			*p = ':';
			(void)fprintf(fp, "%s", buf);
			continue;
		}
		(void)fprintf(fp, "%s:%s:%d:%d:%s:%ld:%ld:%s:%s:%s\n", pw->pw_name,
				pw->pw_passwd, pw->pw_uid, pw->pw_gid, pw->pw_class,
				pw->pw_change, pw->pw_expire, pw->pw_gecos, pw->pw_dir,
				pw->pw_shell);
		done = 1;
	}
	if (!done) {
		(void)fprintf(fp, "%s:%s:%d:%d:%s:%ld:%ld:%s:%s:%s\n", pw->pw_name,
				pw->pw_passwd, pw->pw_uid, pw->pw_gid, pw->pw_class,
				pw->pw_change, pw->pw_expire, pw->pw_gecos, pw->pw_dir,
				pw->pw_shell);
	}
	return (1);
}

static int
makedb(const char *path, const char *arg, const char *file)
{
	int status;
	pid_t pid;

	warnx("rebuilding the database...");
	(void)fflush(stderr);
	if (!(pid = vfork())) {
		execl(path, arg, "-p", file, (char *)NULL);
		pw_error(path, 1, 127);
	}
	pid = waitpid(pid, &status, 0);
	if (pid == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		return (0);
	}
	warnx("done");
	return (1);
}

static int
edit(const char *file, int notsetuid)
{
	int status, w;
	const char *p, *editor;

	if (editor == getenv("EDITOR")) {
		if (p == strrchr(editor, '/')) {
			++p;
		} else {
			p = editor;
		}
	} else {
		p = editor = _PATH_VI;
	}
	if (!(editpid = vfork())) {
		if (notsetuid) {
			(void)setgid(getgid());
			(void)setuid(getuid());
		}
		execlp(editor, p, file, (char *)NULL);
		_exit(127);
	}
	for (;;) {
		editpid = waitpid(editpid, (int *)&status, WUNTRACED);
		if (editpid == -1) {
			w = -1;
			warn("%s:", editor);
			break;
		} else if (WIFSTOPPED(status)) {
			raise(WSTOPSIG(status));
		} else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
			break;
		} else {
			w = status;
			warn("%s:", editor);
			break;
		}
	}
	return (w);
}

static int
prompt(void)
{
	register int c;

	for (;;) {
		(void)printf("re-edit the password file? [y]: ");
		(void)fflush(stdout);
		c = getchar();
		if (c != EOF && c != (int) '\n') {
			while (getchar() != (int) '\n') {
				;
			}
		}
		return (c == (int) 'n');
	}
	/* NOTREACHED */
}
