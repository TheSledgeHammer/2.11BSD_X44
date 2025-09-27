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

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/queue.h>

#include <errno.h>
#include <rune.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "setlocale.h"

struct localetable {
	const char 		*encoding;
	int 		(*init)(_RuneLocale *);
	_RuneLocale *runelocale;
	LIST_ENTRY(localetable) next;
};

LIST_HEAD(localetable_head, localetable) lthead = LIST_HEAD_INITIALIZER(lthead);

struct localetable *lookuplocaletable(const char *);
int         initlocaletable(const char *);
void        loadlocaletable(_RuneLocale *);
int         initrunelocale(_RuneLocale *);


struct localetable *
lookuplocaletable(encoding)
    const char *encoding;
{
    struct localetable *lt;

    LIST_FOREACH(lt, &lthead, next) {
        if ((!strcmp(encoding, lt->encoding))) {
            return (lt);
        }
    }
    return (NULL);
}

int
initlocaletable(encoding)
    const char *encoding;
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
	addrunelocale(rl, ENCODING_NONE, _NONE_init);
	addrunelocale(rl, ENCODING_EUC, _EUC_init);
}

/* add rune to table */
void
addrunelocale(rl, encoding, init)
    _RuneLocale *rl;
    const char *encoding;
    int (*init)(_RuneLocale *);
{
	struct localetable *lt;

	lt = (struct localetable *)malloc(sizeof(struct localetable));
	lt->runelocale = rl;
	lt->encoding = encoding;
	lt->init = init;
	LIST_INSERT_HEAD(&lthead, lt, next);
}

/* delete rune from table */
void
delrunelocale(encoding)
    const char *encoding;
{
	struct localetable *lt;

	LIST_FOREACH(lt, &lthead, next) {
		if (strcmp(encoding, lt->encoding) == 0) {
			LIST_REMOVE(lt, next);
		}
	}
}

/* initializes all runes in table */
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

/* create new rune's table */
int
newrunelocale(rl)
    _RuneLocale *rl;
{
	int ret;

	/* load table */
	loadlocaletable(rl);
	/* initialize runes from table */
	ret = initrunelocale(rl);
	if (ret != 0) {
		return (ret);
	}
	return (0);
}

/* search table for rune */
_RuneLocale *
findrunelocale(encoding)
    const char *encoding;
{
	struct localetable *lt;
	_RuneLocale *rl;

	lt = lookuplocaletable(encoding);
	if (lt == NULL) {
		return (NULL);
	}
	rl = lt->runelocale;
	if (rl == NULL) {
		return (NULL);
	}
	return (rl);
}

/* validate rune is not empty */
int
validrunelocale(rl, encoding, variable, variable_len)
	_RuneLocale *rl;
	const char *encoding;
	void *variable;
	int variable_len;
{
	int ret;

	ret = -1;
	if (rl != NULL) {
		if ((strcmp(rl->encoding, encoding) == 0)
				&& (rl->variable == variable)
				&& (rl->variable_len == variable_len)) {
			ret = 0;
		}
		if (rl->ops != NULL) {
			if ((rl->ops->ro_sgetrune != NULL)
					|| (rl->ops->ro_sputrune != NULL)
					|| (rl->ops->ro_sgetmbrune != NULL)
					|| (rl->ops->ro_sputmbrune != NULL)
					|| (rl->ops->ro_sgetcsrune != NULL)
					|| (rl->ops->ro_sputcsrune != NULL)
					|| (rl->ops->ro_module_init != NULL)
					|| (rl->ops->ro_module_uninit != NULL)
					) {
				ret = 0;
			}
		}
	}
	return (ret);
}

/*
 * Convert source rune to destination rune.
 * - Source rune must match the _CurrentRuneLocale.
 */
int
convertrunelocale(src, srcname, dst, dstname)
	_RuneLocale *src, *dst;
	const char *srcname, *dstname;
{
	_RuneLocale *cur;

	cur = _CurrentRuneLocale;
	src = findrunelocale(srcname);
	if (src != NULL) {
		if (src != cur) {
			printf("encoding %s does not match current encoding %s\n", srcname, cur->encoding);
			return (-1);
		}
	} else {
		printf("encoding %s does not exist\n", srcname);
		return (-1);
	}
	dst = findrunelocale(dstname);
	if (dst != NULL) {
		if (dst == src) {
			printf("encoding selected %s is already set to current encoding %s\n", dstname, srcname);
			return (-1);
		}
		printf("encoding selected %s is now set as the current encoding\n", dstname);
		(void)setrunelocale(__UNCONST(dstname));
	} else {
		printf("encoding %s does not exist\n", dstname);
		return (-1);
	}
	return (0);
}
