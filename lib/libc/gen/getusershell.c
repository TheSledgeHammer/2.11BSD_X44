/*
 * Copyright (c) 1985 Regents of the University of California.
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getusershell.c	5.5 (Berkeley) 7/21/88";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <paths.h>

/*
 * Do not add local shells here.  They should be added in /etc/shells
 */
static char *okshells[] = { _PATH_BSHELL, _PATH_CSHELL, NULL };

static char **shells, *strings;
static char **curshell = NULL;
extern char **initshells(void);

/*
 * Get a list of shells from SHELLS, if it exists.
 */
char *
getusershell()
{
	char *ret;

	if (curshell == NULL)
		curshell = initshells();
	ret = *curshell;
	if (ret != NULL)
		curshell++;
	return (ret);
}

void
endusershell()
{
	
	if (shells != NULL)
		free((char *)shells);
	shells = NULL;
	if (strings != NULL)
		free(strings);
	strings = NULL;
	curshell = NULL;
}

void
setusershell()
{

	curshell = initshells();
}

static char **
initshells()
{
	register char **sp, *cp;
	register FILE *fp;
	struct stat statb;
	extern char *malloc(), *calloc();

	if (shells != NULL)
		free((char*) shells);
	shells = NULL;
	if (strings != NULL)
		free(strings);
	strings = NULL;
	if ((fp = fopen(_PATH_SHELLS, "r")) == (FILE*) 0)
		return (okshells);
	if (fstat(fileno(fp), &statb) == -1) {
		(void) fclose(fp);
		return (okshells);
	}
	if ((strings = malloc((unsigned) statb.st_size)) == NULL) {
		(void) fclose(fp);
		return (okshells);
	}
	shells = (char**) calloc((unsigned) statb.st_size / 3, sizeof(char*));
	if (shells == NULL) {
		(void) fclose(fp);
		free(strings);
		strings = NULL;
		return (okshells);
	}
	sp = shells;
	cp = strings;
	while (fgets(cp, MAXPATHLEN + 1, fp) != NULL) {
		while (*cp != '#' && *cp != '/' && *cp != '\0')
			cp++;
		if (*cp == '#' || *cp == '\0')
			continue;
		*sp++ = cp;
		while (!isspace(*cp) && *cp != '#' && *cp != '\0')
			cp++;
		*cp++ = '\0';
	}
	*sp = (char*) 0;
	(void) fclose(fp);
	return (shells);
}
