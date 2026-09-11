/*-
 * Copyright (c) 1993
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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ls.c	8.1 (Berkeley) 6/11/93";
#endif /* not lint */

#include <sys/param.h>
//#include <sys/ttychars.h>

//#include <libsa/ufs.h>
#include <lib/libsa/stand.h>

void
lsfd(int *fdp, const char *path)
{
	struct stat sb;
	struct open_file *f;
	int fd;
	size_t size;
	const char *fname = 0;
	char *p;

	if ((fd = open(path, 0)) < 0
	    || fstat(fd, &sb) < 0
	    || (sb.st_mode & S_IFMT) != S_IFDIR) {
		/* Path supplied isn't a directory, open parent
		   directory and list matching files. */
		if (fd >= 0) {
			close(fd);
		}
		fname = strrchr(path, '/');
		if (fname) {
			size = fname - path;
			fname++;
			p = alloc(size + 1);
			if (!p)
				goto out;
			memcpy(p, path, size);
			p[size] = 0;
			fd = open(p, 0);
			free(p, size + 1);
		} else {
			fd = open("", 0);
			fname = path;
		}

		if (fd < 0) {
			printf("ls: %s\n", strerror(errno));
			*fdp = fd;
			return;
		}
		if (fstat(fd, &sb) < 0) {
			printf("stat: %s\n", strerror(errno));
			goto out;
		}
		if ((sb.st_mode & S_IFMT) != S_IFDIR) {
			printf("%s: %s\n", path, strerror(ENOTDIR));
			goto out;
		}
	}
	f = &files[fd];

out:
	close(fd);
	*fdp = fd;
}

void
ls(const char *path)
{
	int fd;

	lsfd(&fd, path);
}

#define CTRL(x)	(x&037)

int
getfile(char *prompt, int mode)
{
	int fd;
	char buf[100];

	do {
		printf("%s: ", prompt);
		gets(buf);
		if (buf[0] == CTRL('d') && buf[1] == 0) {
			return (-1);
		}
	} while ((fd = open(buf, mode)) < 0);
	return (fd);
}

#ifdef notyet

static void ls2(int);
static void ls_read(int, char *, size_t);

main()
{
	struct dinode *ip;
	int fd;

	for (;;) {
		fd = getfile("ls", 0);
		if (fd == -1)
			exit();
		ip = &iob[fd - 3].i_ino;
		if ((ip->di_mode & IFMT) != IFDIR) {
			printf("ls: not a directory\n");
			continue;
		}
		if (ip->di_size == 0) {
			printf("ls: zero length directory\n");
			continue;
		}
		ls2(fd);
	}
}

typedef struct dirent DP;

static void
ls2(int fd)
{
	register int size;
	register char *dp;
	char dirbuf[DIRBLKSIZ];

	printf("\ninode\tname\n");
	ls_read(fd, dirbuf, DIRBLKSIZ);
}

static void
ls_read(int fd, char *dest, size_t bcount)
{
	register int size;
	register char *dp;

	while ((size = read(fd, dest, bcount)) == bcount) {
		for (dp = dest; (dp < (dest + size)) &&
		    (dp + ((DP *)dp)->d_reclen) < (dest + size);
		    dp += ((DP *)dp)->d_reclen) {
			if (((DP *)dp)->d_ino == 0)
				continue;
			if (((DP *)dp)->d_namlen > MAXNAMLEN + 1) {
				printf("Corrupt file name length!  Run fsck soon!\n");
				return;
			}
			printf("%d\t%s\n", ((DP *)dp)->d_ino, ((DP *)dp)->d_name);
		}
	}
}

#endif
