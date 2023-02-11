/*	$OpenBSD: aes.h,v 1.4 2020/07/22 13:54:30 tobhe Exp $	*/
/*
 * Copyright (c) 2016 Thomas Pornin <pornin@bolet.org>
 * Copyright (c) 2016 Mike Belopuhov
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Modified AES encryption. Combines the rijndael keysetup with OpenBSD's aes keysetup.
 */

#ifndef	_CRYPTO_AES_AES_H
#define	_CRYPTO_AES_AES_H

#ifndef AES_MAXROUNDS
#define AES_MAXROUNDS	(14)
#endif

typedef struct {
	uint32_t		aes_ek[60]; 	/* encrypt key schedule */
	uint32_t		aes_dk[60];		/* decrypt key schedule */
	unsigned 		aes_rounds;
} AES_CTX;

int		AES_Setkey(AES_CTX *, const uint8_t *, int);
void	AES_Encrypt(AES_CTX *, const uint8_t *, uint8_t *);
void	AES_Decrypt(AES_CTX *, const uint8_t *, uint8_t *);

int		AES_KeySetup_Encrypt(uint32_t *, const uint8_t *, int);
int		AES_KeySetup_Decrypt(uint32_t *, const uint8_t *, int);

#endif /* _CRYPTO_AES_AES_H */
