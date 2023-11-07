/*      $NetBSD: bswap.h,v 1.2 2000/02/03 02:03:12 cgd Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _SYS_BSWAP_H_
#define _SYS_BSWAP_H_

#ifndef _LOCORE
#include <sys/cdefs.h>
#include <sys/types.h>

#include <machine/bswap.h>

__BEGIN_DECLS
#if defined(_KERNEL) || defined(_STANDALONE) || !defined(__BSWAP_RENAME)
u_int16_t       bswap16(u_int16_t) __attribute__((__const__));
u_int32_t       bswap32(u_int32_t) __attribute__((__const__));
#else
u_int16_t       bswap16(u_int16_t) __RENAME(__bswap16) __attribute__((__const__));
u_int32_t       bswap32(u_int32_t) __RENAME(__bswap32) __attribute__((__const__));
#endif
u_int64_t       bswap64(u_int64_t) __attribute__((__const__));
__END_DECLS
#endif /* !_LOCORE */

#endif /* !_SYS_BSWAP_H_ */
