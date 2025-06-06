/*      $NetBSD: bswap.h,v 1.2 1999/08/21 05:39:52 simonb Exp $      */

/* Written by Manuel Bouyer. Public domain */

#ifndef _MACHINE_BSWAP_H_
#define	_MACHINE_BSWAP_H_

#define __BSWAP_RENAME
#include <sys/bswap.h>

#include <machine/byte_swap.h>

#define bswap16(x)      __byte_swap_word(x)
#define bswap32(x)      __byte_swap_long(x)

#endif /* !_MACHINE_BSWAP_H_ */
