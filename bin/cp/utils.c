/*-
 * Copyright (c) 1991, 1993, 1994
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)utils.c	8.3 (Berkeley) 4/1/94";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

int
copy_file(entp, dne)
	FTSENT *entp;
	int dne;
{
	static char buf[MAXBSIZE];
	struct stat to_stat, *fs;
	int ch, checkch, from_fd, rcount, rval, to_fd, wcount;
#ifdef VM_AND_BUFFER_CACHE_SYNCHRONIZED
	char *p;
#endif
	
	if ((from_fd = open(entp->fts_path, O_RDONLY, 0)) == -1) {
		warn("%s", entp->fts_path);
		return (1);
	}

	fs = entp->fts_statp;

	/*
	 * If the file exists and we're interactive, verify with the user.
	 * If the file DNE, set the mode to be the from file, minus setuid
	 * bits, modified by the umask; arguably wrong, but it makes copying
	 * executables work right and it's been that way forever.  (The
	 * other choice is 666 or'ed with the execute bits on the from file
	 * modified by the umask.)
	 */
	if (!dne) {
		if (iflag) {
			(void)fprintf(stderr, "overwrite %s? ", to.p_path);
			checkch = ch = getchar();
			while (ch != '\n' && ch != EOF)
				ch = getchar();
			if (checkch != 'y') {
				(void)close(from_fd);
				return (0);
			}
		}
		to_fd = open(to.p_path, O_WRONLY | O_TRUNC, 0);
	} else
		to_fd = open(to.p_path, O_WRONLY | O_TRUNC | O_CREAT,
		    fs->st_mode & ~(S_ISUID | S_ISGID));

	if (to_fd == -1) {
		warn("%s", to.p_path);
		(void)close(from_fd);
		return (1);;
	}

	rval = 0;

	/*
	 * Mmap and write if less than 8M (the limit is so we don't totally
	 * trash memory on big files.  This is really a minor hack, but it
	 * wins some CPU back.
	 */
#ifdef VM_AND_BUFFER_CACHE_SYNCHRONIZED
	if (fs->st_size <= 8 * 1048576) {
		if ((p = mmap(NULL, (size_t)fs->st_size, PROT_READ,
		    0, from_fd, (off_t)0)) == (char *)-1) {
			warn("%s", entp->fts_path);
			rval = 1;
		} else {
			if (write(to_fd, p, fs->st_size) != fs->st_size) {
				warn("%s", to.p_path);
				rval = 1;
			}
			/* Some systems don't unmap on close(2). */
			if (munmap(p, fs->st_size) < 0) {
				warn("%s", entp->fts_path);
				rval = 1;
			}
		}
	} else
#endif
	{
		while ((rcount = read(from_fd, buf, MAXBSIZE)) > 0) {
			wcount = write(to_fd, buf, rcount);
			if (rcount != wcount || wcount == -1) {
				warn("%s", to.p_path);
				rval = 1;
				break;
			}
		}
		if (rcount < 0) {
			warn("%s", entp->fts_path);
			rval = 1;
		}
	}

	/* If the copy went bad, lose the file. */
	if (rval == 1) {
		(void)unlink(to.p_path);
		(void)close(from_fd);
		(void)close(to_fd);
		return (1);
	}

	if (pflag && setfile(fs, to_fd)) {
		rval = 1;
	/*
	 * If the source was setuid or setgid, lose the bits unless the
	 * copy is owned by the same user and group.
	 */
#define	RETAINBITS \
	(S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO)
	} else if ((fs->st_mode & (S_ISUID | S_ISGID)) && fs->st_uid == myuid) {
		if (fstat(to_fd, &to_stat)) {
			warn("%s", to.p_path);
			rval = 1;
		} else if (fs->st_gid == to_stat.st_gid &&
		    fchmod(to_fd, fs->st_mode & RETAINBITS & ~myumask)) {
			warn("%s", to.p_path);
			rval = 1;
		}
    }
	(void)close(from_fd);
	if (close(to_fd)) {
		warn("%s", to.p_path);
		rval = 1;
	}
	return (rval);
}

int
copy_link(p, exists)
	FTSENT *p;
	int exists;
{
	int len;
	char link[MAXPATHLEN];

	if ((len = readlink(p->fts_path, link, sizeof(link))) == -1) {
		warn("readlink: %s", p->fts_path);
		return (1);
	}
	link[len] = '\0';
	if (exists && unlink(to.p_path)) {
		warn("unlink: %s", to.p_path);
		return (1);
	}
	if (symlink(link, to.p_path)) {
		warn("symlink: %s", link);
		return (1);
	}
	return (0);
}

int
copy_fifo(from_stat, exists)
	struct stat *from_stat;
	int exists;
{
	if (exists && unlink(to.p_path)) {
		warn("unlink: %s", to.p_path);
		return (1);
	}
	if (mkfifo(to.p_path, from_stat->st_mode)) {
		warn("mkfifo: %s", to.p_path);
		return (1);
	}
	return (pflag ? setfile(from_stat, 0) : 0);
}

int
copy_special(from_stat, exists)
	struct stat *from_stat;
	int exists;
{
	if (exists && unlink(to.p_path)) {
		warn("unlink: %s", to.p_path);
		return (1);
	}
	if (mknod(to.p_path, from_stat->st_mode, from_stat->st_rdev)) {
		warn("mknod: %s", to.p_path);
		return (1);
	}
	return (pflag ? setfile(from_stat, 0) : 0);
}


int
setfile(fs, fd)
	register struct stat *fs;
	int fd;
{
	static struct timeval tv[2];
	int rval;

	rval = 0;
	fs->st_mode &= S_ISUID | S_ISGID | S_IRWXU | S_IRWXG | S_IRWXO;

	TIMESPEC_TO_TIMEVAL(&tv[0], &fs->st_atimespec);
	TIMESPEC_TO_TIMEVAL(&tv[1], &fs->st_mtimespec);
	if (utimes(to.p_path, tv)) {
		warn("utimes: %s", to.p_path);
		rval = 1;
	}
	/*
	 * Changing the ownership probably won't succeed, unless we're root
	 * or POSIX_CHOWN_RESTRICTED is not set.  Set uid/gid before setting
	 * the mode; current BSD behavior is to remove all setuid bits on
	 * chown.  If chown fails, lose setuid/setgid bits.
	 */
	if (fd ? fchown(fd, fs->st_uid, fs->st_gid) :
	    chown(to.p_path, fs->st_uid, fs->st_gid)) {
		if (errno != EPERM) {
			warn("chown: %s", to.p_path);
			rval = 1;
		}
		fs->st_mode &= ~(S_ISUID | S_ISGID);
	}
	if (fd ? fchmod(fd, fs->st_mode) : chmod(to.p_path, fs->st_mode)) {
		warn("chown: %s", to.p_path);
		rval = 1;
	}

	if (fd ?
	    fchflags(fd, fs->st_flags) : chflags(to.p_path, fs->st_flags)) {
		warn("chflags: %s", to.p_path);
		rval = 1;
	}
	return (rval);
}

void
usage()
{
	(void)fprintf(stderr, "%s\n%s\n",
"usage: cp [-R [-H | -L | -P] [-fip] src target",
"       cp [-R [-H | -L | -P] [-fip] src1 ... srcN directory");
	exit(1);
}
