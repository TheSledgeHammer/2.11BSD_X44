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

#include <netinet6/ipsec/ipsec.h>
#include <netinet6/ipsec/ah.h>
#include <netinet6/ipsec/esp.h>
#include <netinet6/ipsec/xform.h>

#include <net/pfkeyv2.h>
#include <netkey/key.h>
//#include <netkey/key_debug.h>

#include <net/net_osdep.h>

#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/cryptosoft.h>
#include <crypto/opencrypto/xform.h>

static size_t 	esp_max_schedlen; 	/* max sched length over all algorithms */
static int 		esp_max_ivlen;		/* max iv length over all algorithms */
static int		esp_derived;		/* derived 0: no 1: yes */

static int esp_input(struct mbuf *, struct secasvar *, int, int, int);
static int esp_input_cb(struct cryptop *);
static int esp_output(struct mbuf *, struct ipsecrequest *, struct mbuf **, int, int, int);
static int esp_output_cb(struct cryptop *);

#define MAXIVLEN	16

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

u_int16_t
esp_padbound(sav, txform)
	struct secasvar *sav;
	const struct enc_xform *txform;
{
	if (!txform) {
		panic("esp_padbound: unknown algorithm");
	}
	return (txform->blocksize);
}

u_int16_t
esp_ivlen(sav, txform)
	struct secasvar *sav;
	const struct enc_xform *txform;
{
	if (!txform) {
		panic("esp_ivlen: unknown algorithm");
	}
	return (txform->ivsize);
}

int
esp_encrypt(m, off, plen, isr, txform, ivlen)
	struct mbuf *m;
	size_t off, plen;
	struct ipsecrequest *isr;
	const struct enc_xform *txform;
	int ivlen;
{
	struct tdb *tdb;

	tdb = isr->sav->tdb_tdb;
	if (txform != tdb->tdb_encalgxform) {
		return (EINVAL);
	}
	return (esp_output(m, isr, &m, off, ivlen, 0));
}

int
esp_decrypt(m, off, sav, txform, ivlen)
	struct mbuf *m;
	size_t off;
	struct secasvar *sav;
	const struct enc_xform *txform;
	int ivlen;
{
	struct tdb *tdb;

	tdb = sav->tdb_tdb;
	if (txform != tdb->tdb_encalgxform) {
		return (EINVAL);
	}
	return (esp_input(m, sav, off, ivlen, 0));
}

int
esp_mature(sav)
	struct secasvar *sav;
{
	const struct enc_xform *txform;
	int keylen;

	if (sav->flags & SADB_X_EXT_OLD) {
		ipseclog((LOG_ERR,
		    "esp_mature: algorithm incompatible with esp-old\n"));
		return (1);
	}
	if (sav->flags & SADB_X_EXT_DERIV) {
		ipseclog((LOG_ERR,
		    "esp_mature: algorithm incompatible with derived\n"));
		return (1);
	}

	if (!sav->key_enc) {
		ipseclog((LOG_ERR, "esp_mature: no key is given.\n"));
		return (1);
	}

	txform = esp_algorithm_lookup(sav->alg_enc);
	if (!txform) {
		ipseclog((LOG_ERR,
		    "esp_mature %s: unsupported algorithm.\n", txform->name));
		return (1);
	}

	keylen = sav->key_enc->sadb_key_bits;
	if (keylen < txform->minkey || txform->maxkey < keylen) {
		ipseclog((LOG_ERR,
		    "esp_mature %s: invalid key length %d.\n",
			txform->name, sav->key_enc->sadb_key_bits));
		return (1);
	}

	return (0);
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
	const struct auth_hash *thash;
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
	struct tdb *tdb;
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

	sav->ivlen = esp_ivlen(sav, txform);
	if (sav->ivlen == 0) {
		sav->ivlen = (txform == &enc_xform_null ? 0 : txform->blocksize);
	}
	sav->iv = (caddr_t)malloc(sav->ivlen, M_XDATA, M_WAITOK);
	if (sav->iv == NULL) {
		ipseclog((LOG_ERR, "esp_init: no memory for IV\n"));
		return (EINVAL);
	}
	key_sa_stir_iv(sav);	/*XXX*/

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
	tdb = tdb_init(sav, xsp, txform, NULL, NULL, IPPROTO_ESP);
	if (tdb == NULL) {
		ipseclog((LOG_ERR, "esp_init: no memory for tunnel descriptor block\n"));
		return (EINVAL);
	}

