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

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>

static void
usage()
{
	errx(1, "usage: mknod name [b | c] major minor [owner:group]\n");
}

static u_long
id(name, type)
	char *name, *type;
{
	u_long val;
	char *ep;

	/*
	 * XXX
	 * We know that uid_t's and gid_t's are unsigned longs.
	 */
	errno = 0;
	val = strtoul(name, &ep, 10);
	if (errno) {
		err(1, "%s", name);
	}
	if (*ep != '\0') {
		errx(1, "%s: illegal %s name", name, type);
	}
	return (val);
}

static gid_t
a_gid(s)
	char *s;
{
	struct group *gr;
	gid_t gid;

	if (*s == '\0')	{
		errx(1, "group must be specified when the owner is");
	}

	gr = getgrnam(s);
	gid = ((gr == NULL) ? id(s, "group") : gr->gr_gid);
	return (gid);
}

static uid_t
a_uid(s)
	char *s;
{
	struct passwd *pw;
	uid_t uid;

	if (*s == '\0')	{
		errx(1, "owner must be specified when the group is");
	}

	pw = getpwnam(s);
	uid = ((pw == NULL) ? id(s, "user") : pw->pw_uid);
	return (uid);
}

static dev_t
mkdevice(argc, argv)
	int argc;
	char **argv;
{
	dev_t dev;
	char *endp;
	unsigned long major, minor;

	if (argc < 4) {
		usage();
	}

	major = strtoul(argv[2], &endp, 0);
	if (endp == argv[2] || *endp != '\0') {
		errx(1, "invalid major number '%s'", argv[2]);
	}
	if (errno == ERANGE && major == ULONG_MAX) {
		errx(1, "major number too large: '%s'", argv[2]);
	}
	errno = 0;
	minor = strtoul(argv[3], &endp, 0);
	if (endp == argv[3] || *endp != '\0') {
		errx(1, "invalid minor number '%s'", argv[3]);
	}
	if (errno == ERANGE && minor == ULONG_MAX) {
		errx(1, "minor number too large: '%s'", argv[3]);
	}

	dev = makedev(major, minor);
	if (major(dev) != major || minor(dev) != minor) {
		errx(1, "major or minor number too large (%lu %lu)", major, minor);
	}
	return (dev);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	dev_t dev;
	mode_t mode;
	uid_t uid;
	gid_t gid;
	char *cp;
	int error;

	if (argc != 5 && argc != 6) {
		usage();
	}

	mode = 0666;
	if (argv[0][1] != '\0') {
		goto badtype;
	}
	switch (*argv[0]) {
	case 'c':
		mode |= S_IFCHR;
		break;

	case 'b':
		mode |= S_IFBLK;
		break;

	default:
 badtype:
		errx(1, "node type must be 'b', 'c'.");
	}

	dev = mkdevice(argc, argv);
	uid = -1;
	gid = -1;
	if (6 == argc) {
		cp = strchr(argv[5], ':');
		/* have owner:group */
		if (cp != NULL) {
			*cp++ = '\0';
			gid = a_gid(cp);
		} else {
			usage();
			uid = a_uid(argv[5]);
		}
	}

	error = mknod(argv[1], mode, dev);
	if (error != 0) {
		err(1, "%s", argv[1]);
	}
	if (6 == argc) {
		if (chown(argv[1], uid, gid)) {
			err(1, "setting ownership on %s", argv[1]);
		}
	}
	exit(0);
}
