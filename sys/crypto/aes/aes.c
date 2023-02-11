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
#include <sys/systm.h>
#include <sys/types.h>

#include <crypto/rijndael/rijndael.h>
#include <crypto/aes/aes.h>

int
AES_Setkey(AES_CTX *ctx, const uint8_t *key, int len)
{
	ctx->aes_rounds = AES_KeySetup_Encrypt(ctx->aes_ek, key, len);
	if (ctx->aes_rounds == 0) {
		return (-1);
	}
	AES_KeySetup_Decrypt(ctx->aes_dk, key, len);
	return (0);
}

void
AES_Encrypt(AES_CTX *ctx, const uint8_t *src, uint8_t *dst)
{
	rijndaelEncrypt(ctx->aes_ek, ctx->aes_rounds, src, dst);
}

void
AES_Decrypt(AES_CTX *ctx, const uint8_t *src, uint8_t *dst)
{
	rijndaelDecrypt(ctx->aes_dk, ctx->aes_rounds, src, dst);
}

int
AES_KeySetup_Encrypt(uint32_t *skey, const uint8_t *key, int len)
{
	unsigned r, u;
	uint32_t tkey[60];

	r = rijndaelKeySetupEnc(tkey, key, len);
	if (r == 0) {
		return (0);
	}
	for (u = 0; u < ((r + 1) << 2); u ++) {
		uint32_t w;

		w = tkey[u];
		skey[u] = (w << 24)
			| ((w & 0x0000FF00) << 8)
			| ((w & 0x00FF0000) >> 8)
			| (w >> 24);
	}
	return (r);
}

/*
 * Reduce value x modulo polynomial x^8+x^4+x^3+x+1. This works as
 * long as x fits on 12 bits at most.
 */
static inline uint32_t
redgf256(uint32_t x)
{
	uint32_t h;

	h = x >> 8;
	return (x ^ h ^ (h << 1) ^ (h << 3) ^ (h << 4)) & 0xFF;
}

/*
 * Multiplication by 0x09 in GF(256).
 */
static inline uint32_t
mul9(uint32_t x)
{
	return redgf256(x ^ (x << 3));
}

/*
 * Multiplication by 0x0B in GF(256).
 */
static inline uint32_t
mulb(uint32_t x)
{
	return redgf256(x ^ (x << 1) ^ (x << 3));
}

/*
 * Multiplication by 0x0D in GF(256).
 */
static inline uint32_t
muld(uint32_t x)
{
	return redgf256(x ^ (x << 2) ^ (x << 3));
}

/*
 * Multiplication by 0x0E in GF(256).
 */
static inline uint32_t
mule(uint32_t x)
{
	return redgf256((x << 1) ^ (x << 2) ^ (x << 3));
}

int
AES_KeySetup_Decrypt(uint32_t *skey, const uint8_t *key, int len)
{
	unsigned r, u;
	uint32_t tkey[60];

	r = rijndaelKeySetupDec(tkey, key, len);
	if (r == 0) {
		return (0);
	}

	for (u = 4; u < (r << 2); u ++) {
		uint32_t sk, sk0, sk1, sk2, sk3;
		uint32_t tk, tk0, tk1, tk2, tk3;

		sk = tkey[u];
		sk0 = sk >> 24;
		sk1 = (sk >> 16) & 0xFF;
		sk2 = (sk >> 8) & 0xFF;
		sk3 = sk & 0xFF;
		tk0 = mule(sk0) ^ mulb(sk1) ^ muld(sk2) ^ mul9(sk3);
		tk1 = mul9(sk0) ^ mule(sk1) ^ mulb(sk2) ^ muld(sk3);
		tk2 = muld(sk0) ^ mul9(sk1) ^ mule(sk2) ^ mulb(sk3);
		tk3 = mulb(sk0) ^ muld(sk1) ^ mul9(sk2) ^ mule(sk3);
		tk = (tk0 << 24) ^ (tk1 << 16) ^ (tk2 << 8) ^ tk3;
		skey[((r - (u >> 2)) << 2) + (u & 3)] = tk;
	}

	return (r);
}