	/* Initialize crypto session. */
	bzero(&crie, sizeof (crie));
	crie.cri_alg = tdb->tdb_encalgxform->type;
	crie.cri_klen = _KEYBITS(sav->key_enc);
	crie.cri_key = _KEYBUF(sav->key_enc);
	/* XXX Rounds ? */

	if (tdb->tdb_authalgxform && tdb->tdb_encalgxform) {
		/* init both auth & enc */
		crie.cri_next = &cria;
		error = crypto_newsession(&tdb->tdb_cryptoid, &crie, crypto_support);
	} else if (tdb->tdb_encalgxform) {
		error = crypto_newsession(&tdb->tdb_cryptoid, &crie, crypto_support);
	} else if (tdb->tdb_authalgxform) {
		error = crypto_newsession(&tdb->tdb_cryptoid, &cria, crypto_support);
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
esp_zeroize(sav)
    struct secasvar *sav;
{
	/* NB: ah_zerorize free's the crypto session state */
	int error = ah_zeroize(sav);

	if (sav->key_enc) {
		bzero(_KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc));
	}
	(void)tdb_zeroize(sav->tdb_tdb);
	return (error);
}

static int
esp_hash_schedule(sav, txform)
	struct secasvar *sav;
	const struct enc_xform *txform;
{
	return ((*txform->setkey)(sav->sched, _KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc)));
}

static void
esp_hash_encrypt(sav, txform, key, data)
	struct secasvar *sav;
	const struct enc_xform *txform;
	caddr_t key;
	u_int8_t *data;
{
	sav->sched = key;
	(*txform->encrypt)(key, data);
}

