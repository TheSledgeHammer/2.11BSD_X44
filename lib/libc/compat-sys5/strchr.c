#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)strchr.c	5.2 (Berkeley) 86/03/09";
#endif
#endif LIBC_SCCS and not lint

#include <string.h>

/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 *
 * this routine is just "index" renamed.
 */

char *
strchr(sp, c)
    register char *sp, c;
{
	do {
		if (*sp == c)
			return(sp);
	} while (*sp++);
	return (NULL);
}
