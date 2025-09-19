/*-
 * Copyright (c) 2024
 *	Martin Kelly. All rights reserved.
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

#ifndef _CITRUS_FRUNE_H_
#define _CITRUS_FRUNE_H_

/* frune */
struct _citrus_frune_encoding {
	_ENCODING_INFO 		*fe_info;
	_ENCODING_STATE 	*fe_state;
	_RuneLocale		*fe_runelocale;
};

/* prototypes */
__BEGIN_DECLS
int _citrus_frune_open(struct _citrus_frune_encoding **, char *, void *, size_t);
void _citrus_frune_close(struct _citrus_frune_encoding *);
void _citrus_frune_save_encoding_state(struct _citrus_frune_encoding *, void *, void *);
void _citrus_frune_restore_encoding_state(struct _citrus_frune_encoding *, void *, void *);
void _citrus_frune_init_encoding_state(struct _citrus_frune_encoding *, void *);
int _citrus_frune_mbtocsx(struct _citrus_frune_encoding *, _csid_t *, _index_t *, const char **, size_t, size_t *);
int _citrus_frune_cstombx(struct _citrus_frune_encoding *, char *, size_t, _csid_t, _index_t, size_t *);
int _citrus_frune_wctombx(struct _citrus_frune_encoding *, char *, size_t, _wc_t, size_t *);
int _citrus_frune_put_state_resetx(struct _citrus_frune_encoding *, char *, size_t, size_t *);
int _citrus_frune_get_state_desc_gen(struct _citrus_frune_encoding *, int *);
int _citrus_getops(void *, const void *, size_t, u_int32_t, u_int32_t);
__END_DECLS

#endif /* _CITRUS_FRUNE_H_ */
