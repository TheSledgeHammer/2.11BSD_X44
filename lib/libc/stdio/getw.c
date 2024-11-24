#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getw.c	5.2.1 (2.11BSD GTE) 1/1/94";
#endif
#endif

#include <stdio.h>

int
getw(iop)
	register FILE *iop;
{
	register int i;
	register char *p;
	int w;

	p = (char *)&w;
	for (i = sizeof(w); --i >= 0;) {
		*p++ = getc(iop);
	}
	if (feof(iop)) {
		return (EOF);
	}
	return (w);
}

#ifdef pdp11
long
getlw(iop)
	register FILE *iop;
{
	register i;
	register char *p;
	long w;

	p = (char *)&w;
	for (i=sizeof(long); --i>=0;)
		*p++ = getc(iop);
	if (feof(iop))
		return(EOF);
	return(w);
}
#endif
