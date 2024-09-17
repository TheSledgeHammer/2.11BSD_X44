/* $NetBSD: md2hl.c,v 1.4 2008/04/13 02:04:31 dholland Exp $ */

/*
 * Derived from code written by Jason R. Thorpe <thorpej@NetBSD.org>,
 * April 29, 1997.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: md2hl.c,v 1.4 2008/04/13 02:04:31 dholland Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#if HAVE_NBTOOL_CONFIG_H
#include <md2.h>
#else
#include <hash/md2.h>
#endif

#define	HASH_ALGORITHM	MD2

#if HAVE_NBTOOL_CONFIG_H
#define HASH_INCLUDE <md2.h>
#else
#define HASH_INCLUDE <hash/md2.h>
#endif

#include "../hashhl.c"
