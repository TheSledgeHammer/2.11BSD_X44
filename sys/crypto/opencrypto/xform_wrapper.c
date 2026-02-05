/*	$NetBSD: xform.c,v 1.13 2003/11/18 23:01:39 jonathan Exp $ */
/*	$FreeBSD: src/sys/opencrypto/xform.c,v 1.1.2.1 2002/11/21 23:34:23 sam Exp $	*/
/*	$OpenBSD: xform.c,v 1.19 2002/08/16 22:47:25 dhartmei Exp $	*/

/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr) and
 * Niels Provos (provos@physnet.uni-hamburg.de).
 *
 * This code was written by John Ioannidis for BSD/OS in Athens, Greece,
 * in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis.
 *
 * Additional transforms and features in 1997 and 1998 by Angelos D. Keromytis
 * and Niels Provos.
 *
 * Additional features in 1999 by Angelos D. Keromytis.
 *
 * Copyright (C) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
 *
 * Copyright (C) 2001, Angelos D. Keromytis.
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software.
 * You may use this code under the GNU public license if you so wish. Please
 * contribute changes back to the authors under this freer than GPL license
 * so that we may further the use of strong encryption without limitations to
 * all.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <crypto/md5/md5.h>
#include <crypto/sha1/sha1.h>
#include <crypto/sha2/sha2.h>
#include <crypto/blowfish/blowfish.h>
#include <crypto/cast128/cast128.h>
#include <crypto/chachapoly/chachapoly.h>
#include <crypto/des/des.h>
#include <crypto/rijndael/rijndael.h>
#include <crypto/ripemd160/rmd160.h>
#include <crypto/skipjack/skipjack.h>
#include <crypto/aes/aes.h>
#include <crypto/mars/mars.h>
#include <crypto/camellia/camellia.h>
#include <crypto/twofish/twofish.h>
#include <crypto/serpent/serpent.h>
#include <crypto/gmac/gmac.h>

#include <crypto/opencrypto/deflate.h>
#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/xform.h>
#include <crypto/opencrypto/xform_wrapper.h>

#define AESCTR_NONCESIZE		4
#define AESCTR_IVSIZE			8
#define AESCTR_BLOCKSIZE		16

#define AES_XTS_BLOCKSIZE		16
#define AES_XTS_IVSIZE			8
#define AES_XTS_ALPHA			0x87	/* GF(2^128) generator polynomial */

#define TWOFISH_XTS_BLOCKSIZE	16
#define TWOFISH_XTS_IVSIZE		8

#define SERPENT_XTS_BLOCKSIZE	16
#define SERPENT_XTS_IVSIZE		8

#define MARS_XTS_BLOCKSIZE		16
#define MARS_XTS_IVSIZE			8

struct aes_ctr_ctx {
	aes_ctx			ac_key;
	u_int8_t		ac_block[AESCTR_BLOCKSIZE];
};

struct aes_xts_ctx {
	rijndael_ctx 	key1;
	rijndael_ctx 	key2;
	u_int8_t 		tweak[AES_XTS_BLOCKSIZE];
};

struct twofish_xts_ctx {
	twofish_ctx   	key1;
	twofish_ctx   	key2;
	u_int8_t      	tweak[TWOFISH_XTS_BLOCKSIZE];
};

struct serpent_xts_ctx {
	serpent_ctx   	key1;
	serpent_ctx   	key2;
	u_int8_t      	tweak[SERPENT_XTS_BLOCKSIZE];
};

struct mars_xts_ctx {
	mars_ctx   		key1;
	mars_ctx   		key2;
	u_int8_t      	tweak[MARS_XTS_BLOCKSIZE];
};

/*
 * Encryption wrapper routines.
 */
void
null_encrypt(caddr_t key, u_int8_t *blk)
{

}

void
null_decrypt(caddr_t key, u_int8_t *blk)
{

}

int
null_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	*sched = NULL;
	return 0;
}

void
null_zerokey(u_int8_t **sched)
{
	*sched = NULL;
}

void
des1_encrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb_encrypt(cb, cb, p[0], DES_ENCRYPT);
}

void
des1_decrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb_encrypt(cb, cb, p[0], DES_DECRYPT);
}

