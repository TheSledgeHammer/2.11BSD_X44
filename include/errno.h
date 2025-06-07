/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)errno.h	7.1 (Berkeley) 6/4/86
 */

#ifndef _ERRNO_H_
#define _ERRNO_H_

#include <sys/cdefs.h>
#include <sys/errno.h>

__BEGIN_DECLS
#ifndef __errno
int *__errno(void);
#define __errno __errno
#endif

#ifndef errno
#define errno (*__errno())
#endif

/* 
 * Compatability with 4.4BSD Lite2/2.11BSD:
 * Cannot #define _errno. Will cause numerous 
 * conflicts
 */
int *_errno(void); 
__END_DECLS

#endif /* !_ERRNO_H_ */
