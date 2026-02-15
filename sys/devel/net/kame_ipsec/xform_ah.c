/*	$NetBSD: ah_core.c,v 1.36 2004/03/10 03:45:04 itojun Exp $	*/
/*	$KAME: ah_core.c,v 1.57 2003/07/25 09:33:36 itojun Exp $	*/

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
/*	$NetBSD: xform_ah.c,v 1.6.2.1.4.1 2007/12/01 17:32:28 bouyer Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform_ah.c,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$OpenBSD: ip_ah.c,v 1.63 2001/06/26 06:18:58 angelos Exp $ */
/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr) and
 * Niels Provos (provos@physnet.uni-hamburg.de).
 *
 * The original version of this code was written by John Ioannidis
 * for BSD/OS in Athens, Greece, in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis.
 *
 * Additional transforms and features in 1997 and 1998 by Angelos D. Keromytis
 * and Niels Provos.
 *
 * Additional features in 1999 by Angelos D. Keromytis and Niklas Hallqvist.
 *
 * Copyright (c) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
 * Copyright (c) 1999 Niklas Hallqvist.
 * Copyright (c) 2001 Angelos D. Keromytis.
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
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_ecn.h>
#include <netinet/ip6.h>

#include <kame_ipsec/ipsec.h>
#include <kame_ipsec/ah.h>
#include <kame_ipsec/esp.h>
#include <kame_ipsec/xform.h>

#ifdef INET6
#include <netinet6/ip6_var.h>
#endif

#include <net/pfkeyv2.h>
#include <netkey/key.h>
//#include <netkey/key_debug.h>

#include <net/net_osdep.h>

#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/xform.h>

static int ah_sumsiz_1216(struct secasvar *);
static int ah_sumsiz_zero(struct secasvar *);

static int ah_input(struct mbuf *, struct secasvar *, int, int, int);
static int ah_input_cb(struct cryptop *);
static int ah_output(struct mbuf *, struct ipsecrequest *, struct mbuf **, int, int, int);
static int ah_output_cb(struct cryptop *);
static void ah_update_mbuf(struct mbuf *, int, int, struct secasvar *, const struct auth_hash *, struct xform_state *);
#ifdef INET
static int ah4_calccksum(struct mbuf *, u_int8_t *, size_t, const struct auth_hash *, struct secasvar *);
#endif
#ifdef INET6
static int ah6_calccksum(struct mbuf *, u_int8_t *, size_t, const struct auth_hash *, struct secasvar *);
#endif

const struct auth_hash *
ah_algorithm_lookup(int alg)
{
	if (alg >= AH_ALG_MAX)
		return NULL;
	switch (alg) {
	case SADB_X_AALG_NULL:
		return &auth_hash_null;
	case SADB_AALG_MD5HMAC:
		return &auth_hash_hmac_md5_96;
	case SADB_AALG_SHA1HMAC:
		return &auth_hash_hmac_sha1_96;
	case SADB_X_AALG_RIPEMD160HMAC:
		return &auth_hash_hmac_ripemd_160_96;
	case SADB_X_AALG_MD5:
		return &auth_hash_key_md5;
	case SADB_X_AALG_SHA:
		return &auth_hash_key_sha1;
	case SADB_X_AALG_SHA2_256:
		return &auth_hash_hmac_sha2_256;
	case SADB_X_AALG_SHA2_384:
		return &auth_hash_hmac_sha2_384;
	case SADB_X_AALG_SHA2_512:
		return &auth_hash_hmac_sha2_512;
	case SADB_X_AALG_AES_XCBC_MAC:
		return &auth_hash_aes_xcbc_mac_96;
	}
	return (NULL);
}

/*
 * compute AH header size.
 * transport mode only.  for tunnel mode, we should implement
 * virtual interface, and control MTU/MSS by the interface MTU.
 */
size_t
ah_hdrsiz(isr)
	struct ipsecrequest *isr;
{
	const struct auth_hash *thash;
	size_t hdrsiz;

	/* sanity check */
	if (isr == NULL)
		panic("ah_hdrsiz: NULL was passed.");

	if (isr->saidx.proto != IPPROTO_AH)
		panic("unsupported mode passed to ah_hdrsiz");

	if (isr->sav == NULL) {
		goto estimate;
	}
	if (isr->sav->state != SADB_SASTATE_MATURE
			&& isr->sav->state != SADB_SASTATE_DYING) {
		goto estimate;
	}

	/* we need transport mode AH. */
	thash = ah_algorithm_lookup(isr->sav->alg_auth);
	if (!thash) {
		goto estimate;
	}

	/*
	 * XXX
	 * right now we don't calcurate the padding size.  simply
	 * treat the padding size as constant, for simplicity.
	 *
	 * XXX variable size padding support
	 */
	hdrsiz = ((ah_sumsiz(isr->sav, thash) + 3) & ~(4 - 1));
	if (isr->sav->flags & SADB_X_EXT_OLD) {
		hdrsiz += sizeof(struct ah) + sizeof(u_int32_t) + 16;
	} else {
		hdrsiz += sizeof(struct newah) + sizeof(u_int32_t) + 16;
	}

