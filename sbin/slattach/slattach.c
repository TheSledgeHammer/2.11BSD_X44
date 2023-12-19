/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Adams.
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
"@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)slattach.c	8.2 (Berkeley) 1/7/94";
#endif /* not lint */

#include <sys/param.h>
#include <sgtty.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <net/if.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define DEFAULT_BAUD	9600
int	slipdisc = SLIPDISC;

char	devname[32];
char	hostname[MAXHOSTNAMELEN];

static int findspeed(int);
static void	usage(void);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	register int fd;
	register char *dev = argv[1];
	struct sgttyb sgtty;
	struct termios tty;
	int ch, speed;
	sigset_t nsigset;
	int opt_detach = 1;

	while ((ch = getopt(argc, argv, "hHlmns:")) != -1) {
		switch (ch) {
		case 'h':
			cflag |= CRTSCTS;
			break;
		case 'H':
			cflag |= CDTRCTS;
			break;
		case 'l':
			cflag |= CLOCAL;
			break;
		case 'm':
			cflag &= ~HUPCL;
			break;
		case 'n':
			opt_detach = 0;
			break;
		case 's':
			speed = atoi(optarg) ? findspeed(atoi(optarg)) : findspeed(DEFAULT_BAUD);
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage();
	}

	if (speed == 0) {
		fprintf(stderr, "unknown speed %s", argv[2]);
		exit(1);
	}

	dev = *argv;
	if (strncmp(_PATH_DEV, dev, sizeof(_PATH_DEV) - 1)) {
		(void)snprintf(devname, sizeof(devname), "%s%s", _PATH_DEV, dev);
		dev = devname;
	}
	if ((fd = open(dev, O_RDWR | O_NDELAY)) < 0) {
		perror(dev);
		exit(1);
	}

	sgtty.sg_flags = RAW | ANYP;
	sgtty.sg_ispeed = speed;
	sgtty.sg_ospeed = speed;
	tty.c_cflag = CREAD | CS8 | cflag;
	tty.c_iflag = 0;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
	cfsetspeed(&tty, speed);
	if (tcsetattr(fd, TCSADRAIN, &tty) < 0) {
		perror("tcsetattr");
		exit(1);
	}
	if (ioctl(fd, TIOCSDTR, 0) < 0 && errno != ENOTTY) {
		perror("TIOCSDTR");
		exit(1);
	}
	if (ioctl(fd, TIOCSETP, &sgtty) < 0) {
		perror("ioctl(TIOCSETP)");
		exit(1);
	}
	if (ioctl(fd, TIOCSETD, &slipdisc) < 0) {
		perror("ioctl(TIOCSETD)");
		exit(1);
	}
	if (opt_detach && daemon(0, 0) != 0) {
		perror("couldn't detach");
		exit(1);
	}
	sigemptyset(&nsigset);
	for (;;) {
		sigsuspend(&nsigset);
	}
}

struct sg_spds {
	int sp_val, sp_name;
}       spds[] = {
#ifdef B50
	{ 50, B50 },
#endif
#ifdef B75
	{ 75, B75 },
#endif
#ifdef B110
	{ 110, B110 },
#endif
#ifdef B150
	{ 150, B150 },
#endif
#ifdef B200
	{ 200, B200 },
#endif
#ifdef B300
	{ 300, B300 },
#endif
#ifdef B600
	{ 600, B600 },
#endif
#ifdef B1200
	{ 1200, B1200 },
#endif
#ifdef B1800
	{ 1800, B1800 },
#endif
#ifdef B2000
	{ 2000, B2000 },
#endif
#ifdef B2400
	{ 2400, B2400 },
#endif
#ifdef B3600
	{ 3600, B3600 },
#endif
#ifdef B4800
	{ 4800, B4800 },
#endif
#ifdef B7200
	{ 7200, B7200 },
#endif
#ifdef B9600
	{ 9600, B9600 },
#endif
#ifdef EXTA
	{ 19200, EXTA },
#endif
#ifdef EXTB
	{ 38400, EXTB },
#endif
	{ 0, 0 }
};

static int
findspeed(speed)
	register int speed;
{
	register struct sg_spds *sp;

	sp = spds;
	while (sp->sp_val && sp->sp_val != speed) {
		sp++;
	}
	return (sp->sp_name);
}


static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-hHlmn] [-s baudrate] ttyname\n", getprogname());
	exit(1);
}
