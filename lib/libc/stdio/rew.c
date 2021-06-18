#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)rew.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include	<stdio.h>

void
rewind(iop)
	register FILE *iop;
{
	(void) fseek(fp, 0L, SEEK_SET);
	clearerr(iop);
}
