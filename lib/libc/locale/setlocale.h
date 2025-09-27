/*-
 * Copyright (c)1999 Citrus Project,
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
 */

#ifndef _SETLOCALE_H_
#define	_SETLOCALE_H_

#include <locale.h>

/* Encoding Name */
#define ENCODING_UTF8 		"UTF-8"
#define ENCODING_UTF1632 	"UTF-1632"
#define ENCODING_UES 		"UES"
#define ENCODING_UTF2 		"UTF-2"
#define ENCODING_ISO2022 	"ISO2022"
#define ENCODING_NONE 		"NONE"
#define ENCODING_EUC 		"EUC"

#define _C_LOCALE			"C"
#define _POSIX_LOCALE		"POSIX"

#define ENCODING_LEN 		31

extern const char 		*PathLocale;

typedef void 			*locale_part_t;

struct _locale {
    struct lconv 		*part_lconv;
    const char 			*part_name[_LC_LAST];
    char 			part_category[_LC_LAST][ENCODING_LEN+1];
    locale_part_t 		part_impl[_LC_LAST];
};

static __inline struct _locale *
__get_locale(void)
{
	return (&global_locale);
}

__BEGIN_DECLS

#include <runetype.h>

int			_NONE_init(_RuneLocale *);
int			_UES_init(_RuneLocale *);
int			_UTF1632_init(_RuneLocale *);
int			_UTF2_init(_RuneLocale *);
int			_UTF8_init(_RuneLocale *);
int			_EUC_init(_RuneLocale *);
int			_ISO2022_init(_RuneLocale *);

/* rune.c */
void        		wctype_init(_RuneLocale *);
void	    		wctrans_init(_RuneLocale *);

/* setlocale.c */
const char 		*__get_locale_env(int);

/* setrunelocale.c */
void 			addrunelocale(_RuneLocale *, const char *, int (*)(_RuneLocale *));
void 			delrunelocale(const char *);
int			newrunelocale(_RuneLocale *);
_RuneLocale 		*findrunelocale(const char *);
int			validrunelocale(_RuneLocale *, const char *, void *, int);
int			convertrunelocale(_RuneLocale *, const char *, _RuneLocale *, const char *);
__END_DECLS

#endif /* !_SETLOCALE_H_ */
