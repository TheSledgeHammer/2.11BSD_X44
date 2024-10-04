#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)index.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <string.h>

/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 */

//#define	NULL	0

char *
index(sp, c)
	register char *sp, c;
{
	do {
		if (*sp == c)
			return (sp);
	} while (*sp++);
	return (NULL);
}
