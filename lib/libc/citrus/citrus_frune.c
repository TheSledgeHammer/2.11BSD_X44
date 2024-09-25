/*-
 * Copyright (c) 2024
 *	Martin Kelly. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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

#include <stddef.h>
#include <stdlib.h>

#include "citrus_rune.h"

/* file rune */

struct frune_encoding {
	_ENCODING_INFO 			*fe_info;
	_ENCODING_STATE 		*fe_state;
	_RuneLocale				*fe_runelocale;
};

int
frune_open(rfe, encoding, variable, lenvar)
	struct frune_encoding **rfe;
	char *encoding;
	void *variable;
	size_t lenvar;
{
	struct frune_encoding *fe;
	int ret;

	fe = malloc(sizeof(*fe));
	if (fe == NULL) {
		frune_close(fe);
		return (errno);
	}

	fe->fe_info = NULL;
	fe->fe_state = NULL;
	fe->fe_runelocale = NULL;

	fe->fe_runelocale = findrunelocale(encoding);
	if (fe->fe_runelocale == NULL) {
		frune_close(fe);
		return (errno);
	}
	ret = validrunelocale(fe->fe_runelocale, encoding, variable, lenvar);
	if (ret != 0) {
		return (ret);
	}

	fe->fe_info = malloc(sizeof(*fe->fe_info));
	if (fe->fe_info == NULL) {
		frune_close(fe);
		return (errno);
	}

	fe->fe_state = malloc(sizeof(*fe->fe_state));
	if (fe->fe_state == NULL) {
		frune_close(fe);
		return (errno);
	}

	*rfe = fe;
	return (0);
}

void
frune_close(fe)
	struct frune_encoding *fe;
{
	if (fe->fe_runelocale != NULL) {
		if (fe->fe_info != NULL) {
			if (fe->fe_state != NULL) {
				free(fe->fe_state);
			}
			free(fe->fe_info);
		}
		free(fe->fe_runelocale);
	}
	free(fe);
}
