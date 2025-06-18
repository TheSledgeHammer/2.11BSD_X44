/*
 * Copyright (c) 1989, 1993, 1994
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
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>

#if	!defined(lint) && !defined(DOSCCS)
#if 0
static char sccsid[] = "@(#)chmod.c	5.5.1 (2.11BSD GTE) 11/4/94";
static char sccsid[] = "@(#)chmod.c	8.8 (Berkeley) 4/1/94";
#endif
#endif /* not lint */
#if	!defined(lint)
#if 0
__COPYRIGHT(
"@(#) Copyright (c) 1989, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif
#endif /* not lint */

/*
 * chmod options mode files
 * where
 *	mode is [ugoa][+-=][rwxXstugo] or an octal number
 *	options are -Rf
 */
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *fchdirmsg = "Can't fchdir() back to starting directory";
int	status;
int Rflag, Pflag, Lflag, hflag, fflag;

void usage(void);
int stat_traversal(int, char **, struct stat *, void *, int, int);
int fts_option(void);
int fts_traversal(char **, FTS *, FTSENT *, int, void *, int, int);
int chmodr(struct stat *, char *, mode_t, int);
int error(const char *, char *);
void fatal(int, const char *, char *);
int Perror(char *);
int newmode(char *, int *, int *, void *, int, unsigned int);
int abs_filemode(char *);
void assert_filemode(char *, int *, int *, void *);
int who(char *, int);
int what(char *);
int where(char *, int);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FTS ftsp;
	FTSENT p;
	struct stat st;
	void *set;
	int oct, omode;
	int fts_options;
	char *mode;

	omode = 0;
	while ((ch = getopt(argc, argv, "HLPRXfgorstuwx")) != EOF) {
		switch (ch) {
		case 'H':
			Hflag++;
			Lflag = 0;
			Pflag = 0;
			fflag = 0;
			break;
		case 'L':
			Lflag++;
			Hflag = 0;
			Pflag = 0;
			fflag = 0;
			break;
		case 'P':
			Pflag++;
			Hflag = 0;
			Lflag = 0;
			fflag = 0;
			break;
		case 'R':
			Rflag++;
			break;
		case 'f':
			fflag++;
			break;
		case 'h':
			/*
			 * In System V (and probably POSIX.2) the -h option
			 * causes chmod to change the mode of the symbolic
			 * link.  4.4BSD's symbolic links don't have modes,
			 * so it's an undocumented noop.  Do syntax checking,
			 * though.
			 */
			hflag++;
			break;
			/*
			 * XXX
			 * "-[rwx]" are valid mode commands.  If they are the entire
			 * argument, getopt has moved past them, so decrement optind.
			 * Regardless, we're done argument processing.
			 */
		case 'g':
		case 'o':
		case 'r':
		case 's':
		case 't':
		case 'u':
		case 'w':
		case 'X':
		case 'x':
			if (argc > 0 && argv[optind - 1][0] == '-' && argv[optind - 1][1] == ch
					&& argv[optind - 1][2] == '\0') {
				--optind;
			}
			goto done;
		case '?':
		default:
			usage();
		}
	}

done:
	argv += optind;
	argc -= optind;

	if (argc < 2) {
		usage();
	}

	fts_options = fts_option();

	mode = *argv;
	mask = umask(0);
	(void)newmode(mode, &omode, &oct, set, mask, 0);

	if ((Hflag != 0 || Lflag != 0 || Pflag != 0) && (fflag == 0)) {
		status = fts_traversal(argv, &ftsp, &p, fts_options, set, oct, omode);
	} else {
		status = stat_traversal(argc, argv, &st, set, oct, omode);
	}
	exit(status);
}

void
usage(void)
{
	(void)fprintf(stderr, "Usage: chmod [-Rf [-H | -L | -P]] [ugoa][+-=][rwxXstugo] file ...\n");
	exit(-1);
}

