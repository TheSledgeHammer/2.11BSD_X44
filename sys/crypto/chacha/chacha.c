/*	$OpenBSD: chacha_private.h,v 1.4 2020/07/22 13:54:30 tobhe Exp $	*/
/*
chacha-merged.c version 20080118
D. J. Bernstein
Public domain.
*/

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/systm.h>

#include <sys/endian.h>

#include <crypto/chacha/chacha.h>

void
hchacha20(u32 derived_key[8], const u8 nonce[16], const u8 key[32])
{
	int i;
	uint32_t x[];

	U8TO32_LITTLE(x[0], sigma + 0);
	U8TO32_LITTLE(x[1], sigma + 4);
	U8TO32_LITTLE(x[2], sigma + 8);
	U8TO32_LITTLE(x[3], sigma + 12);
	U8TO32_LITTLE(x[4], key + 0);
	U8TO32_LITTLE(x[5], key + 4);
	U8TO32_LITTLE(x[6], key + 8);
	U8TO32_LITTLE(x[7], key + 12);
	U8TO32_LITTLE(x[8], key + 16);
	U8TO32_LITTLE(x[9], key + 20);
	U8TO32_LITTLE(x[10], key + 24);
	U8TO32_LITTLE(x[11], key + 28);
	U8TO32_LITTLE(x[12], nonce + 0);
	U8TO32_LITTLE(x[13], nonce + 4);
	U8TO32_LITTLE(x[14], nonce + 8);
	U8TO32_LITTLE(x[15], nonce + 12);

	for (i = 20; i > 0; i -= 2) {
		QUARTERROUND(x[0], x[4], x[8], x[12]);
		QUARTERROUND(x[1], x[5], x[9], x[13]);
		QUARTERROUND(x[2], x[6], x[10], x[14]);
		QUARTERROUND(x[3], x[7], x[11], x[15]);
		QUARTERROUND(x[0], x[5], x[10], x[15]);
		QUARTERROUND(x[1], x[6], x[11], x[12]);
		QUARTERROUND(x[2], x[7], x[8], x[13]);
		QUARTERROUND(x[3], x[4], x[9], x[14]);
	}

	memcpy(derived_key + 0, x + 0, sizeof(u32) * 4);
	memcpy(derived_key + 4, x + 12, sizeof(u32) * 4);
}

void
chacha_keysetup(chacha_ctx *x, const u8 *k, u32 kbits)
{
	const char *constants;

	U8TO32_LITTLE(x->input[4], k + 0);
	U8TO32_LITTLE(x->input[5], k + 4);
	U8TO32_LITTLE(x->input[6], k + 8);
	U8TO32_LITTLE(x->input[7], k + 12);
	if (kbits == 256) { /* recommended */
		k += 16;
		constants = sigma;
	} else { /* kbits == 128 */
		constants = tau;
	}
	U8TO32_LITTLE(x->input[8], k + 0);
	U8TO32_LITTLE(x->input[9], k + 4);
	U8TO32_LITTLE(x->input[10], k + 8);
	U8TO32_LITTLE(x->input[11], k + 12);
	U8TO32_LITTLE(x->input[0], constants + 0);
	U8TO32_LITTLE(x->input[1], constants + 4);
	U8TO32_LITTLE(x->input[2], constants + 8);
	U8TO32_LITTLE(x->input[3], constants + 12);
}

void
chacha_ivsetup(chacha_ctx *x, const u8 *iv, const u8 *counter)
 {
	U8TO32_LITTLE(x->input[12], counter == NULL ? 0 : counter + 0);
	U8TO32_LITTLE(x->input[13], counter == NULL ? 0 : counter + 4);
	U8TO32_LITTLE(x->input[14], iv + 0);
	U8TO32_LITTLE(x->input[15], iv + 4);
}

