/*
 * Copyright (c) 2025 Martin Kelly
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>

#include <err.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <util.h>

#include "extern.h"

/* pw_gensalt types: */
const char * const pw_types[] = { "old", "new", "newsalt", "md5", "blowfish", "sha1" };
/* pw_gensalt options: */
const char * const pw_options[] = { "localcipher", "ypcipher" };

const char *pwd_crypto(const char *type);
const char *pwd_cipher(const char *option);
static int pwd_compare(const char *type, const char *option, const char *default_type, const char *default_option);
#ifdef USE_YP
static int pwd_yp_conf(const char **type, const char **option);
#else
static int pwd_local_conf(const char **type, const char **option);
#endif

#define nelems(x) (sizeof(x) / sizeof((x)[0]))

const char *
pwd_crypto(const char *type)
{
	const char *crypto;
	int i;

	crypto = pw_types[0];
	for (i = 0; i < nelems(pw_types); i++) {
		crypto = pw_types[i];
		if (crypto != NULL) {
			if (type != NULL) {
				if (strcasecmp(crypto, type)) {
					crypto = pw_types[i];
				}
			} else {
#ifdef USE_YP
				/* sets crypto to default: old */
				crypto = pwd_default_yp_type;
#else
				/* sets crypto to default: blowfish */
				crypto = pwd_default_local_type;
#endif
			}
			return (crypto);
		}
	}
	return (NULL);
}

const char *
pwd_cipher(const char *option)
{
	const char *cipher;
	int i;

	cipher = pw_options[0];
	for (i = 0; i < nelems(pw_options); i++) {
		cipher = pw_options[i];
		if (cipher != NULL) {
			if (option != NULL) {
				if (strcasecmp(cipher, option)) {
					cipher = pw_options[i];
				}
			} else {
#ifdef USE_YP
				/* sets cipher to default: ypcipher */
				cipher = pwd_default_yp_option;
#else
				/* sets cipher to default: localcipher */
				cipher = pwd_default_local_option;
#endif
			}
			return (cipher);
		}
	}
	return (NULL);
}

int
pwd_conf(const char **type, const char **option)
{
	int rval;

#ifdef USE_YP
	rval = pwd_yp_conf(type, option);
#else
	rval = pwd_local_conf(type, option);
#endif
	return (rval);
}

static int
pwd_compare(const char *type, const char *option, const char *default_type, const char *default_option)
{
	if (strcasecmp(type, default_type)
			&& !strcasecmp(option, default_option)) {
		return (1);
	} else if (!strcasecmp(type, default_type)
			&& strcasecmp(option, default_option)) {
		return (-1);
	} else {
		return (0);
	}
}

#ifdef USE_YP
static int
pwd_yp_conf(const char **type, const char **option)
{
	const char *cipher, *crypto;
	int rval;

	cipher = pwd_cipher(*option);
	if (cipher == NULL) {
		errx(1, "Couldn't find specified cipher.\n");
	}

	crypto = pwd_crypto(*type);
	if (crypto == NULL) {
		errx(1, "Couldn't find specified algorithm.\n");
	}

	rval = pwd_compare(crypto, cipher, pwd_default_yp_type, pwd_default_yp_option);
	switch (rval) {
	case 0:
		*type = crypto;
		*option = cipher;
		break;
	case 1:
		*option = pwd_default_yp_option;
		break;
	case -1:
		*type = pwd_default_yp_type;
		break;
	default:
		*type = crypto;
		*option = cipher;
		break;
	}
	return (0);
}

#else

static int
pwd_local_conf(const char **type, const char **option)
{
	const char *cipher, *crypto;
	int rval;

	cipher = pwd_cipher(*option);
	if (cipher == NULL) {
		errx(1, "Couldn't find specified cipher.\n");
	}

	crypto = pwd_crypto(*type);
	if (crypto == NULL) {
		errx(1, "Couldn't find specified algorithm.\n");
	}

	rval = pwd_compare(crypto, cipher, pwd_default_local_type, pwd_default_local_option);
	switch (rval) {
	case 0:
		*type = crypto;
		*option = cipher;
		break;
	case 1:
		*option = pwd_default_local_option;
		break;
	case -1:
		*type = pwd_default_local_type;
		break;
	default:
		*type = crypto;
		*option = cipher;
		break;
	}
	return (0);
}

#endif
