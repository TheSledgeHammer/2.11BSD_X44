#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)strrchr.c	5.2 (berkeley) 86/03/09";
#endif
#endif /* LIBC_SCCS and not lint */

#include <string.h>

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
 *
 * This routine is just "rindex" renamed.
 */

char *
#if __STDC__
strrchr(const char *sp, int c)
#else
strrchr(sp, c)
	register const char *sp;
    int c;
#endif
{
	register char *r;

	r = NULL;
	do {
		if (*sp == c) {
			r = __UNCONST(sp);
        }
	} while (*sp++);
	return (r);
}
