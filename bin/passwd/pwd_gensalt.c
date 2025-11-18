/*	$NetBSD: pwd_gensalt.c,v 1.10 2003/07/14 11:54:06 itojun Exp $	*/

/*
 * Copyright 1997 Niels Provos <provos@physnet.uni-hamburg.de>
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
 *      This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 * from OpenBSD: pwd_gensalt.c,v 1.9 1998/07/05 21:08:32 provos Exp
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pwd_gensalt.c,v 1.10 2003/07/14 11:54:06 itojun Exp $");
#endif /* not lint */

#include <sys/param.h>

#include <err.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <util.h>

#include "extern.h"

#ifdef NOPWGENSALT
static unsigned char itoa64[] =		/* 0 ... 63 => ascii - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static int pwd_gensalt_old(char *salt, int max, struct passwd *pw, const char *option);
static void to64(char *s, long v, int n);
static int new_salt(char *salt, int max, struct passwd *pw, const char *option);
static int old_salt(char *salt, int max, struct passwd *pw, const char *option);
static int md5_salt(char *salt, int max, struct passwd *pw, const char *option);
static int blowfish_salt(char *salt, int max, struct passwd *pw, const char *option);
static int sha1_salt(char *salt, int max, struct passwd *pw, const char *option);
#endif
static void pwd_usage(void);

/* pw_gensalt options available */
const char *pw_options[] = { "old", "new", "md5", "blowfish", "sha1" };

#define nelems(x) (sizeof(x) / sizeof((x)[0]))

const char *
pwd_crypto(const char *option)
{
	const char *crypt;
    int i;

    crypt = &pw_options[0];
    for (i = 0; i < nelems(pw_options); i++) {
        crypt = &pw_options[i];
        if (strcmp(crypt, option) == 0) {
        	return (crypt);
        }
    }
    return (NULL);
}

void
pwd_algorithm(const char *option, int flags)
{
	int i;

	if (flags == 1) {
		if (option != NULL) {
			if (!strcasecmp(option, optarg)) {
				goto valid;
			}
		} else {
			goto lookup;
		}
	} else {
lookup:
		option = &pw_options[0];
		for (i = 0; i < nelems(pw_options); i++) {
			option = &pw_options[i];
			if (option != NULL) {
				if (!strcasecmp(option, optarg)) {
					break;
				}
			}
		}
	}

valid:
	if (option == NULL) {
		if (!strcasecmp(optarg, "old")) {
			option = "old";
		} else if (!strcasecmp(optarg, "new")) {
			option = "old";
		} else if (!strcasecmp(optarg, "md5")) {
			option = "md5";
		} else if (!strcasecmp(optarg, "blowfish")) {
			option = "blowfish";
		} else if (!strcasecmp(optarg, "sha1")) {
			option = "sha1";
		} else {
			warnx("illegal argument to -a option");
			pwd_usage();
		}
	}
}

static void
pwd_usage(void)
{
	fprintf(stderr, "usage: passwd [-a] [type] [user]\n");
	(void)fprintf(stderr, "       old\n");
	(void)fprintf(stderr, "       new\n");
	(void)fprintf(stderr, "       md5\n");
	(void)fprintf(stderr, "       blowfish\n");
	(void)fprintf(stderr, "       sha1\n");
	exit(1);
}

void
pwd_gensalt(char *salt, int max, struct passwd *pw, const char *option)
{
	const char *crypto;

	crypto = pwd_crypto(option);
	if (crypto == NULL) {
		(void)printf("Couldn't find specified cryptographic algorithm.\n");
	}

	if (crypto == NULL) {
		goto bad;
	}

#ifdef NOPWGENSALT
	*salt = '\0';

	/* grab a random printable character that isn't a colon */
	(void)srandom((int)time((time_t *)NULL));
	rval = pwd_gensalt_old(salt, max, pw, crypto);
#else
	rval = pw_gensalt(salt, max, pw, crypto);
#endif
	if (!rval) {
bad:
		(void)printf("Couldn't generate salt.\n");
	}
}

#ifdef NOPWGENSALT

static void
to64(char *s, long v, int n)
{
	while (--n >= 0) {
		*s++ = itoa64[v & 0x3f];
		v >>= 6;
	}
}

static int
pwd_gensalt_old(char *salt, int max, struct passwd *pw, const char *option)
{
	const char *crypto;
	int i, rval;

	crypto = pwd_crypto(option);
	if (crypto == NULL) {
		(void)printf("Couldn't find specified cryptographic algorithm.\n");
		(void)printf("Known cryptographic algorithms: old, new, md5, sha1.\n");
	}

	for (i = 0; i < nelems(pw_options); i++) {
		switch (i) {
		case 0:
			rval = old_salt(salt, max, pw, crypto);
			break;
		case 1:
			rval = new_salt(salt, max, pw, crypto);
			break;
		case 2:
			rval = md5_salt(salt, max, pw, crypto);
			break;
		case 3:
			rval = blowfish_salt(salt, max, pw, crypto);
			break;
		case 4:
			rval = sha1_salt(salt, max, pw, crypto);
			break;
		default:
			rval = -1;
			break;
		}
		break;
	}

	return (rval);
}

static int
new_salt(char *salt, int max, struct passwd *pw, const char *option)
{
	int rounds;

	rounds = 0;
	if (strcmp(option, "new") == 0) {
		rounds = atol(option);
		if (max < 10) {
			return (0);
		}
		/* Check rounds, 24 bit is max */
		if (rounds < 7250) {
			rounds = 7250;
		} else if (rounds > 0xffffff) {
			rounds = 0xffffff;
		} else {
			rounds = (long)(29 * 25);
		}
		salt[0] = _PASSWORD_EFMT1;
		to64(&salt[1], (long)rounds, 4);
		to64(&salt[5], arc4random(), 4);
		salt[9] = '\0';
		return (1);
	}
	return (0);
}

static int
old_salt(char *salt, int max, struct passwd *pw, const char *option)
{
	/*
	while ((salt[0] = random() % 93 + 33) == ':');
	while ((salt[1] = random() % 93 + 33) == ':');
	*/
	if (strcmp(option, "old") == 0) {
		if (max < 3) {
			return (0);
		}
		to64(&salt[0], (arc4random() % 93 + 33), 2);
		salt[2] = '\0';
		return (1);
	}
	return (0);
}

static int
md5_salt(char *salt, int max, struct passwd *pw, const char *option)
{
	if (strcmp(option, "md5") == 0) {
		if (max < 13) {  /* $1$8salt$\0 */
			return (0);
		}
		salt[0] = _PASSWORD_NONDES;
		salt[1] = '1';
		salt[2] = '$';
		to64(&salt[3], arc4random(), 4);
		to64(&salt[7], arc4random(), 4);
		salt[11] = '$';
		salt[12] = '\0';
		return (1);
	}
	return (0);
}

static int
blowfish_salt(char *salt, int max, struct passwd *pw, const char *option)
{
	int rounds;

	rounds = 0;
	if (strcmp(option, "blowfish") == 0) {
		rounds = atoi(option);
		if (rounds < 4) {
			rounds = 4;
		}
		strlcpy(salt, bcrypt_gensalt(rounds), max);
		return (1);
	}
	return (0);
}

static int
sha1_salt(char *salt, int max, struct passwd *pw, const char *option)
{
	if (strcmp(option, "sha1") == 0) {
		(void)printf("sha1 is not implemented.\n");
		return (0);
	}
	return (0);
}
#endif
