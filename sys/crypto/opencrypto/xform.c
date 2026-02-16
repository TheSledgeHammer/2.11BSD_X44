/*	$NetBSD: xform.c,v 1.31 2020/06/30 04:14:56 riastradh Exp $ */
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
//__KERNEL_RCSID(0, "$NetBSD: xform.c,v 1.31 2020/06/30 04:14:56 riastradh Exp $");

#include <sys/param.h>
#include <sys/malloc.h>

#include <crypto/md5/md5.h>
#include <crypto/sha1/sha1.h>
#include <crypto/sha2/sha2.h>
#include <crypto/blowfish/blowfish.h>
#include <crypto/cast128/cast128.h>
#include <crypto/chachapoly/chachapoly.h>
#include <crypto/aes/aes.h>
#include <crypto/des/des.h>
#include <crypto/mars/mars.h>
#include <crypto/rijndael/rijndael.h>
#include <crypto/ripemd160/rmd160.h>
#include <crypto/skipjack/skipjack.h>
#include <crypto/aes/aesxcbcmac.h>
#include <crypto/camellia/camellia.h>
#include <crypto/twofish/twofish.h>
#include <crypto/serpent/serpent.h>
#include <crypto/gmac/gmac.h>

#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/xform_wrapper.h>
#include <crypto/opencrypto/xform.h>

const u_int8_t hmac_ipad_buffer[128] = {
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
	0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36
};

const u_int8_t hmac_opad_buffer[128] = {
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C,
	0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C, 0x5C
};