int
des1_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	des_key_schedule *p;
	int err;

	MALLOC(p, des_key_schedule *, sizeof (des_key_schedule), M_CRYPTO_DATA, M_NOWAIT);
	if (p != NULL) {
		bzero(p, sizeof(des_key_schedule));
		des_set_key((des_cblock *) key, p[0]);
		err = 0;
	} else
		err = ENOMEM;
	*sched = (u_int8_t *) p;
	return err;
}

void
des1_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof (des_key_schedule));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
des3_encrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb3_encrypt(cb, cb, p[0], p[1], p[2], DES_ENCRYPT);
}

void
des3_decrypt(caddr_t key, u_int8_t *blk)
{
	des_cblock *cb = (des_cblock *) blk;
	des_key_schedule *p = (des_key_schedule *) key;

	des_ecb3_encrypt(cb, cb, p[0], p[1], p[2], DES_DECRYPT);
}

int
des3_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	des_key_schedule *p;
	int err;

	MALLOC(p, des_key_schedule *, 3*sizeof (des_key_schedule), M_CRYPTO_DATA, M_NOWAIT);
	if (p != NULL) {
		bzero(p, 3*sizeof(des_key_schedule));
		des_set_key((des_cblock *)(key +  0), p[0]);
		des_set_key((des_cblock *)(key +  8), p[1]);
		des_set_key((des_cblock *)(key + 16), p[2]);
		err = 0;
	} else
		err = ENOMEM;
	*sched = (u_int8_t *) p;
	return err;
}

void
des3_zerokey(u_int8_t **sched)
{
	bzero(*sched, 3*sizeof (des_key_schedule));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
blf_encrypt(caddr_t key, u_int8_t *blk)
{
	BF_ecb_encrypt(blk, blk, (BF_KEY *)key, 1);
}

void
blf_decrypt(caddr_t key, u_int8_t *blk)
{

	BF_ecb_encrypt(blk, blk, (BF_KEY *)key, 0);
}

int
blf_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

#define	BLF_SIZ	sizeof(BF_KEY)

	MALLOC(*sched, u_int8_t *, BLF_SIZ, M_CRYPTO_DATA, M_NOWAIT);
	if (*sched != NULL) {
		bzero(*sched, BLF_SIZ);
		BF_set_key((BF_KEY *) *sched, len, key);
		err = 0;
	} else {
		err = ENOMEM;
	}
	return err;
}

