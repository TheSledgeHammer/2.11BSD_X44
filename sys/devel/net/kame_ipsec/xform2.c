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
#include <netkey/key_debug.h>

#include <net/net_osdep.h>

#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/xform.h>

typedef int (*callback_t)(struct cryptop *);

/* Kame IPsec: xform compatibility */
void
xform_set_desc(m, sav, isr, crp, tc, nxt, ptr, skip, protoff, callback)
	struct mbuf *m;
	struct secasvar *sav;
	struct ipsecrequest *isr;
	struct cryptop *crp;
	struct tdb_crypto *tc;
	u_int8_t nxt;
	caddr_t ptr;
	int skip, protoff;
	callback_t callback;
{

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = callback;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = (caddr_t)tc;

	/* These are passed as-is to the callback. */
	tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_src = sav->sah->saidx.src;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_nxt = nxt;
	tc->tc_skip = skip;
	tc->tc_protoff = protoff;
	tc->tc_ptr = ptr;
}

int
xform_esp_input(m, sav, nxt, ptr, skip, protoff)
	struct mbuf *m;
	struct secasvar *sav;
	u_int8_t nxt;
	caddr_t ptr;
	int skip, protoff;
{
	const struct enc_xform *espx;
	const struct auth_hash *esph;
	struct tdb_crypto *tc;
	struct cryptop *crp;
	int alen, hlen;

	struct cryptodesc *crda, *crde;

	espx = sav->tdb_encalgxform;
	esph = sav->tdb_authalgxform;

	/* Determine the ESP header length */
	if (sav->flags & SADB_X_EXT_OLD) {
		hlen = sizeof (struct esp) + sav->ivlen;
	} else {
		hlen = sizeof (struct newesp) + sav->ivlen;
	}
	/* Authenticator hash size */
	alen = esph ? AH_HMAC_HASHLEN : 0;

	/* Get crypto descriptors */
	crp = crypto_getreq(esph && espx ? 2 : 1);
	if (crp == NULL) {
		DPRINTF(("esp_input: failed to acquire crypto descriptors\n"));
		espstat.esps_crypto++;
		m_freem(m);
		return (ENOBUFS);
	}

	/* Get IPsec-specific opaque pointer */
	if (esph == NULL) {
		tc = (struct tdb_crypto *)malloc(sizeof(struct tdb_crypto),
		    M_XDATA, M_NOWAIT|M_ZERO);
	} else {
		tc = (struct tdb_crypto *)malloc(sizeof(struct tdb_crypto) + alen,
		    M_XDATA, M_NOWAIT|M_ZERO);
	}
	if (tc == NULL) {
		crypto_freereq(crp);
		DPRINTF(("esp_input: failed to allocate tdb_crypto\n"));
		espstat.esps_crypto++;
		m_freem(m);
		return (ENOBUFS);
	}

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
	crp->crp_callback = xform_callback;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = (caddr_t)tc;

	/* These are passed as-is to the callback. */
	//tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_src = sav->sah->saidx.src;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_nxt = nxt;
	tc->tc_skip = skip;
	tc->tc_protoff = protoff;
	tc->tc_ptr = ptr;

	if (espx) {
		/* Decryption descriptor */
		crde->crd_skip = skip + hlen;
		crde->crd_len = m->m_pkthdr.len - (skip + hlen + alen);
		crde->crd_inject = skip + hlen - sav->ivlen;

		crde->crd_alg = espx->type;
		crde->crd_key = _KEYBUF(sav->key_enc);
		crde->crd_klen = _KEYBITS(sav->key_enc);
	}

	if (ptr == NULL) {
		return (crypto_dispatch(crp));
	}
	return (xform_callback(crp));
}

