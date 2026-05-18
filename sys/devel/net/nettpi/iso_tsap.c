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

int
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

int
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
tsap_connect(struct tsap_iso *tsap, void *arg, long subnet, long protocol, int af)
{
	int error, sid;

	sid = sap_select_af_to_sid(af);
	if (af != sap_select_sid_to_af(sid)) {
		return (EAFNOSUPPORT);
	}
	switch (af) {
	case AF_INET:
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)arg;
		if (sin->sin_family != AF_INET) {
			error = EAFNOSUPPORT;
			break;
		}
		if (sin->sin_port == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		switch (protocol) {
		case TSAP_PROTOCOL_TCP:
			error = tsap_canconnect(tsap, (struct sockaddr_in *)sin,
					NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4, TSAP_PROTOCOL_TCP,
					NSAP_CLASS_CLNS, sid, AF_INET);
			break;
		case TSAP_PROTOCOL_UDP:
			error = tsap_canconnect(tsap, (struct sockaddr_in *)sin,
					NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4, TSAP_PROTOCOL_UDP,
					NSAP_CLASS_CLNS, sid, AF_INET);
			break;
		default:
			error = tsap_canconnect(tsap, (struct sockaddr_in *)sin,
					NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4, TSAP_PROTOCOL_UNKNOWN,
					NSAP_CLASS_CLNS, sid, AF_INET);
			break;
		}
		break;
	}
	case AF_INET6:
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)arg;
		if (sin6->sin6_family != AF_INET6) {
			error = EAFNOSUPPORT;
			break;
		}
		if (sin6->sin6_port == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		switch (protocol) {
		case TSAP_PROTOCOL_TCP:
			error = tsap_canconnect(tsap, (struct sockaddr_in6 *)sin6,
					NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6, TSAP_PROTOCOL_TCP,
					NSAP_CLASS_CLNS, sid, AF_INET6);
			break;
		case TSAP_PROTOCOL_UDP:
			error = tsap_canconnect(tsap, (struct sockaddr_in6 *)sin6,
					NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6, TSAP_PROTOCOL_UDP,
					NSAP_CLASS_CLNS, sid, AF_INET6);
			break;
		default:
			error = tsap_canconnect(tsap, (struct sockaddr_in6 *)sin6,
					NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6, TSAP_PROTOCOL_UNKNOWN,
					NSAP_CLASS_CLNS, sid, AF_INET6);
			break;
		}
		break;
	}
	case AF_ISO:
	{
		struct sockaddr_iso *siso = (struct sockaddr_iso *)arg;
		if (siso->siso_family != AF_ISO) {
			error = EAFNOSUPPORT;
			break;
		}
		if (siso->siso_nlen == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		switch (protocol) {
		case TSAP_PROTOCOL_TP0:
			error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_CONS, TSAP_PROTOCOL_TP0,
					NSAP_CLASS_CONS, sid, AF_ISO);
			break;
		case TSAP_PROTOCOL_TP1:
			error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_CONS, TSAP_PROTOCOL_TP1,
					NSAP_CLASS_CONS, sid, AF_ISO);
			break;
		case TSAP_PROTOCOL_TP2:
			error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_CONS, TSAP_PROTOCOL_TP2,
					NSAP_CLASS_CONS, sid, AF_ISO);
			break;
		case TSAP_PROTOCOL_TP3:
			error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_CONS, TSAP_PROTOCOL_TP3,
					NSAP_CLASS_CONS, sid, AF_ISO);
			break;
		case TSAP_PROTOCOL_TP4:
		{
			switch (subnet) {
			case NSAP_SUBNET_CLNS:
				error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_CLNS, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			case NSAP_SUBNET_CLNP:
				error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_CLNP, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			case NSAP_SUBNET_ISIS:
				error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_ISIS, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			case NSAP_SUBNET_ESIS:
				error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_ESIS, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			default:
				error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_UNKNOWN, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			}
			break;
		}
		default:
			error = tsap_canconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_UNKNOWN, TSAP_PROTOCOL_UNKNOWN,
					NSAP_CLASS_UNKNOWN, sid, AF_ISO);
			break;
		}
		break;
	}
	case AF_NS:
	{
		struct sockaddr_ns *sns = (struct sockaddr_ns *)arg;
		if (sns->sns_family != AF_NS) {
			error = EAFNOSUPPORT;
			break;
		}
		if (sns->sns_port == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		error = tsap_canconnect(tsap, (struct sockaddr_ns *)sns,
				NSAP_TYPE_SNS, NSAP_SUBNET_IDP, TSAP_PROTOCOL_SPP,
				NSAP_CLASS_CLNS, sid, AF_NS);
		break;
	}
	case AF_CCITT:
	{
		struct sockaddr_x25 *sx25 = (struct sockaddr_x25 *)arg;
		if (sx25->x25_family != AF_CCITT) {
			error = EAFNOSUPPORT;
			break;
		}
		if (sx25->x25_len == 0) {
			error = EADDRNOTAVAIL;
			break;
		}
		error = tsap_canconnect(tsap, (struct sockaddr_x25 *)sx25,
				NSAP_TYPE_SX25, NSAP_SUBNET_X25, TSAP_PROTOCOL_X25,
				NSAP_CLASS_CONS, sid, AF_CCITT);
		break;
	}
	case AF_NATM:
	case AF_IPX:
	case AF_SNA:
	default:
		error = tsap_canconnect(tsap, NULL, NSAP_TYPE_UNKNOWN,
				NSAP_SUBNET_UNKNOWN, TSAP_PROTOCOL_UNKNOWN,
				NSAP_CLASS_UNKNOWN, sid, AF_UNSPEC);
		break;
	}
	return (error);
}

