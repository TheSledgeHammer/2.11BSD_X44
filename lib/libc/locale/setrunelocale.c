/*-
 * Copyright (c) 1993
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/queue.h>

#include <ctype.h>
#include <rune.h>
#include <stdio.h>
#include <stdlib.h>

#include "runefile.h"
#include "setlocale.h"

extern int			_none_init(_RuneLocale *);
extern int			_UES_init(_RuneLocale *);
extern int			_UTF1632_init(_RuneLocale *);
extern int			_UTF2_init(_RuneLocale *);
extern int			_UTF8_init(_RuneLocale *);
extern int			_EUC_init(_RuneLocale *);
extern int			_ISO2022_init(_RuneLocale *);

struct localetable_head;
LIST_HEAD(localetable_head, localetable);
struct localetable {
	char 		encoding[32];
	int 		(*init)(_RuneLocale *);
	_RuneLocale *runelocale;
	LIST_ENTRY(localetable) next;
};

struct localetable_head lthead;// = LIST_HEAD_INITIALIZER(lthead);

struct localetable *
lookuplocaletable(encoding)
	char *encoding;
{
    struct localetable *lt = NULL;

    LIST_FOREACH(lt, &lthead, next) {
        if ((!strcmp(encoding, lt->encoding))) {
            return (lt);
        }
    }
    return (NULL);
}

int
initlocaletable(encoding)
	char *encoding;
{
	struct localetable *lt;
	int ret;

	lt = lookuplocaletable(encoding);
	if (lt == NULL) {
		ret = ENOMEM;
	} else {
		ret = (*lt->init)(lt->runelocale);
	}

	return (ret);
}

void
loadlocaletable(rl)
	_RuneLocale *rl;
{
	addrunelocale(rl, ENCODING_UTF8, _UTF8_init);
	addrunelocale(rl, ENCODING_UTF1632, _UTF1632_init);
	addrunelocale(rl, ENCODING_UES, _UES_init);
	addrunelocale(rl, ENCODING_UTF2, _UTF2_init);
	addrunelocale(rl, ENCODING_ISO2022, _ISO2022_init);
	addrunelocale(rl, ENCODING_NONE, _none_init);
	addrunelocale(rl, ENCODING_EUC, _EUC_init);
}

void
addrunelocale(rl, encoding, init)
	_RuneLocale *rl;
	char *encoding;
	int (*init)(_RuneLocale *);
{
	struct localetable *lt;

	lt = (struct localetable *)malloc(sizeof(struct localetable));
	lt->runelocale = rl;
	lt->encoding = encoding;
	lt->init = init;
	LIST_INSERT_HEAD(&lthead, lt, next);
}

void
delrunelocale(encoding)
	char *encoding;
{
	struct localetable *lt;

	LIST_FOREACH(lt, &lthead, next) {
		if (strcmp(encoding, lt->encoding) == 0) {
			LIST_REMOVE(lt, next);
		}
	}
}

int
initrunelocale(rl)
	_RuneLocale *rl;
{
	if (!rl->encoding[0] || !strcmp(rl->encoding, ENCODING_UTF8)) {
		return (initlocaletable(ENCODING_UTF8));
	} else if (!strcmp(rl->encoding, ENCODING_UTF1632)) {
		return (initlocaletable(ENCODING_UTF1632));
	} else if (!strcmp(rl->encoding, ENCODING_UES)) {
		return (initlocaletable(ENCODING_UES));
	} else if (!strcmp(rl->encoding, ENCODING_UTF2)) {
		return (initlocaletable(ENCODING_UTF2));
	} else if (!strcmp(rl->encoding, ENCODING_ISO2022)) {
		return (initlocaletable(ENCODING_ISO2022));
	} else if (!strcmp(rl->encoding, ENCODING_NONE)) {
		return (initlocaletable(ENCODING_NONE));
	} else if (!strcmp(rl->encoding, ENCODING_EUC)) {
		return (initlocaletable(ENCODING_EUC));
	} else {
		return (EINVAL);
	}
}

int
newrunelocale(rl)
	_RuneLocale *rl;
{
	int ret;

	LIST_INIT(&lthead);

	/* load table */
	loadlocaletable(rl);
	/* initialize runes from table */
	ret = initrunelocale(rl);
	if (ret != 0) {
		return (ret);
	}
	return (0);
}