	return (hdrsiz);

estimate:

	/*
	 * ASSUMING:
	 *	sizeof(struct newah) > sizeof(struct ah).
	 *	AH_MAXSUMSIZE is multiple of 4.
	 */
	return ((sizeof(struct newah) + AH_MAXSUMSIZE));
}

/* Calculate AH length */
int
ah_hdrlen(sav)
	struct secasvar *sav;
{
	const struct auth_hash *thash;
	int plen, ahlen;

	thash = ah_algorithm_lookup(sav->alg_auth);
	if (!thash) {
		return (0);
	}
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1826 */
		plen = (ah_sumsiz(sav, thash) + 3) & ~(4 - 1);	/* XXX pad to 8byte? */
		ahlen = plen + sizeof(struct ah);
	} else {
		/* RFC 2402 */
		plen = (ah_sumsiz(sav, thash) + 3) & ~(4 - 1);	/* XXX pad to 8byte? */
		ahlen = plen + sizeof(struct newah);
	}
	return (ahlen);
}


int
ah_mature(sav)
	struct secasvar *sav;
{
	const struct auth_hash *thash;

	if (!sav->key_auth) {
		ipseclog((LOG_ERR, "ah_mature: no key is given.\n"));
		return (1);
	}

	thash = ah_algorithm_lookup(sav->alg_auth);
	if (!thash) {
		ipseclog((LOG_ERR, "ah_mature: unsupported algorithm.\n"));
		return (1);
	}

	if (sav->key_auth->sadb_key_bits < algo->keymin ||
	    thash->keysize < sav->key_auth->sadb_key_bits) {
		ipseclog((LOG_ERR,
		    "ah_mature: invalid key length %d for %s.\n",
		    sav->key_auth->sadb_key_bits, thash->name));
		return (1);
	}
	return (0);
}

/*
 * NB: public for use by esp_init.
 */
int
ah_init0(sav, xsp, cria)
	struct secasvar *sav;
	struct xformsw *xsp;
	struct cryptoini *cria;
{
	struct tdb *tdb;
	const struct auth_hash *thash;
	int keylen;

	thash = ah_algorithm_lookup(sav->alg_auth);
	if (thash == NULL) {
		ipseclog((LOG_ERR, "ah_init: unsupported authentication algorithm %u\n", sav->alg_auth));
		return EINVAL;
	}
	/*
	 * Verify the replay state block allocation is consistent with
	 * the protocol type.  We check here so we can make assumptions
	 * later during protocol processing.
	 */
	/* NB: replay state is setup elsewhere (sigh) */
	if (((sav->flags & SADB_X_EXT_OLD) == 0) ^ (sav->replay != NULL)) {
		ipseclog((LOG_ERR, "ah_init: replay state block inconsistency, "
						"%s algorithm %s replay state\n",
						(sav->flags & SADB_X_EXT_OLD) ? "old" : "new",
						sav->replay == NULL ? "without" : "with"));
		return (EINVAL);
	}
	if (sav->key_auth == NULL) {
		ipseclog((LOG_ERR, "ah_init: no authentication key for %s "
				"algorithm\n", thash->name));
		return (EINVAL);
	}
	keylen = _KEYLEN(sav->key_auth);
	if (keylen != thash->keysize && thash->keysize != 0) {
		ipseclog((LOG_ERR, "ah_init: invalid keylength %d, algorithm "
						"%s requires keysize %d\n", keylen, thash->name, thash->keysize));
		return (EINVAL);
	}

	tdb = tdb_init(sav, xsp, NULL, thash, NULL, IPPROTO_AH);
	if (tdb == NULL) {
		ipseclog((LOG_ERR, "ah_init: no memory for tunnel descriptor block\n"));
		return (EINVAL);
	}

	/* Initialize crypto session. */
	bzero(cria, sizeof(*cria));
	cria->cri_alg = tdb->tdb_authalgxform->type;
	cria->cri_klen = _KEYBITS(sav->key_auth);
	cria->cri_key = _KEYBUF(sav->key_auth);

	return (0);
}

/*
 * ah_init() is called when an SPI is being set up.
 */
static int
ah_init(sav, xsp)
	struct secasvar *sav;
	struct xformsw *xsp;
{
	struct cryptoini cria;
	int error;

	error = ah_init0(sav, xsp, &cria);
	return (error ? error :
		 crypto_newsession(&sav->tdb_tdb->tdb_cryptoid, &cria, crypto_support));
}

/*
 * Paranoia.
 *
 * NB: public for use by esp_zeroize (XXX).
 */
int
ah_zeroize(sav)
    struct secasvar *sav;
{
	if (sav->key_auth) {
		bzero(_KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
	}
	return (tdb_zeroize(sav->tdb_tdb));
}

static int
ah_sumsiz_1216(sav)
	struct secasvar *sav;
{
	int authsize;

