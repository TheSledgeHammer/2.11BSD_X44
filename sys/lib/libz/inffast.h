/*	$OpenBSD: inffast.h,v 1.6 2003/12/16 23:57:48 millert Exp $	*/
/* inffast.h -- header to use inffast.c
 * Copyright (C) 1995-2003 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

void inflate_fast __P((z_streamp strm, unsigned start));
