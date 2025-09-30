/*
 * Copyright (c) 2000  Peter Wemm <peter@freebsd.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD$"); */

#include <sys/types.h>
#include <sys/sysctl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include <kenv.h>
#include <unistd.h>

static void	usage(void);
static int	kdumpenv(void);
static int	kgetenv(char *);
static int	ksetenv(char *, char *);
static int	kunsetenv(char *);

static int hflag = 0;
static int uflag = 0;

static void
usage(void)
{
	(void)fprintf(stderr, "%s\n%s\n%s\n",
	    "usage: kenv [-h]",
	    "       kenv variable[=value]",
	    "       kenv -u variable");
	exit(1);
}

int
main(int argc, char **argv)
{
	char *env, *eq, *val;
	int ch, error;

	error = 0;
	val = NULL;
	env = NULL;
	while ((ch = getopt(argc, argv, "hu")) != -1) {
		switch (ch) {
		case 'h':
			hflag++;
			break;
		case 'u':
			uflag++;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc > 0) {
		env = argv[0];
		eq = strchr(env, '=');
		if (eq != NULL) {
			*eq++ = '\0';
			val = eq;
		}
		argv++;
		argc--;
	}
	if (hflag && (env != NULL))
		usage();
	if ((argc > 0) || (uflag && (env == NULL)))
		usage();
	if (env == NULL)
		kdumpenv();
	else if (val == NULL) {
		if (uflag) {
			error = kunsetenv(env);
			if (error)
				warnx("unable to unset %s", env);
		} else {
			error = kgetenv(env);
			if (error)
				warnx("unable to get %s", env);
		}
	} else {
		error = ksetenv(env, val);
		if (error)
			warnx("unable to set %s to %s", env, val);
	}
	return (error);
}

static int
kdumpenv(void)
{
	char *buf, *cp;
	int len;

	len = kenv(KENV_DUMP, NULL, NULL, 0);
	len = len * 120 / 100;
	buf = malloc(len);
	if (buf == NULL)
		return (-1);
	/* Be defensive */
	memset(buf, 0, len);
	kenv(KENV_DUMP, NULL, buf, len);
	for (; *buf != '\0'; buf += strlen(buf) + 1) {
		if (hflag) {
			if (strncmp(buf, "hint.", 5) != 0)
				continue;
		}
		cp = strchr(buf, '=');
		if (cp == NULL)
			continue;
		*cp++ = '\0';
		printf("%s=\"%s\"\n", buf, cp);
		buf = cp;
	}
	return (0);
}

static int
kgetenv(char *env)
{
	char buf[1024];
	int ret;

	ret = kenv(KENV_GET, env, buf, sizeof(buf));
	if (ret == -1)
		return (ret);
	printf("%s\n", buf);
	return (0);
}

static int
ksetenv(char *env, char *val)
{
	int ret;

	ret = kenv(KENV_SET, env, val, strlen(val)+1);
	if (ret == 0)
		printf("%s=\"%s\"\n", env, val);
	return (ret);
}

static int
kunsetenv(char *env)
{
	int ret;
	
	ret = kenv(KENV_UNSET, env, NULL, 0);
	return (ret);
}
