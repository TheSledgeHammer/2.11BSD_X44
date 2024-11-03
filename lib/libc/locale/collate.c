/*-
 * Copyright (c) 1995 Alex Tatmanjants <alex@elvisti.kiev.ua>
 *		at Electronni Visti IA, Kiev, Ukraine.
 *			All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__FBSDID("$FreeBSD$");
#endif

#include "namespace.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sysexits.h>
#include <locale.h>

#include "collate.h"
#include "ldpart.h"
#include "setlocale.h"

int __collate_load_error = 1;
int __collate_substitute_nontrivial;
char __collate_version[STR_LEN];
char __collate_substitute_table[UCHAR_MAX + 1][STR_LEN];
struct __collate_st_char_pri __collate_char_pri_table[UCHAR_MAX + 1];
struct __collate_st_chain_pri *__collate_chain_pri_table;

void __collate_err(int, const char *);

int
__collate_load_tables(const char *encoding)
{
	FILE *fp;
	int i, saverr, chains;
	char buf[PATH_MAX];
	static char collate_encoding[32];

	/* 'encoding' must be already checked. */
	if (strcmp(encoding, _C_LOCALE) == 0 || strcmp(encoding, _POSIX_LOCALE) == 0) {
		__collate_load_error = 1;
		return (_LDP_CACHE);
	}

	/*
	 * If the locale name is the same as our cache, use the cache.
	 */
	if (strcmp(encoding, collate_encoding) == 0) {
		__collate_load_error = 0;
		return (_LDP_CACHE);
	}

	/*
	 * Slurp the locale file into the cache.
	 */

	/* 'PathLocale' must be already set & checked. */
	/* Range checking not needed, encoding has fixed size */
	(void)strcpy(buf, PathLocale);
	(void)strcat(buf, "/");
	(void)strcat(buf, encoding);
	(void)strcat(buf, "/LC_COLLATE");
	if ((fp = fopen(buf, "r")) == NULL) {
		return (_LDP_ERROR);
	}
	if (fread(__collate_version, sizeof(__collate_version), 1, fp) != 1) {
		saverr = errno;
		(void)fclose(fp);
		errno = saverr;
		return (_LDP_ERROR);
	}
	chains = -1;
	if (strcmp(__collate_version, COLLATE_VERSION) == 0) {
		chains = 0;
	} else if (strcmp(__collate_version, COLLATE_VERSION1_1) == 0) {
		chains = 1;
	}
	if (chains < 0) {
		(void)fclose(fp);
		errno = EFTYPE;
		return (_LDP_ERROR);
	}

#define FREAD(a, b, c, d) 				\
	do { 								\
		if (fread(a, b, c, d) != c) { 	\
			saverr = errno; 			\
			fclose(d); 					\
			errno = saverr; 			\
			return (_LDP_ERROR); 		\
		} 								\
	} while (0)

	FREAD(__collate_substitute_table, sizeof(__collate_substitute_table), 1, fp);
	FREAD(__collate_char_pri_table, sizeof(__collate_char_pri_table), 1, fp);
	FREAD(__collate_chain_pri_table, sizeof(__collate_chain_pri_table), 1, fp);
	(void)fclose(fp);

	(void)strcpy(collate_encoding, encoding);
	for (i = 0; i < UCHAR_MAX + 1; i++) {
		if (__collate_substitute_table[i][0] != i ||
		    __collate_substitute_table[i][1] != 0) {
			__collate_substitute_nontrivial = 1;
			break;
		}
	}
	__collate_load_error = 0;

	return (_LDP_LOADED);
}

char *
__collate_substitute(s)
    const char *s;
{
    int dest_len, len, nlen;
    int delta = strlen(s);
    char *dest_str = NULL;
    if (s == NULL || *s == '\0') {
        return (__collate_strdup(""));
    }
    delta += delta / 8;
	dest_str = malloc(dest_len = delta);
    if (dest_str == NULL) {
		__collate_err(EX_OSERR, __FUNCTION__);
    }
	len = 0;
    while (*s) {
        nlen = len + strlen(__collate_substitute_table[(const u_char)*s]);
        if (dest_len <= nlen) {
			dest_str = realloc(dest_str, dest_len = nlen + delta);
			if (dest_str == NULL) {
				__collate_err(EX_OSERR, __FUNCTION__);
            }
		}
        (void)strcpy(dest_str + len, __collate_substitute_table[(const u_char)*s++]);
		len = nlen;
    }
    return (dest_str);
}

void
__collate_lookup(t, len, prim, sec)
	const char *t;
	int *len, *prim, *sec;
{
	struct __collate_st_chain_pri *p2;

	*len = 1;
	*prim = *sec = 0;
	for (p2 = __collate_chain_pri_table; p2->str[0] != '\0'; p2++) {
		if (*t == p2->str[0] &&
		    strncmp(t, p2->str, strlen(p2->str)) == 0) {
			*len = strlen(p2->str);
			*prim = p2->prim;
			*sec = p2->sec;
			return;
		}
	}
	*prim = __collate_char_pri_table[(const u_char)*t].prim;
	*sec = __collate_char_pri_table[(const u_char)*t].sec;
}

char *
__collate_strdup(s)
	const char *s;
{
	char *t = strdup(s);

	if (t == NULL) {
		__collate_err(EX_OSERR, __FUNCTION__);
    }
	return (t);
}

void
__collate_err(int ex, const char *f)
{
	const char *s;
	int serrno = errno;

	s = _getprogname();
	write(STDERR_FILENO, s, strlen(s));
	write(STDERR_FILENO, ": ", 2);
	s = f;
	write(STDERR_FILENO, s, strlen(s));
	write(STDERR_FILENO, ": ", 2);
	s = strerror(serrno);
	write(STDERR_FILENO, s, strlen(s));
	write(STDERR_FILENO, "\n", 1);
	exit(ex);
}

#ifdef COLLATE_DEBUG
void
__collate_print_tables(void)
{
	int i;
	struct __collate_st_chain_pri *p2;

	printf("Substitute table:\n");
	for (i = 0; i < UCHAR_MAX + 1; i++)
	    if (i != *__collate_substitute_table[i])
		printf("\t'%c' --> \"%s\"\n", i,
		       __collate_substitute_table[i]);
	printf("Chain priority table:\n");
	for (p2 = __collate_chain_pri_table; p2->str[0] != '\0'; p2++)
		printf("\t\"%s\" : %d %d\n", p2->str, p2->prim, p2->sec);
	printf("Char priority table:\n");
	for (i = 0; i < UCHAR_MAX + 1; i++)
		printf("\t'%c' : %d %d\n", i, __collate_char_pri_table[i].prim,
		       __collate_char_pri_table[i].sec);
}
#endif