static void
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
esp_schedule(sav, txform)
	struct secasvar *sav;
	const struct enc_xform *txform;
{
	int error;

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

static int
esp_decrypt_expanded(m, off, sav, thash, ivlen, ivoff, bodyoff, derived)
	struct mbuf *m;
	size_t off;
	struct secasvar *sav;
	const struct auth_hash *thash;
	int ivlen, ivoff, bodyoff, derived;
{
	struct mbuf *s;
	struct mbuf *d, *d0, *dp;
	int soff, doff;	/* offset from the head of chain, to head of this mbuf */
	int sn, dn;	/* offset from the head of the mbuf, to meat */
	u_int8_t iv[MAXIVLEN], *ivp;
	u_int8_t sbuf[MAXIVLEN], *sp;
	u_int8_t *p, *q;
	struct mbuf *scut;
	int scutoff, i, blocklen;

	if (ivlen != sav->ivlen || ivlen > sizeof(iv)) {
		ipseclog((LOG_ERR, "esp_decrypt %s: "
		    "unsupported ivlen %d\n", thash->name, ivlen));
		m_freem(m);
		return (EINVAL);
	}

	/* assumes blocklen == padbound */
	blocklen = thash->blocksize;

#ifdef DIAGNOSTIC
	if (blocklen > sizeof(iv)) {
		ipseclog((LOG_ERR, "esp_decrypt %s: "
		    "unsupported blocklen %d\n", thash->name, blocklen));
		m_freem(m);
		return EINVAL;
	}
#endif

	/* grab iv */
	m_copydata(m, ivoff, ivlen, (caddr_t)iv);

	/* extend iv */
	if (ivlen == blocklen) {
		;
	} else if (ivlen == 4 && blocklen == 8) {
		bcopy(&iv[0], &iv[4], 4);
		iv[4] ^= 0xff;
		iv[5] ^= 0xff;
		iv[6] ^= 0xff;
		iv[7] ^= 0xff;
	} else {
		ipseclog((LOG_ERR, "esp_decrypt %s: "
		    "unsupported ivlen/blocklen: %d %d\n",
			thash->name, ivlen, blocklen));
		m_freem(m);
		return (EINVAL);
	}

	if (m->m_pkthdr.len < bodyoff) {
		ipseclog((LOG_ERR, "esp_decrypt %s: bad len %d/%lu\n",
				thash->name, m->m_pkthdr.len, (unsigned long)bodyoff));
		m_freem(m);
		return (EINVAL);
	}
	if ((m->m_pkthdr.len - bodyoff) % blocklen) {
		ipseclog((LOG_ERR, "esp_decrypt %s: "
		    "payload length must be multiple of %d\n",
			thash->name, blocklen));
		m_freem(m);
		return (EINVAL);
	}

	s = m;
	d = d0 = dp = NULL;
	soff = doff = sn = dn = 0;
	ivp = sp = NULL;

	/* skip bodyoff */
	while (soff < bodyoff) {
		if (soff + s->m_len > bodyoff) {
			sn = bodyoff - soff;
			break;
		}

		soff += s->m_len;
		s = s->m_next;
	}
	scut = s;
	scutoff = sn;

	/* skip over empty mbuf */
	while (s && s->m_len == 0) {
		s = s->m_next;
	}

	while (soff < m->m_pkthdr.len) {
		/* source */
		if (sn + blocklen <= s->m_len) {
			/* body is continuous */
			sp = mtod(s, u_int8_t *) + sn;
		} else {
			/* body is non-continuous */
			m_copydata(s, sn, blocklen, (caddr_t) sbuf);
			sp = sbuf;
		}

		/* destination */
		if (!d || dn + blocklen > d->m_len) {
			if (d) {
				dp = d;
			}
			MGET(d, M_DONTWAIT, MT_DATA);
			i = m->m_pkthdr.len - (soff + sn);
			if (d && i > MLEN) {
				MCLGET(d, M_DONTWAIT);
				if ((d->m_flags & M_EXT) == 0) {
					m_free(d);
					d = NULL;
				}
			}
			if (!d) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				return (ENOBUFS);
			}
			if (!d0) {
				d0 = d;
			}
			if (dp) {
				dp->m_next = d;
			}
			d->m_len = 0;
			d->m_len = (M_TRAILINGSPACE(d) / blocklen) * blocklen;
			if (d->m_len > i) {
				d->m_len = i;
			}
			dn = 0;
		}

		/* decrypt */
		esp_hash_decrypt(sav, thash, sp, mtod(d, u_int8_t *) + dn);

		/* xor */
		p = ivp ? ivp : iv;
		q = mtod(d, u_int8_t *) + dn;
		for (i = 0; i < blocklen; i++) {
			q[i] ^= p[i];
		}

		/* next iv */
		if (sp == sbuf) {
			bcopy(sbuf, iv, blocklen);
			ivp = NULL;
		} else {
			ivp = sp;
		}

		sn += blocklen;
		dn += blocklen;

		/* find the next source block */
		while (s && sn >= s->m_len) {
			sn -= s->m_len;
			soff += s->m_len;
			s = s->m_next;
		}

		/* skip over empty mbuf */
		while (s && s->m_len == 0) {
			s = s->m_next;
		}
	}

	m_freem(scut->m_next);
	scut->m_len = scutoff;
	scut->m_next = d0;

	/* just in case */
	bzero(iv, sizeof(iv));
	bzero(sbuf, sizeof(sbuf));

	return (0);
}

