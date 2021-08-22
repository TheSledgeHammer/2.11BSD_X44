/* $NetBSD: sha384hl.c,v 1.8 2008/04/13 02:04:32 dholland Exp $ */

/*
 * Derived from code written by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: sha384hl.c,v 1.8 2008/04/13 02:04:32 dholland Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sha2.h>

#define	HASH_ALGORITHM	SHA384
#define	HASH_FNPREFIX	SHA384_
#define HASH_INCLUDE	<sha2.h>

#include "../hashhl.c"
