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

#include <kame_ipsec/ipsec.h>
#include <kame_ipsec/ah.h>
#include <kame_ipsec/esp.h>

#include <kame_ipsec/xform.h>

#include <net/pfkeyv2.h>
#include <netkey/key.h>
#include <netkey/key_debug.h>

#include <net/net_osdep.h>

#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/xform.h>

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
