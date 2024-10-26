/*
 * Copyright (c) 1988, 1993
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)ttyname.c	5.2 (Berkeley) 3/9/86";
static char sccsid[] = "@(#)ttyname.c	8.2 (Berkeley) 1/27/94";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

/*
 * ttyname(f): return "/dev/ttyXX" which the the name of the
 * tty belonging to file f.
 *  NULL if it is not a tty
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <dirent.h>
#include <db.h>
#include <string.h>
#include <paths.h>
#include <unistd.h>

#if defined(USE_SGTTY) && (USE_SGTTY == 0)
#include <sgtty.h>
#else
#include <sys/termios.h>
#endif

#ifdef __weak_alias
__weak_alias(ttyname,_ttyname)
#endif

static char buf[sizeof(_PATH_DEV) + MAXNAMLEN] = _PATH_DEV;
static char *oldttyname(int);

#if defined(USE_SGTTY) && (USE_SGTTY == 0)
static int ttyname_sgtty(int);
#else
static int ttyname_termios(int);
#endif

char *
ttyname(fd)
	int fd;
{
	struct stat sb;
	DB *db;
	DBT data, key;
	struct {
		mode_t type;
		dev_t dev;
	} bkey;

	/* Must be a terminal. */
#if defined(USE_SGTTY) && (USE_SGTTY == 0)
	if (ttyname_sgtty(fd) < 0) {
		return (NULL);
	}
#else
	if (ttyname_termios(fd) < 0) {
		return (NULL);
	}
#endif
	/* Must be a character device. */
	if (fstat(fd, &sb) || !S_ISCHR(sb.st_mode)) {
		return (NULL);
	}
	if ((db = dbopen(_PATH_DEVDB, O_RDONLY, 0, DB_HASH, NULL))) {
		memset(&bkey, 0, sizeof(bkey));
		bkey.type = S_IFCHR;
		bkey.dev = sb.st_rdev;
		key.data = &bkey;
		key.size = sizeof(bkey);
		if (!(db->get)(db, &key, &data, 0)) {
			bcopy(data.data, buf + sizeof(_PATH_DEV) - 1, data.size);
			(void)(db->close)(db);
			return (buf);
		}
		(void)(db->close)(db);
	}
	return (oldttyname(fd));
}

static char *
oldttyname(f)
	int f;
{
	struct stat fsb;
	struct stat tsb;
	register struct dirent *db;
	register DIR *df;
	static char rbuf[sizeof(_PATH_DEV) + MAXNAMLEN];

	if (isatty(f) == 0)
		return (NULL);
	if (fstat(f, &fsb) < 0)
		return (NULL);
	if ((fsb.st_mode & S_IFMT) != S_IFCHR)
		return (NULL);
	if ((df = opendir(_PATH_DEV)) == NULL)
		return (NULL);
	while ((db = readdir(df)) != NULL) {
		if (db->d_ino != fsb.st_ino)
			continue;
		strncpy(rbuf + sizeof(_PATH_DEV) - 1, _PATH_DEV, db->d_namlen + 1);
		strcat(rbuf, db->d_name);
		if (stat(rbuf, &tsb) < 0)
			continue;
		if (tsb.st_dev == fsb.st_dev && tsb.st_ino == fsb.st_ino) {
			closedir(df);
			return (rbuf);
		}
	}
	closedir(df);
	return (NULL);
}

#if defined(USE_SGTTY) && (USE_SGTTY == 0)
static int
ttyname_sgtty(fd)
	int fd;
{
	struct sgttyb ttyb;

	if (ioctl(fd, TIOCGETP, &ttyb) < 0) {
		return (-1);
	}
	return (0);
}

#else

static int
ttyname_termios(fd)
	int fd;
{
	struct termios term;

	if (tcgetattr(fd, &term) < 0) {
		return (-1);
	}
	return (0);
}
#endif
