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

#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/xform.h>
#include <crypto/opencrypto/xform_wrapper.h>

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
	.Init		= MD5Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= MD5Update_int,
	.Final		= MD5Final,
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
	.Init		= RMD160Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= RMD160Update_int,
	.Final		= RMD160Final,
};

const struct auth_hash auth_hash_hmac_md5_96 = {
	.type		= CRYPTO_MD5_HMAC_96,
	.name		= "HMAC-MD5-96",
	.keysize	= 16,
	.hashsize	= 16,
	.authsize	= 12,
	.blocksize	= 64,
	.ctxsize	= sizeof(MD5_CTX),
	.Init		= MD5Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= MD5Update_int,
	.Final		= MD5Final,
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
	.Init		= RMD160Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= RMD160Update_int,
	.Final		= RMD160Final,
};

const struct auth_hash auth_hash_key_md5 = {
	.type		= CRYPTO_MD5_KPDK,
	.name		= "Keyed MD5",
	.keysize	= 0,
	.hashsize	= 16,
	.authsize	= 16,
	.blocksize	= 0,
	.ctxsize	= sizeof(MD5_CTX),
	.Init		= MD5Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= MD5Update_int,
	.Final		= MD5Final,
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
	.Init		= MD5Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= MD5Update_int,
	.Final		= MD5Final,
};

const struct auth_hash auth_hash_sha1 = {
	.type		= CRYPTO_SHA1,
	.name		= "SHA1",
	.keysize	= 0,
	.hashsize	= 20,
	.authsize	= 20,
	.blocksize	= 0,
	.ctxsize	= sizeof(SHA1_CTX),
	.Init		= SHA1Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA1Update_int,
	.Final		= SHA1Final,
};

const struct auth_hash auth_hash_hmac_sha2_256 = {
	.type		= CRYPTO_SHA2_256_HMAC,
	.name		= "HMAC-SHA2",
	.keysize	= 32,
	.hashsize	= 32,
	.authsize	= 16,
	.blocksize	= 64,
	.ctxsize	= sizeof(SHA256_CTX),
	.Init		= SHA256_Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA256Update_int,
	.Final		= SHA256_Final,
};

const struct auth_hash auth_hash_hmac_sha2_384 = {
	.type		= CRYPTO_SHA2_384_HMAC,
	.name		= "HMAC-SHA2-384",
	.keysize	= 48,
	.hashsize	= 48,
	.authsize	= 24,
	.blocksize	= 128,
	.ctxsize	= sizeof(SHA384_CTX),
	.Init		= SHA384_Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA384Update_int,
	.Final		= SHA384_Final,
};

const struct auth_hash auth_hash_hmac_sha2_512 = {
	.type		= CRYPTO_SHA2_512_HMAC,
	.name		= "HMAC-SHA2-512",
	.keysize	= 64,
	.hashsize	= 64,
	.authsize	= 32,
	.blocksize	= 128,
	.ctxsize	= sizeof(SHA512_CTX),
	.Init		= SHA512_Init,
	.Setkey		= NULL,
	.Reinit		= NULL,
	.Update		= SHA512Update_int,
	.Final		= SHA512_Final,
};

/* Compression instance */
const struct comp_algo comp_algo_deflate = {
	.type		= CRYPTO_DEFLATE_COMP,
	.name		= "Deflate",
	.minlen		= 90,
	.compress 	= deflate_compress,
	.decompress	= deflate_decompress,
};
