/*
 * xform.c
 *
 *  Created on: Feb 6, 2026
 *      Author: marti
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

void
tdb_zeroize(tdb)
	struct tdb *tdb;
{
	switch (tdb->tdb_proto) {
	case IPPROTO_ESP:
		tdb->tdb_encalgxform = NULL;
		break;
	case IPPROTO_AH:
		tdb->tdb_cryptoid = 0;
		tdb->tdb_authalgxform = NULL;
		break;
	case IPPROTO_IPCOMP:
		tdb->tdb_compalgxform = NULL;
		break;
	}
	tdb->tdb_xform = NULL;
}