static int
esp_encrypt_expanded(m, off, plen, sav, thash, ivlen, ivoff, bodyoff, derived)
	struct mbuf *m;
	size_t off;
	size_t plen;
	struct secasvar *sav;
	const struct auth_hash *thash;
	int ivlen, ivoff, bodyoff, derived;
{
	struct mbuf *s;
	struct mbuf *d, *d0, *dp;
	int soff, doff;	/* offset from the head of chain, to head of this mbuf */
	int sn, dn;	/* offset from the head of the mbuf, to meat */
	u_int8_t iv[MAXIVLEN], *ivp;
	u_int8_t sbuf[MAXIVLEN], *sp;
	u_int8_t *p, *q;
	struct mbuf *scut;
	int scutoff, i, blocklen;

	if (ivlen != sav->ivlen || ivlen > sizeof(iv)) {
		ipseclog((LOG_ERR, "esp_encrypt %s: "
		    "unsupported ivlen %d\n", thash->name, ivlen));
		m_freem(m);
		return (EINVAL);
	}

	blocklen = thash->blocksize;

#ifdef DIAGNOSTIC
	if (blocklen > sizeof(iv)) {
		ipseclog((LOG_ERR, "esp_encrypt %s: "
		    "unsupported blocklen %d\n", thash->name, blocklen));
		m_freem(m);
		return EINVAL;
	}
#endif

	/* put iv into the packet.  if we are in derived mode, use seqno. */
	if (derived) {
		m_copydata(m, ivoff, ivlen, (caddr_t)iv);
	} else {
		bcopy(sav->iv, iv, ivlen);
		/* maybe it is better to overwrite dest, not source */
		m_copyback(m, ivoff, ivlen, (caddr_t)iv);
	}

	/* extend iv */
	if (ivlen == blocklen) {
		;
	} else if (ivlen == 4 && blocklen == 8) {
		bcopy(&iv[0], &iv[4], 4);
		iv[4] ^= 0xff;
		iv[5] ^= 0xff;
		iv[6] ^= 0xff;
		iv[7] ^= 0xff;
	} else {
		ipseclog((LOG_ERR, "esp_encrypt %s: "
		    "unsupported ivlen/blocklen: %d %d\n",
			thash->name, ivlen, blocklen));
		m_freem(m);
		return (EINVAL);
	}

	if (m->m_pkthdr.len < bodyoff) {
		ipseclog((LOG_ERR, "esp_encrypt %s: bad len %d/%lu\n",
				thash->name, m->m_pkthdr.len, (unsigned long)bodyoff));
		m_freem(m);
		return (EINVAL);
	}
	if ((m->m_pkthdr.len - bodyoff) % blocklen) {
		ipseclog((LOG_ERR, "esp_encrypt %s: "
		    "payload length must be multiple of %d\n",
			thash->name, blocklen));
		m_freem(m);
		return (EINVAL);
	}

	s = m;
	d = d0 = dp = NULL;
	soff = doff = sn = dn = 0;
	ivp = sp = NULL;

	/* skip bodyoff */
	while (soff < bodyoff) {
		if (soff + s->m_len > bodyoff) {
			sn = bodyoff - soff;
			break;
		}

		soff += s->m_len;
		s = s->m_next;
	}
	scut = s;
	scutoff = sn;

	/* skip over empty mbuf */
	while (s && s->m_len == 0) {
		s = s->m_next;
	}

	while (soff < m->m_pkthdr.len) {
		/* source */
		if (sn + blocklen <= s->m_len) {
			/* body is continuous */
			sp = mtod(s, u_int8_t *) + sn;
		} else {
			/* body is non-continuous */
			m_copydata(s, sn, blocklen, (caddr_t) sbuf);
			sp = sbuf;
		}

		/* destination */
		if (!d || dn + blocklen > d->m_len) {
			if (d) {
				dp = d;
			}
			MGET(d, M_DONTWAIT, MT_DATA);
			i = m->m_pkthdr.len - (soff + sn);
			if (d && i > MLEN) {
				MCLGET(d, M_DONTWAIT);
				if ((d->m_flags & M_EXT) == 0) {
					m_free(d);
					d = NULL;
				}
			}
			if (!d) {
				m_freem(m);
				if (d0) {
					m_freem(d0);
				}
				return (ENOBUFS);
			}
			if (!d0) {
				d0 = d;
			}
			if (dp) {
				dp->m_next = d;
			}
			d->m_len = 0;
			d->m_len = (M_TRAILINGSPACE(d) / blocklen) * blocklen;
			if (d->m_len > i) {
				d->m_len = i;
			}
			dn = 0;
		}

		/* xor */
		p = ivp ? ivp : iv;
		q = sp;
		for (i = 0; i < blocklen; i++) {
			q[i] ^= p[i];
		}

		/* encrypt */
		esp_hash_encrypt(sav, thash, sp, mtod(d, u_int8_t *) + dn);

		/* next iv */
		ivp = mtod(d, u_int8_t *) + dn;

		sn += blocklen;
		dn += blocklen;

		/* find the next source block */
		while (s && sn >= s->m_len) {
			sn -= s->m_len;
			soff += s->m_len;
			s = s->m_next;
		}

		/* skip over empty mbuf */
		while (s && s->m_len == 0) {
			s = s->m_next;
		}
	}

	m_freem(scut->m_next);
	scut->m_len = scutoff;
	scut->m_next = d0;

	/* just in case */
	bzero(iv, sizeof(iv));
	bzero(sbuf, sizeof(sbuf));

	key_sa_stir_iv(sav);

	return (0);
}

