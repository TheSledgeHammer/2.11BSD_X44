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
struct nsapisotable tsapisotable; /* tsap iso table */

static void tsap_attach_af(struct tsap_iso *, int);
static void tsap_detach_af(struct tsap_iso *, int);

/* TSAP's */
void
tsap_init(struct tsap_iso *tsap)
{
	nsap_init(&tsapisotable);
}

void
tsap_attach(struct tsap_iso *tsap, int af)
{
	int sid;

	if (tsap != NULL) {
		tsap_attach_af(tsap, af);
		sid = sap_select_af_to_sid(af);
		sap_select_init(&tsap->tsi_select, sid, af);
	}
}

void
tsap_detach(struct tsap_iso *tsap, int af)
{
	if (tsap != NULL) {
		tsap_detach_af(tsap, af);
	}
}

int
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

int
tsap_acknowledge(struct tsap_iso *tsap, struct sockaddr_nsap *snsap, struct nsap_addr *nsapa, long protocol, int sid, int af)
{
	struct nsap_iso *nsap;
	long sap_type, sap_subnet, sap_protocol;
	int sap_class, error;

	sap_type = sap_select_lookup_type(sid, af, snsap->snsap_type);
	sap_subnet = sap_select_lookup_subnet(sid, af, snsap->snsap_subnet);
	sap_protocol = sap_select_lookup_protocol(sid, af, protocol);
	sap_class = sap_select_lookup_class(sid, af, nsapa->nsapa_service_class);

	/* check tsap id */
	error = tsap_id(sap_type, sap_subnet, sap_protocol, sap_class);
	if (error == 0) {
		return (1);
	}
	nsap = nsap_lookup(&tsapisotable, snsap, nsapa, sap_type, sap_subnet, sap_class);
	if (nsap == NULL) {
		error = 1;
	} else {
		error = tsap_validate(tsap, nsap, sid, af);
	}
	return (error);
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

/*
 * tsap_connect:
 * - Makes the same assumptions as nsap_connect.
 */
int
tsap_connect(struct mbuf *nam, struct sockaddr_nsap *snsap, long subnet, int class, int af)
{
	int error;

	if (snsap == NULL) {
		return (EINVAL);
	}
	switch (class) {
	case NSAP_CLASS_CONS: {
		switch (af) {
		case AF_INET:
			printf(
					"tsap_connect: Cannot connect IPV4 while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_INET6:
			printf(
					"tsap_connect: Cannot connect IPV6 while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_NS:
			printf(
					"tsap_connect: Cannot connect IDP while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_ISO:
			if (subnet != NSAP_SUBNET_CONS) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_CONS, NSAP_CLASS_CONS,
			AF_ISO);
			break;
		case AF_CCITT:
			if (subnet != NSAP_SUBNET_X25) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_X25, NSAP_CLASS_CONS,
			AF_CCITT);
			break;
		case AF_NATM:
			printf(
					"tsap_connect: Cannot connect ATM while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_IPX:
			printf(
					"tsap_connect: Cannot connect IPX while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_SNA:
			printf(
					"tsap_connect: Cannot connect SNA while in a connection oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		default:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_UNKNOWN,
			NSAP_CLASS_CONS, AF_UNSPEC);
			break;
		}
		break;
	}
	case NSAP_CLASS_CLNS: {
		switch (af) {
		case AF_INET:
			if ((subnet != NSAP_SUBNET_IPV4) || (subnet != NSAP_SUBNET_IPV6)) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, subnet, NSAP_CLASS_CLNS, AF_INET);
			break;
		case AF_INET6:
			if ((subnet != NSAP_SUBNET_IPV4) || (subnet != NSAP_SUBNET_IPV6)) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, subnet, NSAP_CLASS_CLNS, AF_INET6);
			break;
		case AF_ISO:
			if ((subnet != NSAP_SUBNET_CLNS) || (subnet != NSAP_SUBNET_CLNP)
					|| (subnet != NSAP_SUBNET_ISIS)
					|| (subnet != NSAP_SUBNET_ESIS)) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, subnet, NSAP_CLASS_CLNS, AF_ISO);
			break;
		case AF_NS:
			if (subnet != NSAP_SUBNET_IDP) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_IDP, NSAP_CLASS_CLNS,
			AF_NS);
			break;
		case AF_CCITT:
			printf(
					"tsap_connect: Cannot connect X.25 while in a connection-less oriented mode\n");
			error = EAFNOSUPPORT;
			break;
		case AF_NATM:
			if (subnet != NSAP_SUBNET_ATM) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_ATM, NSAP_CLASS_CLNS,
			AF_NATM);
			break;
		case AF_IPX:
			if (subnet != NSAP_SUBNET_IPX) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_IPX, NSAP_CLASS_CLNS,
			AF_IPX);
			break;
		case AF_SNA:
			if (subnet != NSAP_SUBNET_SNA) {
				error = EAFNOSUPPORT;
				break;
			}
			error = nsap_connect(nam, snsap, NSAP_SUBNET_SNA, NSAP_CLASS_CLNS,
			AF_SNA);
			break;
		default:
			error = nsap_connect(nam, snsap, NSAP_SUBNET_UNKNOWN,
			NSAP_CLASS_CLNS, AF_UNSPEC);
			break;
		}
		break;
	}
	default:
		error = nsap_connect(nam, snsap, NSAP_SUBNET_UNKNOWN, NSAP_CLASS_UNKNOWN, AF_UNSPEC);
		break;
	}
	return (error);
}

