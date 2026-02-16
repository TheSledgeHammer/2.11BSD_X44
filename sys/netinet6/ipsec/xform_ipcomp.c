/*	$NetBSD: ipcomp_core.c,v 1.20 2002/11/02 07:30:59 perry Exp $	*/
/*	$KAME: ipcomp_core.c,v 1.25 2001/07/26 06:53:17 jinmei Exp $	*/

/*
 * Copyright (C) 1999 WIDE Project.
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
/*	$NetBSD: xform_ipcomp.c,v 1.4 2003/10/06 22:05:15 tls Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform_ipcomp.c,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
/* $OpenBSD: ip_ipcomp.c,v 1.1 2001/07/05 12:08:52 jjbg Exp $ */

/*
 * Copyright (c) 2001 Jean-Jacques Bernard-Gundol (jj@wabbitt.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * RFC2393 IP payload compression protocol (IPComp).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipcomp_core.c,v 1.20 2002/11/02 07:30:59 perry Exp $");

#include "opt_inet.h"

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
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>
#include <net/netisr.h>
#include <net/zlib.h>
#include <machine/cpu.h>

#include <netinet6/ipsec/ipsec.h>
#include <netinet6/ipsec/ipcomp.h>
#include <netinet6/ipsec/xform.h>

#include <machine/stdarg.h>

#include <net/pfkeyv2.h>
#include <netkey/key.h>

#include <net/net_osdep.h>

#include <crypto/opencrypto/cryptodev.h>

static int ipcomp_deflate_common(struct mbuf *, struct mbuf *, struct secasvar *, struct ipsecrequest *, u_int8_t *, int);
static int ipcomp_deflate_compress(const struct comp_algo *, u_int8_t *, u_int32_t, u_int8_t **);
static int ipcomp_deflate_decompress(const struct comp_algo *, u_int8_t *, u_int32_t, u_int8_t **);
static int ipcomp_input(struct mbuf *, struct secasvar *, int, int, int);
static int ipcomp_output(struct mbuf *, struct ipsecrequest *, struct mbuf **, int, int, int);
static int ipcomp_input_cb(struct cryptop *crp);
static int ipcomp_output_cb(struct cryptop *crp);

const struct comp_algo *
ipcomp_algorithm_lookup(alg)
	int alg;
{
	if (alg >= IPCOMP_ALG_MAX) {
		return (NULL);
	}
	switch (alg) {
	case SADB_X_CALG_DEFLATE:
		return (&comp_algo_deflate);
	}
	return (NULL);
}

static int
ipcomp_deflate_compress(tcomp, data, size, out)
	const struct comp_algo *tcomp;
	u_int8_t *data;
	u_int32_t size;
	u_int8_t **out;
{
	u_int32_t result;

	result = (*tcomp->compress)(data, size, out);
	if (result == 0) {
		return (EINVAL);
	}
	return (0);
}

static int
ipcomp_deflate_decompress(tcomp, data, size, out)
	const struct comp_algo *tcomp;
	u_int8_t *data;
	u_int32_t size;
	u_int8_t **out;
{
	u_int32_t result;

	result = (*tcomp->decompress)(data, size, out);
	if (result == 0) {
		return (EINVAL);
	}
	return (0);
}

/*
 * Notes:
 * - Potential Issues: missing kame ipcomp's mprev chain.
 */
static int
ipcomp_deflate_common(m, md, sav, isr, out, mode)
	struct mbuf *m;
	struct mbuf *md;
	struct secasvar *sav;
	struct ipsecrequest *isr;
	u_int8_t *out;
	int mode;	/* 0: compress (output) 1: decompress (input) */
{
	struct mbuf *p;
	u_int8_t *data;
	u_int32_t size;
	int error;

	error = 0;
	p = md;
	data = NULL;
	size = 0;
	while (p != NULL && p->m_len == 0) {
		p = p->m_next;
	}

	while (p && p->m_len != 0) {
		if (p->m_len != 0) {
			data = mtod(p, u_int8_t *);
			size = p->m_len;
			p = p->m_next;
			while (p && p->m_len == 0) {
				p = p->m_next;
			}
		}
		break;
	}

	if (data == NULL && size == 0) {
		return (EINVAL);
	}

	switch (mode) {
	case 0:
		error = ipcomp_output(m, isr, &p, (int)data, (int)size, *out);
		break;
	case 1:
		error = ipcomp_input(m, sav, (int)data, (int)size, *out);
		break;
	}
	return (error);
}

