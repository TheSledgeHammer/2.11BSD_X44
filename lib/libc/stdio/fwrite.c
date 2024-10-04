/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)fwrite.c	8.1 (Berkeley) 6/4/93";
static char sccsid[] = "@(#)fwrite.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>
#include "local.h"
#include "fvwrite.h"

/*
 * Write `count' objects (each size `size') from memory to the given file.
 * Return the number of whole objects written.
 */
size_t
fwrite(buf, size, count, fp)
	const void *buf;
	size_t size, count;
	FILE *fp;
{
	size_t n;
	struct __suio uio;
	struct __siov iov;

	iov.iov_base = (void *)buf;
	uio.uio_resid = iov.iov_len = n = count * size;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;

	/*
	 * The usual case is success (__sfvwrite returns 0);
	 * skip the divide if this happens, since divides are
	 * generally slow and since this occurs whenever size==0.
	 */
	if (__sfvwrite(fp, &uio) == 0)
		return (count);
	return ((n - uio.uio_resid) / size);
}
