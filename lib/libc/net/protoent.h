/*	$NetBSD: protoent.h,v 1.3 2024/01/20 14:52:48 christos Exp $	*/

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

struct protoent_data {
	FILE *protof;
	char **aliases;
	size_t maxaliases;
	int stayopen;
	//void *db;
	//struct servent serv;
	//char *line;
};

#define _GETPENT_R_SIZE_MAX 	1024

extern struct protoent_data 	_pts_protod;
extern struct protoent 			_pts_proto;
extern char 					_pts_protobuf[_GETPENT_R_SIZE_MAX];

int  getprotoent_r(struct protoent *, struct protoent_data *, char *, size_t, struct protoent **);
int	 getprotobyname_r(struct protoent *, struct protoent_data *, const char *, char *, size_t, struct protoent **);
int  getprotobynumber_r(struct protoent *, struct protoent_data *, int, char *, size_t, struct protoent **);
void setprotoent_r(int, struct protoent_data *);
void endprotoent_r(struct protoent_data *);
