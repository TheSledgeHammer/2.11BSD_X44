#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)strlen.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <string.h>
/*
 * Returns the number of
 * non-NULL bytes in string argument.
 */
int
strlen(s)
	register char *s;
{
	register n;

	n = 0;
	while (*s++)
		n++;
	return(n);
}
