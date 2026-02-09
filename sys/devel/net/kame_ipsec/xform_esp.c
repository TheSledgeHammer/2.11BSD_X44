/*	$NetBSD: esp_core.c,v 1.33 2003/08/27 00:08:31 thorpej Exp $	*/
/*	$KAME: esp_core.c,v 1.53 2001/11/27 09:47:30 sakane Exp $	*/

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
/*	$NetBSD: xform_esp.c,v 1.5.18.1 2006/03/30 15:31:10 riz Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform_esp.c,v 1.2.2.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$OpenBSD: ip_esp.c,v 1.69 2001/06/26 06:18:59 angelos Exp $ */

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
 * Additional features in 1999 by Angelos D. Keromytis.
 *
 * Copyright (C) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
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

#include <netinet6/ipsec.h>
#include <netinet6/ah.h>
#include <netinet6/esp.h>

#include <kame_ipsec/xform.h>

#include <net/pfkeyv2.h>
#include <netkey/key.h>

#include <net/net_osdep.h>

#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/xform.h>

static size_t esp_max_schedlen; /* max sched length over all algorithms */
static	int esp_max_ivlen;		/* max iv length over all algorithms */

const struct enc_xform *
esp_algorithm_lookup(alg)
	int alg;
{
	if (alg >= ESP_ALG_MAX) {
		return (NULL);
	}
	switch (alg) {
	case SADB_EALG_DESCBC:
		return (&enc_xform_des);
	case SADB_EALG_3DESCBC:
		return (&enc_xform_3des);
	case SADB_X_EALG_AES:
		return (&enc_xform_rijndael128);
	case SADB_X_EALG_BLOWFISHCBC:
		return (&enc_xform_blf);
	case SADB_X_EALG_CAST128CBC:
		return (&enc_xform_cast5);
	case SADB_X_EALG_SKIPJACK:
		return (&enc_xform_skipjack);
	case SADB_EALG_NULL:
		return (&enc_xform_null);
	}
	return (NULL);
}

/*
 * compute ESP header size.
 */
size_t
esp_hdrsiz(isr)
	struct ipsecrequest *isr;
{
	struct secasvar *sav;
	const struct enc_xform *txform;
	const struct ah_algorithm *thash;
	size_t ivlen;
	size_t authlen;
	size_t hdrsiz;

	/* sanity check */
	if (isr == NULL) {
		panic("esp_hdrsiz: NULL was passed.");
	}

	sav = isr->sav;

	if (isr->saidx.proto != IPPROTO_ESP) {
		panic("unsupported mode passed to esp_hdrsiz");
	}

	if (sav == NULL) {
		goto estimate;
	}
	if (sav->state != SADB_SASTATE_MATURE && sav->state != SADB_SASTATE_DYING) {
		goto estimate;
	}

	/* we need transport mode ESP. */
	txform = esp_algorithm_lookup(sav->alg_enc);
	if (!txform) {
		goto estimate;
	}
	ivlen = sav->ivlen;
	if (ivlen < 0) {
		goto estimate;
	}

	/*
	 * XXX
	 * right now we don't calcurate the padding size.  simply
	 * treat the padding size as constant, for simplicity.
	 *
	 * XXX variable size padding support
	 */
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1827 */
		hdrsiz = sizeof(struct esp) + ivlen + txform->blocksize - 1 + 2;
	} else {
		/* RFC 2406 */
		thash = ah_algorithm_lookup(sav->alg_auth);
		if (thash && sav->replay && sav->key_auth) {
			authlen = ah_sumsiz(sav, thash);
		} else {
			authlen = 0;
		}
		hdrsiz = sizeof(struct newesp) + ivlen + txform->blocksize - 1 + 2 + authlen;
	}

	return (hdrsiz);

estimate:
	/*
	 * ASSUMING:
	 *	sizeof(struct newesp) > sizeof(struct esp).
	 *	esp_max_ivlen() = max ivlen for CBC mode
	 *	esp_max_padbound - 1 =
	 *	   maximum padding length without random padding length
	 *	2 = (Pad Length field) + (Next Header field).
	 *	AH_MAXSUMSIZE = maximum ICV we support.
	 */
	return (sizeof(struct newesp) + esp_max_ivlen + txform->blocksize - 1 + 2 + AH_MAXSUMSIZE);
}

static int
esp_init(sav, xsp)
	struct secasvar *sav;
	struct xformsw *xsp;
{
	const struct enc_xform *txform;
	struct cryptoini cria, crie;
	int error;

