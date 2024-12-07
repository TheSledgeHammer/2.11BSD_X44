#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)strchr.c	5.2 (Berkeley) 86/03/09";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <assert.h>
#include <string.h>

/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 *
 * this routine is just "index" renamed.
 */

char *
#if __STDC__
strchr(const char *sp, int c)
#else
strchr(sp, c)
    register const char *sp;
    int c;
#endif
{
	do {
		if (*sp == c) {
			return (__UNCONST(sp));
        }
	} while (*sp++);
	return (NULL);
}
