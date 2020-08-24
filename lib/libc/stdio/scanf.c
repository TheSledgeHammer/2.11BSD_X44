#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)scanf.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include	<stdio.h>

int
scanf(fmt, args)
	char *fmt;
	register int args;
{
	return(_doscan(stdin, fmt, &args));
}

int
fscanf(iop, fmt, args)
	FILE *iop;
	char *fmt;
	register int args;
{
	return(_doscan(iop, fmt, &args));
}

int
sscanf(str, fmt, args)
	register char *str;
	char *fmt;
	register int args;
{
	FILE _strbuf;

	_strbuf._flag = _IOREAD|_IOSTRG;
	_strbuf._ptr = _strbuf._base = str;
	_strbuf._cnt = 0;
	while (*str++)
		_strbuf._cnt++;
	_strbuf._bufsiz = _strbuf._cnt;
	return(_doscan(&_strbuf, fmt, &args));
}
