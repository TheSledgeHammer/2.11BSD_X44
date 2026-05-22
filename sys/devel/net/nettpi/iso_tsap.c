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

/*
 * TODO:
 * - rename: functions that are layer independent or have commonality between
 * each layer.
 * - most sap_select functions fall into this category.
 */
/*
 * NSAP & TSAP network stack dependent setup/initialization:
 * - will occur from a tp_protosw callback function. (labelled tppw_init)
 * - the tppw_init callback will run from within the tp_init function.
 */

#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iso_nsap.h"

uint32_t tsap_valid_ids[SAP_TABLE_MAX];

static int tsap_validate(struct tsap_iso *, struct nsap_iso *, int, int);
static int tsap_acknowledge(struct tsap_iso *, struct nsap_iso *, long, long, long, int, int, int);
static int tsap_canconnect(struct tsap_iso *, void *, long, long, long, int, int, int);
static int tsap_candisconnect(struct tsap_iso *, void *, long, long, long, int, int, int);

/*
 * TSAP MAC ID:
 * - TSAP MAC ID is the expanded TSAP ID in notation format.
 * - Can be in ascii or hex.
 * Current Format is:
 * - ID (ACSII): WIP
 * - ID (HEX): cn.type.subnet.protocol.class
 *
 * Format Legend:
 * - cn: connection number
 * - type: sap_type
 * - subnet: sap_subnet
 * - protocol: sap_protocol
 * - class: sap_class
 */

#define TSAP_MACID_LEN 32

void
tsap_mac_id(int cn, long type, long subnet, long protocol, int clazz)
{
    uint32_t cn_id, type_id, subnet_id, protocol_id, class_id;
    int len = TSAP_MACID_LEN;

    char tsap_hexid[len];

    cn_id = enhanced_double_hash(cn, len);
    type_id = sap_type_hash(type, len);
    subnet_id = sap_subnet_hash(subnet, len);
    protocol_id = sap_protocol_hash(protocol, len);
    class_id = sap_class_hash(clazz, len);
    (void)snprintf(tsap_hexid, sizeof(tsap_hexid), "%X.%X.%X.%X.%X\n", cn_id, type_id, subnet_id, protocol_id, class_id);
    //printf("%s\n", tsap_hexid);
}

/* TSAP's */
struct tsap_iso *
tsap_create(struct nsap_iso *nsap, int af)
{
	struct tsap_iso *tsap;

	MALLOC(tsap, struct tsap_iso *, sizeof(*tsap), M_ISOSAP, M_WAITOK);
	if (tsap == NULL) {
		return (NULL);
	}
	bzero((caddr_t)tsap, sizeof(*tsap));
	bcopy(nsap, tsap->tsi_nsaps, sizeof(tsap->tsi_nsaps));
	return (tsap);
}

void
tsap_destroy(struct tsap_iso *tsap)
{
	if (tsap != NULL) {
		FREE(tsap, M_ISOSAP);
	}
}

void
tsap_attach(struct tsap_iso *tsap, struct nsap_iso *nsap, int af)
{
	struct tsap_iso *tsiso;
	int sid;

	tsiso = tsap_create(nsap, af);
	if (tsiso != NULL) {
		sid = sap_select_af_to_sid(af);
		sap_select_init(&tsiso->tsi_select, sid, af);
		tsap = tsiso;
	}
	tsap = NULL;
}

void
tsap_detach(struct tsap_iso *tsap, struct nsap_iso *nsap, int af)
{
	if (tsap != NULL) {
		nsap_detach(nsap, af);
		bcopy(nsap, tsap->tsi_nsaps, sizeof(tsap->tsi_nsaps));
		if (tsap->tsi_nsaps == NULL) {
			tsap_destroy(tsap);
		}
	}
}

/*
 * returns an nsap from tsap
 */
struct nsap_iso *
tsap_to_nsap(struct tsap_iso *tsap)
{
	struct nsap_iso *nsap;

	nsap = &tsap->tsi_nsaps;
	if (nsap != NULL) {
		return (nsap);
	}
	return (NULL);
}