void
blf_zerokey(u_int8_t **sched)
{
	bzero(*sched, BLF_SIZ);
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
cast5_encrypt(caddr_t key, u_int8_t *blk)
{
	cast128_encrypt((cast128_key *) key, blk, blk);
}

void
cast5_decrypt(caddr_t key, u_int8_t *blk)
{
	cast128_decrypt((cast128_key *) key, blk, blk);
}

int
cast5_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	MALLOC(*sched, u_int8_t *, sizeof(cast128_key), M_CRYPTO_DATA, M_NOWAIT);
	if (*sched != NULL) {
		bzero(*sched, sizeof(cast128_key));
		cast128_setkey((cast128_key *)*sched, key, len);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

void
cast5_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(cast128_key));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
skipjack_encrypt(caddr_t key, u_int8_t *blk)
{
	skipjack_forwards(blk, blk, (u_int8_t **) key);
}

void
skipjack_decrypt(caddr_t key, u_int8_t *blk)
{
	skipjack_backwards(blk, blk, (u_int8_t **) key);
}

int
skipjack_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	/* NB: allocate all the memory that's needed at once */
	/* XXX assumes bytes are aligned on sizeof(u_char) == 1 boundaries.
	 * Will this break a pdp-10, Cray-1, or GE-645 port?
	 */
	MALLOC(*sched, u_int8_t *, 10 * (sizeof(u_int8_t *) + 0x100), M_CRYPTO_DATA, M_NOWAIT);

	if (*sched != NULL) {

		u_int8_t** key_tables = (u_int8_t**) *sched;
		u_int8_t* table = (u_int8_t*) &key_tables[10];
		int k;

		bzero(*sched, 10 * sizeof(u_int8_t *)+0x100);

		for (k = 0; k < 10; k++) {
			key_tables[k] = table;
			table += 0x100;
		}
		subkey_table_gen(key, (u_int8_t **) *sched);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

void
skipjack_zerokey(u_int8_t **sched)
{
	bzero(*sched, 10 * (sizeof(u_int8_t *) + 0x100));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
rijndael128_encrypt(caddr_t key, u_int8_t *blk)
{
	rijndael_encrypt((rijndael_ctx *) key, (u_char *) blk, (u_char *) blk);
}

void
rijndael128_decrypt(caddr_t key, u_int8_t *blk)
{
	rijndael_decrypt((rijndael_ctx *) key, (u_char *) blk,
	    (u_char *) blk);
}

int
rijndael128_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	MALLOC(*sched, u_int8_t *, sizeof(rijndael_ctx), M_CRYPTO_DATA, M_WAITOK);
	if (*sched != NULL) {
		bzero(*sched, sizeof(rijndael_ctx));
		rijndael_set_key((rijndael_ctx *) *sched, key, len * 8);
		err = 0;
	} else
		err = ENOMEM;
	return err;
}

void
rijndael128_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(rijndael_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
aes_encrypt(caddr_t key, u_int8_t *blk)
{
	AES_Encrypt((aes_ctx *)key, blk, blk);
}

void
aes_decrypt(caddr_t key, u_int8_t *blk)
{
	AES_Decrypt((aes_ctx *)key, blk, blk);
}

int
aes_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	MALLOC(*sched, u_int8_t *, sizeof(aes_ctx), M_CRYPTO_DATA, M_WAITOK);
	if (*sched != NULL) {
		bzero(*sched, sizeof(aes_ctx));
		AES_Setkey((aes_ctx *)sched, key, len * 8);
		err = 0;
	} else {
		err = ENOMEM;
	}
	return err;
}

void
aes_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(aes_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
aes_ctr_crypt(caddr_t key, u_int8_t *blk)
{
	struct aes_ctr_ctx *ctx;
	u_int8_t keystream[AESCTR_BLOCKSIZE];
	int i;

	ctx = (struct aes_ctr_ctx *)key;
	/* increment counter */
	for (i = AESCTR_BLOCKSIZE - 1; i >= AESCTR_NONCESIZE + AESCTR_IVSIZE; i--) {
		if (++ctx->ac_block[i]) { /* continue on overflow */
			break;
		}
	}
	AES_Encrypt(&ctx->ac_key, ctx->ac_block, keystream);
	for (i = 0; i < AESCTR_BLOCKSIZE; i++) {
		blk[i] ^= keystream[i];
	}
	bzero(keystream, sizeof(keystream));
}

int
aes_ctr_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	struct aes_ctr_ctx *ctx;

	if (len < AESCTR_NONCESIZE) {
		return -1;
	}
	
	ctx = (struct aes_ctr_ctx *)sched;
	AES_Setkey(&ctx->ac_key, key, len - AESCTR_NONCESIZE);
	bcopy(key + len - AESCTR_NONCESIZE, ctx->ac_block, AESCTR_NONCESIZE);
	return 0;
}

void
aes_ctr_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(struct aes_ctr_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
aes_ctr_reinit(void *key, const u_int8_t *iv, u_int8_t *ivout)
{
	struct aes_ctr_ctx *ctx;

	ctx = (struct aes_ctr_ctx *)key;
	bcopy(iv, ctx->ac_block + AESCTR_NONCESIZE, AESCTR_IVSIZE);

	/* reset counter */
	bcopy(iv, ctx->ac_block + AESCTR_NONCESIZE, AESCTR_IVSIZE);
}

void
aes_xts_crypt(struct aes_xts_ctx *ctx, u_int8_t *data, u_int do_encrypt)
{
	u_int8_t block[AES_XTS_BLOCKSIZE];
	u_int i, carry_in, carry_out;

	for (i = 0; i < AES_XTS_BLOCKSIZE; i++) {
		block[i] = data[i] ^ ctx->tweak[i];
	}

	if (do_encrypt) {
		rijndael_encrypt(&ctx->key1, block, data);
	} else {
		rijndael_decrypt(&ctx->key1, block, data);
	}

	for (i = 0; i < AES_XTS_BLOCKSIZE; i++) {
		data[i] ^= ctx->tweak[i];
	}

	/* Exponentiate tweak */
	carry_in = 0;
	for (i = 0; i < AES_XTS_BLOCKSIZE; i++) {
		carry_out = ctx->tweak[i] & 0x80;
		ctx->tweak[i] = (ctx->tweak[i] << 1) | carry_in;
		carry_in = carry_out >> 7;
	}
	ctx->tweak[0] ^= (AES_XTS_ALPHA & -carry_in);
	bzero(block, sizeof(block));
}

void
aes_xts_encrypt(caddr_t key, u_int8_t *blk)
{
	aes_xts_crypt((struct aes_xts_ctx *)key, blk, 1);
}

void
aes_xts_decrypt(caddr_t key, u_int8_t *blk)
{
	aes_xts_crypt((struct aes_xts_ctx *)key, blk, 0);
}

int
aes_xts_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	struct aes_xts_ctx *ctx;

	if (len != 32 && len != 64) {
		return -1;
	}

	ctx = (struct aes_xts_ctx *)sched;
	rijndael_set_key(&ctx->key1, key, len * 4);
	rijndael_set_key(&ctx->key2, key + (len / 2), len * 4);

	return 0;
}

void
aes_xts_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(struct aes_xts_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
aes_xts_reinit(void *key, const u_int8_t *iv, u_int8_t *ivout)
{
	struct aes_xts_ctx *ctx;
	u_int64_t blocknum;
	u_int i;
	
	ctx = (struct aes_xts_ctx *)key;
	bcopy(iv, &blocknum, AES_XTS_IVSIZE);
	for (i = 0; i < AES_XTS_IVSIZE; i++) {
		ctx->tweak[i] = blocknum & 0xff;
		blocknum >>= 8;
	}
	/* Last 64 bits of IV are always zero */
	bzero(ctx->tweak + AES_XTS_IVSIZE, AES_XTS_IVSIZE);

	rijndael_encrypt(&ctx->key2, ctx->tweak, ctx->tweak);
}

void
aes_gcm_reinit(void *key, const u_int8_t *iv, u_int8_t *ivout)
{
	struct aes_ctr_ctx *ctx;

	ctx = (struct aes_ctr_ctx *)key;
	bcopy(iv, ctx->ac_block + AESCTR_NONCESIZE, AESCTR_IVSIZE);

	/* reset counter */
	bzero(ctx->ac_block + AESCTR_NONCESIZE + AESCTR_IVSIZE, 4);
	ctx->ac_block[AESCTR_BLOCKSIZE - 1] = 1; /* GCM starts with 1 */
}

void
cml_encrypt(caddr_t key, u_int8_t *blk)
{
	camellia_encrypt((const camellia_ctx *)key, blk, blk);
}

void
cml_decrypt(caddr_t key, u_int8_t *blk)
{
	camellia_decrypt((const camellia_ctx *)key, blk, blk);
}

int
cml_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	if (len != 16 && len != 24 && len != 32) {
		return (EINVAL);
	}

	MALLOC(*sched, u_int8_t *, sizeof(camellia_ctx), M_CRYPTO_DATA, M_WAITOK | M_ZERO);
	if (*sched != NULL) {
		bzero(*sched, sizeof(camellia_ctx));
		camellia_set_key((camellia_ctx *) *sched, key, len * 8);
		err = 0;
	} else {
		err = ENOMEM;
	}

	return err;
}

void
cml_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(camellia_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
twofish128_encrypt(caddr_t key, u_int8_t *blk)
{
	twofish_encrypt((twofish_ctx *)key, blk, blk);
}

void
twofish128_decrypt(caddr_t key, u_int8_t *blk)
{
	twofish_decrypt((twofish_ctx *) key, blk, blk);
}

int
twofish128_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	if (len != 16 && len != 24 && len != 32) {
		return (EINVAL);
	}

	MALLOC(*sched, u_int8_t *, sizeof(twofish_ctx), M_CRYPTO_DATA, M_WAITOK | M_ZERO);
	if (*sched != NULL) {
		twofish_set_key((twofish_ctx *) *sched, key, len * 8);
		err = 0;
	} else {
		err = ENOMEM;
	}
	return err;
}

void
twofish128_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(twofish_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
serpent128_encrypt(caddr_t key, u_int8_t *blk)
{
	serpent_encrypt((serpent_ctx *) key, blk, blk);
}

void
serpent128_decrypt(caddr_t key, u_int8_t *blk)
{
	serpent_decrypt((serpent_ctx *) key, blk, blk);
}

int
serpent128_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	if (len != 16 && len != 24 && len != 32) {
		return (EINVAL);
	}
	MALLOC(*sched, u_int8_t *, sizeof(serpent_ctx), M_CRYPTO_DATA, M_WAITOK | M_ZERO);
	if (*sched != NULL) {
		serpent_set_key((serpent_ctx *) *sched, key, len * 8);
		err = 0;
	} else {
		err = ENOMEM;
	}
	return err;
}

void
serpent128_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(serpent_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
mars128_encrypt(caddr_t key, u_int8_t *blk)
{
	mars_encrypt((mars_ctx *)key, blk, blk);
}

void
mars128_decrypt(caddr_t key, u_int8_t *blk)
{
	mars_decrypt((mars_ctx *) key, blk, blk);
}

int
mars128_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	int err;

	if (len != 16 && len != 24 && len != 32) {
		return (EINVAL);
	}

	MALLOC(*sched, u_int8_t *, sizeof(mars_ctx), M_CRYPTO_DATA, M_WAITOK | M_ZERO);
	if (*sched != NULL) {
		mars_set_key((mars_ctx *) *sched, key, len * 8);
		err = 0;
	} else {
		err = ENOMEM;
	}
	return err;
}

void
mars128_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(mars_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
twofish_xts_reinit(void *key, const u_int8_t *iv, u_int8_t *ivout)
{
	struct twofish_xts_ctx *ctx;
	u_int64_t blocknum;
	u_int i;
	
	ctx = (struct twofish_xts_ctx *)key;
	bcopy(iv, &blocknum, TWOFISH_XTS_IVSIZE);
	for (i = 0; i < TWOFISH_XTS_IVSIZE; i++) {
		ctx->tweak[i] = blocknum & 0xff;
		blocknum >>= 8;
	}

	/* Last 64 bits of IV are always zero */
	bzero(ctx->tweak + TWOFISH_XTS_IVSIZE, TWOFISH_XTS_IVSIZE);

	twofish_encrypt(&ctx->key2, ctx->tweak, ctx->tweak);
}

void
twofish_xts_crypt(struct twofish_xts_ctx *ctx, u_int8_t *data, u_int do_encrypt)
{
	u_int8_t block[TWOFISH_XTS_BLOCKSIZE];
	u_int i, carry_in, carry_out;

	for (i = 0; i < TWOFISH_XTS_BLOCKSIZE; i++)
		block[i] = data[i] ^ ctx->tweak[i];

	if (do_encrypt) {
		twofish_encrypt(&ctx->key1, block, data);
	} else {
		twofish_decrypt(&ctx->key1, block, data);
	}

	for (i = 0; i < TWOFISH_XTS_BLOCKSIZE; i++) {
		data[i] ^= ctx->tweak[i];
	}

	/* Exponentiate tweak */
	carry_in = 0;
	for (i = 0; i < TWOFISH_XTS_BLOCKSIZE; i++) {
		carry_out = ctx->tweak[i] & 0x80;
		ctx->tweak[i] = (ctx->tweak[i] << 1) | (carry_in ? 1 : 0);
		carry_in = carry_out;
	}
	if (carry_in) {
		ctx->tweak[0] ^= AES_XTS_ALPHA;
	}
	bzero(block, sizeof(block));
}

void
twofish_xts_encrypt(caddr_t key, u_int8_t *data)
{
	twofish_xts_crypt((struct twofish_xts_ctx *)key, data, 1);
}

void
twofish_xts_decrypt(caddr_t key, u_int8_t *data)
{
	twofish_xts_crypt((struct twofish_xts_ctx *)key, data, 0);
}

int
twofish_xts_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	struct twofish_xts_ctx *ctx;

	if (len != 32 && len != 64) {
		return -1;
	}

	MALLOC(*sched, u_int8_t *, sizeof(struct twofish_xts_ctx), M_CRYPTO_DATA, M_WAITOK | M_ZERO);
	ctx = (struct twofish_xts_ctx *)*sched;

	twofish_set_key(&ctx->key1, key, len * 4);
	twofish_set_key(&ctx->key2, key + (len / 2), len * 4);

	return 0;
}

void
twofish_xts_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(struct twofish_xts_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
serpent_xts_reinit(void *key, const u_int8_t *iv, u_int8_t *ivout)
{
	struct serpent_xts_ctx *ctx;
	u_int64_t blocknum;
	u_int i;
	
	ctx = (struct serpent_xts_ctx *)key;
	bcopy(iv, &blocknum, SERPENT_XTS_IVSIZE);
	for (i = 0; i < SERPENT_XTS_IVSIZE; i++) {
		ctx->tweak[i] = blocknum & 0xff;
		blocknum >>= 8;
	}

	/* Last 64 bits of IV are always zero */
	bzero(ctx->tweak + SERPENT_XTS_IVSIZE, SERPENT_XTS_IVSIZE);

	serpent_encrypt(&ctx->key2, ctx->tweak, ctx->tweak);
}

void
serpent_xts_crypt(struct serpent_xts_ctx *ctx, u_int8_t *data, u_int do_encrypt)
{
	u_int8_t block[SERPENT_XTS_BLOCKSIZE];
	u_int i, carry_in, carry_out;

	for (i = 0; i < SERPENT_XTS_BLOCKSIZE; i++)
		block[i] = data[i] ^ ctx->tweak[i];

	if (do_encrypt) {
		serpent_encrypt(&ctx->key1, block, data);
	} else {
		serpent_decrypt(&ctx->key1, block, data);
	}

	for (i = 0; i < SERPENT_XTS_BLOCKSIZE; i++) {
		data[i] ^= ctx->tweak[i];
	}

	/* Exponentiate tweak */
	carry_in = 0;
	for (i = 0; i < SERPENT_XTS_BLOCKSIZE; i++) {
		carry_out = ctx->tweak[i] & 0x80;
		ctx->tweak[i] = (ctx->tweak[i] << 1) | (carry_in ? 1 : 0);
		carry_in = carry_out;
	}
	if (carry_in) {
		ctx->tweak[0] ^= AES_XTS_ALPHA;
	}
	bzero(block, sizeof(block));
}

void
serpent_xts_encrypt(caddr_t key, u_int8_t *data)
{
	serpent_xts_crypt((struct serpent_xts_ctx *)key, data, 1);
}

void
serpent_xts_decrypt(caddr_t key, u_int8_t *data)
{
	serpent_xts_crypt((struct serpent_xts_ctx *)key, data, 0);
}

int
serpent_xts_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	struct serpent_xts_ctx *ctx;

	if (len != 32 && len != 64) {
		return -1;
	}
	MALLOC(*sched, u_int8_t *, sizeof(struct serpent_xts_ctx), M_CRYPTO_DATA, M_WAITOK | M_ZERO);
	ctx = (struct serpent_xts_ctx *)*sched;

	serpent_set_key(&ctx->key1, key, len * 4);
	serpent_set_key(&ctx->key2, key + (len / 2), len * 4);

	return 0;
}

void
serpent_xts_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(struct serpent_xts_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
mars_xts_reinit(void *key, const u_int8_t *iv, u_int8_t *ivout)
{
	struct mars_xts_ctx *ctx;
	u_int64_t blocknum;
	u_int i;

	ctx = (struct mars_xts_ctx *)key;
	bcopy(iv, &blocknum, MARS_XTS_IVSIZE);
	for (i = 0; i < MARS_XTS_IVSIZE; i++) {
		ctx->tweak[i] = blocknum & 0xff;
		blocknum >>= 8;
	}

	/* Last 64 bits of IV are always zero */
	bzero(ctx->tweak + MARS_XTS_IVSIZE, MARS_XTS_IVSIZE);

	mars_encrypt(&ctx->key2, ctx->tweak, ctx->tweak);
}

void
mars_xts_crypt(struct mars_xts_ctx *ctx, u_int8_t *data, u_int do_encrypt)
{
	u_int8_t block[MARS_XTS_BLOCKSIZE];
	u_int i, carry_in, carry_out;

	for (i = 0; i < MARS_XTS_BLOCKSIZE; i++)
		block[i] = data[i] ^ ctx->tweak[i];

	if (do_encrypt) {
		mars_encrypt(&ctx->key1, block, data);
	} else {
		mars_decrypt(&ctx->key1, block, data);
	}

	for (i = 0; i < MARS_XTS_BLOCKSIZE; i++) {
		data[i] ^= ctx->tweak[i];
	}

	/* Exponentiate tweak */
	carry_in = 0;
	for (i = 0; i < MARS_XTS_BLOCKSIZE; i++) {
		carry_out = ctx->tweak[i] & 0x80;
		ctx->tweak[i] = (ctx->tweak[i] << 1) | (carry_in ? 1 : 0);
		carry_in = carry_out;
	}
	if (carry_in) {
		ctx->tweak[0] ^= AES_XTS_ALPHA;
	}
	bzero(block, sizeof(block));
}

void
mars_xts_encrypt(caddr_t key, u_int8_t *data)
{
	mars_xts_crypt((struct mars_xts_ctx *)key, data, 1);
}

void
mars_xts_decrypt(caddr_t key, u_int8_t *data)
{
	mars_xts_crypt((struct mars_xts_ctx *)key, data, 0);
}

int
mars_xts_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
	struct mars_xts_ctx *ctx;

	if (len != 32 && len != 64) {
		return -1;
	}
	MALLOC(*sched, u_int8_t *, sizeof(struct mars_xts_ctx), M_CRYPTO_DATA, M_WAITOK | M_ZERO);
	ctx = (struct mars_xts_ctx *)*sched;

	mars_set_key(&ctx->key1, key, len * 4);
	mars_set_key(&ctx->key2, key + (len / 2), len * 4);

	return 0;
}

void
mars_xts_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(struct mars_xts_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
chachapoly_encrypt(caddr_t key, u_int8_t *data)
{
	chacha20_crypt(key, data);
}

void
chachapoly_decrypt(caddr_t key, u_int8_t *data)
{
	chacha20_crypt(key, data);
}

int
chachapoly_setkey(u_int8_t **sched, const u_int8_t *key, int len)
{
    u_int8_t *blocknum;

	MALLOC(*sched, u_int8_t *, sizeof(struct chacha20_ctx), M_CRYPTO_DATA, M_WAITOK | M_ZERO);
    bcopy(key, blocknum, sizeof(key));
	return (chacha20_setkey(*sched, blocknum, len));
}

void
chachapoly_zerokey(u_int8_t **sched)
{
	bzero(*sched, sizeof(struct chacha20_ctx));
	FREE(*sched, M_CRYPTO_DATA);
	*sched = NULL;
}

void
chachapoly_reinit(void *key, const u_int8_t *iv, u_int8_t *ivout)
{
    u_int8_t *blocknum;
    
    bcopy(iv, blocknum, sizeof(iv));
	chacha20_reinit(key, blocknum);
}


/*
 * And now for auth.
 */

void
null_init(void *ctx)
{

}

int
null_update(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	return 0;
}

void
null_final(u_int8_t *buf, void *ctx)
{
	if (buf != (u_int8_t *) 0) {
		bzero(buf, 12);
	}
}

int
RMD160Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	RMD160Update(ctx, buf, len);
	return 0;
}

int
MD5Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	MD5Update(ctx, buf, len);
	return 0;
}

void
SHA1Init_int(void *ctx)
{
	SHA1Init(ctx);
}

int
SHA1Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA1Update(ctx, buf, len);
	return 0;
}

void
SHA1Final_int(u_int8_t *blk, void *ctx)
{
	SHA1Final(blk, ctx);
}

int
SHA256Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA256_Update(ctx, buf, len);
	return 0;
}

int
SHA384Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA384_Update(ctx, buf, len);
	return 0;
}

int
SHA512Update_int(void *ctx, const u_int8_t *buf, u_int16_t len)
{
	SHA512_Update(ctx, buf, len);
	return 0;
}

/*
 * And compression
 */

u_int32_t
deflate_compress(data, size, out)
	u_int8_t *data;
	u_int32_t size;
	u_int8_t **out;
{
	return deflate_global(data, size, 0, out);
}

u_int32_t
deflate_decompress(data, size, out)
	u_int8_t *data;
	u_int32_t size;
	u_int8_t **out;
{
	return deflate_global(data, size, 1, out);
}
