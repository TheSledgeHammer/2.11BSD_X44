/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2011 The FreeBSD Foundation
 *
 * This software was developed by David Chisnall under sponsorship from
 * the FreeBSD Foundation.
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
 */

#include <sys/cdefs.h>

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "ldpart.h"
#include "lmessages.h"
#include "lmonetary.h"
#include "lnumeric.h"
#include "ltime.h"
#include "setlocale.h"

static locale_part_t
locale_part_loader(locale, name, category, func)
	locale_t locale;
	const char *name;
	int category;
	int (*func)(const char *name);
{
	locale_part_t part[_LC_LAST];
	int ret;

	ret = func(name);
	if (ret != _LDP_ERROR) {
		part[category] = (locale_part_t)func;
		locale->part_impl[category] = part[category];
		return (locale->part_impl[category]);
	}
	return (NULL);
}

static locale_part_t
locale_loader(locale, name, category)
	locale_t locale;
	const char *name;
	int category;
{
	switch (category) {
	case LC_ALL:
		return ((locale_part_t)NULL);
	case LC_COLLATE:
		return ((locale_part_t)NULL);
	case LC_CTYPE:
		return ((locale_part_t)_CurrentRuneLocale);
	case LC_MONETARY:
		return (locale_part_loader(locale, name, category, __monetary_load_locale));
	case LC_NUMERIC:
		return (locale_part_loader(locale, name, category, __numeric_load_locale));
	case LC_TIME:
		return (locale_part_loader(locale, name, category, __time_load_locale));
	case LC_MESSAGES:
		return (locale_part_loader(locale, name, category, __messages_load_locale));
	}
	return (NULL);
}

static locale_part_t
find_part(locale, name, category)
	locale_t locale;
	const char *name;
	int category;
{
    if (category >= LC_ALL && category < LC_LAST) {
        if (strcmp(locale->part_category[category], name) == 0) {
            return (locale_loader(locale, name, category));
        }
    }
    return (NULL);
}

static int
duppart(category, src, dst)
	int category;
	locale_t src, dst;
{
	if (&global_locale == src) {
		dst->part_impl[category] = find_part(locale, src->part_category[category], category);
		if (dst->part_impl[category]) {
			strncpy(dst->part_category[category], src->part_category[category], ENCODING_LEN);
		}
	} else if (src->part_category[category]) {
		dst->part_impl[category] = src->part_impl[category];
	} else {
        return (1);
    }
	return (0 != dst->part_category[category]);
}

locale_t
newlocale(mask, name, src)
	int mask;
	const char *name;
	locale_t src;
{
	struct locale *dst;
	const char *realname;
    int useenv = 0;
    int success = 1;
	int i;

	realname = name;

	if (name == NULL) {
		realname = "C";
	} else if ('\0' == name[0]) {
		useenv = 1;
	}

	dst = malloc(sizeof(*dst));
	if (dst == NULL) {
		return (NULL);
	}
	if (src == NULL) {
		src = __get_locale();
	}

	memcpy(dst, src, sizeof(*src));

	for (i = 1; i < _LC_LAST; ++i) {
		if (mask & (1 << i)) {
			if (useenv) {
				realname = __get_locale_env(i);
			}
			dst->part_impl[i] = find_part(dst, realname, i);
			if (dst->part_impl[i]) {
				if (strcmp(realname, "C") != 0) {
					strncpy(dst->part_category[i], realname, ENCODING_LEN);
				}
			}
		} else {
			if (!duppart(i, src, dst)) {
                success = 0;
                break;
			}
		}
		mask >>= 1;
	}
    if (success == 0) {
        dst = NULL;
    }
	return (dst);
}

void
freelocale(locale)
	locale_t locale;
{
	_DIAGASSERT(locale != &global_locale);
	_DIAGASSERT(locale != &c_locale);
	_DIAGASSERT(locale != NULL);
	free(locale);
}

locale_t
duplocale(src)
	locale_t src;
{
	struct _locale *dst;

	dst = malloc(sizeof(*dst));
	if (dst != NULL) {
		memcpy(dst, src, sizeof(*dst));
	}
	return (dst);
}
