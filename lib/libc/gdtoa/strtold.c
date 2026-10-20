/*	$211BSD: Makefile,v 1.0 2025/04/04 11:34:27 martin Exp $ */

#include "namespace.h"
#include "gdtoaimp.h"

#include <locale.h>
#include "setlocale.h"

#ifdef __weak_alias
__weak_alias(strtold_l, _strtold_l)
__weak_alias(strtold, _strtold)
#endif

long double
strtold_l(const char *nptr, char **endptr, locale_t loc)
{
    long double ld;

    (void)strtopx_l(nptr, endptr, &ld, loc);
    return (ld);
}

long double
strtold(const char *nptr, char **endptr)
{
    long double ld;

    (void)strtopx(nptr, endptr, &ld);
    return (ld);
}
