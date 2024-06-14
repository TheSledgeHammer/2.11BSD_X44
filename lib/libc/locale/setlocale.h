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

extern char *PathLocale;

__BEGIN_DECLS
void addrunelocale(_RuneLocale *, char *, int (*)(_RuneLocale *));
void delrunelocale(char *);
int	newrunelocale(_RuneLocale *);
__END_DECLS

#endif /* !_SETLOCALE_H_ */
