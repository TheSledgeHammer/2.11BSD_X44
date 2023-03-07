/*	$OpenBSD: aes.c,v 1.2 2020/07/22 13:54:30 tobhe Exp $	*/
/*
 * Copyright (c) 2016 Thomas Pornin <pornin@bolet.org>
 *
 * Modified for OpenBSD by Thomas Pornin and Mike Belopuhov.
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

#include <sys/cdefs.h>
#include <sys/types.h>

#include <crypto/rijndael/rijndael.h>
#include <crypto/aes/aes.h>

void
AES_Setkey(aes_ctx *ctx, const uint8_t *key, int len)
{
	ctx->aes_rounds = AES_KeySetup_Encrypt(ctx->aes_ek, key, len);
	AES_KeySetup_Decrypt(ctx->aes_dk, key, len);
}

void
AES_Encrypt(aes_ctx *ctx, const uint8_t *src, uint8_t *dst)
{
	rijndaelEncrypt(ctx->aes_ek, ctx->aes_rounds, src, dst);
}

void
AES_Decrypt(aes_ctx *ctx, const uint8_t *src, uint8_t *dst)
{
	rijndaelDecrypt(ctx->aes_dk, ctx->aes_rounds, src, dst);
}

int
AES_KeySetup_Encrypt(uint32_t *skey, const uint8_t *key, int len)
{
	int r;

	r = rijndaelKeySetupEnc(skey, key, len);
	if (r == 0) {
		return (0);
	}
	return (r);
}

int
AES_KeySetup_Decrypt(uint32_t *skey, const uint8_t *key, int len)
{
	int r;

	r = rijndaelKeySetupDec(skey, key, len);
	if (r == 0) {
		return (0);
	}
	return (r);
}