	txform = esp_algorithm_lookup(sav->alg_enc);
	if (txform == NULL) {
		ipseclog((LOG_ERR, "esp_init: unsupported encryption algorithm %d\n",
			sav->alg_enc));
		return (EINVAL);
	}
	if (sav->key_enc == NULL) {
		ipseclog((LOG_ERR, "esp_init: no encoding key for %s algorithm\n", txform->name));
		return (EINVAL);
	}
	if ((sav->flags & (SADB_X_EXT_OLD | SADB_X_EXT_IV4B)) == SADB_X_EXT_IV4B) {
		ipseclog((LOG_ERR, "esp_init: 4-byte IV not supported with protocol\n"));
		return (EINVAL);
	}
	/* check for key length */
	if (_KEYBITS(sav->key_enc) < txform->minkey
			|| _KEYBITS(sav->key_enc) > txform->maxkey) {
		ipseclog(
				(LOG_ERR, "esp_init %s: unsupported key length %d: "
						"needs %d to %d bits\n", txform->name, _KEYBITS(
						sav->key_enc), txform->minkey, txform->maxkey));
		return (EINVAL);
	}

	/*
	 * NB: The null xform needs a non-zero blocksize to keep the
	 *      crypto code happy but if we use it to set ivlen then
	 *      the ESP header will be processed incorrectly.  The
	 *      compromise is to force it to zero here.
	 */
	sav->ivlen = (txform == &enc_xform_null ? 0 : txform->blocksize);
	sav->iv = (caddr_t)malloc(sav->ivlen, M_XDATA, M_WAITOK);
	if (sav->iv == NULL) {
		ipseclog((LOG_ERR, "esp_init: no memory for IV\n"));
		return (EINVAL);
	}
	key_randomfill(sav->iv, sav->ivlen);	/*XXX*/

	/*
	 * Setup AH-related state.
	 */
	if (sav->alg_auth != 0) {
		error = ah_init0(sav, xsp, &cria);
		if (error) {
			return (error);
		}
	}

	/* NB: override anything set in ah_init0 */
	sav->tdb_xform = xsp;
	sav->tdb_encalgxform = txform;

	/* Initialize crypto session. */
	bzero(&crie, sizeof (crie));
	crie.cri_alg = sav->tdb_encalgxform->type;
	crie.cri_klen = _KEYBITS(sav->key_enc);
	crie.cri_key = _KEYBUF(sav->key_enc);
	/* XXX Rounds ? */

	if (sav->tdb_authalgxform && sav->tdb_encalgxform) {
		/* init both auth & enc */
		crie.cri_next = &cria;
		error = crypto_newsession(&sav->tdb_cryptoid, &crie, crypto_support);
	} else if (sav->tdb_encalgxform) {
		error = crypto_newsession(&sav->tdb_cryptoid, &crie, crypto_support);
	} else if (sav->tdb_authalgxform) {
		error = crypto_newsession(&sav->tdb_cryptoid, &cria, crypto_support);
	} else {
		/* XXX cannot happen? */
		ipseclog((LOG_ERR, "esp_newsession: no encoding OR authentication xform!\n"));
		error = EINVAL;
	}
	return (error);
}

/*
 * Paranoia.
 */
static int
esp_zeroize(struct secasvar *sav)
{
	/* NB: ah_zerorize free's the crypto session state */
	int error = ah_zeroize(sav);

	if (sav->key_enc) {
		bzero(_KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc));
	}
	/* NB: sav->iv is freed elsewhere, even though we malloc it! */
	sav->tdb_encalgxform = NULL;
	sav->tdb_xform = NULL;
	return (error);
}

int
esp_hash_schedule(sav, txform)
	struct secasvar *sav;
	const struct enc_xform *txform;
{
	return ((*txform->setkey)(&sav->sched, _KEYLEN(sav->key_enc), _KEYBUF(sav->key_enc)));
}

void
esp_hash_encrypt(sav, txform, key, data)
	struct secasvar *sav;
	const struct enc_xform *txform;
	caddr_t key;
	u_int8_t *data;
{
	sav->sched = key;
	(*txform->encrypt)(key, data);
}

void
esp_hash_decrypt(sav, txform, key, data)
	struct secasvar *sav;
	const struct enc_xform *txform;
	caddr_t key;
	u_int8_t *data;
{
	sav->sched = key;
	(*txform->decrypt)(key, data);
}

int
esp_schedule(sav, xsp)
	struct secasvar *sav;
	struct xformsw *xsp;
{
	const struct enc_xform *txform;
	int error;

