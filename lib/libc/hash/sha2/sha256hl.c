/* $NetBSD: sha256hl.c,v 1.8 2008/04/13 02:04:32 dholland Exp $ */

/*
 * Derived from code written by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: sha256hl.c,v 1.8 2008/04/13 02:04:32 dholland Exp $");
#endif /* LIBC_SCCS and not lint */

#define	HASH_ALGORITHM	SHA256
#define	HASH_FNPREFIX	SHA256_

#if HAVE_NBTOOL_CONFIG_H
#define HASH_INCLUDE    <sha2.h>
#else
#define HASH_INCLUDE    <hash/sha2.h>
#endif

#include "../hashhl.c"