int
stat_traversal(int argc, char *argv[], struct stat *stp, void *set, int oct, int omode)
{
	register char *p;
	register int i;
	int	fcurdir;

	fcurdir = 0;
	if (Rflag) {
		fcurdir = open(".", O_RDONLY);
		if (fcurdir < 0) {
			fatal(255, "Can't open .");
		}
	}
	for (i = 1; i < argc; i++) {
		p = argv[i];
		/* do stat for directory arguments */
		if (lstat(p, stp) < 0) {
			status += Perror(p);
			continue;
		}
		if (Rflag && (stp->st_mode & S_IFMT) == S_IFDIR) {
			status += chmodr(stp, p, getmode(set, stp->st_mode), fcurdir);
			continue;
		}
		if ((stp->st_mode & S_IFMT) == S_IFLNK && stat(p, stp) < 0) {
			status += Perror(p);
			continue;
		}
		if (chmod(p, oct ? omode : getmode(set, stp->st_mode)) < 0) {
			status += Perror(p);
			continue;
		}
	}
	close(fcurdir);
	return (status);
}

int
fts_option(void)
{
	int fts_options;

	fts_options = FTS_PHYSICAL;
	if (Rflag) {
		if (hflag)
			errx(1, "the -R and -h options may not be specified together.");
		if (Hflag) {
			fts_options |= FTS_COMFOLLOW;
		}
		if (Lflag) {
			fts_options &= ~FTS_PHYSICAL;
			fts_options |= FTS_LOGICAL;
		}
	}
	return (fts_options);
}

int
fts_traversal(char *argv[], FTS *ftsp, FTSENT *p, int fts_options, void *set, int oct, int omode)
{
	ftsp = fts_open(++argv, fts_options, 0);
	if (ftsp == NULL) {
		err(1, NULL);
	}

	p = fts_read(ftsp);
	for (status = 0; p != NULL;) {
		switch (p->fts_info) {
		case FTS_D:
			if (Rflag) {/* Change it at FTS_DP. */
				continue;
			}
			fts_set(ftsp, p, FTS_SKIP);
			break;
		case FTS_DNR: /* Warn, chmod, continue. */
			warnx("%s: %s", p->fts_path, strerror(p->fts_errno));
			status = 1;
			break;
		case FTS_ERR: /* Warn, continue. */
		case FTS_NS:
			warnx("%s: %s", p->fts_path, strerror(p->fts_errno));
			status = 1;
			continue;
		case FTS_SL: /* Ignore. */
		case FTS_SLNONE:
			/*
			 * The only symlinks that end up here are ones that
			 * don't point to anything and ones that we found
			 * doing a physical walk.
			 */
			continue;
		default:
			break;
		}
		if (chmod(p->fts_accpath,
				oct ? omode : getmode(set, p->fts_statp->st_mode)) && !fflag) {
			warn(p->fts_path);
			status = 1;
		}
	}
	return (status);
}

int
chmodr(struct stat *stp, char *dir, mode_t mode, int savedir)
{
	register DIR *dirp;
	register struct direct *dp;
	int ecode;

	if (chmod(dir, mode) < 0 && Perror(dir)) {
		return (1);
	}
	if (chdir(dir) < 0) {
		Perror(dir);
		return (1);
	}
	if ((dirp = opendir(".")) == NULL) {
		Perror(dir);
		return (1);
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (lstat(dp->d_name, stp) < 0) {
			ecode = Perror(dp->d_name);
			if (ecode) {
				break;
			}
			continue;
		}
		if ((stp->st_mode & S_IFMT) == S_IFDIR) {
			ecode = chmodr(stp, dp->d_name, mode, dirfd(dirp));
			if (ecode) {
				break;
			}
			continue;
		}
		if ((stp->st_mode & S_IFMT) == S_IFLNK) {
			continue;
		}
		if (chmod(dp->d_name, mode) < 0
				&& (ecode = Perror(dp->d_name))) {
			break;
		}
	}
	if (fchdir(savedir) < 0) {
		fatal(255, fchdirmsg);
	}
	closedir(dirp);
	return (ecode);
}

