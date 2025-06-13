/*
 *	Program Name:   symcompact.c
 *	Date: December 3, 1994
 *	Author: S.M. Schultz
 *
 *	-----------------   Modification History   ---------------
 *      Version Date            Reason For Modification
 *      1.0     03Dec94         1. Initial release into the public domain.
*/

#include <sys/cdefs.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

static	const char *fmsg = "Can't fchdir() back to starting directory";
static	int	oct, status, fflag, rflag, hflag, lflag, pflag;
static	u_short	set, clear;
static  struct stat st;

int stat_traversal(char **, struct stat *);
int fts_option(void);
int fts_traversal(char **, FTS *, FTSENT *, int);
int recurse(struct stat *, char *, int);
void die(const char *, ...);
int warning(char *);
int warningx(char *, int);
int newflags(u_short);
void usage(void);

int
main(int argc, char	*argv[])
{
	FTS ftsp;
	FTSENT p;
	char *flags, *ep;
	int ch, fts_options;
	long tmp;
	
	while ((ch = getopt(argc, argv, "HLPRf")) != EOF) {
		switch (ch) {
		case 'H':
			hflag++;
			lflag = 0;
			pflag = 0;
			fflag = 0;
			break;
		case 'L':
			lflag++;
			hflag = 0;
			pflag = 0;
			fflag = 0;
			break;
		case 'P':
			pflag++;
			hflag = 0;
			lflag = 0;
			fflag = 0;
			break;
		case 'R':
			rflag++;
			break;
		case 'f':
			fflag++;
			break;
		case '?':
		default:
			usage();
		}
	}
	argv += optind;
	argc += optind;
	if (argc < 2) {
		usage();
	}

	fts_options = fts_option();

	flags = *argv++;
	if (*flags >= '0' && *flags <= '7') {
		tmp = strtol(flags, &ep, 8);
		if (tmp < 0 || tmp >= 64L * 1024 * 1024 || *ep) {
			die("invalid flags: %s", flags);
		}
		oct = 1;
		set = tmp;
	} else {
		if (string_to_flags(&flags, (u_long *)&set, (u_long *)&clear)) {
			die("invalid flag: %s", flags);
		}
		clear = ~clear;
		oct = 0;
	}

	if ((hflag != 0 || lflag != 0 || pflag != 0) && (fflag == 0)) {
		status = fts_traversal(argv, &ftsp, &p, fts_options);
	} else {
		status = stat_traversal(argv, &st);
	}
	exit(status);
}

int
stat_traversal(char *argv[], struct stat *stp)
{
	register char *p;
	int fcurdir;
	int stat_status;

	stat_status = 0;
	fcurdir = 0;
	if (rflag) {
		fcurdir = open(".", O_RDONLY);
		if (fcurdir < 0) {
			die("Can't open .");
		}
	}
	while ((p = *argv++)) {
		if (lstat(p, stp) < 0) {
			stat_status |= warning(p);
			continue;
		}
		if (rflag && (stp->st_mode & S_IFMT) == S_IFDIR) {
			stat_status |= recurse(stp, p, fcurdir);
			continue;
		}
		if ((stp->st_mode & S_IFMT) == S_IFLNK && stat(p, stp) < 0) {
			stat_status |= warning(p);
			continue;
		}
		if (chflags(p, newflags(stp->st_flags)) < 0) {
			stat_status |= warning(p);
			continue;
		}
	}
	close(fcurdir);
	return (stat_status);
}

int
fts_option(void)
{
	int fts_options;

	fts_options = FTS_PHYSICAL;
	if (rflag) {
		if (hflag) {
			fts_options |= FTS_COMFOLLOW;
		}
		if (lflag) {
			fts_options &= ~FTS_PHYSICAL;
			fts_options |= FTS_LOGICAL;

		}
	}
	return (fts_options);
}

int
fts_traversal(char *argv[], FTS *ftsp, FTSENT *p, int fts_options)
{
	int fts_status;

	fts_status = 0;
	ftsp = fts_open(argv, fts_options , 0);
	if (ftsp == NULL) {
		die("fts_open");
	}

	p = fts_read(ftsp);
	while (p != NULL) {
		switch (p->fts_info) {
		case FTS_D:
			if (rflag) { /* Change it at FTS_DP. */
				continue;
			}
			fts_set(ftsp, p, FTS_SKIP);
			break;
		case FTS_DNR: /* Warn, chflag, continue. */
			fts_status |= warningx(p->fts_path, p->fts_errno);
			break;
		case FTS_ERR:  /* Warn, continue. */
		case FTS_NS:
			fts_status |= warningx(p->fts_path, p->fts_errno);
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
		if (chflags(p->fts_accpath, newflags(p->fts_statp->st_flags)) < 0) {
			fts_status |= warning(p->fts_path);
			continue;
		}
	}
	if (errno) {
		die("fts_read");
	}
	return (fts_status);
}

int
recurse(struct stat *stp, char *dir, int savedir)
{
	register DIR *dirp;
	register struct dirent *dp;
	int ecode;

	if (chdir(dir) < 0) {
		warning(dir);
		return (1);
	}
	if ((dirp = opendir(".")) == NULL) {
		warning(dir);
		return (1);
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	ecode = 0;
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (lstat(dp->d_name, stp) < 0) {
			ecode = warning(dp->d_name);
			if (ecode) {
				break;
			}
			continue;
		}
		if ((stp->st_mode & S_IFMT) == S_IFDIR) {
			ecode = recurse(dp->d_name, dirfd(dirp));
			if (ecode) {
				break;
			}
			continue;
		}
		if ((stp->st_mode & S_IFMT) == S_IFLNK) {
			continue;
		}
		if (chflags(dp->d_name, newflags(stp->st_flags)) < 0
				&& (ecode = warning(dp->d_name))) {
			break;
		}
	}
	/*
	 * Lastly change the flags on the directory we are in before returning to
	 * the previous level.
	 */
	if (fstat(dirfd(dirp), stp) < 0) {
		die("can't fstat .");
	}
	if (fchflags(dirfd(dirp), newflags(stp->st_flags)) < 0) {
		ecode = warning(dir);
	}
	if (fchdir(savedir) < 0) {
		die(fmsg);
	}
	closedir(dirp);
	return (ecode);
}

/* VARARGS1 */
void
die(const char *fmt, ...)
{
    va_list	ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
    exit(1);
}

int
warning(msg)
	char *msg;
{
	return (warningx(msg, errno));
}

int
warningx(msg, errnum)
	char *msg;
	int errnum;
{
	if (!fflag) {
		fprintf(stderr, "chflags: %s: %s\n", msg, strerror(errnum));
	}
	return (!fflag);
}

int
newflags(flags)
	u_short	flags;
{
	if (oct) {
		flags = set;
	} else {
		flags |= set;
		flags &= clear;
	}
	return (flags);
}

void
usage(void)
{
	fputs("usage: chflags [-Rf [-H | -L | -P]] flags file ...\n", stderr);
	exit(1);
}
