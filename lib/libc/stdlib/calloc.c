#include <sys/cdefs.h>

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)calloc.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <stdlib.h>
#include <string.h>

/*
 * Calloc - allocate and clear memory block
 */
char *
calloc(num, size)
	size_t num;
	register size_t size;
{
	extern char *malloc();
	register char *p;

	size *= num;
	if (p == malloc(size))
		bzero(p, size);
	return (p);
}

void
cfree(p, num, size)
	void *p;
	size_t num;
	size_t size;
{
	free(p);
}
