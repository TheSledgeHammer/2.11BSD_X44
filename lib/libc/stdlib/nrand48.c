/*	$NetBSD: nrand48.c,v 1.8 2000/01/22 22:19:19 mycroft Exp $	*/

/*
 * Copyright (c) 1993 Martin Birgmeier
 * All rights reserved.
 *
 * You may redistribute unmodified or modified versions of this source
 * code provided that the above copyright notice and this and the
 * following conditions are retained.
 *
 * This software is provided ``as is'', and comes with no warranties
 * of any kind. I shall in no event be liable for anything that happens
 * to anyone/anything when using this software.
 */

#include "namespace.h"

#include <assert.h>

#include "rand48.h"

#ifdef __weak_alias
__weak_alias(nrand48,_nrand48)
#endif

long
nrand48(unsigned short xseed[3])
{
	_DIAGASSERT(xseed != NULL);

	__dorand48(xseed);
	return ((long)((unsigned long) xseed[2] << 15) +
	    ((unsigned long) xseed[1] >> 1));
}
