/* $NetBSD: aesxcbcmac.c,v 1.3 2020/06/29 23:34:48 riastradh Exp $ */

/*
 * Copyright (C) 1995, 1996, 1997, 1998 and 2003 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aesxcbcmac.c,v 1.3 2020/06/29 23:34:48 riastradh Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <crypto/aes/aes.h>
#include <crypto/aes/aesxcbcmac.h>

int
aes_xcbc_mac_init(void *vctx, const uint8_t *key, u_int16_t keylen)
{
	static const uint8_t k1seed[AES_BLOCKSIZE] =
	    { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
	static const uint8_t k2seed[AES_BLOCKSIZE] =
	    { 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2 };
	static const uint8_t k3seed[AES_BLOCKSIZE] =
	    { 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
	aes_ctx r_ks;
	aesxcbc_ctx *ctx;
	uint8_t k1[AES_BLOCKSIZE];

	ctx = vctx;
	bzero(ctx, sizeof(*ctx));
	
	switch (keylen) {
	case 16:
		ctx->r_nr = AES_SetkeyEnc(&r_ks, key, 128);
		break;
	case 24:
		ctx->r_nr = AES_SetkeyEnc(&r_ks, key, 192);
		break;
	case 32:
		ctx->r_nr = AES_SetkeyEnc(&r_ks, key, 256);
		break;
	}

	AES_Encrypt(&r_ks, k1seed, k1);
	AES_Encrypt(&r_ks, k2seed, ctx->k2);
	AES_Encrypt(&r_ks, k3seed, ctx->k3);
	AES_Setkey(&ctx->r_k1s, k1, 128);

	bzero(&r_ks, sizeof(r_ks));
	bzero(k1, sizeof(k1));

	return 0;
}

int
aes_xcbc_mac_loop(void *vctx, const uint8_t *addr, u_int16_t len)
{
	uint8_t buf[AES_BLOCKSIZE];
	aesxcbc_ctx *ctx;
	const uint8_t *ep;
	int i;

	ctx = vctx;
	ep = addr + len;

	if (ctx->buflen == sizeof(ctx->buf)) {
		for (i = 0; i < sizeof(ctx->e); i++) {
			ctx->buf[i] ^= ctx->e[i];
		}
		AES_Encrypt(&ctx->r_k1s, ctx->buf, ctx->e);
		ctx->buflen = 0;
	}
	if (ctx->buflen + len < sizeof(ctx->buf)) {
		bcopy(addr, ctx->buf + ctx->buflen, len);
		ctx->buflen += len;
		return 0;
	}
	if (ctx->buflen && ctx->buflen + len > sizeof(ctx->buf)) {
		bcopy(addr, ctx->buf + ctx->buflen, sizeof(ctx->buf) - ctx->buflen);
		for (i = 0; i < sizeof(ctx->e); i++) {
			ctx->buf[i] ^= ctx->e[i];
		}
		AES_Encrypt(&ctx->r_k1s, ctx->buf, ctx->e);
		addr += sizeof(ctx->buf) - ctx->buflen;
		ctx->buflen = 0;
	}
	/* due to the special processing for M[n], "=" case is not included */
	while (ep - addr > AES_BLOCKSIZE) {
		bcopy(addr, buf, AES_BLOCKSIZE);
		for (i = 0; i < sizeof(buf); i++) {
			buf[i] ^= ctx->e[i];
		}
		AES_Encrypt(&ctx->r_k1s, buf, ctx->e);
		addr += AES_BLOCKSIZE;
	}
	if (addr < ep) {
		bcopy(addr, ctx->buf + ctx->buflen, ep - addr);
		ctx->buflen += ep - addr;
	}
	return 0;
}

void
aes_xcbc_mac_result(uint8_t *addr, void *vctx)
{
	uint8_t digest[AES_BLOCKSIZE];
	aesxcbc_ctx *ctx;
	int i;

	ctx = vctx;

	if (ctx->buflen == sizeof(ctx->buf)) {
		for (i = 0; i < sizeof(ctx->buf); i++) {
			ctx->buf[i] ^= ctx->e[i];
			ctx->buf[i] ^= ctx->k2[i];
		}
		AES_Encrypt(&ctx->r_k1s, ctx->buf, digest);
	} else {
		for (i = ctx->buflen; i < sizeof(ctx->buf); i++) {
			ctx->buf[i] = (i == ctx->buflen) ? 0x80 : 0x00;
		}
		for (i = 0; i < sizeof(ctx->buf); i++) {
			ctx->buf[i] ^= ctx->e[i];
			ctx->buf[i] ^= ctx->k3[i];
		}
		AES_Encrypt(&ctx->r_k1s, ctx->buf, digest);
	}

	bcopy(digest, addr, sizeof(digest));
}
