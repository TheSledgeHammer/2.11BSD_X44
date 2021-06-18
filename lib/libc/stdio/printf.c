#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)printf.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include	<stdio.h>

int
printf(fmt, args)
	char *fmt;
	register int args;
{
	doprnt(fmt, &args, stdout);
	return(ferror(stdout)? EOF: 0);
}
