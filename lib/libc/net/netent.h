/*	$NetBSD: servent.h,v 1.3 2008/04/28 20:23:00 martin Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

struct netent_data {
	FILE *netf;
	char **aliases;
	size_t maxaliases;
	int stayopen;
};

#define _GETNENT_R_SIZE_MAX 1024

extern struct netent_data 	_nvs_netd;
extern struct netent 		_nvs_net;
extern char 			_nvs_netbuf[_GETNENT_R_SIZE_MAX];

int getnetent_r(struct netent *, struct netent_data *, char *, size_t, struct netent **);
void setnetent_r(int, struct netent_data *);
void endnetent_r(struct netent_data *);

/* getnbyaddr.c */
int getnetbyaddr_r(struct netent *, struct netent_data *, long, int, char *, size_t, struct netent **);

/* getnbyname.c */
int getnetbyname_r(struct netent *, struct netent_data *, const char *, char *, size_t, struct netent **);
