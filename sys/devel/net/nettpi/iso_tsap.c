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
 */

#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iso_nsap.h"

void tsap_select_init(struct sap_select *, long *, long *, long *, int *);
void tsap_selector_init(struct sap_select *, long, long, long, int);

static void tsap_attach_af(struct tsap_iso *, int);
static void tsap_detach_af(struct tsap_iso *, int);

struct nsapisotable tsapisotable; /* tsap iso table */

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
};


/* TSAP's */
void
tsap_init(struct tsap_iso *tsap)
{
	nsap_init(&tsapisotable);
}

void
tsap_attach(struct tsap_iso *tsap, int af)
{
	if (tsap != NULL) {
		tsap_attach_af(tsap, af);
	}
}

void
tsap_detach(struct tsap_iso *tsap, long protocol, int af)
{
	if (tsap != NULL) {
		tsap_detach_af(&tsapisotable, af);
	}
	tsap_sap_protocol(tsap, protocol);
}

void
sap_init(struct sap_select *select, int sid)
{
	bzero(select, sizeof(*select));
	select = &sap_table[sid];

	long type, subnet, protocol;
	int clazz;


	long sap_type = select->ss_type[type];
	long sap_subnet = select->ss_subnet[subnet];
	long sap_protocol = select->ss_protocol[protocol];
	int sap_class = select->ss_class[clazz];

	tsap_selector_init(select, sap_type, sap_subnet, sap_protocol, sap_class);
}

void
tsap_selector_id(struct tsap_iso *tsap, int af)
{
	int sid;

	sid = sap_get_sid_from_af(af);
	if (sid == -1) {
		return;
	}
	if ((sap_table[sid].ss_sid == sid) && (sap_table[sid].ss_af == af)) {

		tsap_select_init(&sap_table[sid], sap_table[sid].ss_type, sap_table[sid].ss_subnet, sap_table[sid].ss_protocol, sap_table[sid].ss_class);
	}
}

/*
 * tsap_acknowledge:
 * looks up nsap information from what tsap has provided.
 * returns 0 on success.
 */
int
tsap_acknowledge(struct tsap_iso *tsap, struct sockaddr_nsap *snsap, struct nsap_addr *nsapa, long protocol)
{
	struct nsap_iso *nsap;
	uint32_t id;

	if (tsap != NULL) {
		nsap = nsap_lookup(&tsapisotable, snsap, nsapa, tsap->tsi_type, tsap->tsi_subnet, tsap->tsi_class);
		if (nsap != NULL) {
			if (tsap->tsi_protocol == protocol) {
				id = tsap_id(tsap->tsi_type, tsap->tsi_subnet, tsap->tsi_protocol, tsap->tsi_class);
			} else {
				id = tsap_id(tsap->tsi_type, tsap->tsi_subnet, protocol, tsap->tsi_class);
			}
			if (tsap->tsi_selector[0] == id) {
				if (tsap->tsi_nsaps != *nsap) {
					return (1);
				}
				return (0);
			}
		}
	}
	return (1);
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

static int tsap_select_initd = 1; /* tsap select init switch */

void
tsap_select_init(struct sap_select *select, long *type, long *subnet, long *protocol, int *class)
{
    bzero(select, sizeof(*select));
    bcopy(type, select->ss_type, sizeof(*select->ss_type));
    bcopy(subnet, select->ss_subnet, sizeof(*select->ss_subnet));
    bcopy(protocol, select->ss_protocol, sizeof(*select->ss_protocol));
    bcopy(class, select->ss_class, sizeof(*select->ss_class));

    if (tsap_select_initd == 1) {
        tsap_select_initd = 0;
    }
}

void
tsap_selector_init(struct sap_select *select, long type, long subnet, long protocol, int class)
{
    if (tsap_select_initd == 1) {
        return;
    }
	select->ss_selector[0] = tsap_id(type, subnet, protocol, class);
}

static __inline int
sap_get_af_from_sid(int sid)
{
	int af;

	switch (sid) {
	case 0:
		af = AF_INET;
		break;
	case 1:
		af = AF_INET6;
		break;
	case 2:
		af = AF_NS;
		break;
	case 3:
		af = AF_ISO;
		break;
	case 4:
		af = AF_CCITT;
		break;
	case 5:
		af = AF_NATM;
		break;
	case 6:
		af = AF_IPX;
		break;
	case 7:
		af = AF_SNA;
		break;
	case -1:
		/* FALLTHROUGH */
	default:
		af = AF_UNSPEC;
		break;
	}
	return (af);
}

static __inline int
sap_get_sid_from_af(int af)
{
	int sid;

	switch (af) {
	case AF_INET:
		sid = 0;
		break;
	case AF_INET6:
		sid = 1;
		break;
	case AF_NS:
		sid = 2;
		break;
	case AF_ISO:
		sid = 3;
		break;
	case AF_CCITT:
		sid = 4;
		break;
	case AF_NATM:
		sid = 5;
		break;
	case AF_IPX:
		sid = 6;
		break;
	case AF_SNA:
		sid = 7;
		break;
	case AF_UNSPEC:
		/* FALLTHROUGH */
	default:
		sid = -1;
		break;
	}
	return (sid);
}
