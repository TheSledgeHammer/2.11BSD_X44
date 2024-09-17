/* $NetBSD: sha224hl.c,v 1.2 2014/12/11 21:54:13 riastradh Exp $ */

/*
 * Derived from code written by Jason R. Thorpe <thorpej@NetBSD.org>,
 * May 20, 2009.
 * Public domain.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sha224hl.c,v 1.2 2014/12/11 21:54:13 riastradh Exp $");

#include "namespace.h"

#if HAVE_NBTOOL_CONFIG_H
#include <sha2.h>
#else
#include <hash/sha2.h>
#endif

#define	HASH_ALGORITHM	SHA224
#define	HASH_FNPREFIX	SHA224_

#if HAVE_NBTOOL_CONFIG_H
#define HASH_INCLUDE    <sha2.h>
#else
#define HASH_INCLUDE    <hash/sha2.h>
#endif

#include "../hashhl.c"
