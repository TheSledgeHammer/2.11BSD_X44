/*	$NetBSD: esp.h,v 1.21 2003/07/20 03:24:03 itojun Exp $	*/
/*	$KAME: esp.h,v 1.19 2001/09/04 08:43:19 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

/*
 * RFC1827/2406 Encapsulated Security Payload.
 */

#ifndef _NETINET6_ESP_H_
#define _NETINET6_ESP_H_

#include "opt_inet.h"

struct esp {
	u_int32_t	esp_spi;	/* ESP */
	/* variable size, 32bit bound */	/* Initialization Vector */
	/* variable size */		/* Payload data */
	/* variable size */		/* padding */
	/* 8bit */			/* pad size */
	/* 8bit */			/* next header */
	/* variable size, 32bit bound */ /* Authentication data (new IPsec) */
};

struct newesp {
	u_int32_t	esp_spi;	/* ESP */
	u_int32_t	esp_seq;	/* Sequence number */
	/* variable size */		/* (IV and) Payload data */
	/* variable size */		/* padding */
	/* 8bit */			/* pad size */
	/* 8bit */			/* next header */
	/* variable size, 32bit bound *//* Authentication data */
};

struct esptail {
	u_int8_t	esp_padlen;	/* pad length */
	u_int8_t	esp_nxt;	/* Next header */
	/* variable size, 32bit bound *//* Authentication data (new IPsec)*/
};

#ifdef _KERNEL
struct secasvar;

/* crypt routines */
size_t esp_hdrsiz(struct ipsecrequest *);
int esp4_output(struct mbuf *, struct ipsecrequest *);
void esp4_input(struct mbuf *, ...);
void *esp4_ctlinput(int, struct sockaddr *, void *);

#ifdef INET6
int esp6_output(struct mbuf *, u_char *, struct mbuf *, struct ipsecrequest *);
int esp6_input(struct mbuf **, int *, int);
void esp6_ctlinput(int, struct sockaddr *, void *);
#endif /* INET6 */

int esp_auth(struct mbuf *, size_t, size_t, struct secasvar *, u_char *);

#ifdef IPSEC_CRYPTO

struct esp_algorithm {
	size_t padbound;	/* pad boundary, in byte */
#define blocksize padbound
	int ivlenval;		/* iv length, in byte */
	int (*mature)(struct secasvar *);
	int keymin;	/* in bits */
	int keymax;	/* in bits */
	size_t (*schedlen)(const struct esp_algorithm *);
	const char *name;
	int (*ivlen)(const struct esp_algorithm *, struct secasvar *);
	int (*decrypt)(struct mbuf *, size_t,
		struct secasvar *, const struct esp_algorithm *, int);
	int (*encrypt)(struct mbuf *, size_t, size_t,
		struct secasvar *, const struct esp_algorithm *, int);
	/* not supposed to be called directly */
	int (*schedule)(const struct esp_algorithm *, struct secasvar *);
	int (*blockdecrypt)(const struct esp_algorithm *,
		struct secasvar *, u_int8_t *, u_int8_t *);
	int (*blockencrypt)(const struct esp_algorithm *,
		struct secasvar *, u_int8_t *, u_int8_t *);
};

/* esp macros */
#define esp_mature(algo, sav)						(*algo->mature)(sav)
#define esp_schedlen(algo)							(*algo->schedlen)(algo)
#define esp_ivlen(algo, sav)						(*algo->ivlen)(algo, sav)
#define esp_decrypt(m, off, sav, algo, ivlen)		(*algo->decrypt)(m, off, sav, algo, ivlen)
#define esp_encrypt(m, off, plen, sav, algo, ivlen)	(*algo->encrypt)(m, off, plen, sav, algo, ivlen)

const struct esp_algorithm *esp_algorithm_lookup(int);
int esp_max_padbound(void);
int esp_max_ivlen(void);

int esp_schedule(const struct esp_algorithm *, struct secasvar *);

#endif /* IPSEC_CRYPTO */

#ifdef IPSEC_XFORM

#define	ESP_ALG_MAX	256		/* NB: could be < but skipjack is 249 */

const struct enc_xform *esp_algorithm_lookup(int);
u_int16_t esp_padbound(struct secasvar *, const struct enc_xform *);
u_int16_t esp_ivlen(struct secasvar *, const struct enc_xform *);
int esp_mature(struct secasvar *);

int esp_encrypt(struct mbuf *, size_t, size_t, struct ipsecrequest *, const struct enc_xform *, int);
int esp_decrypt(struct mbuf *, size_t, struct secasvar *, const struct enc_xform *, int);

int esp_schedule(const struct enc_xform *, struct secasvar *);

#endif /* IPSEC_XFORM */

#endif /* _KERNEL */

#endif /* _NETINET6_ESP_H_ */
