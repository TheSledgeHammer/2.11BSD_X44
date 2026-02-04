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

#ifndef _CRYPTO_XFORM_WRAPPER_H_
#define _CRYPTO_XFORM_WRAPPER_H_

void 	null_encrypt(caddr_t, u_int8_t *);
void 	null_decrypt(caddr_t, u_int8_t *);
int 	null_setkey(u_int8_t **, const u_int8_t *, int);
void 	null_zerokey(u_int8_t **);

int 	des1_setkey(u_int8_t **, const u_int8_t *, int);
void 	des1_encrypt(caddr_t, u_int8_t *);
void 	des1_decrypt(caddr_t, u_int8_t *);
void 	des1_zerokey(u_int8_t **);

int 	des3_setkey(u_int8_t **, const u_int8_t *, int);
void 	des3_encrypt(caddr_t, u_int8_t *);
void 	des3_decrypt(caddr_t, u_int8_t *);
void 	des3_zerokey(u_int8_t **);

int 	blf_setkey(u_int8_t **, const u_int8_t *, int);
void 	blf_encrypt(caddr_t, u_int8_t *);
void 	blf_decrypt(caddr_t, u_int8_t *);
void 	blf_zerokey(u_int8_t **);

int 	cast5_setkey(u_int8_t **, const u_int8_t *, int);
void 	cast5_encrypt(caddr_t, u_int8_t *);
void 	cast5_decrypt(caddr_t, u_int8_t *);
void 	cast5_zerokey(u_int8_t **);

int 	skipjack_setkey(u_int8_t **, const u_int8_t *, int);
void 	skipjack_encrypt(caddr_t, u_int8_t *);
void 	skipjack_decrypt(caddr_t, u_int8_t *);
void 	skipjack_zerokey(u_int8_t **);

int 	rijndael128_setkey(u_int8_t **, const u_int8_t *, int);
void 	rijndael128_encrypt(caddr_t, u_int8_t *);
void 	rijndael128_decrypt(caddr_t, u_int8_t *);
void 	rijndael128_zerokey(u_int8_t **);

int 	aes_setkey(u_int8_t **, const u_int8_t *, int);
void 	aes_encrypt(caddr_t, u_int8_t *);
void 	aes_decrypt(caddr_t, u_int8_t *);
void	aes_zerokey(u_int8_t **);

int 	aes_ctr_setkey(u_int8_t **, const u_int8_t *, int);
void	aes_ctr_crypt(caddr_t, u_int8_t *);
void	aes_ctr_zerokey(u_int8_t **);
void	aes_ctr_reinit(void *, const u_int8_t *, u_int8_t *);

int 	aes_xts_setkey(u_int8_t **, const u_int8_t *, int);
void	aes_xts_encrypt(caddr_t, u_int8_t *);
void	aes_xts_decrypt(caddr_t, u_int8_t *);
void	aes_xts_zerokey(u_int8_t **);
void	aes_xts_reinit(void *, const u_int8_t *, u_int8_t *);

void	aes_gcm_reinit(void *, const u_int8_t *, u_int8_t *);

int 	cml_setkey(u_int8_t **, const u_int8_t *, int);
void 	cml_encrypt(caddr_t, u_int8_t *);
void 	cml_decrypt(caddr_t, u_int8_t *);
void	cml_zerokey(u_int8_t **);

int 	twofish128_setkey(u_int8_t **, const u_int8_t *, int);
void 	twofish128_encrypt(caddr_t, u_int8_t *);
void 	twofish128_decrypt(caddr_t, u_int8_t *);
void 	twofish128_zerokey(u_int8_t **);

int 	twofish_xts_setkey(u_int8_t **, const u_int8_t *, int);
void 	twofish_xts_encrypt(caddr_t, u_int8_t *);
void 	twofish_xts_decrypt(caddr_t, u_int8_t *);
void 	twofish_xts_zerokey(u_int8_t **);
void	twofish_xts_reinit(void *, const u_int8_t *, u_int8_t *);

int 	serpent128_setkey(u_int8_t **, const u_int8_t *, int);
void 	serpent128_encrypt(caddr_t, u_int8_t *);
void 	serpent128_decrypt(caddr_t, u_int8_t *);
void 	serpent128_zerokey(u_int8_t **);

int 	serpent_xts_setkey(u_int8_t **, const u_int8_t *, int);
void 	serpent_xts_encrypt(caddr_t, u_int8_t *);
void 	serpent_xts_decrypt(caddr_t, u_int8_t *);
void 	serpent_xts_zerokey(u_int8_t **);
void	serpent_xts_reinit(void *, const u_int8_t *, u_int8_t *);

int 	mars128_setkey(u_int8_t **, const u_int8_t *, int);
void 	mars128_encrypt(caddr_t, u_int8_t *);
void 	mars128_decrypt(caddr_t, u_int8_t *);
void 	mars128_zerokey(u_int8_t **);

int 	mars_xts_setkey(u_int8_t **, const u_int8_t *, int);
void 	mars_xts_encrypt(caddr_t, u_int8_t *);
void 	mars_xts_decrypt(caddr_t, u_int8_t *);
void 	mars_xts_zerokey(u_int8_t **);
void	mars_xts_reinit(void *, const u_int8_t *, u_int8_t *);

int 	chachapoly_setkey(u_int8_t **, const u_int8_t *, int);
void 	chachapoly_encrypt(caddr_t, u_int8_t *);
void 	chachapoly_decrypt(caddr_t, u_int8_t *);
void 	chachapoly_zerokey(u_int8_t **);
void	chachapoly_reinit(void *, const u_int8_t *, u_int8_t *);

void 	null_init(void *);
int 	null_update(void *, const u_int8_t *, u_int16_t);
void 	null_final(u_int8_t *, void *);

int		MD5Update_int(void *, const u_int8_t *, u_int16_t);
void 	SHA1Init_int(void *);
int 	SHA1Update_int(void *, const u_int8_t *, u_int16_t);
void 	SHA1Final_int(u_int8_t *, void *);

int 	RMD160Update_int(void *, const u_int8_t *, u_int16_t);
int 	SHA1Update_int(void *, const u_int8_t *, u_int16_t);
void 	SHA1Final_int(u_int8_t *, void *);
int 	RMD160Update_int(void *, const u_int8_t *, u_int16_t);
int 	SHA256Update_int(void *, const u_int8_t *, u_int16_t);
int 	SHA384Update_int(void *, const u_int8_t *, u_int16_t);
int 	SHA512Update_int(void *, const u_int8_t *, u_int16_t);

u_int32_t deflate_compress(u_int8_t *, u_int32_t, u_int8_t **);
u_int32_t deflate_decompress(u_int8_t *, u_int32_t, u_int8_t **);

#endif /* _CRYPTO_XFORM_WRAPPER_H_ */
