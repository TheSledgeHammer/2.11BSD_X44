/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kevin Fall.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if !defined(lint)
#if 0
static char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif
#endif /* not lint */
#if !defined(lint)
#if 0
static char sccsid[] = "@(#)cat.c	8.2 (Berkeley) 4/27/95";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int bflag, eflag, fflag, lflag, nflag, sflag, tflag, vflag;
size_t bsize;
int rval;
const char *filename;

int  main(int argc, char *argv[]);
void cook_args(char *argv[]);
void cook_buf(FILE *);
void raw_args(char *argv[]);
void raw_cat(int);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	struct flock stdout_lock;
	int ch;

	while ((ch = getopt(argc, argv, "B:beflnstuv")) != -1)
		switch (ch) {
		case 'B':
			bsize = (size_t) strtol(optarg, NULL, 0);
			break;
		case 'b':
			bflag = nflag = 1; /* -b implies -n */
			break;
		case 'e':
			eflag = vflag = 1; /* -e implies -v */
			break;
		case 'f':
			fflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		case 'n':
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case 't':
			tflag = vflag = 1; /* -t implies -v */
			break;
		case 'u':
			setbuf(stdout, (char*) NULL);
			break;
		case 'v':
			vflag = 1;
			break;
		default:
		case '?':
			(void) fprintf(stderr,
					"usage: cat [-beflnstuv] [-B bsize] [-] [file ...]\n");
			exit(1);
		}
	argv += optind;

	if (lflag) {
		stdout_lock.l_len = 0;
		stdout_lock.l_start = 0;
		stdout_lock.l_type = F_WRLCK;
		stdout_lock.l_whence = SEEK_SET;
		if (fcntl(STDOUT_FILENO, F_SETLKW, &stdout_lock) == -1) {
			err(EXIT_FAILURE, "stdout");
		}
	}

	if (bflag || eflag || nflag || sflag || tflag || vflag)
		cook_args(argv);
	else
		raw_args(argv);
	if (fclose(stdout))
		err(1, "stdout");
	exit(rval);
}

void
cook_args(argv)
	char **argv;
{
	register FILE *fp;

	fp = stdin;
	filename = "stdin";
	do {
		if (*argv) {
			if (!strcmp(*argv, "-"))
				fp = stdin;
			else if ((fp = fopen(*argv, "r")) == NULL) {
				warn("%s", *argv);
				++argv;
				continue;
			}
			filename = *argv++;
		}
		cook_buf(fp);
		if (fp != stdin)
			(void)fclose(fp);
	} while (*argv);
}

void
cook_buf(fp)
	register FILE *fp;
{
	register int ch, gobble, line, prev;

	line = gobble = 0;
	for (prev = '\n'; (ch = getc(fp)) != EOF; prev = ch) {
		if (prev == '\n') {
			if (ch == '\n') {
				if (sflag) {
					if (!gobble && putchar(ch) == EOF)
						break;
					gobble = 1;
					continue;
				}
				if (nflag && !bflag) {
					(void)fprintf(stdout, "%6d\t", ++line);
					if (ferror(stdout))
						break;
				}
			} else if (nflag) {
				(void)fprintf(stdout, "%6d\t", ++line);
				if (ferror(stdout))
					break;
			}
		}
		gobble = 0;
		if (ch == '\n') {
			if (eflag)
				if (putchar('$') == EOF)
					break;
		} else if (ch == '\t') {
			if (tflag) {
				if (putchar('^') == EOF || putchar('I') == EOF)
					break;
				continue;
			}
		} else if (vflag) {
			if (!isascii(ch)) {
				if (putchar('M') == EOF || putchar('-') == EOF)
					break;
				ch = toascii(ch);
			}
			if (iscntrl(ch)) {
				if (putchar('^') == EOF ||
				    putchar(ch == '\177' ? '?' :
				    ch | 0100) == EOF)
					break;
				continue;
			}
		}
		if (putchar(ch) == EOF)
			break;
	}
	if (ferror(fp)) {
		warn("%s", filename);
		clearerr(fp);
	}
	if (ferror(stdout))
		err(1, "stdout");
}

void
raw_args(argv)
	char **argv;
{
	register int fd;

	fd = fileno(stdin);
	filename = "stdin";
	do {
		if (*argv) {
			if (!strcmp(*argv, "-")) {
				fd = fileno(stdin);
			} else if (fflag) {
				struct stat st;
				fd = open(*argv, O_RDONLY|O_NONBLOCK, 0);
				if (fd < 0)
					goto skip;

				if (fstat(fd, &st) == -1) {
					close(fd);
					goto skip;
				}
				if (!S_ISREG(st.st_mode)) {
					close(fd);
					warnx("%s: not a regular file", *argv);
					goto skipnomsg;
				}
			} else if ((fd = open(*argv, O_RDONLY, 0)) < 0) {
skip:
				warn("%s", *argv);
skipnomsg:
				rval = EXIT_FAILURE;
				++argv;
				continue;
			}
			filename = *argv++;
		} else if (fd < 0) {
			err(EXIT_FAILURE, "stdin");
		}
		raw_cat(fd);
		if (fd != fileno(stdin))
			(void)close(fd);
	} while (*argv);
}

void
raw_cat(rfd)
	register int rfd;
{
	register int nr, nw, off, wfd;
	static char *buf, fb_buf[BUFSIZ];
	struct stat sbuf;

	wfd = fileno(stdout);
	if (buf == NULL) {
		if (fstat(wfd, &sbuf))
			err(1, "%s", filename);
		bsize = MAX(sbuf.st_blksize, 1024);
		if ((buf = malloc((u_int)bsize)) == NULL) {
			err(1, NULL);
		}
		if (buf == NULL) {
			bsize = sizeof(fb_buf);
			buf = fb_buf;
		}
	}
	while ((nr = read(rfd, buf, bsize)) > 0)
		for (off = 0; nr; nr -= nw, off += nw)
			if ((nw = write(wfd, buf + off, nr)) < 0)
				err(1, "stdout");
	if (nr < 0)
		warn("%s", filename);
}
