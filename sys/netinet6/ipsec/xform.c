/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>

#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/queue.h>
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
#include <netkey/key_debug.h>

#include <net/net_osdep.h>

#include <crypto/opencrypto/cryptodev.h>

struct tdb *
tdb_alloc(size)
	int size;
{
	struct tdb *tdb;

	tdb = (struct tdb *)malloc(sizeof(struct tdb) + size, M_XDATA, M_NOWAIT|M_ZERO);
	if (tdb != NULL) {
		return (tdb);
	}
	return (NULL);
}

void
tdb_free(tdb)
	struct tdb *tdb;
{
	free(tdb, M_XDATA);
}

struct tdb *
tdb_init(sav, xsp, txform, thash, tcomp, proto)
	struct secasvar *sav;
	struct xformsw *xsp;
	const struct enc_xform *txform;
	const struct auth_hash *thash;
	const struct comp_algo *tcomp;
	u_int8_t proto;
{
	struct tdb *tdb;

	tdb = tdb_alloc(0);
	if (tdb == NULL) {
		return (NULL);
	}

	sav->tdb_tdb = tdb;
	tdb->tdb_sav = sav;
	tdb->tdb_proto = proto;
	tdb->tdb_xform = xsp;
	if (txform != NULL) {
		tdb->tdb_encalgxform = txform;
	} else {
		tdb->tdb_encalgxform = NULL;
	}
	if (thash != NULL) {
		tdb->tdb_authalgxform = thash;
	} else {
		tdb->tdb_authalgxform = NULL;
	}
	if (tcomp != NULL) {
		tdb->tdb_compalgxform = tcomp;
	} else {
		tdb->tdb_compalgxform = NULL;
	}
	return (tdb);
}

int
tdb_zeroize(tdb)
	struct tdb *tdb;
{
	int error;

	switch (tdb->tdb_proto) {
	case IPPROTO_ESP:
		error = 0;
		tdb->tdb_encalgxform = NULL;
		tdb->tdb_xform = NULL;
		break;
	case IPPROTO_AH:
		error = crypto_freesession(tdb->tdb_cryptoid);
		tdb->tdb_cryptoid = 0;
		tdb->tdb_authalgxform = NULL;
		tdb->tdb_xform = NULL;
		break;
	case IPPROTO_IPCOMP:
		error = crypto_freesession(tdb->tdb_cryptoid);
		tdb->tdb_cryptoid = 0;
		break;
	}
	return (error);
}

/* output: 0: src 1: dst */
struct sockaddr_in *
tdb_get_sin(tdb, output)
    struct tdb *tdb;
    int output;
{
    struct sockaddr_in *src, *dst;

    switch (output) {
    case 0:
        src = (struct sockaddr_in *)&tdb->tdb_src;
         return (src);
    case 1:
        dst = (struct sockaddr_in *)&tdb->tdb_dst;
        return (dst);
    default:
        break;
    }
    return (NULL);
}

/* output: 0: src 1: dst */
struct in_addr *
tdb_get_in(tdb, output)
    struct tdb *tdb;
    int output;
{
    struct sockaddr_in *ssin, *dsin;
    struct in_addr *src, *dst;

    switch (output) {
	case 0:
		ssin = tdb_get_sin(tdb, 0);
		src = &ssin->sin_addr;
		return (src);
	case 1:
		dsin = tdb_get_sin(tdb, 1);
		dst = &dsin->sin_addr;
		return (dst);
	default:
		break;
	}
    return (NULL);
}

/* output: 0: src 1: dst */
struct sockaddr_in6 *
tdb_get_sin6(tdb, output)
    struct tdb *tdb;
    int output;
{
    struct sockaddr_in6 *src, *dst;

    switch (output) {
    case 0:
        src = (struct sockaddr_in6 *)&tdb->tdb_src;
         return (src);
    case 1:
        dst = (struct sockaddr_in6 *)&tdb->tdb_dst;
        return (dst);
    default:
        break;
    }
    return (NULL);
}

/* output: 0: src 1: dst */
struct in6_addr *
tdb_get_in6(tdb, output)
    struct tdb *tdb;
    int output;
{
    struct sockaddr_in6 *ssin, *dsin;
    struct in6_addr *src, *dst;

    switch (output) {
	case 0:
		ssin = tdb_get_sin6(tdb, 0);
		src = &ssin->sin6_addr;
		return (src);
	case 1:
		dsin = tdb_get_sin6(tdb, 1);
		dst = &dsin->sin6_addr;
		return (dst);
	default:
		break;
	}
    return (NULL);
}


static struct xformsw* xforms = NULL;

/*
 * Register a transform; typically at system startup.
 */
void
xform_register(xsp)
	struct xformsw* xsp;
{
	xsp->xf_next = xforms;
	xforms = xsp;
}

/*
 * Initialize transform support in an sav.
 */
int
xform_init(sav, xftype)
	struct secasvar *sav;
	int xftype;
{
	struct xformsw *xsp;
	struct tdb *tdb;

	tdb = sav->tdb_tdb;
	if (tdb != NULL) {
		if (tdb->tdb_xform != NULL)	{ /* previously initialized */
			return (0);
		}
	}

	for (xsp = xforms; xsp; xsp = xsp->xf_next) {
		if (xsp->xf_type == xftype) {
			return ((*xsp->xf_init)(sav, xsp));
		}
	}

	ipseclog((LOG_DEBUG, "xform_init: no match for xform type %d\n", xftype));
	return (EINVAL);
}

void
ipsec_attach(void)
{
	printf("initializing IPsec...");
	ah_attach();
	esp_attach();
	ipcomp_attach();
	//ipe4_attach();
	printf(" done\n");
}

void
tdb_keycleanup(sav)
	struct secasvar *sav;
{
	struct tdb *tdb;

	/*
	 * Cleanup xform state.  Note that zeroize'ing causes the
	 * keys to be cleared; otherwise we must do it ourself.
	 */
	tdb = sav->tdb_tdb;
	if (tdb != NULL) {
		if (tdb->tdb_xform != NULL) {
			(*tdb->tdb_xform->xf_zeroize)(sav);
			tdb->tdb_xform = NULL;
		}
	} else {
		if (sav->key_auth != NULL) {
			bzero(_KEYBUF(sav->key_auth), _KEYLEN(sav->key_auth));
		}
		if (sav->key_enc != NULL) {
			bzero(_KEYBUF(sav->key_enc), _KEYLEN(sav->key_enc));
		}
	}
}

int
tdb_keysetsaval(sav, satype)
	struct secasvar *sav;
	int satype;
{
	int error;
	switch (satype) {
	case SADB_SATYPE_AH:
		error = xform_init(sav, XF_AH);
		break;
	case SADB_SATYPE_ESP:
		error = xform_init(sav, XF_ESP);
		break;
	case SADB_X_SATYPE_IPCOMP:
		error = xform_init(sav, XF_IPCOMP);
		break;
	default:
		ipseclog((LOG_DEBUG, "key_setsaval: invalid SA type.\n"));
		error = EINVAL;
		break;
	}
	if (error) {
		ipseclog(
				(LOG_DEBUG, "key_setsaval: unable to initialize SA type %u.\n", satype));
	}
	return (error);
}

