/*	$NetBSD: wcstoul.c,v 1.2 2003/03/11 09:21:24 tshiozak Exp $	*/

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: wcstoul.c,v 1.2 2003/03/11 09:21:24 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

#include "__wctoint.h"

#define	_FUNCNAME	wcstoul
#define	__UINT		unsigned long
#define	__UINT_MAX	ULONG_MAX

#include "_wcstoul.h"