	if (!sav) {
		panic("ah_sumsiz_1216: null pointer is passed");
	}
	if (sav->flags & SADB_X_EXT_OLD) {
		authsize = 16;
	} else {
		authsize = sav->tdb_tdb->tdb_authalgxform->authsize;
	}
	return (authsize);
}

static int
ah_sumsiz_zero(sav)
	struct secasvar *sav;
{
	if (!sav) {
		panic("ah_sumsiz_zero: null pointer is passed");
	}
	return (0);
}

int
ah_sumsiz(sav, thash)
	struct secasvar *sav;
	const struct auth_hash *thash;
{
	if (thash == &auth_hash_null) {
		return (ah_sumsiz_zero(sav));
	}
	return (ah_sumsiz_1216(sav));
}

int
ah_hash_init(state, sav, thash)
	struct xform_state *state;
	struct secasvar *sav;
	const struct auth_hash *thash;
{
	if (!state) {
		panic("xform_ah_init: what?");
		return (ENOBUFS);
	}

	state->xs_sav = sav;
	state->xs_ctx = (void *)malloc(sizeof(thash->ctxsize), M_TEMP, M_NOWAIT);
	if (state->xs_ctx == NULL) {
		return (ENOBUFS);
	}

	(*thash->Init)(state->xs_ctx);
	if (state->xs_sav) {
		(void)(*thash->Update)(state->xs_ctx, _KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
	}
	return (0);
}

void
ah_hash_update(state, sav, thash, buf, len)
	struct xform_state *state;
	struct secasvar *sav;
	const struct auth_hash *thash;
	const u_int8_t *buf;
	u_int16_t len;
{
	if (!state || !state->xs_ctx) {
		panic("xform_ah_update: what?");
	}
	(void)(*thash->Update)(state->xs_ctx, buf, len);
}

void
ah_hash_result(state, sav, thash, addr, len)
	struct xform_state *state;
	struct secasvar *sav;
	const struct auth_hash *thash;
	u_int8_t *addr;
	size_t len;
{
	u_int8_t digest[64];

	if (!state || !state->xs_ctx) {
		panic("xform_ah_result: what?");
	}

	switch (sav->alg_auth) {
	case SADB_X_AALG_NULL:
		return;
	case SADB_X_AALG_MD5:
	case SADB_X_AALG_SHA:
		if (state->xs_sav) {
			(*thash->Update)(state->xs_ctx, _KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
		}
		(*thash->Final)(digest, state->xs_ctx);
		break;
	case SADB_AALG_MD5HMAC:
	case SADB_AALG_SHA1HMAC:
		(*thash->Final)(digest, state->xs_ctx);
		(*thash->Init)(state->xs_ctx);
		(*thash->Update)(state->xs_ctx, _KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
		(*thash->Update)(state->xs_ctx, _KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
		(*thash->Final)(digest, state->xs_ctx);
		break;
	case SADB_X_AALG_RIPEMD160HMAC:
	case SADB_X_AALG_SHA2_256:
	case SADB_X_AALG_SHA2_384:
	case SADB_X_AALG_SHA2_512:
		(*thash->Final)(digest, state->xs_ctx);
		bzero(state->xs_ctx, sizeof(*state->xs_ctx));
		(*thash->Init)(state->xs_ctx);
		(*thash->Update)(state->xs_ctx, _KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
		(*thash->Update)(state->xs_ctx, _KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
		(*thash->Final)(digest, state->xs_ctx);
		break;
	case SADB_X_AALG_AES_XCBC_MAC:
		(*thash->Final)(digest, state->xs_ctx);
		break;
	default:
		return;
	}
	bcopy(digest, addr, sizeof(digest) > len ? len : sizeof(digest));
	free(state->xs_ctx, M_TEMP);
}

/*------------------------------------------------------------*/

static int
ah_input(m, sav, skip, length, offset)
	struct mbuf *m;
	struct secasvar *sav;
	int skip, length, offset;
{
	const struct auth_hash *ahx;
	struct tdb *tdb;
	struct cryptop *crp;
	int rplen;

	struct cryptodesc *crda, *crde;

	tdb = sav->tdb_tdb;
	ahx = tdb->tdb_authalgxform;

	/* Figure out header size. */
	rplen = (ah_sumsiz(sav, ahx) + 3) & ~(4 - 1);
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1826 */
		offset = sizeof(struct ah);
		length = rplen;
	} else {
		/* RFC 2402 */
		offset = sizeof(struct newah);
		length = rplen;
	}

	/* Get crypto descriptors. */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		ipseclog((LOG_ERR, "ah_input: failed to acquire crypto descriptor\n"));
		//ahstat.ahs_crypto++;
		m_freem(m);
		return (ENOBUFS);
	}

	if (ahx) {
		crda = crp->crp_desc;
		crda->crd_skip = 0;
		crda->crd_len = m->m_pkthdr.len;
		crda->crd_inject = skip + rplen;

		/* Authentication operation. */
		crda->crd_alg = ahx->type;
		crda->crd_key = _KEYBUF(sav->key_auth);
		crda->crd_klen = _KEYBITS(sav->key_auth);
	}

	tdb = tdb_alloc(0);
	if (tdb == NULL) {
		ipseclog((LOG_ERR, "ah_input: failed to allocate tdb_crypto\n"));
		//ahstat.ahs_crypto++;
		crypto_freereq(crp);
		m_freem(m);
		return (ENOBUFS);
	}

	sav->tdb_tdb = tdb;
	tdb->tdb_sav = sav;

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = ah_input_cb;
	crp->crp_sid = tdb->tdb_cryptoid;
	crp->crp_opaque = (caddr_t)tdb;

	/* These are passed as-is to the callback. */
	tdb->tdb_spi = sav->spi;
	tdb->tdb_dst = sav->sah->saidx.dst;
	tdb->tdb_src = sav->sah->saidx.src;
	tdb->tdb_proto = sav->sah->saidx.proto;
	tdb->tdb_skip = skip;
	tdb->tdb_length = length;
	tdb->tdb_offset = offset;

	return (ah_input_cb(crp));
}

static int
ah_input_cb(crp)
	struct cryptop *crp;
{
	struct secasvar *sav;
	struct mbuf *m;
	struct tdb *tdb;
	const struct auth_hash *ahx;
	int s, error, skip, length, offset;

	tdb = (struct tdb *)crp->crp_opaque;
	m = (struct mbuf *)crp->crp_buf;
	skip = tdb->tdb_skip;
	length = tdb->tdb_length;
	offset = tdb->tdb_offset;

	s = splsoftnet();
#ifdef INET
	sav = key_allocsa(AF_INET, (caddr_t)tdb_get_in(tdb, 0), (caddr_t)tdb_get_in(tdb, 1), tdb->tdb_proto, tdb->tdb_spi);
#endif
#ifdef INET6
	sav = key_allocsa(AF_INET6, (caddr_t)tdb_get_in6(tdb, 0), (caddr_t)tdb_get_in6(tdb, 1), tdb->tdb_proto, tdb->tdb_spi);
#endif
	if (sav == NULL) {
		ipseclog((LOG_ERR, "ah_input_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	ahx = tdb->tdb_authalgxform;

	/* Check for crypto errors */
	if (crp->crp_etype) {
		/* Reset the session ID */
		if (tdb->tdb_cryptoid != 0)
			tdb->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			key_freesav(sav);
			splx(s);
			return (crypto_dispatch(crp));
		}

		ipseclog((LOG_ERR, "ah_input_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		ipseclog((LOG_ERR, "ah_input_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}

	/* Release the crypto descriptors */
	tdb_free(tdb);
	tdb = NULL;
	crypto_freereq(crp);
	crp = NULL;

#ifdef INET
	error = ah4_calccksum(m, (u_int8_t *)skip, (length + offset), ahx, sav);
#endif
#ifdef INET6
	error = ah6_calccksum(m, (u_int8_t *)skip, (length + offset), ahx, sav);
#endif

	key_freesav(sav);
	splx(s);
	return (error);

bad:
	if (sav) {
		key_freesav(sav);
	}
	splx(s);
	if (m != NULL) {
		m_freem(m);
	}
	if (tdb != NULL) {
		tdb_free(tdb);
	}
	if (crp != NULL) {
		crypto_freereq(crp);
	}
	return (error);
}

static int
ah_output(m, isr, mp, skip, length, offset)
	struct mbuf *m;
	struct ipsecrequest *isr;
	struct mbuf **mp;
	int skip, length, offset;
{
	struct secasvar *sav;
	const struct auth_hash *ahx;
	struct tdb *tdb;
	struct cryptop *crp;
	int error, rplen;
	struct cryptodesc *crda, *crde;

	sav = isr->sav;
	tdb = sav->tdb_tdb;
	ahx = tdb->tdb_authalgxform;

	/* Figure out header size. */
	rplen = (ah_sumsiz(sav, ahx) + 3) & ~(4 - 1);
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1826 */
		offset = sizeof(struct ah);
		length = rplen;
	} else {
		/* RFC 2402 */
		offset = sizeof(struct newah);
		length = rplen;
	}

	/* Get crypto descriptors. */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		ipseclog((LOG_ERR, "ah_output: failed to acquire crypto descriptors\n"));
		//ahstat.ahs_crypto++;
		error = ENOBUFS;
		goto bad;
	}

	crda = crp->crp_desc;

	crda->crd_skip = 0;
	crda->crd_inject = skip + rplen;
	crda->crd_len = m->m_pkthdr.len;

	/* Authentication operation. */
	crda->crd_alg = ahx->type;
	crda->crd_key = _KEYBUF(sav->key_auth);
	crda->crd_klen = _KEYBITS(sav->key_auth);

	/* Allocate IPsec-specific opaque crypto info. */
	tdb = tdb_alloc(skip);
	if (tdb == NULL) {
		crypto_freereq(crp);
		ipseclog((LOG_ERR, "ah_output: failed to allocate tdb_crypto\n"));
		//ahstat.ahs_crypto++;
		error = ENOBUFS;
		goto bad;
	}

	sav->tdb_tdb = tdb;
	tdb->tdb_sav = sav;

	/* Crypto operation descriptor. */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length. */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = ah_output_cb;
	crp->crp_sid = tdb->tdb_cryptoid;
	crp->crp_opaque = (caddr_t)tdb;

	/* These are passed as-is to the callback. */
	tdb->tdb_isr = isr;
	tdb->tdb_spi = sav->spi;
	tdb->tdb_dst = sav->sah->saidx.dst;
	tdb->tdb_src = sav->sah->saidx.src;
	tdb->tdb_proto = sav->sah->saidx.proto;
	tdb->tdb_skip = skip;
	tdb->tdb_length = length;
	tdb->tdb_offset = offset;

	return (ah_output_cb(crp));

bad:
	if (m) {
		m_freem(m);
	}
	return (error);
}

static int
ah_output_cb(crp)
	struct cryptop *crp;
{
	struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m;
	struct tdb *tdb;
	const struct auth_hash *ahx;
	int s, error, skip, length, offset;

	tdb = (struct tdb *)crp->crp_opaque;
	m = (struct mbuf *)crp->crp_buf;
	skip = tdb->tdb_skip;
	length = tdb->tdb_length;
	offset = tdb->tdb_offset;

	s = splsoftnet();
	isr = tdb->tdb_isr;
#ifdef INET
	sav = key_allocsa(AF_INET, (caddr_t)tdb_get_in(tdb, 0), (caddr_t)tdb_get_in(tdb, 1), tdb->tdb_proto, tdb->tdb_spi);
#endif
#ifdef INET6
	sav = key_allocsa(AF_INET6, (caddr_t)tdb_get_in6(tdb, 0), (caddr_t)tdb_get_in6(tdb, 1), tdb->tdb_proto, tdb->tdb_spi);
#endif
	if (sav == NULL) {
		ipseclog((LOG_ERR, "ah_input_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	ahx = tdb->tdb_authalgxform;

	/* Check for crypto errors */
	if (crp->crp_etype) {
		/* Reset the session ID */
		if (tdb->tdb_cryptoid != 0)
			tdb->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			key_freesav(sav);
			splx(s);
			return (crypto_dispatch(crp));
		}

		ipseclog((LOG_ERR, "ah_input_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		ipseclog((LOG_ERR, "ah_input_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}

	/* Release the crypto descriptors */
	tdb_free(tdb);
	tdb = NULL;
	crypto_freereq(crp);
	crp = NULL;

#ifdef INET
	error = ah4_calccksum(m, (u_int8_t *)skip, (length + offset), ahx, sav);
#endif
#ifdef INET6
	error = ah6_calccksum(m, (u_int8_t *)skip, (length + offset), ahx, sav);
#endif

	key_freesav(sav);
	splx(s);
	return (error);

bad:
	if (sav) {
		key_freesav(sav);
	}
	splx(s);
	if (m != NULL) {
		m_freem(m);
	}
	if (tdb != NULL) {
		tdb_free(tdb);
	}
	if (crp != NULL) {
		crypto_freereq(crp);
	}
	return (error);
}

static struct xformsw ah_xformsw = {
		.xf_type = XF_AH,
		.xf_flags = XFT_AUTH,
		.xf_name = "Kame IPsec AH",
		.xf_init = ah_init,
		.xf_zeroize = ah_zeroize,
		.xf_input = ah_input,
		.xf_output = ah_output,
};

void
ah_attach(void)
{
	xform_register(&ah_xformsw);
}

/*------------------------------------------------------------*/

/*
 * go generate the checksum.
 */
static void
ah_update_mbuf(m, off, len, sav, thash, state)
	struct mbuf *m;
	int off, len;
	struct secasvar *sav;
	const struct auth_hash *thash;
	struct xform_state *state;
{
	struct mbuf *n;
	int tlen;

	/* easy case first */
	if (off + len <= m->m_len) {
		ah_hash_update(state, sav, thash, mtod(m, u_int8_t *) + off, len);
		return;
	}

	for (n = m; n; n = n->m_next) {
		if (off < n->m_len) {
			break;
		}

		off -= n->m_len;
	}

	if (!n) {
		panic("ah_update_mbuf: wrong offset specified");
	}

	for (/* nothing */; n && len > 0; n = n->m_next) {
		if (n->m_len == 0) {
			continue;
		}
		if (n->m_len - off < len) {
			tlen = n->m_len - off;
		} else {
			tlen = len;
		}

		ah_hash_update(state, sav, thash, mtod(n, u_int8_t *) + off, tlen);

		len -= tlen;
		off = 0;
	}
}

#ifdef INET

int
ah4_calccksum_input(m, ahdat, len, thash, sav)
	struct mbuf *m;
	u_int8_t *ahdat;
	size_t len;
	const struct auth_hash *thash;
	struct secasvar *sav;
{
    return (ah_input(m, sav, (int)ahdat, (int)len, 0));
}

int
ah4_calccksum_output(m, ahdat, len, thash, isr)
	struct mbuf *m;
	u_int8_t *ahdat;
	size_t len;
	const struct auth_hash *thash;
	struct ipsecrequest *isr;
{
    return (ah_output(m, isr, &m, (int)ahdat, (int)len, 0));
}

#endif

#ifdef INET6

int
ah6_calccksum_input(m, ahdat, len, thash, sav)
	struct mbuf *m;
	u_int8_t *ahdat;
	size_t len;
	const struct auth_hash *thash;
	struct secasvar *sav;
{
    return (ah_input(m, sav, (int)ahdat, (int)len));
}

int
ah6_calccksum_output(m, ahdat, len, thash, isr)
	struct mbuf *m;
	u_int8_t *ahdat;
	size_t len;
	const struct auth_hash *thash;
	struct ipsecrequest *isr;
{
	return (ah_output(m, isr, &m, (int)ahdat, (int)len, 0));
}

#endif

#ifdef INET

/*
 * Go generate the checksum. This function won't modify the mbuf chain
 * except AH itself.
 *
 * NOTE: the function does not free mbuf on failure.
 * Don't use m_copy(), it will try to share cluster mbuf by using refcnt.
 */
static int
ah4_calccksum(m, ahdat, len, thash, sav)
	struct mbuf *m;
	u_int8_t *ahdat;
	size_t len;
	const struct auth_hash *thash;
	struct secasvar *sav;
{
	size_t advancewidth;
	struct xform_state state;
	struct mbuf *n = NULL;
	u_char sumbuf[AH_MAXSUMSIZE];
	int proto, off, ahseen;
	int error = 0;

	if ((m->m_flags & M_PKTHDR) == 0) {
		return (EINVAL);
	}

	ahseen = 0;
	proto = IPPROTO_IP;

	off = 0;
	error = 0;

	error = ah_hash_init(&state, sav, thash);
	if (error) {
		return (error);
	}

	advancewidth = 0;	/* safety */

again:
	/* gory. */
	switch (proto) {
	case IPPROTO_IP: /* first one only */
	{
		/*
		 * copy ip hdr, modify to fit the AH checksum rule,
		 * then take a checksum.
		 */
		struct ip iphdr;
		size_t hlen;

		m_copydata(m, off, sizeof(iphdr), (caddr_t) &iphdr);
		hlen = iphdr.ip_hl << 2;
		iphdr.ip_ttl = 0;
		iphdr.ip_sum = htons(0);
		if (ip4_ah_cleartos) {
			iphdr.ip_tos = 0;
		}
		iphdr.ip_off = htons(ntohs(iphdr.ip_off) & ip4_ah_offsetmask);
		ah_hash_update(&state, sav, thash, (u_int8_t *)&iphdr, sizeof(struct ip));

		if (hlen != sizeof(struct ip)) {
			u_char *p;
			int i, l, skip;

			if (hlen > MCLBYTES) {
				error = EMSGSIZE;
				goto fail;
			}
			MGET(n, M_DONTWAIT, MT_DATA);
			if (n && hlen > MLEN) {
				MCLGET(n, M_DONTWAIT);
				if ((n->m_flags & M_EXT) == 0) {
					m_free(n);
					n = NULL;
				}
			}
			if (n == NULL) {
				error = ENOBUFS;
				goto fail;
			}
			m_copydata(m, off, hlen, mtod(n, caddr_t));

			/*
			 * IP options processing.
			 * See RFC2402 appendix A.
			 */
			p = mtod(n, u_char*);
			i = sizeof(struct ip);
			while (i < hlen) {
				if (i + IPOPT_OPTVAL >= hlen) {
					ipseclog((LOG_ERR, "ah4_calccksum: "
							"invalid IP option\n"));
					error = EINVAL;
					goto fail;
				}
				if (p[i + IPOPT_OPTVAL] == IPOPT_EOL
						|| p[i + IPOPT_OPTVAL] == IPOPT_NOP
						|| i + IPOPT_OLEN < hlen) {
					;
				} else {
					ipseclog((LOG_ERR, "ah4_calccksum: invalid IP option "
							"(type=%02x)\n", p[i + IPOPT_OPTVAL]));
					error = EINVAL;
					goto fail;
				}

				skip = 1;
				switch (p[i + IPOPT_OPTVAL]) {
				case IPOPT_EOL:
				case IPOPT_NOP:
					l = 1;
					skip = 0;
					break;
				case IPOPT_SECURITY: /* 0x82 */
				case 0x85: /* Extended security */
				case 0x86: /* Commercial security */
				case 0x94: /* Router alert */
				case 0x95: /* RFC1770 */
					l = p[i + IPOPT_OLEN];
					if (l < 2) {
						goto invalopt;
					}
					skip = 0;
					break;
				case IPOPT_LSRR:
				case IPOPT_SSRR:
					l = p[i + IPOPT_OLEN];
					if (l < 2) {
						goto invalopt;
					}
					skip = 0;
					break;
				default:
					l = p[i + IPOPT_OLEN];
					if (l < 2) {
						goto invalopt;
					}
					skip = 1;
					break;
				}
				if (l < 1 || hlen - i < l) {
			invalopt:
					ipseclog(
							(LOG_ERR, "ah4_calccksum: invalid IP option "
									"(type=%02x len=%02x)\n", p[i + IPOPT_OPTVAL], p[i
									+ IPOPT_OLEN]));
					error = EINVAL;
					goto fail;
				}
				if (skip) {
					bzero(p + i, l);
				}
				if (p[i + IPOPT_OPTVAL] == IPOPT_EOL) {
					break;
				}
				i += l;
			}
			p = mtod(n, u_char *) + sizeof(struct ip);
			ah_hash_update(&state, sav, thash, p, hlen - sizeof(struct ip));

			m_free(n);
			n = NULL;
		}

		proto = (iphdr.ip_p) & 0xff;
		advancewidth = hlen;
		break;
	}

	case IPPROTO_AH: {
		struct ah ah;
		int siz;
		int hdrsiz;
		int totlen;

		m_copydata(m, off, sizeof(ah), (caddr_t) &ah);
		hdrsiz =
				(sav->flags & SADB_X_EXT_OLD) ?
						sizeof(struct ah) : sizeof(struct newah);
		siz = ah_sumsiz(sav, thash);
		totlen = (ah.ah_len + 2) << 2;

		/*
		 * special treatment is necessary for the first one, not others
		 */
		if (!ahseen) {
			if (totlen > m->m_pkthdr.len - off || totlen > MCLBYTES) {
				error = EMSGSIZE;
				goto fail;
			}
			MGET(n, M_DONTWAIT, MT_DATA);
			if (n && totlen > MLEN) {
				MCLGET(n, M_DONTWAIT);
				if ((n->m_flags & M_EXT) == 0) {
					m_free(n);
					n = NULL;
				}
			}
			if (n == NULL) {
				error = ENOBUFS;
				goto fail;
			}
			m_copydata(m, off, totlen, mtod(n, caddr_t));
			n->m_len = totlen;
			bzero(mtod(n, u_int8_t *) + hdrsiz, siz);
			ah_hash_update(&state, sav, thash, mtod(n, u_int8_t*), n->m_len);
			m_free(n);
			n = NULL;
		} else {
			ah_update_mbuf(m, off, totlen, sav, thash, &state);
		}
		ahseen++;

		proto = ah.ah_nxt;
		advancewidth = totlen;
		break;
	}

	default:
		ah_update_mbuf(m, off, m->m_pkthdr.len - off,  sav, thash, &state);
		advancewidth = m->m_pkthdr.len - off;
		break;
	}

	off += advancewidth;
	if (off < m->m_pkthdr.len) {
		goto again;
	}

	if (len < ah_sumsiz(sav, thash)) {
		error = EINVAL;
		goto fail;
	}

	ah_hash_result(&state, sav, thash, sumbuf, sizeof(sumbuf));
	bcopy(&sumbuf[0], ahdat, ah_sumsiz(sav, thash));

	if (n) {
		m_free(n);
	}
	return (error);

fail:
	if (n) {
		m_free(n);
	}
	return (error);
}

#endif

#ifdef INET6

/*
 * Go generate the checksum. This function won't modify the mbuf chain
 * except AH itself.
 *
 * NOTE: the function does not free mbuf on failure.
 * Don't use m_copy(), it will try to share cluster mbuf by using refcnt.
 */
static int
ah6_calccksum(m, ahdat, len, thash, sav)
	struct mbuf *m;
	u_int8_t *ahdat;
	size_t len;
	const struct auth_hash *thash;
	struct secasvar *sav;
{
	struct xform_state state;
	u_char sumbuf[AH_MAXSUMSIZE];
	struct mbuf *n = NULL;
	int error, ahseen;
	int newoff, off;
	int proto, nxt;

	if ((m->m_flags & M_PKTHDR) == 0) {
		return (EINVAL);
	}

	error = ah_hash_init(&state, sav, thash);
	if (error) {
		return (error);
	}

	off = 0;
	proto = IPPROTO_IPV6;
	nxt = -1;
	ahseen = 0;

again:
	newoff = ip6_nexthdr(m, off, proto, &nxt);
	if (newoff < 0) {
		newoff = m->m_pkthdr.len;
	} else if (newoff <= off) {
		error = EINVAL;
		goto fail;
	}

	switch (proto) {
	case IPPROTO_IPV6:
		/*
		 * special treatment is necessary for the first one, not others
		 */
		if (off == 0) {
			struct ip6_hdr ip6copy;

			if (newoff - off != sizeof(struct ip6_hdr)) {
				error = EINVAL;
				goto fail;
			}

			m_copydata(m, off, newoff - off, (caddr_t) &ip6copy);
			/* RFC2402 */
			ip6copy.ip6_flow = 0;
			ip6copy.ip6_vfc &= ~IPV6_VERSION_MASK;
			ip6copy.ip6_vfc |= IPV6_VERSION;
			ip6copy.ip6_hlim = 0;
			if (IN6_IS_ADDR_LINKLOCAL(&ip6copy.ip6_src)) {
				ip6copy.ip6_src.s6_addr16[1] = 0x0000;
			}
			if (IN6_IS_ADDR_LINKLOCAL(&ip6copy.ip6_dst)) {
				ip6copy.ip6_dst.s6_addr16[1] = 0x0000;
			}
			ah_hash_update(&state, sav, thash, (u_int8_t *)&ip6copy, sizeof(struct ip6_hdr));
		} else {
			newoff = m->m_pkthdr.len;
			ah_update_mbuf(m, off, m->m_pkthdr.len - off, sav, thash, &state);
		}
		break;

	case IPPROTO_AH: {
		int siz;
		int hdrsiz;

		hdrsiz =
				(sav->flags & SADB_X_EXT_OLD) ?
						sizeof(struct ah) : sizeof(struct newah);
		siz = ah_sumsiz(sav, thash);

		/*
		 * special treatment is necessary for the first one, not others
		 */
		if (!ahseen) {
			if (newoff - off > MCLBYTES) {
				error = EMSGSIZE;
				goto fail;
			}
			MGET(n, M_DONTWAIT, MT_DATA);
			if (n && newoff - off > MLEN) {
				MCLGET(n, M_DONTWAIT);
				if ((n->m_flags & M_EXT) == 0) {
					m_free(n);
					n = NULL;
				}
			}
			if (n == NULL) {
				error = ENOBUFS;
				goto fail;
			}
			m_copydata(m, off, newoff - off, mtod(n, caddr_t));
			n->m_len = newoff - off;
			bzero(mtod(n, u_int8_t *) + hdrsiz, siz);
			ah_hash_update(&state, sav, thash, mtod(n, u_int8_t*), n->m_len);
			m_free(n);
			n = NULL;
		} else {
			ah_update_mbuf(m, off, newoff - off, sav, thash, &state);
		}
		ahseen++;
		break;
	}

	case IPPROTO_HOPOPTS:
	case IPPROTO_DSTOPTS: {
		struct ip6_ext *ip6e;
		int hdrlen, optlen;
		u_int8_t *p, *optend, *optp;

		if (newoff - off > MCLBYTES) {
			error = EMSGSIZE;
			goto fail;
		}
		MGET(n, M_DONTWAIT, MT_DATA);
		if (n && newoff - off > MLEN) {
			MCLGET(n, M_DONTWAIT);
			if ((n->m_flags & M_EXT) == 0) {
				m_free(n);
				n = NULL;
			}
		}
		if (n == NULL) {
			error = ENOBUFS;
			goto fail;
		}
		m_copydata(m, off, newoff - off, mtod(n, caddr_t));
		n->m_len = newoff - off;

		ip6e = mtod(n, struct ip6_ext*);
		hdrlen = (ip6e->ip6e_len + 1) << 3;
		if (newoff - off < hdrlen) {
			error = EINVAL;
			m_free(n);
			n = NULL;
			goto fail;
		}
		p = mtod(n, u_int8_t*);
		optend = p + hdrlen;

		/*
		 * ICV calculation for the options header including all
		 * options.  This part is a little tricky since there are
		 * two type of options; mutable and immutable.  We try to
		 * null-out mutable ones here.
		 */
		optp = p + 2;
		while (optp < optend) {
			if (optp[0] == IP6OPT_PAD1) {
				optlen = 1;
			} else {
				if (optp + 2 > optend) {
					error = EINVAL;
					m_free(n);
					n = NULL;
					goto fail;
				}
				optlen = optp[1] + 2;
			}

			if (optp + optlen > optend) {
				error = EINVAL;
				m_free(n);
				n = NULL;
				goto fail;
			}

			if (optp[0] & IP6OPT_MUTABLE) {
				bzero(optp + 2, optlen - 2);
			}

			optp += optlen;
		}

		ah_hash_update(&state, sav, thash, mtod(n, u_int8_t*), n->m_len);
		m_free(n);
		n = NULL;
		break;
	}

	case IPPROTO_ROUTING:
		/*
		 * For an input packet, we can just calculate `as is'.
		 * For an output packet, we assume ip6_output have already
		 * made packet how it will be received at the final
		 * destination.
		 */
		/* FALLTHROUGH */

	default:
		ah_update_mbuf(m, off, newoff - off, sav, thash, &state);
		break;
	}

	if (newoff < m->m_pkthdr.len) {
		proto = nxt;
		off = newoff;
		goto again;
	}

	if (len < ah_sumsiz(sav, thash)) {
		error = EINVAL;
		goto fail;
	}

	ah_hash_result(&state, sav, thash, sumbuf, sizeof(sumbuf));
	bcopy(&sumbuf[0], ahdat, ah_sumsiz(sav, thash));

	/* just in case */
	if (n) {
		m_free(n);
	}
	return (0);

fail:
	/* just in case */
	if (n) {
		m_free(n);
	}
	return (error);
}

#endif
