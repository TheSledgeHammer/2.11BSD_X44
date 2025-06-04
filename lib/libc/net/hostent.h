/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <stdio.h>

struct hostent_data {
	FILE *hostf;
	char **aliases;
	size_t maxaliases;
	int stayopen;
};

#define _GETHTENT_R_SIZE_MAX 1024

extern struct hostent_data 	_hvs_hostd;
extern struct hostent 		_hvs_host;
extern char 			_hvs_hostbuf[_GETHTENT_R_SIZE_MAX];

int getanswer_r(struct hostent *, struct hostent_data *, res_state, querybuf *, int, int, char *, size_t, struct hostent **);
struct hostent *getanswer(querybuf *, int, int);
int gethostbyname2_r(struct hostent *, struct hostent_data *, const char *, int, char *, size_t, struct hostent **);
struct hostent *gethostbyname2(const char *, int);
int gethostbyname_r(struct hostent *, struct hostent_data *, const char *, char *, size_t, struct hostent **);
struct hostent *gethostbyname(const char *);
int  gethostbyaddr_r(struct hostent *, struct hostent_data *, const char *, int, int, char *, size_t, struct hostent **);
struct hostent *gethostbyaddr(const char *, int, int);
int  gethtent_r(struct hostent *, struct hostent_data *, char *, size_t, struct hostent **);
int  gethtbyname_r(struct hostent *, struct hostent_data *, char *, char *, size_t, struct hostent **);
int  gethtbyaddr_r(struct hostent *, struct hostent_data *, const char *, int, int, char *, size_t, struct hostent **);
void sethtent_r(int, struct hostent_data *);
void endhtent_r(struct hostent_data *);
struct hostent *gethtent(void);
void sethtent(int);
void endhtent(void);
struct hostent *gethtbyname(char *);
struct hostent *gethtbyaddr(const char *, int, int);