int
xform_esp_output(m, isr, mp, skip, protoff)
	struct mbuf *m;
	struct ipsecrequest *isr;
	struct mbuf **mp;
	int skip;
	int protoff;
{
	const struct enc_xform *espx;
	const struct auth_hash *esph;
	struct tdb_crypto *tc;
	struct secasvar *sav;
	struct cryptop *crp;
	int error, alen, hlen;

	struct cryptodesc *crda, *crde;

	sav = isr->sav;
	esph = sav->tdb_authalgxform;
	espx = sav->tdb_encalgxform;

	if (sav->flags & SADB_X_EXT_OLD) {
		hlen = sizeof (struct esp) + sav->ivlen;
	} else {
		hlen = sizeof (struct newesp) + sav->ivlen;
	}

	if (esph) {
		alen = AH_HMAC_HASHLEN;
	} else {
		alen = 0;
	}

	/* Get crypto descriptors. */
	crp = crypto_getreq(esph && espx ? 2 : 1);
	if (crp == NULL) {
		DPRINTF(("esp_output: failed to acquire crypto descriptors\n"));
		espstat.esps_crypto++;
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
		crde->crd_inject = skip + hlen - sav->ivlen;

		/* Encryption operation. */
		crde->crd_alg = espx->type;
		crde->crd_key = _KEYBUF(sav->key_enc);
		crde->crd_klen = _KEYBITS(sav->key_enc);
		/* XXX Rounds ? */
	} else {
		crda = crp->crp_desc;
	}

	/* IPsec-specific opaque crypto info. */
	tc = (struct tdb_crypto *)malloc(sizeof(struct tdb_crypto),
		M_XDATA, M_NOWAIT|M_ZERO);
	if (tc == NULL) {
		crypto_freereq(crp);
		DPRINTF(("esp_output: failed to allocate tdb_crypto\n"));
		espstat.esps_crypto++;
		error = ENOBUFS;
		goto bad;
	}

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = xform_callback;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = (caddr_t)tc;

	/* These are passed as-is to the callback. */
	tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_src = sav->sah->saidx.src;
	tc->tc_proto = sav->sah->saidx.proto;

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

	return (crypto_dispatch(crp));

bad:
	if (m) {
		m_freem(m);
	}
	return (error);
}


int
xform_ah_input(m, sav, nxt, ptr, skip, protoff)
	struct mbuf *m;
	struct secasvar *sav;
	u_int8_t nxt;
	caddr_t ptr;
	int skip, protoff;
{
	const struct auth_hash *ahx;
	struct tdb_crypto *tc;
	struct cryptop *crp;
	int rplen;

	struct cryptodesc *crda, *crde;

	ahx = sav->tdb_authalgxform;

	/* Figure out header size. */
	rplen = ah_sumsiz(sav, ahx);

	/* Get crypto descriptors. */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		DPRINTF(("ah_input: failed to acquire crypto descriptor\n"));
		ahstat.ahs_crypto++;
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

	tc = (struct tdb_crypto *)malloc(sizeof(struct tdb_crypto), M_XDATA, M_NOWAIT|M_ZERO);
	if (tc == NULL) {
		DPRINTF(("ah_input: failed to allocate tdb_crypto\n"));
		ahstat.ahs_crypto++;
		crypto_freereq(crp);
		m_freem(m);
		return (ENOBUFS);
	}

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = xform_callback;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = (caddr_t)tc;

	/* These are passed as-is to the callback. */
	//tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_src = sav->sah->saidx.src;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_nxt = nxt;
	tc->tc_skip = skip;
	tc->tc_protoff = protoff;
	tc->tc_ptr = ptr;

	if (ptr == NULL) {
		return (crypto_dispatch(crp));
	}
	return (xform_callback(crp));
}

int
xform_ah_output(m, isr, mp, skip, protoff)
	struct mbuf *m;
	struct ipsecrequest *isr;
	struct mbuf **mp;
	int skip;
	int protoff;
{
	struct secasvar *sav;
	const struct auth_hash *ahx;
	struct tdb_crypto *tc;
	struct cryptop *crp;
	int error, rplen;
	struct cryptodesc *crda, *crde;

	sav = isr->sav;
	ahx = sav->tdb_authalgxform;

	/* Figure out header size. */
	rplen = ah_sumsiz(sav, ahx);

	/* Get crypto descriptors. */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		DPRINTF(("ah_output: failed to acquire crypto descriptors\n"));
		ahstat.ahs_crypto++;
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
	tc = (struct tdb_crypto *)malloc(
		sizeof(struct tdb_crypto) + skip, M_XDATA, M_NOWAIT|M_ZERO);
	if (tc == NULL) {
		crypto_freereq(crp);
		DPRINTF(("ah_output: failed to allocate tdb_crypto\n"));
		ahstat.ahs_crypto++;
		error = ENOBUFS;
		goto bad;
	}

	/* Crypto operation descriptor. */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length. */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = xform_callback;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = (caddr_t)tc;

	/* These are passed as-is to the callback. */
	tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_src = sav->sah->saidx.src;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_skip = skip;
	tc->tc_protoff = protoff;

	return (crypto_dispatch(crp));

bad:
	if (m) {
		m_freem(m);
	}
	return (error);
}

