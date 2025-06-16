/*
 * Copyright (c) 1980, 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1980, 1983 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)mkpasswd.c	5.4.1 (2.11BSD) 1996/1/12";
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <ndbm.h>
#include <pwd.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

static FILE *_pw_fp;
static struct passwd _pw_passwd;
static off_t offset;

#define	MAXLINELENGTH	256
static char line[MAXLINELENGTH];

static int  compact(struct passwd *, FILE *, char *, DBM *, datum, datum, char *, char *, FILE *, int);
static int  storekeybyname(struct passwd *, DBM *, datum, datum, char *);
static int  storekeybyuid(struct passwd *, DBM *, datum, datum, char *);
static int  storekeybykeynum(struct passwd *, DBM *, datum, datum, char *);
static void rmall(char *);
static int  scanpw(struct passwd *, FILE *, char *);
static void usage(void);

/*
 * Mkpasswd does two things -- use the ``arg'' file to create ``arg''.{pag,dir}
 * for ndbm, and, if the -p flag is on, create a password file in the original
 * format.  It doesn't use the getpwent(3) routines because it has to figure
 * out offsets for the encrypted passwords to put in the dbm files.  One other
 * problem is that, since the addition of shadow passwords, getpwent(3) has to
 * use the dbm databases rather than simply scanning the actual file.  This
 * required the addition of a flag field to the dbm database to distinguish
 * between a record keyed by name, and one keyed by uid.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	register int makeold;
	FILE *oldfp;
	DBM *dp;
	datum key, content;
	int ch;
	char buf[256], nbuf[50];

	while ((ch = getopt(argc, argv, "pv")) != EOF) {
		switch(ch) {
		case 'p':			/* create ``password.orig'' */
			makeold = 1;
			/* FALLTHROUGH */
		case 'v':			/* backward compatible */
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

	if (!(_pw_fp = fopen(*argv, "r"))) {
		(void)fprintf(stderr, "mkpasswd: %s: can't open for reading.\n", *argv);
		exit(1);
	}

	rmall(*argv);
	(void)umask(0);

	/* open old password format file, dbm files */
	if (makeold) {
		int oldfd;

		(void)sprintf(buf, "%s.orig", *argv);
		if ((oldfd = open(buf, O_WRONLY|O_CREAT|O_EXCL, 0644)) < 0) {
			(void)fprintf(stderr, "mkpasswd: %s: %s\n", buf, strerror(errno));
			exit(1);
		}
		if (!(oldfp = fdopen(oldfd, "w"))) {
			(void)fprintf(stderr, "mkpasswd: %s: fdopen failed.\n", buf);
			exit(1);
		}
	}
	if (!(dp = dbm_open(*argv, O_WRONLY|O_CREAT|O_EXCL, 0644))) {
		(void)fprintf(stderr, "mkpasswd: %s: %s\n", *argv, strerror(errno));
		exit(1);
	}
	if (compact(&_pw_passwd, _pw_fp, line, dp, content, key, buf, nbuf, oldfp, makeold) != 0) {
		(void)fprintf(stderr, "mkpasswd: dbm_store failed.\n");
		rmall(*argv);
		exit(1);
	}
	exit(0);
}

