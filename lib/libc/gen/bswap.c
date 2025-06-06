/*  $NetBSD: bswap16.c,v 1.2 2003/07/26 19:24:42 salo Exp $    */
/*  $NetBSD: bswap32.c,v 1.2 2003/07/26 19:24:42 salo Exp $    */
/*  $NetBSD: bswap64.c,v 1.5 2003/08/10 08:24:52 mrg Exp $    */

/*
 * Written by Manuel Bouyer <bouyer@NetBSD.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: bswap16.c,v 1.2 2003/07/26 19:24:42 salo Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/bswap.h>

#undef bswap64
#undef bswap32
#undef bswap16

u_int32_t bswap32_high(u_int64_t);
u_int32_t bswap32_low(u_int64_t);

#ifdef _LP64

u_int64_t
bswap64(x)
	u_int64_t x;
{
	/*
	 * Assume we have wide enough registers to do it without touching
	 * memory.
	 */
	return  ((x << 56) & 0xff00000000000000UL) |
			((x << 40) & 0x00ff000000000000UL) |
			((x << 24) & 0x0000ff0000000000UL) |
			((x <<  8) & 0x000000ff00000000UL) |
			((x >>  8) & 0x00000000ff000000UL) |
			((x >> 24) & 0x0000000000ff0000UL) |
			((x >> 40) & 0x000000000000ff00UL) |
			((x >> 56) & 0x00000000000000ffUL);
}

#else

u_int64_t
bswap64(x)
	u_int64_t x;
{
	/*
	 * Split the operation in two 32bit steps.
	 */
	u_int32_t tl, th;

	tl = bswap32_low(x);
	th = bswap32_high(x);
	return (((u_int64_t)th << 32) | tl);
}

#endif

u_int32_t
bswap32(x)
	u_int32_t x;
{
	return ((x << 24) & 0xff000000) |
			((x << 8) & 0x00ff0000)	|
			((x >> 8) & 0x0000ff00) |
			((x >> 24) & 0x000000ff);
}

u_int16_t
bswap16(x)
	u_int16_t x;
{
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
}

u_int32_t
bswap32_high(x)
	u_int64_t x;
{
	return (bswap32((u_int32_t)(x & 0x00000000ffffffffULL)));
}

u_int32_t
bswap32_low(x)
	u_int64_t x;
{
	return (bswap32((u_int32_t)((x >> 32) & 0x00000000ffffffffULL)));
}