/*
 * tsap_iso_compare:
 * - compares nsap_iso and sap_select
 * returns -1 if a, 1 if b and 0 if equal
 */
int
tsap_iso_compare(struct tsap_iso *a, struct tsap_iso *b)
{
	int error;

	if (a != b) {
		error = nsap_iso_compare(&a->tsi_nsaps, &b->tsi_nsaps);
		if (error != 0) {
			return (error);
		}
		error = sap_select_compare(&a->tsi_select, &b->tsi_select);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}

static int
tsap_validate(struct tsap_iso *tsap, struct nsap_iso *nsap, int sid, int af)
{
	if (tsap != NULL) {
		if ((tsap->tsi_selector[sid] != 0) && (tsap_valid_ids[sid] != 0)) {
			if (tsap->tsi_selector[sid] == tsap_valid_ids[sid]) {
				if (tsap->tsi_nsaps != *nsap) {
					return (1);
				}
				return (0);
			}
		}
	}
	return (1);
}

static int
tsap_acknowledge(struct tsap_iso *tsap, struct nsap_iso *nsap, long type, long subnet, long protocol, int class, int sid, int af)
{
	long sap_type, sap_subnet, sap_protocol;
	int sap_class, error;

	sap_type = sap_select_lookup_type(sid, af, type);
	sap_subnet = sap_select_lookup_subnet(sid, af, subnet);
	sap_protocol = sap_select_lookup_protocol(sid, af, protocol);
	sap_class = sap_select_lookup_class(sid, af, class);

	/* check tsap id */
	error = tsap_id(sap_type, sap_subnet, sap_protocol, sap_class);
	if (error == 0) {
		return (EAFNOSUPPORT);
	}
	error = tsap_validate(tsap, nsap, sid, af);
	if (error != 0) {
		return (EAFNOSUPPORT);
	}
	return (0);
}

static int
tsap_canconnect(struct tsap_iso *tsap, void *arg, long type, long subnet, long protocol, int class, int sid, int af)
{
	struct nsap_iso *nsap;
	int error;

	nsap = tsap_to_nsap(tsap);
	if (nsap == NULL) {
		return (EINVAL);
	}
	error = tsap_acknowledge(tsap, nsap, type, subnet, protocol, class, sid, af);
	if (error != 0) {
		return (error);
	}
	return (nsap_canconnect(nsap->nsi_snsap, arg, type, subnet, class));
}

static int
tsap_candisconnect(struct tsap_iso *tsap, void *arg, long type, long subnet, long protocol, int class, int sid, int af)
{
	struct nsap_iso *nsap;
	int error;

	nsap = tsap_to_nsap(tsap);
	if (nsap == NULL) {
		return (EINVAL);
	}
	error = tsap_acknowledge(tsap, nsap, type, subnet, protocol, class, sid, af);
	if (error != 0) {
		return (error);
	}
	return (nsap_candisconnect(nsap->nsi_snsap, arg, type, subnet, class));
}

int
tsap_connect(struct tsap_iso *tsap, void *arg, int af)
{
	long type, subnet, protocol;
	int class;
	int error, sid;

	sid = sap_select_af_to_sid(af);
	if (af != sap_select_sid_to_af(sid)) {
		return (EAFNOSUPPORT);
	}
	for (type = 0; type < SAP_TYPE_MAX; type++) {
		if (sap_select_lookup_type(sid, af, type)) {
			break;
		}
	}
	for (subnet = 0; subnet < SAP_SUBNET_MAX; subnet++) {
		if (sap_select_lookup_subnet(sid, af, subnet)) {
			break;
		}
	}
	for (protocol = 0; protocol < SAP_PROTOCOL_MAX; protocol++) {
		if (sap_select_lookup_protocol(sid, af, protocol)) {
			break;
		}
	}
	for (class = 0; class < SAP_CLASS_MAX; class++) {
		if (sap_select_lookup_class(sid, af, class)) {
			break;
		}
	}
	error = tsap_canconnect(tsap, arg, type, subnet, protocol, class, sid, af);
	if (error != 0) {
		return (error);
	}
	return (0);
}

int
tsap_disconnect(struct tsap_iso *tsap, void *arg, int af)
{
	long type, subnet, protocol;
	int class;
	int error, sid;

	sid = sap_select_af_to_sid(af);
	if (af != sap_select_sid_to_af(sid)) {
		return (EAFNOSUPPORT);
	}

	for (type = 0; type < SAP_TYPE_MAX; type++) {
		if (sap_select_lookup_type(sid, af, type)) {
			break;
		}
	}
	for (subnet = 0; subnet < SAP_SUBNET_MAX; subnet++) {
		if (sap_select_lookup_subnet(sid, af, subnet)) {
			break;
		}
	}
	for (protocol = 0; protocol < SAP_PROTOCOL_MAX; protocol++) {
		if (sap_select_lookup_protocol(sid, af, protocol)) {
			break;
		}
	}
	for (class = 0; class < SAP_CLASS_MAX; class++) {
		if (sap_select_lookup_class(sid, af, class)) {
			break;
		}
	}
	error = tsap_candisconnect(tsap, arg, type, subnet, protocol, class, sid, af);
	if (error != 0) {
		return (error);
	}
	return (0);
}


/* ISO */
int
tpiso_pcbconnect(struct tsap_iso *tsap, void *v, struct mbuf *nam)
{
	struct isopcb *isop;
	struct sockaddr_iso *siso;
	int error;

	isop = (struct isopcb *)v;
	siso = mtod(nam, struct sockaddr_iso);
	if (nam->m_len != sizeof(siso)) {
		return (EINVAL);
	}
	if (siso->siso_family != AF_ISO) {
		return (EAFNOSUPPORT);
	}
	if (siso->siso_nlen == 0) {
		return (EADDRNOTAVAIL);
	}
	error = tsap_connect(tsap, siso, AF_ISO);
	if (error != 0) {
		return (error);
	}
	return (iso_pcbconnect(isop, nam));
}

void
tpiso_pcbdisconnect(struct tsap_iso *tsap, void *v)
{
	struct isopcb *isop;
	struct sockaddr_iso *siso;
	int error;

	isop = (struct isopcb *)v;
	siso = &tsap->tsi_nsaps->nsi_snsap->snsap_siso;
	if (siso == NULL) {
		return;
	}
	error = tsap_disconnect(tsap, siso, AF_ISO);
	if (error != 0) {
		return;
	}
	if (isop) {
		iso_pcbdisconnect(isop);
	}
}

/* ISO TPCONS */
int
tpcons_pcbconnect(struct tsap_iso *tsap, void *v, struct mbuf *nam)
{
	struct isopcb *isop;
	struct sockaddr_iso *siso;
	int error;

	isop = (struct isopcb *)v;
	siso = mtod(nam, struct sockaddr_iso);
	if (nam->m_len != sizeof(siso)) {
		return (EINVAL);
	}
	if (siso->siso_family != AF_ISO) {
		return (EAFNOSUPPORT);
	}
	if (siso->siso_nlen == 0) {
		return (EADDRNOTAVAIL);
	}

	error = tsap_connect(tsap, siso, AF_ISO);
	if (error != 0) {
		return (error);
	}
	return (tpcons_pcbconnect(isop, nam));
}

void
tpcons_pcbdisconnect(struct tsap_iso *tsap, void *v)
{
	struct isopcb *isop;
	struct sockaddr_iso *siso;
	int error;

	isop = (struct isopcb *)v;
	siso = &tsap->tsi_nsaps->nsi_snsap->snsap_siso;
	if (siso == NULL) {
		return;
	}
	error = tsap_disconnect(tsap, siso, AF_ISO);
	if (error != 0) {
		return;
	}
	if (isop) {
		iso_pcbdisconnect(isop);
	}
}

/* INET */
int
tpip_pcbconnect(struct tsap_iso *tsap, void *v, struct mbuf *nam)
{
	struct inpcb *inp;
	struct sockaddr_in *sin;
	int error;

	inp = (struct inpcb *)v;
	sin = mtod(nam, struct sockaddr_in);
	if (nam->m_len != sizeof(sin)) {
		return (EINVAL);
	}
	if (sin->sin_family != AF_INET) {
		return (EAFNOSUPPORT);
	}
	if (sin->sin_port == 0) {
		return (EADDRNOTAVAIL);
	}
	error = tsap_connect(tsap, sin, AF_INET);
	if (error != 0) {
		return (error);
	}
	return (in_pcbconnect(inp, nam));
}

void
tpip_pcbdisconnect(struct tsap_iso *tsap, void *v)
{
	struct inpcb *inp;
	struct sockaddr_in *sin;
	int error;

	inp = (struct inpcb *)v;
	sin = &tsap->tsi_nsaps->nsi_snsap->snsap_sin4;
	if (sin == NULL) {
		return;
	}
	error = tsap_disconnect(tsap, sin, AF_INET);
	if (error != 0) {
		return;
	}
	if (inp) {
		in_pcbdisconnect(inp);
	}
}

/* INET6 */
int
tpip6_pcbconnect(struct tsap_iso *tsap, void *v, struct mbuf *nam)
{
	struct in6pcb *in6p;
	struct sockaddr_in6 *sin6;
	int error;

	in6p = (struct in6pcb *)v;
	sin6 = mtod(nam, struct sockaddr_in6);
	if (nam->m_len != sizeof(sin6)) {
		return (EINVAL);
	}
	if (sin6->sin6_family != AF_INET6) {
		return (EAFNOSUPPORT);
	}
	if (sin6->sin6_port == 0) {
		return (EADDRNOTAVAIL);
	}
	error = tsap_connect(tsap, sin6, AF_INET6);
	if (error != 0) {
		return (error);
	}
	return (in6_pcbconnect(in6p, nam));
}

void
tpip6_pcbdisconnect(struct tsap_iso *tsap, void *v)
{
	struct in6pcb *in6p;
	struct sockaddr_in6 *sin6;
	int error;

	in6p = (struct in6pcb *)v;
	sin6 = &tsap->tsi_nsaps->nsi_snsap->snsap_sin6;
	if (sin6 == NULL) {
		return;
	}
	error = tsap_disconnect(tsap, sin6, AF_INET6);
	if (error != 0) {
		return;
	}
	if (in6p) {
		in6_pcbdisconnect(in6p);
	}
}

/* XNS */
int
tpns_pcbconnect(struct tsap_iso *tsap, void *v, struct mbuf *nam)
{
	struct nspcb *nsp;
	struct sockaddr_ns *sns;
	int error;

	nsp = (struct nspcb *)v;
	sns = mtod(nam, struct sockaddr_ns);
	if (nam->m_len != sizeof(sns)) {
		return (EINVAL);
	}
	if (sns->sns_family != AF_NS) {
		return (EAFNOSUPPORT);
	}
	if (sns->sns_port == 0) {
		return (EADDRNOTAVAIL);
	}
	error = tsap_connect(tsap, sns, AF_NS);
	if (error != 0) {
		return (error);
	}
	return (ns_pcbconnect(nsp, nam));
}

void
tpns_pcbdisconnect(struct tsap_iso *tsap, void *v)
{
	struct nspcb *nsp;
	struct sockaddr_ns *sns;
	int error;

	nsp = (struct nspcb *)v;
	sns = &tsap->tsi_nsaps->nsi_snsap->snsap_sns;
	if (sns == NULL) {
		return;
	}
	error = tsap_disconnect(tsap, sns, AF_NS);
	if (error != 0) {
		return;
	}
	if (nsp) {
		ns_pcbdisconnect(nsp);
	}
}