int
tsap_disconnect(struct tsap_iso *tsap, void *arg, long subnet, long protocol, int af)
{
	int error, sid;

	sid = sap_select_af_to_sid(af);
	if (af != sap_select_sid_to_af(sid)) {
		return (EAFNOSUPPORT);
	}
	switch (af) {
	case AF_INET:
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)arg;
		if (sin->sin_family != AF_INET) {
			error = EAFNOSUPPORT;
			break;
		}
		switch (protocol) {
		case TSAP_PROTOCOL_TCP:
			error = tsap_candisconnect(tsap, (struct sockaddr_in *)sin,
					NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4, TSAP_PROTOCOL_TCP,
					NSAP_CLASS_CLNS, sid, AF_INET);
			break;
		case TSAP_PROTOCOL_UDP:
			error = tsap_candisconnect(tsap, (struct sockaddr_in *)sin,
					NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4, TSAP_PROTOCOL_UDP,
					NSAP_CLASS_CLNS, sid, AF_INET);
			break;
		default:
			error = tsap_candisconnect(tsap, (struct sockaddr_in *)sin,
					NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4, TSAP_PROTOCOL_UNKNOWN,
					NSAP_CLASS_CLNS, sid, AF_INET);
			break;
		}
		break;
	}
	case AF_INET6:
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)arg;
		if (sin6->sin6_family != AF_INET6) {
			error = EAFNOSUPPORT;
			break;
		}
		switch (protocol) {
		case TSAP_PROTOCOL_TCP:
			error = tsap_candisconnect(tsap, (struct sockaddr_in6 *)sin6,
					NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6, TSAP_PROTOCOL_TCP,
					NSAP_CLASS_CLNS, sid, AF_INET6);
			break;
		case TSAP_PROTOCOL_UDP:
			error = tsap_candisconnect(tsap, (struct sockaddr_in6 *)sin6,
					NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6, TSAP_PROTOCOL_UDP,
					NSAP_CLASS_CLNS, sid, AF_INET6);
			break;
		default:
			error = tsap_candisconnect(tsap, (struct sockaddr_in6 *)sin6,
					NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6, TSAP_PROTOCOL_UNKNOWN,
					NSAP_CLASS_CLNS, sid, AF_INET6);
			break;
		}
		break;
	}
	case AF_ISO:
	{
		struct sockaddr_iso *siso = (struct sockaddr_iso *)arg;
		if (siso->siso_family != AF_ISO) {
			error = EAFNOSUPPORT;
			break;
		}
		switch (protocol) {
		case TSAP_PROTOCOL_TP0:
			error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_CONS, TSAP_PROTOCOL_TP0,
					NSAP_CLASS_CONS, sid, AF_ISO);
			break;
		case TSAP_PROTOCOL_TP1:
			error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_CONS, TSAP_PROTOCOL_TP1,
					NSAP_CLASS_CONS, sid, AF_ISO);
			break;
		case TSAP_PROTOCOL_TP2:
			error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_CONS, TSAP_PROTOCOL_TP2,
					NSAP_CLASS_CONS, sid, AF_ISO);
			break;
		case TSAP_PROTOCOL_TP3:
			error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_CONS, TSAP_PROTOCOL_TP3,
					NSAP_CLASS_CONS, sid, AF_ISO);
			break;
		case TSAP_PROTOCOL_TP4:
		{
			switch (subnet) {
			case NSAP_SUBNET_CLNS:
				error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_CLNS, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			case NSAP_SUBNET_CLNP:
				error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_CLNP, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			case NSAP_SUBNET_ISIS:
				error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_ISIS, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			case NSAP_SUBNET_ESIS:
				error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_ESIS, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			default:
				error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
						NSAP_TYPE_SISO, NSAP_SUBNET_UNKNOWN, TSAP_PROTOCOL_TP4,
						NSAP_CLASS_CLNS, sid, AF_ISO);
				break;
			}
			break;
		}
		default:
			error = tsap_candisconnect(tsap, (struct sockaddr_iso *)siso,
					NSAP_TYPE_SISO, NSAP_SUBNET_UNKNOWN, TSAP_PROTOCOL_UNKNOWN,
					NSAP_CLASS_UNKNOWN, sid, AF_ISO);
			break;
		}
		break;
	}
	case AF_NS:
	{
		struct sockaddr_ns *sns = (struct sockaddr_ns *)arg;
		if (sns->sns_family != AF_NS) {
			error = EAFNOSUPPORT;
			break;
		}
		error = tsap_candisconnect(tsap, (struct sockaddr_ns *)sns,
				NSAP_TYPE_SNS, NSAP_SUBNET_IDP, TSAP_PROTOCOL_SPP,
				NSAP_CLASS_CLNS, sid, AF_NS);
		break;
	}
	case AF_CCITT:
	{
		struct sockaddr_x25 *sx25 = (struct sockaddr_x25 *)arg;
		if (sx25->x25_family != AF_CCITT) {
			error = EAFNOSUPPORT;
			break;
		}
		error = tsap_candisconnect(tsap, (struct sockaddr_x25 *)sx25,
				NSAP_TYPE_SX25, NSAP_SUBNET_X25, TSAP_PROTOCOL_X25,
				NSAP_CLASS_CONS, sid, AF_CCITT);
		break;
	}
	case AF_NATM:
	case AF_IPX:
	case AF_SNA:
	default:
		error = tsap_candisconnect(tsap, NULL, NSAP_TYPE_UNKNOWN,
				NSAP_SUBNET_UNKNOWN, TSAP_PROTOCOL_UNKNOWN,
				NSAP_CLASS_UNKNOWN, sid, AF_UNSPEC);
		break;
	}
	return (error);
}