static int
compact(pw, fp, buffer, dp, content, key, buf, nbuf, oldfp, old)
	struct passwd *pw;
	char *buffer;
	DBM *dp;
	datum content, key;
	char *buf, *nbuf;
	FILE *oldfp, *fp;
	int old;
{
	register char *p, *t, *flag;

	content.dptr = buf;
	while (scanpw(pw, fp, buffer)) {
		/* create dbm entry */
		p = buf;

#define MACRO(a)	do { a } while (0)
#define	COMPACT(e)	MACRO(t = e; while ((*p++ = *t++));)
#define	SCALAR(v)	MACRO(memmove(&(v), p, sizeof v); p += sizeof v;)

		COMPACT(pw->pw_name);
		(void)sprintf(nbuf, "%ld", offset);
		COMPACT(nbuf);
		SCALAR(pw->pw_uid);
		SCALAR(pw->pw_gid);
		SCALAR(pw->pw_change);
		COMPACT(pw->pw_class);
		COMPACT(pw->pw_gecos);
		COMPACT(pw->pw_dir);
		COMPACT(pw->pw_shell);
		SCALAR(pw->pw_expire);
		*flag = _PW_KEYBYNAME;
		p++ = flag;
		content.dsize = (p - buf);
#ifdef debug
		(void)printf("store %s, uid %d\n", pw->pw_name, pw->pw_uid);
#endif
		if (storekeybyname(pw, dp, content, key, flag) != 0) {
			return (1);
		}
		*flag = _PW_KEYBYUID;
		if (storekeybyuid(pw, dp, content, key, flag) != 0) {
			return (1);
		}
		*flag = _PW_KEYBYNUM;
		if (storekeybykeynum(pw, dp, content, key, flag) != 0) {
			return (1);
		}
		if (!old) {
			continue;
		}
		fprintf(oldfp, "%s:*:%d:%d:%s:%s:%s\n", pw->pw_name,
				pw->pw_uid, pw->pw_gid, pw->pw_gecos,
				pw->pw_dir, pw->pw_shell);
	}
	dbm_close(dp);
	return (0);
}

static int
storekeybykeynum(pw, dp, content, key, flag)
	struct passwd *pw;
	DBM *dp;
	datum content, key;
	char *flag;
{
	if (flag == _PW_KEYBYNUM) {
		key.dptr = pw->pw_name;
		key.dsize = strlen(pw->pw_name);
		if (dbm_store(dp, key, content, DBM_INSERT) < 0) {
			return (1);
		}
		return (0);
	}
	return (1);
}

static int
storekeybyname(pw, dp, content, key, flag)
	struct passwd *pw;
	DBM *dp;
	datum content, key;
	char *flag;
{
	if (flag == _PW_KEYBYNAME) {
		key.dptr = pw->pw_name;
		key.dsize = strlen(pw->pw_name);
		if (dbm_store(dp, key, content, DBM_INSERT) < 0) {
			return (1);
		}
		return (0);
	}
	return (1);
}

static int
storekeybyuid(pw, dp, content, key, flag)
	struct passwd *pw;
	DBM *dp;
	datum content, key;
	char *flag;
{
	if (flag == _PW_KEYBYUID) {
		key.dptr = (char *)&pw->pw_uid;
		key.dsize = sizeof(int);
		if (dbm_store(dp, key, content, DBM_INSERT) < 0) {
			return (1);
		}
		return (0);
	}
	return (1);
}

static void
rmall(fname)
	char *fname;
{
	register char *p;
	char buf[MAXPATHLEN];

	for (p = strcpy(buf, fname); *p; ++p);
	bcopy(".pag", p, 5);
	(void)unlink(buf);
	bcopy(".dir", p, 5);
	(void)unlink(buf);
	bcopy(".orig", p, 6);
	(void)unlink(buf);
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: mkpasswd [-p] passwd_file\n");
	exit(1);
}

/* copied from libc/gen/getpwent.c */

static int
scanpw(struct passwd *pw, FILE *fp, char *buffer)
{
	register char *cp;
	char *bp;

	for (;;) {
		offset = ftell(fp);
		if (!(fgets(buffer, sizeof(buffer), fp))) {
			return (0);
		}
		bp = line;
		/* skip lines that are too big */
		if (!(cp = index(buffer, '\n'))) {
			int ch;
			while ((ch = getc(fp)) != '\n' && ch != EOF);
			continue;
		}
		*cp = '\0';
		pw->pw_name = strsep(&bp, ":");
		pw->pw_passwd = strsep(&bp, ":");
		offset += pw->pw_passwd - line;
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		pw->pw_uid = atoi(cp);
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		pw->pw_gid = atoi(cp);
		pw->pw_class = strsep(&bp, ":");
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		pw->pw_change = atol(cp);
		if (!(cp = strsep(&bp, ":"))) {
			continue;
		}
		pw->pw_expire = atol(cp);
		pw->pw_gecos = strsep(&bp, ":");
		pw->pw_dir = strsep(&bp, ":");
		pw->pw_shell = strsep(&bp, ":");
		return (1);
	}
	/* NOTREACHED */
	return (1);
}
