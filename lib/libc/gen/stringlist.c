/*	$NetBSD: stringlist.c,v 1.10 2000/01/25 16:24:40 enami Exp $	*/

/*-
 * Copyright (c) 1994, 1999 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: stringlist.c,v 1.10 2000/01/25 16:24:40 enami Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stringlist.h>

#ifdef __weak_alias
__weak_alias(sl_add,_sl_add)
__weak_alias(sl_find,_sl_find)
__weak_alias(sl_free,_sl_free)
__weak_alias(sl_init,_sl_init)
//__weak_alias(sl_delete,_sl_delete)
#endif

#define _SL_CHUNKSIZE	20

/*
 * sl_init(): Initialize a string list
 */
StringList *
sl_init(void)
{
	StringList *sl;

	sl = malloc(sizeof(StringList));
	if (sl == NULL)
		return (NULL);

	sl->sl_cur = 0;
	sl->sl_max = _SL_CHUNKSIZE;
	sl->sl_str = malloc(sl->sl_max * sizeof(char *));
	if (sl->sl_str == NULL) {
		free(sl);
		sl = NULL;
	}
	return (sl);
}


/*
 * sl_add(): Add an item to the string list
 */
int
sl_add(sl, name)
	StringList *sl;
	char *name;
{

	_DIAGASSERT(sl != NULL);

	if (sl->sl_cur == sl->sl_max - 1) {
		char	**new;

		new = (char **)realloc(sl->sl_str,
		    (sl->sl_max + _SL_CHUNKSIZE) * sizeof(char *));
		if (new == NULL)
			return (-1);
		sl->sl_max += _SL_CHUNKSIZE;
		sl->sl_str = new;
	}
	sl->sl_str[sl->sl_cur++] = name;
	return (0);
}


/*
 * sl_free(): Free a stringlist
 */
void
sl_free(sl, all)
	StringList *sl;
	int all;
{
	size_t i;

	if (sl == NULL)
		return;
	if (sl->sl_str) {
		if (all)
			for (i = 0; i < sl->sl_cur; i++)
				free(sl->sl_str[i]);
		free(sl->sl_str);
	}
	free(sl);
}


/*
 * sl_find(): Find a name in the string list
 */
char *
sl_find(sl, name)
	StringList *sl;
	const char *name;
{
	size_t i;

	_DIAGASSERT(sl != NULL);

	for (i = 0; i < sl->sl_cur; i++)
			/*
			 * XXX check sl->sl_str[i] != NULL?
			 */
		if (strcmp(sl->sl_str[i], name) == 0)
			return (sl->sl_str[i]);

	return (NULL);
}

int
sl_delete(sl, name, all)
    StringList *sl;
    const char *name;
    int all;
{
	size_t i, j;

	for (i = 0; i < sl->sl_cur; i++)
		if (strcmp(sl->sl_str[i], name) == 0) {
			if (all)
				free(sl->sl_str[i]);
			for (j = i + 1; j < sl->sl_cur; j++)
				sl->sl_str[j - 1] = sl->sl_str[j];
			sl->sl_str[--sl->sl_cur] = NULL;
			return 0;
		}
	return -1;
}
