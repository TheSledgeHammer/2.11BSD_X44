/*	$OpenBSD: explicit_bzero.c,v 1.4 2015/08/31 02:53:57 guenther Exp $ */
/*
 * Public domain.
 * Written by Matthew Dempsky.
 */
/* $NetBSD: explicit_memset.c,v 1.4 2014/06/24 16:39:39 drochner Exp $ */
/*
 * Written by Matthias Drochner <drochner@NetBSD.org>.
 * Public domain.
 */

#include "namespace.h"

#include <string.h>

#ifdef __weak_alias
__weak_alias(explicit_bzero, _explicit_bzero)
__weak_alias(explicit_memset, _explicit_memset)
#endif

void __explicit_bzero_hook(void *, size_t);

__attribute__((weak)) void
__explicit_bzero_hook(void *buf, size_t len)
{

}

void
explicit_bzero(void *buf, size_t len)
{
	explicit_memset(buf, 0, len);
	__explicit_bzero_hook(buf, len);
}

/*
 * The use of a volatile pointer guarantees that the compiler
 * will not optimise the call away.
 */
void * (*volatile explicit_memset_impl)(void *, int, size_t) = memset;

void *
explicit_memset(void *b, int c, size_t len)
{
	return ((*explicit_memset_impl)(b, c, len));
}
