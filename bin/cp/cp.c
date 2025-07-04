/*
 * Copyright (c) 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * David Hitz of Auspex Systems Inc.
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char copyright[] =
"@(#) Copyright (c) 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)cp.c	8.5 (Berkeley) 4/29/95";
#endif
#endif /* not lint */

/*
 * Cp copies source files to target files.
 * 
 * The global PATH_T structure "to" always contains the path to the
 * current target file.  Since fts(3) does not change directories,
 * this path can be either absolute or dot-relative.
 * 
 * The basic algorithm is to initialize "to" and use fts(3) to traverse
 * the file hierarchy rooted in the argument list.  A trivial case is the
 * case of 'cp file1 file2'.  The more interesting case is the case of
 * 'cp file1 file2 ... fileN dir' where the hierarchy is traversed and the
 * path (relative to the root of the traversal) is appended to dir (stored
 * in "to") to form the final target path.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#define	STRIP_TRAILING_SLASH(p) {					\
        while ((p).p_end > (p).p_path && (p).p_end[-1] == '/')		\
                *--(p).p_end = '\0';					\
}

static char empty[] = "";
PATH_T to = { .p_end = to.p_path, .target_end = empty };

uid_t myuid;
int Hflag, Lflag, Pflag, Rflag, iflag, pflag, rflag;
int myumask;

enum op { FILE_TO_FILE, FILE_TO_DIR, DIR_TO_DNE };

int copy(char *[], enum op, int);
int mastercmp(const FTSENT **, const FTSENT **);

int
main(int argc, char *argv[])
{
	struct stat to_stat, tmp_stat;
	enum op type;
	int ch, fts_options, r;
	char *target;

	Hflag = Lflag = Pflag = Rflag = 0;
	while ((ch = getopt(argc, argv, "HLPRfipr")) != EOF)
		switch (ch) {
		case 'H':
			Hflag = 1;
			Lflag = 0;
			Pflag = 0;
			break;
		case 'L':
			Lflag = 1;
			Hflag = 0;
			Pflag = 0;
			break;
		case 'P':
			Pflag = 1;
			Hflag = 0;
			Lflag = 0;
			break;
		case 'R':
			Rflag = 1;
			break;
		case 'f':
			iflag = 0;
			break;
		case 'i':
			iflag = isatty(fileno(stdin));
			break;
		case 'p':
			pflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case '?':
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	fts_options = FTS_NOCHDIR | FTS_PHYSICAL;
	if (rflag) {
		if (Rflag)
			errx(1,
		    "the -R and -r options may not be specified together.");
		if (Hflag || Lflag || Pflag)
			errx(1,
	"the -H, -L, and -P options may not be specified with the -r option.");
		fts_options &= ~FTS_PHYSICAL;
		fts_options |= FTS_LOGICAL;
	}
	if (Rflag) {
		if (Hflag)
			fts_options |= FTS_COMFOLLOW;
		if (Lflag) {
			fts_options &= ~FTS_PHYSICAL;
			fts_options |= FTS_LOGICAL;
		}
	} else {
		fts_options &= ~FTS_PHYSICAL;
		fts_options |= FTS_LOGICAL;
	}

	myuid = getuid();

	/* Copy the umask for explicit mode setting. */
	myumask = umask(0);
	(void)umask(myumask);

	/* Save the target base in "to". */
	target = argv[--argc];
	if (strlen(target) > MAXPATHLEN)
		errx(1, "%s: name too long", target);
	(void) strcpy(to.p_path, target);
	to.p_end = to.p_path + strlen(to.p_path);
	if (to.p_path == to.p_end) {
		*to.p_end++ = '.';
		*to.p_end = 0;
	}
	STRIP_TRAILING_SLASH(to);
	to.target_end = to.p_end;

	/* Set end of argument list for fts(3). */
	argv[argc] = NULL;     
	
	/*
	 * Cp has two distinct cases:
	 *
	 * cp [-R] source target
	 * cp [-R] source1 ... sourceN directory
	 *
	 * In both cases, source can be either a file or a directory.
	 *
	 * In (1), the target becomes a copy of the source. That is, if the
	 * source is a file, the target will be a file, and likewise for
	 * directories.
	 *
	 * In (2), the real target is not directory, but "directory/source".
	 */
	r = stat(to.p_path, &to_stat);
	if (r == -1 && errno != ENOENT)
		err(1, "%s", to.p_path);
	if (r == -1 || !S_ISDIR(to_stat.st_mode)) {
		/*
		 * Case (1).  Target is not a directory.
		 */
		if (argc > 1) {
			usage();
			exit(1);
		}
		/*
		 * Need to detect the case:
		 *	cp -R dir foo
		 * Where dir is a directory and foo does not exist, where
		 * we want pathname concatenations turned on but not for
		 * the initial mkdir().
		 */
		if (r == -1) {
			if (rflag || (Rflag && (Lflag || Hflag)))
				stat(*argv, &tmp_stat);
			else
				lstat(*argv, &tmp_stat);

			if (S_ISDIR(tmp_stat.st_mode) && (Rflag || rflag))
				type = DIR_TO_DNE;
			else
				type = FILE_TO_FILE;
		} else
			type = FILE_TO_FILE;
	} else
		/*
		 * Case (2).  Target is a directory.
		 */
		type = FILE_TO_DIR;

	exit(copy(argv, type, fts_options));
}