int
ipcomp_compress(m, md, sav, lenp)
	struct mbuf *m, *md;
	struct secasvar *sav;
	size_t *lenp;
{
	if (!m) {
		panic("m == NULL in deflate_compress");
	}
	if (!md) {
		panic("md == NULL in deflate_compress");
	}
	if (!lenp) {
		panic("lenp == NULL in deflate_compress");
	}
	return (ipcomp_deflate_common(m, md, sav, NULL, (u_int8_t *)lenp, 0));
}

int
ipcomp_decompress(m, md, sav, lenp)
	struct mbuf *m, *md;
	struct secasvar *sav;
	size_t *lenp;
{
	if (!m) {
		panic("m == NULL in deflate_decompress");
	}
	if (!md) {
		panic("md == NULL in deflate_decompress");
	}
	if (!lenp) {
		panic("lenp == NULL in deflate_decompress");
	}
	return (ipcomp_deflate_common(m, md, sav, NULL, (u_int8_t *)lenp, 1));
}

/*
 * ipcomp_init() is called when an CPI is being set up.
 */
static int
ipcomp_init(sav, xsp)
	struct secasvar *sav;
	struct xformsw *xsp;
{
	struct tdb *tdb;
	const struct comp_algo *tcomp;
	struct cryptoini cric;

	/* NB: algorithm really comes in alg_enc and not alg_comp! */
	tcomp = ipcomp_algorithm_lookup(sav->alg_enc);
	if (tcomp == NULL) {
		ipseclog((LOG_ERR, "ipcomp_init: unsupported compression algorithm %d\n",
			 sav->alg_comp));
		return (EINVAL);
	}

	sav->alg_comp = sav->alg_enc;		/* set for doing histogram */
	tdb = tdb_init(sav, xsp, NULL, NULL, tcomp, IPPROTO_IPCOMP);
	if (tdb == NULL) {
		ipseclog((LOG_ERR, "ipcomp_init: no memory for tunnel descriptor block\n"));
		return (EINVAL);
	}

	/* Initialize crypto session */
	bzero(&cric, sizeof (cric));
	cric.cri_alg = tdb->tdb_compalgxform->type;
	return (crypto_newsession(&tdb->tdb_cryptoid, &cric, crypto_support));
}

/*
 * ipcomp_zeroize() used when IPCA is deleted
 */
static int
ipcomp_zeroize(sav)
	struct secasvar *sav;
{
	return (tdb_zeroize(sav->tdb_tdb));
}

/*
 * ipcomp_input() gets called to uncompress an input packet
 */
static int
ipcomp_input(m, sav, skip, length, offset)
	struct mbuf *m;
	struct secasvar *sav;
	int skip, length, offset;
{
	struct tdb *tdb;
	struct cryptodesc *crdc;
	struct cryptop *crp;
	int hlen;

	hlen = sizeof(struct ipcomp);

	tdb = sav->tdb_tdb;

	/* Get crypto descriptors */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		m_freem(m);
		ipseclog((LOG_ERR, "ipcomp_input: no crypto descriptors\n"));
		//ipcompstat.ipcomps_crypto++;
		return (ENOBUFS);
	}

	/* Get IPsec-specific opaque pointer */
	tdb = tdb_alloc(0);
	if (tdb == NULL) {
		m_freem(m);
		crypto_freereq(crp);
		ipseclog((LOG_ERR, "ipcomp_input: cannot allocate tdb_crypto\n"));
		//ipcompstat.ipcomps_crypto++;
		return (ENOBUFS);
	}
	sav->tdb_tdb = tdb;
	tdb->tdb_sav = sav;

	crdc = crp->crp_desc;

	crdc->crd_skip = skip + hlen;
	crdc->crd_len = m->m_pkthdr.len - (skip + hlen);
	crdc->crd_inject = skip;

	/* Decompression operation */
	crdc->crd_alg = tdb->tdb_compalgxform->type;

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len - (skip + hlen);
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t)m;
	crp->crp_callback = ipcomp_input_cb;
	crp->crp_sid = tdb->tdb_cryptoid;
	crp->crp_opaque = (caddr_t)tdb;

	/* These are passed as-is to the callback */
	tdb->tdb_spi = sav->spi;
	tdb->tdb_dst = sav->sah->saidx.dst;
	tdb->tdb_src = sav->sah->saidx.src;
	tdb->tdb_proto = sav->sah->saidx.proto;
	tdb->tdb_skip = skip;
	tdb->tdb_length = length;
	tdb->tdb_offset = offset;

	return (ipcomp_input_cb(crp));
}

