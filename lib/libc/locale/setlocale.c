/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)setlocale.c	8.1 (Berkeley) 7/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <rune.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "collate.h"    /* for __collate_load_locale() */
#include "lmonetary.h"	/* for __monetary_load_locale() */
#include "lnumeric.h"	/* for __numeric_load_locale() */
#include "lmessages.h"	/* for __messages_load_locale() */
#include "ltime.h"		/* for __time_load_locale() */
#include "ldpart.h"
#include "setlocale.h"

/*
 * Category names for getenv()
 */
static const char *categories[_LC_LAST] = {
		"LC_ALL",
		"LC_COLLATE",
		"LC_CTYPE",
		"LC_MONETARY",
		"LC_NUMERIC",
		"LC_TIME",
		"LC_MESSAGES",
};

/*
 * Current locales for each category
 */
static const char current_categories[_LC_LAST][32] = {
		"C",
		"C",
		"C",
		"C",
		"C",
		"C",
		"C",
};

/*
 * The locales we are going to try and load
 */
static char new_categories[_LC_LAST][32];
static char current_locale_string[_LC_LAST * 33];
static char *__setlocale(int, const char *);
static char	*currentlocale(void);
static char	*loadlocale(int);
static void	showpathlocale(const char *, int);

static char *
__setlocale(category, locale)
	int category;
	const char *locale;
{
	int found, i, len;
	const char *env;
    char *r;

	if (category < LC_ALL || category >= _LC_LAST) {
		errno = EINVAL;
		return (NULL);
	}

	if (!PathLocale && !(PathLocale = getenv("PATH_LOCALE")))
		PathLocale = _PATH_LOCALE;

	if (category < 0 || category >= _LC_LAST)
		return (NULL);

	if (!locale)
		return (category ? __UNCONST(current_categories[category]) : currentlocale());

	/*
	 * Default to the current locale for everything.
	 */
	for (i = 1; i < _LC_LAST; ++i)
		(void) strcpy(new_categories[i], current_categories[i]);

	/*
	 * Now go fill up new_categories from the locale argument
	 */
	if (!*locale) {
		env = getenv(categories[category]);

		if (!env)
			env = getenv(categories[0]);

		if (!env)
			env = getenv("LANG");

		if (!env)
			env = _C_LOCALE;

		(void) strncpy(new_categories[category], env, 31);
		new_categories[category][31] = 0;
		if (!category) {
			for (i = 1; i < _LC_LAST; ++i) {
				if (!(env = getenv(categories[i])))
					env = new_categories[0];
				(void) strncpy(new_categories[i], env, 31);
				new_categories[i][31] = 0;
			}
		}
	} else if (category) {
		(void) strncpy(new_categories[category], locale, 31);
		new_categories[category][31] = 0;
	} else {
		if ((r = strchr(locale, '/')) == 0) {
			for (i = 1; i < _LC_LAST; ++i) {
				(void) strncpy(new_categories[i], locale, 31);
				new_categories[i][31] = 0;
			}
		} else {
			for (i = 1; r[1] == '/'; ++r)
				;
			if (!r[1])
				return (NULL); /* Hmm, just slashes... */
			do {
				len = r - locale > 31 ? 31 : r - locale;
				(void) strncpy(new_categories[i++], locale, len);
				new_categories[i++][len] = 0;
				locale = r;
				while (*locale == '/')
					++locale;
				while (*++r && *r != '/')
					;
			} while (*locale);
			while (i < _LC_LAST)
				(void) strcpy(new_categories[i], new_categories[i - 1]);
		}
	}

	if (category)
		return (loadlocale(category));

	found = 0;
	for (i = 1; i < _LC_LAST; ++i)
		if (loadlocale(i) != NULL)
			found = 1;
	if (found)
		return (currentlocale());
	return (NULL);
}

char *
setlocale(category, locale)
	int category;
	const char *locale;
{
	/* locale may be NULL */
	__mb_len_max_runtime = MB_LEN_MAX;
	return (__setlocale(category, locale));
}

static char *
currentlocale(void)
{
	int i;

	(void)strcpy(current_locale_string, current_categories[1]);

	for (i = 2; i < _LC_LAST; ++i) {
		if (strcmp(current_categories[1], current_categories[i])) {
			(void)snprintf(current_locale_string,
			    sizeof(current_locale_string), "%s/%s/%s/%s/%s",
			    current_categories[1], current_categories[2],
			    current_categories[3], current_categories[4],
			    current_categories[5]);
			break;
		}
	}
	return (current_locale_string);
}

static char *
loadlocale(category)
	int category;
{
	locale_t locale;
	char *newcat;
	char *curcat;
	int (*func)(const char *);

	locale = &global_locale;
	newcat = new_categories[category];
	curcat = __UNCONST(current_categories[category]);
    func = NULL;

	if (strcmp(newcat, curcat) == 0) {
		return (curcat);
	}

	if (category == LC_CTYPE) {
		if (setrunelocale(newcat)) {
			return (NULL);
		}
		(void)strcpy(curcat, newcat);
		return (curcat);
	}
	if (!strcmp(newcat, _C_LOCALE) || !strcmp(newcat, _POSIX_LOCALE)) {
		/*
		 * Some day this will need to reset the locale to the default
		 * C locale.  Since we have no way to change them as of yet,
		 * there is no need to reset them.
		 */
		(void)strcpy(curcat, newcat);
		return (curcat);
	}

	/*
	 * Some day we will actually look at this file.
	 */
	showpathlocale(PathLocale, category);

	switch (category) {
	case LC_COLLATE:
		func = __collate_load_tables;
		break;
	case LC_MONETARY:
		func = __monetary_load_locale;
		break;
	case LC_NUMERIC:
		func = __numeric_load_locale;
		break;
	case LC_TIME:
		func = __time_load_locale;
		break;
	case LC_MESSAGES:
		func = __messages_load_locale;
		break;
	}

	if (strcmp(newcat, curcat) == 0) {
		return (curcat);
	}

	if (func(newcat) != _LDP_ERROR) {
		(void)strcpy(curcat, newcat);
		(void)strcpy(locale->part_category[category], newcat);
		return (curcat);
	}

	return (NULL);
}

static void
showpathlocale(path, category)
	const char *path;
	int category;
{
	char name[PATH_MAX];

	(void)snprintf(name, sizeof(name), "%s/%s/%s", path, new_categories[category], categories[category]);
}

const char *
__get_locale_env(category)
	int category;
{
        const char *env;

        /* 1. check LC_ALL. */
        env = getenv(categories[0]);

        /* 2. check LC_* */
        if (env == NULL || !*env) {
            env = getenv(categories[category]);
        }

        /* 3. check LANG */
        if (env == NULL || !*env) {
            env = getenv("LANG");
        }

        /* 4. if none is set, fall to "C" */
        if (env == NULL || !*env) {
            env = _C_LOCALE;
        }

        return (env);
}
