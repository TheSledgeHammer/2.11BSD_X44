/* $NetBSD: wcstol.c,v 1.2 2003/03/11 09:21:23 tshiozak Exp $ */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: wcstol.c,v 1.2 2003/03/11 09:21:23 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "__wctoint.h"

#define	_FUNCNAME	wcstol
#define	__INT		long
#define	__INT_MIN	LONG_MIN
#define	__INT_MAX	LONG_MAX

#include "_wcstol.h"
