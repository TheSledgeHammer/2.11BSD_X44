#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strrchr.c	5.2 (berkeley) 86/03/09";
#endif LIBC_SCCS and not lint

#include <string.h>

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
 *
 * This routine is just "rindex" renamed.
 */

char *
strrchr(sp, c)
register char *sp, c;
{
	register char *r;

	r = NULL;
	do {
		if (*sp == c)
			r = sp;
	} while (*sp++);
	return(r);
}