/* Encryption instances */
const struct enc_xform enc_xform_null = {
	.type		= CRYPTO_NULL_CBC,
	.name		= "NULL",
	/* NB: blocksize of 4 is to generate a properly aligned ESP header */
	.blocksize	= 4,
	.ivsize		= 0,
	.minkey		= 0,
	.maxkey		= 256, /* 2048 bits, max key */
	.encrypt	= null_encrypt,
	.decrypt	= null_decrypt,
	.setkey		= null_setkey,
	.zerokey	= null_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_des = {
	.type		= CRYPTO_DES_CBC,
	.name		= "DES",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 8,
	.maxkey		= 8,
	.encrypt	= des1_encrypt,
	.decrypt	= des1_decrypt,
	.setkey		= des1_setkey,
	.zerokey	= des1_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_3des = {
	.type		= CRYPTO_3DES_CBC,
	.name		= "3DES",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 24,
	.maxkey		= 24,
	.encrypt	= des3_encrypt,
	.decrypt	= des3_decrypt,
	.setkey		= des3_setkey,
	.zerokey	= des3_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_blf = {
	.type		= CRYPTO_BLF_CBC,
	.name		= "Blowfish",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 5,
	.maxkey		= 56, /* 448 bits, max key */
	.encrypt	= blf_encrypt,
	.decrypt	= blf_decrypt,
	.setkey		= blf_setkey,
	.zerokey	= blf_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_cast5 = {
	.type		= CRYPTO_CAST_CBC,
	.name		= "CAST-128",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 5,
	.maxkey		= 16,
	.encrypt	= cast5_encrypt,
	.decrypt	= cast5_decrypt,
	.setkey		= cast5_setkey,
	.zerokey	= cast5_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_skipjack = {
	.type		= CRYPTO_SKIPJACK_CBC,
	.name		= "Skipjack",
	.blocksize	= 8,
	.ivsize		= 8,
	.minkey		= 10,
	.maxkey		= 10,
	.encrypt	= skipjack_encrypt,
	.decrypt	= skipjack_decrypt,
	.setkey		= skipjack_setkey,
	.zerokey	= skipjack_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_rijndael128 = {
	.type		= CRYPTO_RIJNDAEL128_CBC,
	.name		= "Rijndael-128/AES",
	.blocksize	= 16,
	.ivsize		= 8,
	.minkey		= 32,
	.maxkey		= 32,
	.encrypt	= rijndael128_encrypt,
	.decrypt	= rijndael128_decrypt,
	.setkey		= rijndael128_setkey,
	.zerokey	= rijndael128_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_aes = {
	.type		= CRYPTO_AES_CBC,
	.name		= "AES",
	.blocksize	= 16,
	.ivsize		= 16,
	.minkey		= 16,
	.maxkey		= 32,
	.encrypt	= aes_encrypt,
	.decrypt	= aes_decrypt,
	.setkey		= aes_setkey,
	.zerokey	= aes_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_arc4 = {
	.type		= CRYPTO_ARC4,
	.name		= "ARC4",
	.blocksize	= 1,
	.ivsize		= 0,
	.minkey		= 1,
	.maxkey		= 32,
	.encrypt	= NULL,
	.decrypt	= NULL,
	.setkey		= NULL,
	.zerokey	= NULL,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_aes_ctr = {
	.type		= CRYPTO_AES_CTR,
	.name		= "AES-CTR",
	.blocksize	= 16,
	.ivsize		= 8,
	.minkey		= 16 + 4,
	.maxkey		= 32 + 4,
	.encrypt	= aes_ctr_crypt,
	.decrypt	= aes_ctr_crypt,
	.setkey		= aes_ctr_setkey,
	.zerokey	= aes_ctr_zerokey,
	.reinit		= aes_ctr_reinit,
};

const struct enc_xform enc_xform_aes_xts = {
	.type		= CRYPTO_AES_XTS,
	.name		= "AES-XTS",
	.blocksize	= 16,
	.ivsize		= 8,
	.minkey		= 32,
	.maxkey		= 64,
	.encrypt	= aes_xts_encrypt,
	.decrypt	= aes_xts_decrypt,
	.setkey		= aes_xts_setkey,
	.zerokey	= aes_xts_zerokey,
	.reinit		= aes_xts_reinit,
};

const struct enc_xform enc_xform_aes_gcm = {
	.type		= CRYPTO_AES_GCM_16,
	.name		= "AES-GCM",
	.blocksize	= 4, /* ??? */
	.ivsize		= 8,
	.minkey		= 16 + 4,
	.maxkey		= 32 + 4,
	.encrypt	= aes_ctr_crypt,
	.decrypt	= aes_ctr_crypt,
	.setkey		= aes_ctr_setkey,
	.zerokey	= aes_ctr_zerokey,
	.reinit		= aes_gcm_reinit,
};

const struct enc_xform enc_xform_aes_gmac = {
	.type		= CRYPTO_AES_GMAC,
	.name		= "AES-GMAC",
	.blocksize	= 4, /* ??? */
	.ivsize		= 8,
	.minkey		= 16 + 4,
	.maxkey		= 32 + 4,
	.encrypt	= NULL,
	.decrypt	= NULL,
	.setkey		= NULL,
	.zerokey	= NULL,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_camellia = {
	.type		= CRYPTO_CAMELLIA_CBC,
	.name		= "Camellia",
	.blocksize	= 16,
	.ivsize		= 16,
	.minkey		= 8,
	.maxkey		= 32,
	.encrypt	= cml_encrypt,
	.decrypt	= cml_decrypt,
	.setkey		= cml_setkey,
	.zerokey	= cml_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_twofish  = {
	.type		= CRYPTO_TWOFISH_CBC,
	.name		= "Twofish",
	.blocksize	= 16,
	.ivsize		= 16,
	.minkey		= 8,
	.maxkey		= 32,
	.encrypt	= twofish128_encrypt,
	.decrypt	= twofish128_decrypt,
	.setkey		= twofish128_setkey,
	.zerokey	= twofish128_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_twofish_xts  = {
	.type		= CRYPTO_TWOFISH_XTS,
	.name		= "TWOFISH-XTS",
	.blocksize	= 16,
	.ivsize		= 8,
	.minkey		= 32,
	.maxkey		= 64,
	.encrypt	= twofish_xts_encrypt,
	.decrypt	= twofish_xts_decrypt,
	.setkey		= twofish_xts_setkey,
	.zerokey	= twofish_xts_zerokey,
	.reinit		= twofish_xts_reinit,
};

const struct enc_xform enc_xform_serpent  = {
	.type		= CRYPTO_SERPENT_CBC,
	.name		= "Serpent",
	.blocksize	= 16,
	.ivsize		= 16,
	.minkey		= 8,
	.maxkey		= 32,
	.encrypt	= serpent128_encrypt,
	.decrypt	= serpent128_decrypt,
	.setkey		= serpent128_setkey,
	.zerokey	= serpent128_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_serpent_xts  = {
	.type		= CRYPTO_SERPENT_XTS,
	.name		= "SERPENT-XTS",
	.blocksize	= 16,
	.ivsize		= 8,
	.minkey		= 32,
	.maxkey		= 64,
	.encrypt	= serpent_xts_encrypt,
	.decrypt	= serpent_xts_decrypt,
	.setkey		= serpent_xts_setkey,
	.zerokey	= serpent_xts_zerokey,
	.reinit		= serpent_xts_reinit,
};

const struct enc_xform enc_xform_mars = {
	.type		= CRYPTO_MARS_CBC,
	.name		= "Mars",
	.blocksize	= 16,
	.ivsize		= 16,
	.minkey		= 8,
	.maxkey		= 32,
	.encrypt	= mars128_encrypt,
	.decrypt	= mars128_decrypt,
	.setkey		= mars128_setkey,
	.zerokey	= mars128_zerokey,
	.reinit		= NULL,
};

const struct enc_xform enc_xform_mars_xts  = {
	.type		= CRYPTO_MARS_XTS,
	.name		= "MARS-XTS",
	.blocksize	= 16,
	.ivsize		= 8,
	.minkey		= 32,
	.maxkey		= 64,
	.encrypt	= mars_xts_encrypt,
	.decrypt	= mars_xts_decrypt,
	.setkey		= mars_xts_setkey,
	.zerokey	= mars_xts_zerokey,
	.reinit		= mars_xts_reinit,
};

const struct enc_xform enc_xform_chacha20_poly1305  = {
	.type		= CRYPTO_CHACHA20_POLY1305,
	.name		= "CHACHA20-POLY1305",
	.blocksize	= 1,
	.ivsize		= 8,
	.minkey		= 32+4,
	.maxkey		= 32+4,
	.encrypt	= chachapoly_encrypt,
	.decrypt	= chachapoly_decrypt,
	.setkey		= chachapoly_setkey,
	.zerokey	= chachapoly_zerokey,
	.reinit		= chachapoly_reinit,
};

/* Authentication instances */
const struct auth_hash auth_hash_null = {
	.type		= CRYPTO_NULL_HMAC,
	.name		= "NULL-HMAC",
	.keysize	= 0,
	.hashsize	= 0,
	.authsize	= 12,
	.blocksize	= 64,
	.ctxsize	= sizeof(int),
	.Init		= null_init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= null_update,
	.Final		= null_final,
};

const struct auth_hash auth_hash_hmac_md5 = {
	.type		= CRYPTO_MD5_HMAC,
	.name		= "HMAC-MD5",
	.keysize	= 16,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 64,
	.ctxsize	= sizeof(MD5_CTX),
	.Init		= (void (*)(void *))MD5Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= MD5Update_int,
	.Final		= (void (*)(u_int8_t *, void *))MD5Final,
};

const struct auth_hash auth_hash_hmac_sha1 = {
	.type		= CRYPTO_SHA1_HMAC,
	.name		= "HMAC-SHA1",
	.keysize	= 20,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 64,
	.ctxsize	= sizeof(SHA1_CTX),
	.Init		= SHA1Init_int,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA1Update_int,
	.Final		= SHA1Final_int,
};

const struct auth_hash auth_hash_hmac_ripemd_160 = {
	.type		= CRYPTO_RIPEMD160_HMAC,
	.name		= "HMAC-RIPEMD-160",
	.keysize	= 20,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 64,
	.ctxsize	= sizeof(RMD160_CTX),
	.Init		= (void (*)(void *))RMD160Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= RMD160Update_int,
	.Final		= (void (*)(u_int8_t *, void *))RMD160Final,
};

const struct auth_hash auth_hash_hmac_md5_96 = {
	.type		= CRYPTO_MD5_HMAC_96,
	.name		= "HMAC-MD5-96",
	.keysize	= 16,
	.hashsize	= 16,
	.authsize	= 12,
	.blocksize	= 64,
	.ctxsize	= sizeof(MD5_CTX),
	.Init		= (void (*)(void *))MD5Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= MD5Update_int,
	.Final		= (void (*)(u_int8_t *, void *))MD5Final,
};

const struct auth_hash auth_hash_hmac_sha1_96 = {
	.type		= CRYPTO_SHA1_HMAC_96,
	.name		= "HMAC-SHA1-96",
	.keysize	= 20,
	.hashsize	= 20,
	.authsize	= 12,
	.blocksize	= 64,
	.ctxsize	= sizeof(SHA1_CTX),
	.Init		= SHA1Init_int,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA1Update_int,
	.Final		= SHA1Final_int,
};

const struct auth_hash auth_hash_hmac_ripemd_160_96 = {
	.type		= CRYPTO_RIPEMD160_HMAC_96,
	.name		= "HMAC-RIPEMD-160",
	.keysize	= 20,
	.hashsize	= 20,
	.authsize	= 12,
	.blocksize	= 64,
	.ctxsize	= sizeof(RMD160_CTX),
	.Init		= (void (*)(void *))RMD160Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= RMD160Update_int,
	.Final		= (void (*)(u_int8_t *, void *))RMD160Final,
};

const struct auth_hash auth_hash_key_md5 = {
	.type		= CRYPTO_MD5_KPDK,
	.name		= "Keyed MD5",
	.keysize	= 0,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 0,
	.ctxsize	= sizeof(MD5_CTX),
	.Init		= (void (*)(void *))MD5Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= MD5Update_int,
	.Final		= (void (*)(u_int8_t *, void *))MD5Final,
};

const struct auth_hash auth_hash_key_sha1 = {
	.type		= CRYPTO_SHA1_KPDK,
	.name		= "Keyed SHA1",
	.keysize	= 0,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 0,
	.ctxsize	= sizeof(SHA1_CTX),
	.Init		= SHA1Init_int,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA1Update_int,
	.Final		= SHA1Final_int,
};

const struct auth_hash auth_hash_md5 = {
	.type		= CRYPTO_MD5,
	.name		= "MD5",
	.keysize	= 0,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 0,
	.ctxsize	= sizeof(MD5_CTX),
	.Init		= (void (*)(void *))MD5Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= MD5Update_int,
	.Final		= (void (*)(u_int8_t *, void *))MD5Final,
};

const struct auth_hash auth_hash_sha1 = {
	.type		= CRYPTO_SHA1,
	.name		= "SHA1",
	.keysize	= 0,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 0,
	.ctxsize	= sizeof(SHA1_CTX),
	.Init		= (void (*)(void *))SHA1Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA1Update_int,
	.Final		= (void (*)(u_int8_t *, void *))SHA1Final,
};

const struct auth_hash auth_hash_hmac_sha2_256 = {
	.type		= CRYPTO_SHA2_256_HMAC,
	.name		= "HMAC-SHA2",
	.keysize	= 32,
	.hashsize	= 32,
	.authsize	= 16,
	.blocksize	= 64,
	.ctxsize	= sizeof(SHA256_CTX),
	.Init		= (void (*)(void *))(void *)SHA256_Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA256Update_int,
	.Final		= (void (*)(u_int8_t *, void *))(void *)SHA256_Final,
};

const struct auth_hash auth_hash_hmac_sha2_384 = {
	.type		= CRYPTO_SHA2_384_HMAC,
	.name		= "HMAC-SHA2-384",
	.keysize	= 48,
	.hashsize	= 48,
	.authsize	= 24,
	.blocksize	= 128,
	.ctxsize	= sizeof(SHA384_CTX),
	.Init		= (void (*)(void *))(void *)SHA384_Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA384Update_int,
	.Final		= (void (*)(u_int8_t *, void *))(void *)SHA384_Final,
};

const struct auth_hash auth_hash_hmac_sha2_512 = {
	.type		= CRYPTO_SHA2_512_HMAC,
	.name		= "HMAC-SHA2-512",
	.keysize	= 64,
	.hashsize	= 64,
	.authsize	= 32,
	.blocksize	= 128,
	.ctxsize	= sizeof(SHA512_CTX),
	.Init		= (void (*)(void *))(void *)SHA512_Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA512Update_int,
	.Final		= (void (*)(u_int8_t *, void *))(void *)SHA512_Final,
};

const struct auth_hash auth_hash_aes_xcbc_mac_96 = {
	.type		= CRYPTO_AES_XCBC_MAC_96,
	.name		= "AES-XCBC-MAC-96",
	.keysize	= 16,
	.hashsize	= 16,
	.authsize	= 12,
	.blocksize	= 0,
	.ctxsize	= sizeof(aesxcbc_ctx),
	.Init		= null_init,
	.Setkey		= (void (*)(void *, const u_int8_t *, u_int16_t))(void *)aes_xcbc_mac_init,
	.Reinit		= NULL,
	.Update		= aes_xcbc_mac_loop,
	.Final		= aes_xcbc_mac_result,
};

const struct auth_hash auth_hash_gmac_aes_128 = {
	.type		= CRYPTO_AES_128_GMAC,
	.name		= "GMAC-AES-128",
	.keysize	= 16 + 4,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 16, /* ??? */
	.ctxsize	= sizeof(AES_GMAC_CTX),
	.Init		= (void (*)(void *))AES_GMAC_Init,
	.Setkey		= (void (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Setkey,
	.Reinit		= (void (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Reinit,
	.Update		= (int (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Update,
	.Final		= (void (*)(u_int8_t *, void *))AES_GMAC_Final,
};

const struct auth_hash auth_hash_gmac_aes_192 = {
	.type		= CRYPTO_AES_192_GMAC,
	.name		= "GMAC-AES-192",
	.keysize	= 24 + 4,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 16, /* ??? */
	.ctxsize	= sizeof(AES_GMAC_CTX),
	.Init		= (void (*)(void *))AES_GMAC_Init,
	.Setkey		= (void (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Setkey,
	.Reinit		= (void (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Reinit,
	.Update		= (int (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Update,
	.Final		= (void (*)(u_int8_t *, void *))AES_GMAC_Final,
};

const struct auth_hash auth_hash_gmac_aes_256 = {
	.type		= CRYPTO_AES_256_GMAC,
	.name		= "GMAC-AES-256",
	.keysize	= 32 + 4,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 16, /* ??? */
	.ctxsize	= sizeof(AES_GMAC_CTX),
	.Init		= (void (*)(void *))AES_GMAC_Init,
	.Setkey		= (void (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Setkey,
	.Reinit		= (void (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Reinit,
	.Update		= (int (*)(void *, const u_int8_t *, u_int16_t))AES_GMAC_Update,
	.Final		= (void (*)(u_int8_t *, void *))AES_GMAC_Final,
};

const struct auth_hash auth_hash_chacha20_poly1305 = {
	.type		= CRYPTO_CHACHA20_POLY1305_MAC,
	.name		= "CHACHA20-POLY1305",
	.keysize	= 32+4,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 64, /* ??? */
	.ctxsize	= sizeof(CHACHA20_POLY1305_CTX),
	.Init		= (void (*)(void *))Chacha20_Poly1305_Init,
	.Setkey		= (void (*)(void *, const u_int8_t *, u_int16_t))Chacha20_Poly1305_Setkey,
	.Reinit		= (void (*)(void *, const u_int8_t *, u_int16_t))Chacha20_Poly1305_Reinit,
	.Update		= (int (*)(void *, const u_int8_t *, u_int16_t))Chacha20_Poly1305_Update,
	.Final		= (void (*)(u_int8_t *, void *))Chacha20_Poly1305_Final,
};

/* Compression instance */
const struct comp_algo comp_algo_deflate = {
	.type		= CRYPTO_DEFLATE_COMP,
	.name		= "Deflate",
	.minlen		= 90,
	.compress 	= deflate_compress,
	.decompress	= deflate_decompress,
};
