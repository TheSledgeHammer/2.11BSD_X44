/*	$211BSD: Makefile,v 1.0 2025/04/04 11:34:27 martin Exp $ */

#include "namespace.h"
#include "gdtoaimp.h"

#ifdef __weak_alias
__weak_alias(strtold, _strtold)
#endif

long double
strtold(const char *nptr, char **endptr)
{
    long double ld;

    (void)strtopx(nptr, endptr, &ld);
    return (ld);
}