	txform = esp_algorithm_lookup(sav->alg_enc);
	if (txform == NULL) {
		ipseclog((LOG_ERR, "esp_schedule: unsupported encryption algorithm %d\n",
			sav->alg_enc));
		return (EINVAL);
	}

	/* check for key length */
	if (_KEYBITS(sav->key_enc) < txform->minkey
			|| _KEYBITS(sav->key_enc) > txform->maxkey) {
		ipseclog(
				(LOG_ERR, "esp_schedule %s: unsupported key length %d: "
						"needs %d to %d bits\n", txform->name, _KEYBITS(
						sav->key_enc), txform->minkey, txform->maxkey));
		return (EINVAL);
	}

	/* already allocated */
	if (sav->sched && sav->schedlen != 0) {
		return (0);
	}

	/* no schedule necessary */
	if (!esp_hash_schedule(sav, txform) || !esp_max_schedlen) {
		return (0);
	}
	sav->schedlen = esp_max_schedlen;
	sav->sched = malloc(sav->schedlen, M_SECA, M_DONTWAIT);
	if (!sav->sched) {
		sav->schedlen = 0;
		return (ENOBUFS);
	}

	error = esp_hash_schedule(sav, txform);
	if (error != 0) {
		ipseclog((LOG_ERR, "esp_schedule %s: error %d\n", txform->name, error));
		bzero(sav->sched, sav->schedlen);
		free(sav->sched, M_SECA);
		sav->sched = NULL;
		sav->schedlen = 0;
	} else {
		error = 0;
	}
	return (error);
}

/*------------------------------------------------------------*/

void
esp_decrypt_descriptor(m, sav, crp, skip, alen, hlen)
	struct mbuf *m;
	struct secasvar *sav;
	struct cryptop *crp;
	int skip, alen, hlen;
{
	const struct enc_xform *espx;
	const struct auth_hash *esph;
	struct cryptodesc *crda, *crde;
	uint8_t abuf[32];

	espx = sav->tdb_encalgxform;
	esph = sav->tdb_authalgxform;
	if (esph) {
		crda = crp->crp_desc;

		/* Authentication descriptor */
		crda->crd_skip = skip;
		crda->crd_len = m->m_pkthdr.len - (skip + alen);
		crda->crd_inject = m->m_pkthdr.len - alen;

		crda->crd_alg = esph->type;
		crda->crd_key = _KEYBUF(sav->key_auth);
		crda->crd_klen = _KEYBITS(sav->key_auth);

		/* Copy the authenticator */
		m_copydata(m, m->m_pkthdr.len - alen, alen, abuf);

		/* Chain authentication request */
		crde = crda->crd_next;
	} else {
		crde = crp->crp_desc;
	}

	if (espx) {
		/* Decryption descriptor */
		crde->crd_skip = skip + hlen;
		crde->crd_len = m->m_pkthdr.len - (skip + hlen + alen);
		crde->crd_inject = skip + hlen - sav->ivlen;

		crde->crd_alg = espx->type;
		crde->crd_key = _KEYBUF(sav->key_enc);
		crde->crd_klen = _KEYBITS(sav->key_enc);
	}
}

void
esp_encrypt_descriptor(m, sav, crp, skip, alen, hlen)
	struct mbuf *m;
	struct secasvar *sav;
	struct cryptop *crp;
	int skip, alen, hlen;
{
	const struct enc_xform *espx;
	const struct auth_hash *esph;
	struct cryptodesc *crda, *crde;

	espx = sav->tdb_encalgxform;
	esph = sav->tdb_authalgxform;
	if (espx) {
		crde = crp->crp_desc;
		crda = crde->crd_next;

		/* Encryption descriptor. */
		crde->crd_skip = skip + hlen;
		crde->crd_len = m->m_pkthdr.len - (skip + hlen + alen);
		crde->crd_flags = CRD_F_ENCRYPT;
		crde->crd_inject = skip + hlen - sav->ivlen;

		/* Encryption operation. */
		crde->crd_alg = espx->type;
		crde->crd_key = _KEYBUF(sav->key_enc);
		crde->crd_klen = _KEYBITS(sav->key_enc);
	} else {
		crda = crp->crp_desc;
	}

	if (esph) {
		/* Authentication descriptor. */
		crda->crd_skip = skip;
		crda->crd_len = m->m_pkthdr.len - (skip + alen);
		crda->crd_inject = m->m_pkthdr.len - alen;

		/* Authentication operation. */
		crda->crd_alg = esph->type;
		crda->crd_key = _KEYBUF(sav->key_auth);
		crda->crd_klen = _KEYBITS(sav->key_auth);
	}
}