void
chacha_encrypt_bytes(chacha_ctx *x, const u8 *m, u8 *c, u32 bytes)
{
	u32 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
	u32 j0, j1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14, j15;
	u8 *ctarget = NULL;
	u8 tmp[64];
	u_int i;

	if (!bytes) {
		return;
	}

	j0 = x->input[0];
	j1 = x->input[1];
	j2 = x->input[2];
	j3 = x->input[3];
	j4 = x->input[4];
	j5 = x->input[5];
	j6 = x->input[6];
	j7 = x->input[7];
	j8 = x->input[8];
	j9 = x->input[9];
	j10 = x->input[10];
	j11 = x->input[11];
	j12 = x->input[12];
	j13 = x->input[13];
	j14 = x->input[14];
	j15 = x->input[15];

	for (;;) {
		if (bytes < 64) {
			for (i = 0; i < bytes; ++i)
				tmp[i] = m[i];
			m = tmp;
			ctarget = c;
			c = tmp;
		}
		x0 = j0;
		x1 = j1;
		x2 = j2;
		x3 = j3;
		x4 = j4;
		x5 = j5;
		x6 = j6;
		x7 = j7;
		x8 = j8;
		x9 = j9;
		x10 = j10;
		x11 = j11;
		x12 = j12;
		x13 = j13;
		x14 = j14;
		x15 = j15;
		for (i = 20; i > 0; i -= 2) {
			QUARTERROUND(x0, x4, x8, x12)
			QUARTERROUND(x1, x5, x9, x13)
			QUARTERROUND(x2, x6, x10, x14)
			QUARTERROUND(x3, x7, x11, x15)
			QUARTERROUND(x0, x5, x10, x15)
			QUARTERROUND(x1, x6, x11, x12)
			QUARTERROUND(x2, x7, x8, x13)
			QUARTERROUND(x3, x4, x9, x14)
		}
		x0 = PLUS(x0, j0);
		x1 = PLUS(x1, j1);
		x2 = PLUS(x2, j2);
		x3 = PLUS(x3, j3);
		x4 = PLUS(x4, j4);
		x5 = PLUS(x5, j5);
		x6 = PLUS(x6, j6);
		x7 = PLUS(x7, j7);
		x8 = PLUS(x8, j8);
		x9 = PLUS(x9, j9);
		x10 = PLUS(x10, j10);
		x11 = PLUS(x11, j11);
		x12 = PLUS(x12, j12);
		x13 = PLUS(x13, j13);
		x14 = PLUS(x14, j14);
		x15 = PLUS(x15, j15);

#ifndef KEYSTREAM_ONLY
		XOR_U8TO32_LITTLE(x0, m + 0);
		XOR_U8TO32_LITTLE(x1, m + 4);
		XOR_U8TO32_LITTLE(x2, m + 8);
		XOR_U8TO32_LITTLE(x3, m + 12);
		XOR_U8TO32_LITTLE(x4, m + 16);
		XOR_U8TO32_LITTLE(x5, m + 20);
		XOR_U8TO32_LITTLE(x6, m + 24);
		XOR_U8TO32_LITTLE(x7, m + 28);
		XOR_U8TO32_LITTLE(x8, m + 32);
		XOR_U8TO32_LITTLE(x9, m + 36);
		XOR_U8TO32_LITTLE(x10, m + 40);
		XOR_U8TO32_LITTLE(x11, m + 44);
		XOR_U8TO32_LITTLE(x12, m + 48);
		XOR_U8TO32_LITTLE(x13, m + 52);
		XOR_U8TO32_LITTLE(x14, m + 56);
		XOR_U8TO32_LITTLE(x15, m + 60);
#endif

		j12 = PLUSONE(j12);
		if (!j12) {
			j13 = PLUSONE(j13);
			/* stopping at 2^70 bytes per nonce is user's responsibility */
		}

		U32TO8_LITTLE(c + 0, x0);
		U32TO8_LITTLE(c + 4, x1);
		U32TO8_LITTLE(c + 8, x2);
		U32TO8_LITTLE(c + 12, x3);
		U32TO8_LITTLE(c + 16, x4);
		U32TO8_LITTLE(c + 20, x5);
		U32TO8_LITTLE(c + 24, x6);
		U32TO8_LITTLE(c + 28, x7);
		U32TO8_LITTLE(c + 32, x8);
		U32TO8_LITTLE(c + 36, x9);
		U32TO8_LITTLE(c + 40, x10);
		U32TO8_LITTLE(c + 44, x11);
		U32TO8_LITTLE(c + 48, x12);
		U32TO8_LITTLE(c + 52, x13);
		U32TO8_LITTLE(c + 56, x14);
		U32TO8_LITTLE(c + 60, x15);

		if (bytes <= 64) {
			if (bytes < 64) {
				for (i = 0; i < bytes; ++i)
					ctarget[i] = c[i];
			}
			x->input[12] = j12;
			x->input[13] = j13;
			return;
		}
		bytes -= 64;
		c += 64;
#ifndef KEYSTREAM_ONLY
		m += 64;
#endif
	}
}