/*------------------------------------------------------------*/

static int
esp_input(m, sav, skip, length, offset)
	struct mbuf *m;
	struct secasvar *sav;
	int skip, length, offset;
{
	const struct enc_xform *espx;
	const struct auth_hash *esph;
	struct tdb *tdb;
	struct cryptop *crp;
	int plen, hlen, alen;
    uint8_t abuf[AH_HMAC_MAX_HASHLEN];

	struct cryptodesc *crda, *crde;

	tdb = sav->tdb_tdb;
	espx = tdb->tdb_encalgxform;
	esph = tdb->tdb_authalgxform;

	/* Determine the ESP header length */
	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1827 */
		offset = sizeof(struct esp);
		hlen = sizeof(struct esp) + length;
		esp_derived = 0;
	} else {
		/* RFC 2406 */
		if (sav->flags & SADB_X_EXT_DERIV) {
			offset = sizeof(struct esp);
			hlen = sizeof(struct esp) + sizeof(u_int32_t);
			length = sizeof(u_int32_t);
			esp_derived = 1;
		} else {
			offset = sizeof(struct newesp);
			hlen = sizeof(struct newesp) + length;
			esp_derived = 0;
		}
	}

	/* Authenticator hash size */
	alen = esph ? AH_HMAC_MAX_HASHLEN : 0;
	plen = m->m_pkthdr.len - (skip + hlen + alen);

	/* Get crypto descriptors */
	crp = crypto_getreq(esph && espx ? 2 : 1);
	if (crp == NULL) {
		ipseclog((LOG_ERR, "esp_input: failed to acquire crypto descriptors\n"));
		//espstat.esps_crypto++;
		m_freem(m);
		return (ENOBUFS);
	}

	/* Get IPsec-specific opaque pointer */
	if (esph == NULL) {
		tdb = tdb_alloc(0);
	} else {
		tdb = tdb_alloc(alen);
	}
	if (tdb == NULL) {
		crypto_freereq(crp);
		ipseclog((LOG_ERR, "esp_input: failed to allocate tdb_crypto\n"));
		//espstat.esps_crypto++;
		m_freem(m);
		return (ENOBUFS);
	}

	sav->tdb_tdb = tdb;
	tdb->tdb_sav = sav;

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

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = esp_input_cb;
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

	if (espx) {
		/* Decryption descriptor */
		crde->crd_skip = skip + hlen;
		crde->crd_len = plen;
		crde->crd_inject = skip + hlen - length;

		crde->crd_alg = espx->type;
		crde->crd_key = _KEYBUF(sav->key_enc);
		crde->crd_klen = _KEYBITS(sav->key_enc);
	}
	return (esp_input_cb(crp));
}