static int
esp_input()
{

}

static int
esp_output()
{

}

static struct xformsw esp_xformsw = {
		.xf_type = XF_ESP,
		.xf_flags = XFT_CONF|XFT_AUTH,
		.xf_name = "Kame IPsec ESP",
		.xf_init = esp_init,
		.xf_zeroize = esp_zeroize,
		.xf_input = esp_input,
		.xf_output = esp_output,
};

void
esp_attach(void)
{
#define	MAXIV(xform)						\
	if (xform.blocksize > esp_max_ivlen)	\
		esp_max_ivlen = xform.blocksize		\

	esp_max_ivlen = 0;
	MAXIV(enc_xform_des);		/* SADB_EALG_DESCBC */
	MAXIV(enc_xform_3des);		/* SADB_EALG_3DESCBC */
	MAXIV(enc_xform_rijndael128);	/* SADB_X_EALG_AES */
	MAXIV(enc_xform_blf);		/* SADB_X_EALG_BLOWFISHCBC */
	MAXIV(enc_xform_cast5);		/* SADB_X_EALG_CAST128CBC */
	MAXIV(enc_xform_skipjack);	/* SADB_X_EALG_SKIPJACK */
	MAXIV(enc_xform_null);		/* SADB_EALG_NULL */

	xform_register(&esp_xformsw);
#undef MAXIV
}

/*------------------------------------------------------------*/

/* does not free m0 on error */
int
esp_auth(m0, skip, length, sav, sum)
	struct mbuf *m0;
	size_t skip;	/* offset to ESP header */
	size_t length;	/* payload length */
	struct secasvar *sav;
	u_char *sum;
{
	struct mbuf *m;
	struct xform_state state;
	const struct auth_hash *thash;
	u_char sumbuf[AH_MAXSUMSIZE];
	size_t siz, off;
	int error;

	/* sanity checks */
	if (m0->m_pkthdr.len < skip) {
		ipseclog((LOG_DEBUG, "esp_auth: mbuf length < skip\n"));
		return (EINVAL);
	}
	if (m0->m_pkthdr.len < skip + length) {
		ipseclog((LOG_DEBUG, "esp_auth: mbuf length < skip + length\n"));
		return (EINVAL);
	}
	/*
	 * length of esp part (excluding authentication data) must be 4n,
	 * since nexthdr must be at offset 4n+3.
	 */
	if (length % 4) {
		ipseclog((LOG_ERR, "esp_auth: length is not multiple of 4\n"));
		return (EINVAL);
	}
	if (!sav) {
		ipseclog((LOG_DEBUG, "esp_auth: NULL SA passed\n"));
		return (EINVAL);
	}

	thash = ah_algorithm_lookup(sav->alg_auth);
	if (!thash) {
		ipseclog(
				(LOG_ERR, "esp_auth: bad ESP auth algorithm passed: %d\n", sav->alg_auth));
		return (EINVAL);
	}

	m = m0;
	off = 0;

	siz = ((ah_sumsiz(sav, thash) + 3) & ~(4 - 1));
	if (sizeof(sumbuf) < siz) {
		ipseclog(
				(LOG_DEBUG, "esp_auth: AH_MAXSUMSIZE is too small: siz=%lu\n", (u_long) siz));
		return (EINVAL);
	}

	/* skip the header */
	while (skip) {
		if (!m) {
			panic("mbuf chain?");
		}
		if (m->m_len <= skip) {
			skip -= m->m_len;
			m = m->m_next;
			off = 0;
		} else {
			off = skip;
			skip = 0;
		}
	}

	error = ah_hash_init(&state, sav, thash);
	if (error) {
		return (error);
	}

	while (0 < length) {
		if (!m) {
			panic("mbuf chain?");
		}

		if (m->m_len - off < length) {
			ah_hash_update(&state, sav, thash, mtod(m, u_char *) + off, m->m_len - off);
			length -= m->m_len - off;
			m = m->m_next;
			off = 0;
		} else {
			ah_hash_update(&state, sav, thash, mtod(m, u_char *) + off, length);
			break;
		}
	}
	ah_hash_result(&state, sav, thash, sumbuf, sizeof(sumbuf));
	bcopy(sumbuf, sum, siz); /* XXX */

	return (0);
}
