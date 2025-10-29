/*	$NetBSD: properties.h,v 1.4 2003/07/04 07:42:04 itojun Exp $	*/

/*  
 * Copyright (c) 2001 Eduardo Horvath.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Eduardo Horvath.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_PROPERTIES_H_
#define _SYS_PROPERTIES_H_

typedef void *opaque_t;		/* Value large enough to hold a pointer */

struct propdb;
typedef struct propdb *propdb_t;

/* malloc type */
#define M_PROP 				93

#define	MAX_KDBNAME			32

#define	PROP_UNKNOWN		0x000000000
#define	PROP_BOOL			0x010000000
#define	PROP_NUMBER			0x020000000
#define	PROP_STRING			0x030000000
#define	PROP_DATA			0x040000000
#define	PROP_AGGREGATE		0x050000000
#define	PROP_TYPE(x)		((x) & PROP_AGGREGATE)

#define	PROP_ARRAY			0x060000000
#define	PROP_CONST			0x160000000
#define	PROP_ELSZ(x)		0x0ffffffff

propdb_t 	propdb_create(const char *);
void 		propdb_destroy(propdb_t);

int 		propdb_set(propdb_t, opaque_t, const char *, void *, size_t, int, int);
size_t 		propdb_objs(propdb_t, opaque_t *, size_t);
size_t 		propdb_list(propdb_t, opaque_t, char *, size_t);
size_t 		propdb_get(propdb_t, opaque_t, const char *, void *, size_t, int *);
int 		propdb_delete(propdb_t, opaque_t, const char *);
int 		propdb_copy(propdb_t, opaque_t, opaque_t, int);
int			propdb_type(opaque_t);

#endif /* _SYS_PROPERTIES_H_ */
