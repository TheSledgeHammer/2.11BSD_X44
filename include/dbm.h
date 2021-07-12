/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dbm.h	5.1 (Berkeley) 3/27/86
 */

#include <sys/null.h>

#include <ndbm.h>

datum	fetch();
datum	firstkey();
datum	nextkey();
#if 0
datum	makdatum();
datum	firsthash();
long	calchash();
long	hashinc();
#endif
