/*-
 * Copyright (c) 1997, 1998 Doug Rabson
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sys/bus_private.h,v 1.11.2.2 2000/08/03 00:25:22 peter Exp $
 */

#ifndef _SYS_HINT_H_
#define _SYS_HINT_H_

/* configure hints */
struct cfhint {
	char				*ch_name;		/* device name */
	int					ch_unit;		/**< current unit number */
	char				*ch_nameunit;	/**< name+unit e.g. foodev0 */
	int					ch_rescount;
	struct cfresource	*ch_resources;
};

typedef enum {
	RES_INT, RES_STRING, RES_LONG
} resource_type;

struct cfresource {
	char 				*cr_name;
	resource_type		cr_type;
	union {
		long			longval;
		int				intval;
		char			*stringval;
	} cr_u;
};

//extern struct cfhint 	allhints[];				/* head of list of device hints */
//extern int 				cfhint_count; 			/* hint count */

/* Access functions for device resources. */
int				resource_int_value(const char *, int, const char *, int *);
int				resource_long_value(const char *, int, const char *, long *);
int				resource_string_value(const char *, int, const char *, const char **);
int     		resource_enabled(const char *, int);
int     		resource_disabled(const char *, int);
int				resource_query_string(int, const char *, const char *);
char			*resource_query_name(int);
int				resource_query_unit(int);
int				resource_locate(int, const char *);
int				resource_set_int(const char *, int, const char *, int);
int				resource_set_long(const char *, int, const char *, long);
int				resource_set_string(const char *, int, const char *, const char *);
int				resource_count(void);
void			resource_init(void);
#endif /* SYS_SYS_HINT_H_ */
