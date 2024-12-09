/*	$NetBSD: infinityl.c,v 1.4 2011/06/06 17:02:30 drochner Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: infinityl.c,v 1.4 2011/06/06 17:02:30 drochner Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>

const union __long_double_u __infinityl = {
#if defined(__i386__)
		/*
		 * IEEE-compatible infinityl.c for little-endian 80-bit format -- public domain.
		 * Note that the representation includes 16 bits of tail padding per i386 ABI.
		 */
		{ 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f, 0, 0 }
#endif
#if defined(__vax__)
		/*
		 * infinityl.c - max. value representable in VAX D_floating  -- public domain.
		 * This is _not_ infinity.
		 */
		{ 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
#endif
#if defined(__x86_64__)
		/*
		 * IEEE-compatible infinityl.c for little-endian 80-bit format -- public domain.
		 * Note that the representation includes 48 bits of tail padding per amd64 ABI.
		 */
		{ 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f, 0, 0, 0, 0, 0, 0 }
#endif
};
