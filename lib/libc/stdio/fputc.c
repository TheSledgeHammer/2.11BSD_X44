#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)fputc.c	5.3 (Berkeley) 3/4/87";
#endif
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "local.h"

int
fputc(c, fp)
	register c;
	register FILE *fp;
{
	return (putc(c, fp));
}