static int
esp_input_cb(crp)
	struct cryptop *crp;
{
	struct secasvar *sav;
	struct mbuf *m;
	struct tdb *tdb;
	const struct auth_hash *esph;
	const struct enc_xform *espx;
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
		ipseclog((LOG_ERR, "esp_input_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	esph = tdb->tdb_authalgxform;
	espx = tdb->tdb_encalgxform;

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

		ipseclog((LOG_ERR, "esp_input_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		ipseclog((LOG_ERR, "esp_input_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}

	/* Release the crypto descriptors */
	tdb_free(tdb);
	tdb = NULL;
	crypto_freereq(crp);
	crp = NULL;

	error = esp_decrypt_expanded(m, skip, sav, espx, length, (skip + offset), (skip + offset + length), esp_derived);

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
esp_output(m, isr, mp, skip, length, offset)
	struct mbuf *m;
	struct ipsecrequest *isr;
	struct mbuf **mp;
	int skip, length, offset;
{
	const struct enc_xform *espx;
	const struct auth_hash *esph;
	struct tdb *tdb;
	struct secasvar *sav;
	struct cryptop *crp;
	int error, plen, hlen, alen;

	struct cryptodesc *crda, *crde;

	sav = isr->sav;
	tdb = sav->tdb_tdb;
	esph = tdb->tdb_authalgxform;
	espx = tdb->tdb_encalgxform;

	if (sav->flags & SADB_X_EXT_OLD) {
		/* RFC 1827 */
		offset = sizeof(struct esp);
		hlen = sizeof(struct esp) + length;
		esp_derived = 0;
	} else {
		/* RFC 2406 */
		if (sav->flags & SADB_X_EXT_DERIV) {
			/*
			 * draft-ietf-ipsec-ciph-des-derived-00.txt
			 * uses sequence number field as IV field.
			 */
			offset = sizeof(struct esp);
			hlen = sizeof(struct esp) + sizeof(u_int32_t);
			length = sizeof(u_int32_t);
			esp_derived = 1;
		} else {
			offset = sizeof(struct newesp);
			hlen = sizeof(struct newesp) + length;
			esp_derived = 0;
		}
	}

	if (esph) {
		alen = AH_HMAC_MAX_HASHLEN;
	} else {
		alen = 0;
	}

	/* Get crypto descriptors. */
	crp = crypto_getreq(esph && espx ? 2 : 1);
	if (crp == NULL) {
		ipseclog((LOG_ERR, "esp_output: failed to acquire crypto descriptors\n"));
		//espstat.esps_crypto++;
		error = ENOBUFS;
		goto bad;
	}

	if (espx) {
		crde = crp->crp_desc;
		crda = crde->crd_next;

		/* Encryption descriptor. */
		crde->crd_skip = skip + hlen;
		crde->crd_len = m->m_pkthdr.len - (skip + hlen + alen);
		crde->crd_flags = CRD_F_ENCRYPT;
		crde->crd_inject = skip + hlen - length;

		/* Encryption operation. */
		crde->crd_alg = espx->type;
		crde->crd_key = _KEYBUF(sav->key_enc);
		crde->crd_klen = _KEYBITS(sav->key_enc);
		/* XXX Rounds ? */
	} else {
		crda = crp->crp_desc;
	}

	/* IPsec-specific opaque crypto info. */
	tdb = tdb_alloc(0);
	if (tdb == NULL) {
		crypto_freereq(crp);
		ipseclog((LOG_ERR, "esp_output: failed to allocate tdb_crypto\n"));
		//espstat.esps_crypto++;
		error = ENOBUFS;
		goto bad;
	}

	sav->tdb_tdb = tdb;
	tdb->tdb_sav = sav;

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = esp_output_cb;
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

	return (esp_output_cb(crp));

bad:
	if (m) {
		m_freem(m);
	}
	return (error);
}

static int
esp_output_cb(crp)
	struct cryptop *crp;
{
	struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m;
	struct tdb *tdb;
	const struct auth_hash *esph;
	const struct enc_xform *espx;
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
		ipseclog((LOG_ERR, "esp_output_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	esph = tdb->tdb_authalgxform;
	espx = tdb->tdb_encalgxform;

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

		ipseclog((LOG_ERR, "esp_output_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		ipseclog((LOG_ERR, "esp_output_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}

	/* Release the crypto descriptors */
	tdb_free(tdb);
	tdb = NULL;
	crypto_freereq(crp);
	crp = NULL;

	error = esp_encrypt_expanded(m, skip, sav, espx, length, (skip + offset), (skip + offset + length), esp_derived);

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
