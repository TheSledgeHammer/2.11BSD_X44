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
 * - Finish re-work on how tsap's retrieve information from
 * sap_select structure (i.e. sap_table below).
 * 		- setting up tsap_id
 * 		- retrieval of types, protocols, classes and subnets.
 * - Sap select now works of each network stack. As define by the sap_table.
 * - tsap_acknowledge: won't work properly with new sap select
 */

#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iso_nsap.h"

uint32_t tsap_valid_ids[8];
struct nsapisotable tsapisotable; /* tsap iso table */

#define SAP_TABLE_MAX 9

struct sap_select sap_table[] = {
		/* 0 - AF_INET */
		{
				.ss_sid = 0,
				.ss_af = AF_INET,
				.ss_type =  { SAP_TYPE_SIN4 },
				.ss_subnet = { SAP_SUBNET_IPV4, SAP_SUBNET_IPV6 },
				.ss_protocol = { SAP_PROTOCOL_TCP,  SAP_PROTOCOL_UDP },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 1 - AF_INET6 */
		{
				.ss_sid = 1,
				.ss_af = AF_INET6,
				.ss_type =  { SAP_TYPE_SIN6 },
				.ss_subnet = { SAP_SUBNET_IPV4, SAP_SUBNET_IPV6 },
				.ss_protocol = { SAP_PROTOCOL_TCP,  SAP_PROTOCOL_UDP },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 2 - AF_NS */
		{
				.ss_sid = 2,
				.ss_af = AF_NS,
				.ss_type =  { SAP_TYPE_SNS },
				.ss_subnet = { SAP_SUBNET_IDP },
				.ss_protocol = { SAP_PROTOCOL_SPP },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 3 - AF_ISO */
		{
				.ss_sid = 3,
				.ss_af = AF_ISO,
				.ss_type =  { SAP_TYPE_SISO },
				.ss_subnet = { SAP_SUBNET_CONS, SAP_SUBNET_CLNS, SAP_SUBNET_CLNP, SAP_SUBNET_ISIS, SAP_SUBNET_ESIS },
				.ss_protocol = { SAP_PROTOCOL_TP0, SAP_PROTOCOL_TP1, SAP_PROTOCOL_TP2, SAP_PROTOCOL_TP3, SAP_PROTOCOL_TP4 },
				.ss_class = { SAP_CLASS_CONS, SAP_CLASS_CLNS },
		},
		/* 4 - AF_CCITT */
		{
				.ss_sid = 4,
				.ss_af = AF_CCITT,
				.ss_type =  { SAP_TYPE_SX25 },
				.ss_subnet = { SAP_SUBNET_X25 },
				.ss_protocol = { SAP_PROTOCOL_X25 },
				.ss_class = { SAP_CLASS_CONS },
		},
		/* 5 - AF_NATM */
		{
				.ss_sid = 5,
				.ss_af = AF_NATM,
				.ss_type =  { SAP_TYPE_SATM },
				.ss_subnet = { SAP_SUBNET_ATM },
				.ss_protocol = { SAP_PROTOCOL_ATM },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 6 - AF_IPX */
		{
				.ss_sid = 6,
				.ss_af = AF_IPX,
				.ss_type =  { SAP_TYPE_SIPX },
				.ss_subnet = { SAP_SUBNET_IPX },
				.ss_protocol = { SAP_PROTOCOL_SPX },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 7 - AF_SNA */
		{
				.ss_sid = 7,
				.ss_af = AF_SNA,
				.ss_type =  { SAP_TYPE_SSNA },
				.ss_subnet = { SAP_SUBNET_SNA },
				.ss_protocol = { SAP_PROTOCOL_SNA },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 8 - AF_UNSPEC */
		{
				.ss_sid = 8,
				.ss_af = AF_UNSPEC,
				.ss_type =  { SAP_TYPE_UNKNOWN },
				.ss_subnet = { SAP_SUBNET_UNKNOWN },
				.ss_protocol = { SAP_PROTOCOL_UNKNOWN },
				.ss_class = { SAP_CLASS_UNKNOWN },
		},
};

void tsap_set_select_id(struct sap_select *select, int sid);
static void tsap_setup_select(struct sap_select *, long *, long *, long *, int *);
static void tsap_setup_select_id(struct sap_select *);
static void tsap_attach_af(struct tsap_iso *, int);
static void tsap_detach_af(struct tsap_iso *, int);


/* TSAP's */
void
tsap_init(struct tsap_iso *tsap)
{
	nsap_init(&tsapisotable);
	tsap_setup_select_id(&tsap->tsi_select);
}

void
tsap_attach(struct tsap_iso *tsap, int af)
{
	int sid;

	if (tsap != NULL) {
		tsap_attach_af(tsap, af);
		sid = tsap_select_af_to_sid(af);
		tsap_select_init(&tsap->tsi_select, sid, af);
	}
}

void
tsap_detach(struct tsap_iso *tsap, int af)
{
	if (tsap != NULL) {
		tsap_detach_af(tsap, af);
	}
}

static void
tsap_select_init(struct sap_select *select, int sid, int af)
{
	bzero(select, sizeof(*select));
	select = tsap_select_lookup(sid, af);
	if (select != NULL) {
		tsap_setup_select(select, select->ss_type, select->ss_subnet, select->ss_protocol, select->ss_class);
		tsap_set_select_id(select, sid);
	}
}

void
tsap_set_select_id(struct sap_select *select, int sid)
{
	if (sid != 8) {
		select->ss_selector[sid] = tsap_valid_ids[sid];
	}
}

int
tsap_validate(struct tsap_iso *tsap, struct nsap_iso *nsap, int sid, int af)
{
	if (tsap != NULL) {
		if ((sid != 8) && (af != AF_UNSPEC)) {
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
	long stype, ssubnet, sprotocol;
	int sclass, error;

	stype = tsap_select_lookup_type(sid, af, snsap->snsap_type);
	ssubnet = tsap_select_lookup_subnet(sid, af, snsap->snsap_subnet);
	sprotocol = tsap_select_lookup_protocol(sid, af, protocol);
	sclass = tsap_select_lookup_class(sid, af, nsapa->nsapa_service_class);

	/* check tsap id */
	error = tsap_id(stype, ssubnet, sprotocol, sclass);
	if (error == 0) {
		return (1);
	}
	nsap = nsap_lookup(&tsapisotable, snsap, nsapa, stype, ssubnet, sclass);
	if (nsap == NULL) {
		error = 1;
	} else {
		error = tsap_validate(tsap, nsap, sid, af);
	}
	return (error);
}

struct sap_select *
tsap_select_lookup(int sid, int af)
{
	struct sap_select *select;

	select = &sap_table[sid];
	if (select != NULL) {
		if ((select->ss_sid == sid) && (select->ss_af == af)) {
			return (select);
		}
	}
	return (NULL);
}

long
tsap_select_lookup_type(int sid, int af, long type)
{
	struct sap_select *select;
	long sap_type;

	select = tsap_select_lookup(sid, af);
	if (select != NULL) {
		sap_type = sap_item_lookup(type, select->ss_type, SAP_TYPE_MAX);
		if (sap_type != sap_type_select(type)) {
			sap_type = SAP_TYPE_UNKNOWN;
		}
	}
	return (sap_type);
}

long
tsap_select_lookup_subnet(int sid, int af, long subnet)
{
	struct sap_select *select;
	long sap_subnet;

	select = tsap_select_lookup(sid, af);
	if (select != NULL) {
		sap_subnet = sap_item_lookup(subnet, select->ss_subnet, SAP_SUBNET_MAX);
		if (sap_subnet != sap_subnet_select(subnet)) {
			sap_subnet = SAP_SUBNET_UNKNOWN;
		}
	}
	return (sap_subnet);
}

long
tsap_select_lookup_protocol(int sid, int af, long protocol)
{
	struct sap_select *select;
	long sap_protocol;

	select = tsap_select_lookup(sid, af);
	if (select != NULL) {
		sap_protocol = sap_item_lookup(protocol, select->ss_protocol,
				SAP_PROTOCOL_MAX);
		if (sap_protocol != sap_protocol_select(protocol)) {
			sap_protocol = SAP_PROTOCOL_UNKNOWN;
		}
	}
	return (sap_protocol);
}

int
tsap_select_lookup_class(int sid, int af, int clazz)
{
	struct sap_select *select;
	long sap_class;

	select = tsap_select_lookup(sid, af);
	if (select != NULL) {
		sap_class = sap_item_lookup(clazz, select->ss_class, SAP_CLASS_MAX);
		if (sap_class != sap_class_select(clazz)) {
			sap_class = SAP_CLASS_UNKNOWN;
		}
	}
	return (sap_class);
}

/*
 * get sap select af from sid
 */
int
tsap_select_sid_to_af(int sid)
{
    int sap_af = sap_table[sid].ss_af;
    return (sap_af);
}

/*
 * get sap select sid from af
 */
int
tsap_select_af_to_sid(int af)
{
    int i, sap_sid;

    for (i = 0; i < SAP_TABLE_MAX; i++) {
        if (af == sap_table[i].ss_af) {
            sap_sid = i;
            break;
        }
    }
    return (sap_sid);
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

static void
tsap_setup_select(struct sap_select *select, long *type, long *subnet, long *protocol, int *class)
{
    bcopy(type, select->ss_type, sizeof(*select->ss_type));
    bcopy(subnet, select->ss_subnet, sizeof(*select->ss_subnet));
    bcopy(protocol, select->ss_protocol, sizeof(*select->ss_protocol));
    bcopy(class, select->ss_class, sizeof(*select->ss_class));
}

static void
tsap_setup_select_id(struct sap_select *select)
{
	int i;

	for (i = 0; i < (SAP_TABLE_MAX - 1); i++) {
		select = &sap_table[i];
		if (select != NULL) {
			tsap_valid_ids[i] = tsap_id(*select->ss_type, *select->ss_subnet, *select->ss_protocol, *select->ss_class);
		}
	}
}