int
copy(argv, type, fts_options)
	char *argv[];
	enum op type;
	int fts_options;
{
	struct stat to_stat;
	FTS *ftsp;
	FTSENT *curr;
	int base, dne, nlen, rval, sval;
	char *p;

    	base = 0;
	if ((ftsp = fts_open(argv, fts_options, mastercmp)) == NULL)
		err(1, NULL);
	for (rval = 0; (curr = fts_read(ftsp)) != NULL;) {
		switch (curr->fts_info) {
		case FTS_NS:
		case FTS_ERR:
			warnx("%s: %s",
			    curr->fts_path, strerror(curr->fts_errno));
			rval = 1;
			continue;
		case FTS_DC:			/* Warn, continue. */
			warnx("%s: directory causes a cycle", curr->fts_path);
			rval = 1;
			continue;
		case FTS_DP:			/* Ignore, continue. */
			continue;
		}

		/*
		 * If we are in case (2) or (3) above, we need to append the 
                 * source name to the target name.  
                 */
		if (type != FILE_TO_FILE) {
			if ((curr->fts_namelen +
			    to.target_end - to.p_path + 1) > MAXPATHLEN) {
				warnx("%s/%s: name too long (not copied)", 
				    to.p_path, curr->fts_name);
				rval = 1;
				continue;
			}

			/*
			 * Need to remember the roots of traversals to create
			 * correct pathnames.  If there's a directory being
			 * copied to a non-existent directory, e.g.
			 *	cp -R a/dir noexist
			 * the resulting path name should be noexist/foo, not
			 * noexist/dir/foo (where foo is a file in dir), which
			 * is the case where the target exists.
			 *
			 * Also, check for "..".  This is for correct path
			 * concatentation for paths ending in "..", e.g.
			 *	cp -R .. /tmp
			 * Paths ending in ".." are changed to ".".  This is
			 * tricky, but seems the easiest way to fix the problem.
			 *
			 * XXX
			 * Since the first level MUST be FTS_ROOTLEVEL, base
			 * is always initialized.
			 */
			if (curr->fts_level == FTS_ROOTLEVEL) {
				if (type != DIR_TO_DNE) {
					p = strrchr(curr->fts_path, '/');
					base = (p == NULL) ? 0 : 
					    (int)(p - curr->fts_path + 1);

					if (!strcmp(&curr->fts_path[base], 
					    ".."))
						base += 1;
				} else {
					base = curr->fts_pathlen;
                }
            }
			if (to.target_end[-1] != '/') {
				*to.target_end = '/';
				*(to.target_end + 1) = 0;
			}
			p = &curr->fts_path[base];
			nlen = curr->fts_pathlen - base;

			(void)strncat(to.target_end + 1, p, nlen);
			to.p_end = to.target_end + nlen + 1;
			*to.p_end = 0;
			STRIP_TRAILING_SLASH(to);
		}

		sval = Pflag ? lstat(to.p_path, &to_stat) : stat(to.p_path, &to_stat);
		/* Not an error but need to remember it happened */
		if (sval == -1)
			dne = 1;
		else {
			if (to_stat.st_dev == curr->fts_statp->st_dev &&
			    to_stat.st_ino == curr->fts_statp->st_ino) {
				warnx("%s and %s are identical (not copied).",
				    to.p_path, curr->fts_path);
				rval = 1;
				if (S_ISDIR(curr->fts_statp->st_mode))
					(void)fts_set(ftsp, curr, FTS_SKIP);
				continue;
			}
			if (!S_ISDIR(curr->fts_statp->st_mode) &&
			    S_ISDIR(to_stat.st_mode)) {
		warnx("cannot overwrite directory %s with non-directory %s",
				    to.p_path, curr->fts_path);
				rval = 1;
				continue;
			}
			dne = 0;
		}

		switch (curr->fts_statp->st_mode & S_IFMT) {
		case S_IFLNK:
			if (copy_link(curr, !dne))
				rval = 1;
			break;
		case S_IFDIR:
			if (!Rflag && !rflag) {
				warnx("%s is a directory (not copied).",
				    curr->fts_path);
				(void)fts_set(ftsp, curr, FTS_SKIP);
				rval = 1;
				break;
			}
			/*
			 * If the directory doesn't exist, create the new
			 * one with the from file mode plus owner RWX bits,
			 * modified by the umask.  Trade-off between being
			 * able to write the directory (if from directory is
			 * 555) and not causing a permissions race.  If the
			 * umask blocks owner writes, we fail..
			 */
			if (dne) {
				if (mkdir(to.p_path,
				    curr->fts_statp->st_mode | S_IRWXU) < 0)
					err(1, "%s", to.p_path);
			} else if (!S_ISDIR(to_stat.st_mode)) {
				errno = ENOTDIR;
				err(1, "%s", to.p_path);
			}
			/*
			 * If not -p and directory didn't exist, set it to be
			 * the same as the from directory, umodified by the 
                         * umask; arguably wrong, but it's been that way
                         * forever.
			 */
			if (pflag && setfile(curr->fts_statp, 0))
				rval = 1;
			else if (dne)
				(void)chmod(to.p_path,
				    curr->fts_statp->st_mode);
			break;
		case S_IFBLK:
		case S_IFCHR:
			if (Rflag) {
				if (copy_special(curr->fts_statp, !dne))
					rval = 1;
			} else
				if (copy_file(curr, dne))
					rval = 1;
			break;
		case S_IFIFO:
			if (Rflag) {
				if (copy_fifo(curr->fts_statp, !dne))
					rval = 1;
			} else
				if (copy_file(curr, dne))
					rval = 1;
			break;
		default:
			if (copy_file(curr, dne))
				rval = 1;
			break;
		}
	}
	if (errno)
		err(1, "fts_read");
	return (rval);
}

/*
 * mastercmp --
 *	The comparison function for the copy order.  The order is to copy
 *	non-directory files before directory files.  The reason for this
 *	is because files tend to be in the same cylinder group as their
 *	parent directory, whereas directories tend not to be.  Copying the
 *	files first reduces seeking.
 */
int
mastercmp(a, b)
	const FTSENT **a, **b;
{
	int a_info, b_info;

	a_info = (*a)->fts_info;
	if (a_info == FTS_ERR || a_info == FTS_NS || a_info == FTS_DNR)
		return (0);
	b_info = (*b)->fts_info;
	if (b_info == FTS_ERR || b_info == FTS_NS || b_info == FTS_DNR)
		return (0);
	if (a_info == FTS_D)
		return (-1);
	if (b_info == FTS_D)
		return (1);
	return (0);
}
