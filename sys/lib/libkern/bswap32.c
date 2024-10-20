/*  $NetBSD: bswap32.c,v 1.3 2003/12/04 13:57:31 keihan Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: bswap32.c,v 1.3 2003/12/04 13:57:31 keihan Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#undef bswap32

u_int32_t
bswap32(x)
	u_int32_t x;
{
	return	((x << 24) & 0xff000000 ) |
		((x <<  8) & 0x00ff0000 ) |
		((x >>  8) & 0x0000ff00 ) |
		((x >> 24) & 0x000000ff );
}