tdb_get_secasvar(tdb, af)
	struct tdb *tdb;
{
	struct secasvar *sav;

	switch (af) {
	case AF_INET:
		sav = key_allocsa(AF_INET, (caddr_t)&tdb->tdb_src.sin.sin_addr, (caddr_t)&tdb->tdb_dst.sin.sin_addr, tdb->tdb_proto, tdb->tdb_spi);
		break;

	case AF_INET6:
		sav = key_allocsa(AF_INET6, (caddr_t)&tdb->tdb_src.sin6.sin6_addr, (caddr_t)&tdb->tdb_dst.sin6.sin6_addr, tdb->tdb_proto, tdb->tdb_spi);
		break;
	}

	tdb->tdb_sav = sav;

	switch (sproto) {
	case IPPROTO_ESP:

	}
}

//#ifdef INET

static int
xform_ip_callback(m, crp, tdb, af, sproto)
	struct mbuf *m;
	struct cryptop *crp;
	struct tdb *tdb;
	int af, sproto;
{
	struct secasvar *sav;
	const struct auth_hash *esph;
	const struct enc_xform *espx;
	int s, error;

	s = splsoftnet();
	switch (af) {
	case AF_INET:
		sav = key_allocsa(AF_INET, (caddr_t)&tdb->tdb_src.sin.sin_addr, (caddr_t)&tdb->tdb_dst.sin.sin_addr, tdb->tdb_proto, tdb->tdb_spi);
		break;

	case AF_INET6:
		sav = key_allocsa(AF_INET6, (caddr_t)&tdb->tdb_src.sin6.sin6_addr, (caddr_t)&tdb->tdb_dst.sin6.sin6_addr, tdb->tdb_proto, tdb->tdb_spi);
		break;
	}
	if (sav == NULL) {
		DPRINTF(("xform_ip_callback: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	tdb->tdb_sav = sav;

	switch (sproto) {
	case IPPROTO_ESP:
		esph = tdb->tdb_authalgxform;
		espx = tdb->tdb_encalgxform;
	case IPPROTO_AH:
		esph = tdb->tdb_authalgxform;
		espx = tdb->tdb_encalgxform;
	}


	/* Check for crypto errors */
	if (crp->crp_etype) {
		/* Reset the session ID */
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			key_freesav(&sav);
			splx(s);
			return (crypto_dispatch(crp));
		}

		DPRINTF(("xform_ip4_callback: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		DPRINTF(("xform_ip4_callback: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}

	/* Release the crypto descriptors */
	tdb_free(tdb);
	tdb = NULL;
	crypto_freereq(crp);
	crp = NULL;

	key_freesav(&sav);
	splx(s);
	return (error);

bad:
	if (sav) {
		key_freesav(&sav);
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

#endif /* INET */

int
xform_callback(crp)
	struct cryptop *crp;
{
	struct mbuf *m;
	struct tdb_crypto *tc;
	u_int8_t nxt;
	caddr_t ptr;
	int error, skip, protoff;

	tc = (struct tdb_crypto *)crp->crp_opaque;
	m = (struct mbuf *)crp->crp_buf;
	skip = tc->tc_skip;
	nxt = tc->tc_nxt;
	protoff = tc->tc_protoff;

	if (tc == NULL) {
		DPRINTF(("xform_callback: null opaque crypto data area!\n"));
	}
#ifdef INET
	error = xform_ip4_callback(m, crp, tc);
#endif
#ifdef INET6
	error = xform_ip6_callback(m, crp, tc);
#endif
	return (error);
}

/* IPCOMP */
int
xform_ipcomp_compress()
{

}

int
xform_ipcomp_decompress()
{

}