static int
ipcomp_input_cb(crp)
	struct cryptop *crp;
{
	struct cryptodesc *crd;
	struct tdb *tdb;
	struct mbuf *m;
	struct secasvar *sav;
	const struct comp_algo *ipcompx;
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
		ipseclog((LOG_ERR, "ipcomp_output_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	ipcompx = tdb->tdb_compalgxform;

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

		ipseclog((LOG_ERR, "ipcomp_input_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		ipseclog((LOG_ERR, "ipcomp_input_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}

	/* Release the crypto descriptors */
	tdb_free(tdb);
	tdb = NULL;
	crypto_freereq(crp);
	crp = NULL;

	error = ipcomp_deflate_decompress(ipcompx, (u_int8_t *)skip, length, (u_int8_t **)offset);

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
ipcomp_output(m, isr, mp, skip, length, offset)
	struct mbuf *m;
	struct ipsecrequest *isr;
	struct mbuf **mp;
	int skip, length, offset;
{
	struct secasvar *sav;
	const struct comp_algo *ipcompx;
	int error, ralen, hlen, maxpacketsize;
	u_int8_t prot;
	struct cryptodesc *crdc;
	struct cryptop *crp;
	struct tdb *tdb;
	struct ipcomp *ipcomp;

	sav = isr->sav;
	tdb = sav->tdb_tdb;
	ipcompx = tdb->tdb_compalgxform;

	hlen = sizeof(struct ipcomp);

	/* Ok now, we can pass to the crypto processing */

	/* Get crypto descriptors */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		//ipcompstat.ipcomps_crypto++;
		ipseclog((LOG_ERR, "ipcomp_output: failed to acquire crypto descriptor\n"));
		error = ENOBUFS;
		goto bad;
	}
	crdc = crp->crp_desc;

	/* Compression descriptor */
	crdc->crd_skip = skip + hlen;
	crdc->crd_len = m->m_pkthdr.len - (skip + hlen);
	crdc->crd_flags = CRD_F_COMP;
	crdc->crd_inject = skip + hlen;

	/* Compression operation */
	crdc->crd_alg = ipcompx->type;

	/* Get IPsec-specific opaque pointer */
	tdb = tdb_alloc(0);
	if (tdb == NULL) {
		//ipcompstat.ipcomps_crypto++;
		ipseclog((LOG_ERR, "ipcomp_output: failed to allocate tdb_crypto\n"));
		crypto_freereq(crp);
		error = ENOBUFS;
		goto bad;
	}
	sav->tdb_tdb = tdb;
	tdb->tdb_sav = sav;

	tdb->tdb_isr = isr;
	tdb->tdb_spi = sav->spi;
	tdb->tdb_dst = sav->sah->saidx.dst;
	tdb->tdb_src = sav->sah->saidx.src;
	tdb->tdb_proto = sav->sah->saidx.proto;
	tdb->tdb_skip = skip;
	tdb->tdb_length = length;
	tdb->tdb_offset = offset;

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len;	/* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = (caddr_t) m;
	crp->crp_callback = ipcomp_output_cb;
	crp->crp_opaque = (caddr_t)tdb;
	crp->crp_sid = tdb->tdb_cryptoid;

	return (ipcomp_output_cb(crp));

bad:
	if (m) {
		m_freem(m);
	}
	return (error);
}

static int
ipcomp_output_cb(crp)
	struct cryptop *crp;
{
	struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m, *md;
	struct tdb *tdb;
	const struct comp_algo *ipcompx;
	int s, error, hlen, skip, length, offset;

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
		ipseclog((LOG_ERR, "ipcomp_output_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	ipcompx = tdb->tdb_compalgxform;

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

		ipseclog((LOG_ERR, "ipcomp_output_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		ipseclog((LOG_ERR, "ipcomp_output_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}

	/* Release the crypto descriptors */
	tdb_free(tdb);
	tdb = NULL;
	crypto_freereq(crp);
	crp = NULL;

	error = ipcomp_deflate_compress(ipcompx, (u_int8_t *)skip, length, (u_int8_t **)offset);

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

static struct xformsw ipcomp_xformsw = {
		.xf_type = XF_IPCOMP,
		.xf_flags = XFT_COMP,
		.xf_name = "Kame IPsec IPcomp",
		.xf_init = ipcomp_init,
		.xf_zeroize = ipcomp_zeroize,
		.xf_input = ipcomp_input,
		.xf_output = ipcomp_output,
};

void
ipcomp_attach(void)
{
	xform_register(&ipcomp_xformsw);
}
