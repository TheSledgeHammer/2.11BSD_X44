/* $NetBSD: nanf.c,v 1.1 2014/08/10 05:47:36 matt Exp $ */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: nanf.c,v 1.1 2014/08/10 05:47:36 matt Exp $");
#endif /* LIBC_SCCS and not lint */

#include <math.h>
#include <machine/endian.h>

/* bytes for quiet NaN (IEEE single precision) */

#if !defined(__ia64__)
#if !defined(__vax__)
const union __float_u __nanf = {
#if defined(__hppa__) || defined(__mips__) || defined(__sh__)
#if BYTE_ORDER == BIG_ENDIAN
		{ 0x7f, 0xa0,    0,    0 }
#else
		{    0,    0, 0xa0, 0x7f }
#endif
#else
#if BYTE_ORDER == BIG_ENDIAN
		{ 0x7f, 0xc0,    0,    0 }
#else
		{    0,    0, 0xc0, 0x7f }
#endif
#endif
};
#endif
#endif
