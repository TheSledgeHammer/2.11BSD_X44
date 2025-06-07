/*      $NetBSD: compat_getopt.h,v 1.2 2007/11/08 20:30:59 christos Exp $ */

/* We unconditionally use the NetBSD getopt.h in libnbcompat. */

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#define option 		__nbcompat_option

#undef no_argument
#undef required_argument
#undef optional_argument

#ifdef _GETOPT_H_
#undef _GETOPT_H_
#endif

#include "../../include/getopt.h"

