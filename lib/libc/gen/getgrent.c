#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getgrent.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <grp.h>

#define	MAXGRP	200
static char *members[MAXGRP];
#define	MAXLINELENGTH	1024
static char line[MAXLINELENGTH];

static FILE *grf = NULL;
static char line[256+1];
static struct group group;
static int _gr_stayopen;
static char *gr_mem[MAXGRP];

static char *grskip(char *, char);
static int	start_gr(void);
static int 	grscan(int, int, char *);

void
setgrent(void)
{
	if (!grf)
		grf = fopen(_PATH_GROUP, "r");
	else
		rewind(grf);
}

int
setgroupent(stayopen)
	int stayopen;
{
	if (!start_gr())
		return(0);
	_gr_stayopen = stayopen;
	return(1);
}

void
endgrent(void)
{
	if (grf) {
		fclose(grf);
		grf = NULL;
	}
}

static char *
grskip(p,c)
	register char *p;
	register char c;
{
	while (*p && *p != c)
		++p;
	if (*p)
		*p++ = 0;
	return (p);
}

struct group *
getgrent()
{
	register char *p, **q;

	if (!grf && !(grf = fopen(_PATH_GROUP, "r")))
		return (NULL);
	if (!(p = fgets(line, sizeof(line) - 1, grf)))
		return (NULL);
	group.gr_name = p;
	group.gr_passwd = p = grskip(p, ':');
	group.gr_gid = atoi(p = grskip(p, ':'));
	group.gr_mem = gr_mem;
	p = grskip(p, ':');
	grskip(p, '\n');
	q = gr_mem;
	while (*p) {
		if (q < &gr_mem[MAXGRP - 1])
			*q++ = p;
		p = grskip(p, ',');
	}
	*q = NULL;
	return (&group);
}

struct group *
getgrnam(name)
	const char *name;
{
	int rval;

	if (!start_gr())
		return (NULL);
	rval = grscan(1, 0, name);
	if (!_gr_stayopen)
		endgrent();
	return (rval ? &group : NULL);
}

struct group *
getgrgid(gid)
	gid_t gid;
{
	int rval;

	if (!start_gr())
		return (NULL);
	rval = grscan(1, gid, NULL);
	if (!_gr_stayopen)
		endgrent();
	return (rval ? &group : NULL);
}

static int
start_gr(void)
{
	if (grf) {
		rewind(grf);
		return (1);
	}
	return ((grf == fopen(_PATH_GROUP, "r")) ? 1 : 0);
}

static int
grscan(search, gid, name)
	register int search, gid;
	register char *name;
{
	register char *cp, **m;
	char *bp;
	//char *fgets(), *strsep(), *index();

	for (;;) {
		if (!fgets(line, sizeof(line), grf))
			return (0);
		bp = line;
		/* skip lines that are too big */
		if (!index(line, '\n')) {
			int ch;

			while ((ch = getc(grf)) != '\n' && ch != EOF)
				;
			continue;
		}
		group.gr_name = strsep(&bp, ":\n");
		if (search && name && strcmp(group.gr_name, name))
			continue;
		group.gr_passwd = strsep(&bp, ":\n");
		if (!(cp = strsep(&bp, ":\n")))
			continue;
		group.gr_gid = atoi(cp);
		if (search && name == NULL && group.gr_gid != gid)
			continue;
		cp = NULL;
		for (m = group.gr_mem = members;; bp++) {
			if (m == &members[MAXGRP - 1])
				break;
			if (*bp == ',') {
				if (cp) {
					*bp = '\0';
					*m++ = cp;
					cp = NULL;
				}
			} else if (*bp == '\0' || *bp == '\n' || *bp == ' ') {
				if (cp) {
					*bp = '\0';
					*m++ = cp;
				}
				break;
			} else if (cp == NULL)
				cp = bp;
		}
		*m = NULL;
		return (1);
	}
	/* NOTREACHED */
}