void
tsap_disconnect(struct sockaddr_nsap *snsap, long subnet, int class, int af)
{
	if (snsap == NULL) {
		printf("tsap_disconnect: tsap is empty or has already been disconnected\n");
		return;
	}
	switch (class) {
	case NSAP_CLASS_CONS: {
		switch (af) {
		case AF_INET:
			printf(
					"tsap_disconnect: Cannot disconnect IPV4 while in a connection oriented mode\n");
			break;
		case AF_INET6:
			printf(
					"tsap_disconnect: Cannot disconnect IPV6 while in a connection oriented mode\n");
			break;
		case AF_NS:
			printf(
					"tsap_disconnect: Cannot disconnect IDP while in a connection oriented mode\n");
			break;
		case AF_ISO:
			if (subnet != NSAP_SUBNET_CONS) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_CONS, AF_ISO);
			break;
		case AF_CCITT:
			if (subnet != NSAP_SUBNET_X25) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_X25, AF_CCITT);
			break;
		case AF_NATM:
			printf(
					"tsap_disconnect: Cannot disconnect ATM while in a connection oriented mode\n");
			break;
		case AF_IPX:
			printf(
					"tsap_disconnect: Cannot disconnect IPX while in a connection oriented mode\n");
			break;
		case AF_SNA:
			printf(
					"tsap_disconnect: Cannot disconnect SNA while in a connection oriented mode\n");
			break;
		default:
			nsap_disconnect(snsap, NSAP_SUBNET_UNKNOWN, AF_UNSPEC);
			break;
		}
		break;
	}
	case NSAP_CLASS_CLNS: {
		switch (af) {
		case AF_INET:
			if ((subnet != NSAP_SUBNET_IPV4) || (subnet != NSAP_SUBNET_IPV6)) {
				break;
			}
			nsap_disconnect(snsap, subnet, AF_INET);
			break;
		case AF_INET6:
			if ((subnet != NSAP_SUBNET_IPV4) || (subnet != NSAP_SUBNET_IPV6)) {
				break;
			}
			nsap_disconnect(snsap, subnet, AF_INET6);
			break;
		case AF_ISO:
			if ((subnet != NSAP_SUBNET_CLNS) || (subnet != NSAP_SUBNET_CLNP)
					|| (subnet != NSAP_SUBNET_ISIS)
					|| (subnet != NSAP_SUBNET_ESIS)) {
				break;
			}
			nsap_disconnect(snsap, subnet, AF_ISO);
			break;
		case AF_NS:
			if (subnet != NSAP_SUBNET_IDP) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_IDP, AF_NS);
			break;
		case AF_CCITT:
			printf(
					"tsap_disconnect: Cannot disconnect X.25 while in a connection-less oriented mode\n");
			break;
		case AF_NATM:
			if (subnet != NSAP_SUBNET_ATM) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_ATM, AF_NATM);
			break;
		case AF_IPX:
			if (subnet != NSAP_SUBNET_IPX) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_IPX, AF_IPX);
			break;
		case AF_SNA:
			if (subnet != NSAP_SUBNET_SNA) {
				break;
			}
			nsap_disconnect(snsap, NSAP_SUBNET_SNA, AF_SNA);
			break;
		default:
			nsap_disconnect(snsap, NSAP_SUBNET_UNKNOWN, AF_UNSPEC);
			break;
		}
		break;
	}
	default:
		nsap_disconnect(snsap, NSAP_SUBNET_UNKNOWN, AF_UNSPEC);
		break;
	}
}

static void
tsap_attach_af(struct tsap_iso *tsap, int af)
{
	switch (af) {
	case AF_INET:
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIN4,
				NSAP_SUBNET_IPV4);
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIN4,
				NSAP_SUBNET_IPV6);
		break;
	case AF_INET6:
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIN6,
				NSAP_SUBNET_IPV4);
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIN6,
				NSAP_SUBNET_IPV6);
		break;
	case AF_NS:
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SNS,
				NSAP_SUBNET_IDP);
		break;
	case AF_ISO:
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_CONS);
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_CLNS);
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_CLNP);
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_ISIS);
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_ESIS);
		break;
	case AF_CCITT:
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SX25,
				NSAP_SUBNET_X25);
		break;
	case AF_NATM:
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SATM,
				NSAP_SUBNET_ATM);
		break;
	case AF_IPX:
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIPX,
				NSAP_SUBNET_IPX);
		break;
	case AF_SNA:
		nsap_attach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SSNA,
				NSAP_SUBNET_SNA);
		break;
	}
}

static void
tsap_detach_af(struct tsap_iso *tsap, int af)
{
	switch (af) {
	case AF_INET:
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIN4,
				NSAP_SUBNET_IPV4);
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIN4,
				NSAP_SUBNET_IPV6);
		break;
	case AF_INET6:
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIN6,
				NSAP_SUBNET_IPV4);
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIN6,
				NSAP_SUBNET_IPV6);
		break;
	case AF_NS:
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SNS,
				NSAP_SUBNET_IDP);
		break;
	case AF_ISO:
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_CONS);
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_CLNS);
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_CLNP);
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_ISIS);
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SISO,
				NSAP_SUBNET_ESIS);
		break;
	case AF_CCITT:
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SX25,
				NSAP_SUBNET_X25);
		break;
	case AF_NATM:
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SATM,
				NSAP_SUBNET_ATM);
		break;
	case AF_IPX:
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SIPX,
				NSAP_SUBNET_IPX);
		break;
	case AF_SNA:
		nsap_detach(&tsapisotable, &tsap->tsi_nsaps, NSAP_TYPE_SSNA,
				NSAP_SUBNET_SNA);
		break;
	}
}
