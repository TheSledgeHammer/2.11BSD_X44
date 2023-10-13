/*	$NetBSD: altqconf.h,v 1.4 2002/09/22 20:09:15 jdolecek Exp $	*/

#ifdef _KERNEL

#include "opt_altq_enabled.h"

#include <sys/conf.h>

#ifdef ALTQ
#define	NALTQ	1
#else
#define	NALTQ	0
#endif

#endif /* _KERNEL */