int
error(const char *fmt, char *a)
{

	if (!fflag) {
		fprintf(stderr, "chmod: ");
		fprintf(stderr, fmt, a);
		putc('\n', stderr);
	}
	return (!fflag);
}

void
fatal(int code, const char *fmt, char *a)
{
	fflag = 0;
	(void) error(fmt, a);
	exit(code);
}

int
Perror(char *s)
{
	if (!fflag) {
		fprintf(stderr, "chmod: ");
		perror(s);
	}
	return (!fflag);
}

int
newmode(msp, omode, oct, set, ump, nm)
	char *msp;
	int *oct, *omode;
	void *set;
	int ump;
	unsigned int nm;
{
	register int o, m, b;
	int savem;

	savem = nm;
	m = abs_filemode(msp);
	if (*msp == '\0') {
		return (m);
	}
	do {
		m = who(msp, ump);
		while ((o = what(msp))) {
			b = where(msp, nm);
			switch (o) {
			case '+':
				nm |= b & m;
				break;
			case '-':
				nm &= ~(b & m);
				break;
			case '=':
				nm &= ~m;
				nm |= b & m;
				break;
			}
		}
	} while (*msp++ == ',');
	if (*--msp) {
		fatal(255, "invalid mode: %s", msp);
	}
	assert_filemode(msp, omode, oct, set);
	return (nm);
}

int
abs_filemode(msp)
	char *msp;
{
	register int c, i;

	while ((c = (unsigned char)*msp++) >= '0' && c <= '7') {
		i = (i << 3) + (c - '0');
	}
	msp--;
	return (i);
}

void
assert_filemode(msp, omode, oct, set)
	char *msp;
	int *oct, *omode;
	void *set;
{
	char *ep;
	long val;

	if (*msp >= '0' && *msp <= '7') {
		errno = 0;
		val = strtol(msp, &ep, 8);
		if (val > INT_MAX || val < 0) {
			errno = ERANGE;
		}
		if (errno) {
			err(1, "invalid file mode: %s", msp);
		}
		if (*ep) {
			errx(1, "invalid file mode: %s", msp);
		}
		*omode = val;
		*oct = 1;
	} else {
		set = setmode(msp);
		if (set == NULL) {
			errx(1, "invalid file mode: %s", msp);
		}
		*oct = 0;
	}
}

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL		01777	/* all (note absence of setuid, etc) */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	STICKY	01000	/* sticky bit */

int
who(msp, ump)
	char *msp;
	int ump;
{
	register int m;

	m = 0;
	for (;;) {
		switch (*msp++) {
		case 'u':
			m |= USER;
			continue;
		case 'g':
			m |= GROUP;
			continue;
		case 'o':
			m |= OTHER;
			continue;
		case 'a':
			m |= ALL;
			continue;
		default:
			msp--;
			if (m == 0)
				m = ALL & ~ump;
			return (m);
		}
	}
}

int
what(msp)
	char *msp;
{
	switch (*msp) {
	case '+':
	case '-':
	case '=':
		return (*msp++);
	}
	return (0);
}

int
where(msp, om)
	char *msp;
	register int om;
{
	register int m;

 	m = 0;
	switch (*msp) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		m &= (READ|WRITE|EXEC);
		m |= (m << 3) | (m << 6);
		++msp;
		return (m);
	}
	for (;;) {
		switch (*msp++) {
		case 'r':
			m |= READ;
			continue;
		case 'w':
			m |= WRITE;
			continue;
		case 'x':
			m |= EXEC;
			continue;
		case 'X':
			if ((om & S_IFDIR) || (om & EXEC))
				m |= EXEC;
			continue;
		case 's':
			m |= SETID;
			continue;
		case 't':
			m |= STICKY;
			continue;
		default:
			msp--;
			return (m);
		}
	}
}
