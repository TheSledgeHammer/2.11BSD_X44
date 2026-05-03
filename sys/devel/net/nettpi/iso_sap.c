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

#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iso_nsap.h"

static uint32_t sap_hashids[SAP_TABLE_MAX];

struct sap_select sap_table[] = {
		/* 0 - AF_UNSPEC */
		{
				.ss_sid = 0,
				.ss_af = AF_UNSPEC,
				.ss_type =  { SAP_TYPE_UNKNOWN },
				.ss_subnet = { SAP_SUBNET_UNKNOWN },
				.ss_protocol = { SAP_PROTOCOL_UNKNOWN },
				.ss_class = { SAP_CLASS_UNKNOWN },
		},
		/* 1 - AF_INET */
		{
				.ss_sid = 1,
				.ss_af = AF_INET,
				.ss_type =  { SAP_TYPE_SIN4 },
				.ss_subnet = { SAP_SUBNET_IPV4, SAP_SUBNET_IPV6 },
				.ss_protocol = { SAP_PROTOCOL_TCP,  SAP_PROTOCOL_UDP },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 2 - AF_INET6 */
		{
				.ss_sid = 2,
				.ss_af = AF_INET6,
				.ss_type =  { SAP_TYPE_SIN6 },
				.ss_subnet = { SAP_SUBNET_IPV4, SAP_SUBNET_IPV6 },
				.ss_protocol = { SAP_PROTOCOL_TCP,  SAP_PROTOCOL_UDP },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 3 - AF_NS */
		{
				.ss_sid = 3,
				.ss_af = AF_NS,
				.ss_type =  { SAP_TYPE_SNS },
				.ss_subnet = { SAP_SUBNET_IDP },
				.ss_protocol = { SAP_PROTOCOL_SPP },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 4 - AF_ISO */
		{
				.ss_sid = 4,
				.ss_af = AF_ISO,
				.ss_type =  { SAP_TYPE_SISO },
				.ss_subnet = { SAP_SUBNET_CONS, SAP_SUBNET_CLNS, SAP_SUBNET_CLNP, SAP_SUBNET_ISIS, SAP_SUBNET_ESIS },
				.ss_protocol = { SAP_PROTOCOL_TP0, SAP_PROTOCOL_TP1, SAP_PROTOCOL_TP2, SAP_PROTOCOL_TP3, SAP_PROTOCOL_TP4 },
				.ss_class = { SAP_CLASS_CONS, SAP_CLASS_CLNS },
		},
		/* 5 - AF_CCITT */
		{
				.ss_sid = 5,
				.ss_af = AF_CCITT,
				.ss_type =  { SAP_TYPE_SX25 },
				.ss_subnet = { SAP_SUBNET_X25 },
				.ss_protocol = { SAP_PROTOCOL_X25 },
				.ss_class = { SAP_CLASS_CONS },
		},
		/* 6 - AF_NATM */
		{
				.ss_sid = 6,
				.ss_af = AF_NATM,
				.ss_type =  { SAP_TYPE_SATM },
				.ss_subnet = { SAP_SUBNET_ATM },
				.ss_protocol = { SAP_PROTOCOL_ATM },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 7 - AF_IPX */
		{
				.ss_sid = 7,
				.ss_af = AF_IPX,
				.ss_type =  { SAP_TYPE_SIPX },
				.ss_subnet = { SAP_SUBNET_IPX },
				.ss_protocol = { SAP_PROTOCOL_SPX },
				.ss_class = { SAP_CLASS_CLNS },
		},
		/* 8 - AF_SNA */
		{
				.ss_sid = 8,
				.ss_af = AF_SNA,
				.ss_type =  { SAP_TYPE_SSNA },
				.ss_subnet = { SAP_SUBNET_SNA },
				.ss_protocol = { SAP_PROTOCOL_SNA },
				.ss_class = { SAP_CLASS_CLNS },
		},
};

int
sap_class_select(int clazz)
{
	int select = -1;

	switch (clazz) {
	case SAP_CLASS_CONS:
		select = SAP_CLASS_CONS;
		break;
	case SAP_CLASS_CLNS:
		select = SAP_CLASS_CLNS;
		break;
	case SAP_CLASS_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_CLASS_UNKNOWN;
		break;
	}
	return (select);
}

long
sap_type_select(long type)
{
	long select = -1;

	switch (type) {
	case SAP_TYPE_SIN4:
		select = SAP_TYPE_SIN4;
		break;
	case SAP_TYPE_SIN6:
		select = SAP_TYPE_SIN6;
		break;
	case SAP_TYPE_SNS:
		select = SAP_TYPE_SNS;
		break;
	case SAP_TYPE_SISO:
		select = SAP_TYPE_SISO;
		break;
	case SAP_TYPE_SX25:
		select = SAP_TYPE_SX25;
		break;
	case SAP_TYPE_SATM:
		select = SAP_TYPE_SATM;
		break;
	case SAP_TYPE_SIPX:
		select = SAP_TYPE_SIPX;
		break;
	case SAP_TYPE_SSNA:
		select = SAP_TYPE_SSNA;
		break;
	case SAP_TYPE_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_TYPE_UNKNOWN;
		break;
	}
	return (select);
}

long
sap_subnet_select(long subnet)
{
	long select = -1;

	switch (subnet) {
	case SAP_SUBNET_IPV4:
		select = SAP_SUBNET_IPV4;
		break;
	case SAP_SUBNET_IPV6:
		select = SAP_SUBNET_IPV6;
		break;
	case SAP_SUBNET_IDP:
		select = SAP_SUBNET_IDP;
		break;
	case SAP_SUBNET_CONS:
		select = SAP_SUBNET_CONS;
		break;
	case SAP_SUBNET_CLNS:
		select = SAP_SUBNET_CLNS;
		break;
	case SAP_SUBNET_CLNP:
		select = SAP_SUBNET_CLNP;
		break;
	case SAP_SUBNET_ISIS:
		select = SAP_SUBNET_ISIS;
		break;
	case SAP_SUBNET_ESIS:
		select = SAP_SUBNET_ESIS;
		break;
	case SAP_SUBNET_X25:
		select = SAP_SUBNET_X25;
		break;
	case SAP_SUBNET_ATM:
		select = SAP_SUBNET_ATM;
		break;
	case SAP_SUBNET_IPX:
		select = SAP_SUBNET_IPX;
		break;
	case SAP_SUBNET_SNA:
		select = SAP_SUBNET_SNA;
		break;
	case SAP_SUBNET_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_SUBNET_UNKNOWN;
		break;
	}
	return (select);
}

long
sap_protocol_select(long protocol)
{
	long select = -1;

	switch (protocol) {
	case SAP_PROTOCOL_TCP:
		select = SAP_PROTOCOL_TCP;
		break;
	case SAP_PROTOCOL_UDP:
		select = SAP_PROTOCOL_UDP;
		break;
	case SAP_PROTOCOL_TP0:
		select = SAP_PROTOCOL_TP0;
		break;
	case SAP_PROTOCOL_TP1:
		select = SAP_PROTOCOL_TP1;
		break;
	case SAP_PROTOCOL_TP2:
		select = SAP_PROTOCOL_TP2;
		break;
	case SAP_PROTOCOL_TP3:
		select = SAP_PROTOCOL_TP3;
		break;
	case SAP_PROTOCOL_TP4:
		select = SAP_PROTOCOL_TP4;
		break;
	case SAP_PROTOCOL_SPP:
		select = SAP_PROTOCOL_SPP;
		break;
	case SAP_PROTOCOL_X25:
		select = SAP_PROTOCOL_X25;
		break;
	case SAP_PROTOCOL_ATM:
		select = SAP_PROTOCOL_ATM;
		break;
	case SAP_PROTOCOL_SPX:
		select = SAP_PROTOCOL_SPX;
		break;
	case SAP_PROTOCOL_SNA:
		select = SAP_PROTOCOL_SNA;
		break;
	case SAP_PROTOCOL_UNKNOWN:
		/* FALLTHROUGH */
	default:
		select = SAP_PROTOCOL_UNKNOWN;
		break;
	}
	return (select);
}

/*
 * sap item lookup:
 * search sap_array for sap_item
 * returns item if found or -1 if not found.
 */
long
sap_item_lookup(long sap_item, long *sap_array, int nelems)
{
	long item = -1;
	int i;

	for (i = 0; i < nelems; i++) {
		item = sap_array[i];
		if (item == sap_item) {
			break;
		}
	}
	return (item);
}

static void
sap_select_setup(struct sap_select *select, long *type, long *subnet, long *protocol, int *class)
{
    bcopy(type, select->ss_type, sizeof(*select->ss_type));
    bcopy(subnet, select->ss_subnet, sizeof(*select->ss_subnet));
    bcopy(protocol, select->ss_protocol, sizeof(*select->ss_protocol));
    bcopy(class, select->ss_class, sizeof(*select->ss_class));
}

static void
sap_select_selector_setup(struct sap_select *select)
{
	int i;
	uint32_t sap_hashtable[SAP_TABLE_MAX];

	for (i = 1; i < SAP_TABLE_MAX; i++) {
		select = &sap_table[i];
		if (select != NULL) {
			sap_hashids[i] = sap_hash(*select->ss_type, *select->ss_subnet, *select->ss_protocol, *select->ss_class);
			tsap_valid_ids[i] = sap_hashids[i];
		}
	}
}

void
sap_select_init(struct sap_select *select, int sid, int af)
{
	bzero(select, sizeof(*select));
	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_select_setup(select, select->ss_type, select->ss_subnet, select->ss_protocol, select->ss_class);
		sap_select_selector_setup(select);
		select->ss_selector[sid] = sap_hashids[sid];
	}
}

/*
 * nsap_addr functions
 */
/*
 * nsap_addr_compare:
 * - compares sap_service
 * returns -1 if a, 1 if b and 0 if equal
 */
int
nsap_addr_compare(struct nsap_addr *a, struct nsap_addr *b)
{
	if (a != b) {
		return (sap_service_compare(&a->nsapa_service, &b->nsapa_service));
	}
	return (0);
}

/*
 * sockaddr_nsap functions
 */
static int
sockaddr_nsap_compare_type(struct sockaddr_nsap *a, struct sockaddr_nsap *b)
{
	if (a->snsap_type > b->snsap_type) {
		return (-1);
	} else if (a->snsap_type < b->snsap_type) {
		return (1);
	} else {
		return (0);
	}
}

static int
sockaddr_nsap_compare_subnet(struct sockaddr_nsap *a, struct sockaddr_nsap *b)
{
	if (a->snsap_subnet > b->snsap_subnet) {
		return (-1);
	} else if (a->snsap_subnet < b->snsap_subnet) {
		return (1);
	} else {
		return (0);
	}
}

/*
 * sockaddr_nsap_compare:
 * - compares type, subnet, class and nsap_addr.
 * returns -1 if a, 1 if b and 0 if equal
 */
int
sockaddr_nsap_compare(struct sockaddr_nsap *a, struct sockaddr_nsap *b)
{
	struct nsap_addr *aa, *bb;
	int error;

	aa = &a->snsap_addr;
	bb = &b->snsap_addr;

	if (a != b) {
		error = sockaddr_nsap_compare_type(a, b);
		if (error != 0) {
			return (error);
		}
		error = sockaddr_nsap_compare_subnet(a, b);
		if (error != 0) {
			return (error);
		}
		error = nsap_addr_compare(aa, bb);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}

/*
 * sap_service functions
 */
static int
sap_service_compare_addr(struct sap_service *a, struct sap_service *b)
{
	if (bcmp(a->ns_addr, b->ns_addr, sizeof(a->ns_addr)) != 0) {
		return (1);
	}
	return (0);
}

static int
sap_service_compare_addrlen(struct sap_service *a, struct sap_service *b)
{
	if (a->ns_addrlen > b->ns_addrlen) {
		return (-1);
	} else if (a->ns_addrlen < b->ns_addrlen) {
		return (1);
	} else {
		return (0);
	}
}

static int
sap_service_compare_class(struct sap_service *a, struct sap_service *b)
{
	if (a->ns_class > b->ns_class) {
		return (-1);
	} else if (a->ns_class < b->ns_class) {
		return (1);
	} else {
		return (0);
	}
}

/*
 * sap_service_check_class:
 * against the specified type and subnet.
 * returns -1 if connection mode, 1 if connectionless mode or
 * 0 if class matches.
 */
int
sap_service_check_class(long type, long subnet, int class)
{
	switch (class) {
	case NSAP_CLASS_CONS:
		if ((type != NSAP_TYPE_SISO) && (subnet != NSAP_SUBNET_CONS)) {
			return (-1);
		}
		if ((type != NSAP_TYPE_SX25) && (subnet != NSAP_SUBNET_X25)) {
			return (-1);
		}
		break;
	case NSAP_CLASS_CLNS:
		if ((type == NSAP_TYPE_SISO) && (subnet == NSAP_SUBNET_CONS)) {
			return (1);
		}
		if ((type == NSAP_TYPE_SX25) && (subnet == NSAP_SUBNET_X25)) {
			return (1);
		}
		break;
	}
	return (0);
}

/*
 * sap_service_compare:
 * - compares addr, addrlen and class.
 * returns -1 if a, 1 if b and 0 if equal
 */
int
sap_service_compare(struct sap_service *a, struct sap_service *b)
{
	int error;

	if (a != b) {
		error = sap_service_compare_addr(a, b);
		if (error != 0) {
			return (error);
		}
		error = sap_service_compare_addrlen(a, b);
		if (error != 0) {
			return (error);
		}
		error = sap_service_compare_class(a, b);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}

/*
 * sap_select functions
 */
struct sap_select *
sap_select_lookup(int sid, int af)
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
sap_select_lookup_type(int sid, int af, long type)
{
	struct sap_select *select;
	long sap_type;

	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_type = sap_item_lookup(type, select->ss_type, SAP_TYPE_MAX);
		if (sap_type != sap_type_select(type)) {
			sap_type = SAP_TYPE_UNKNOWN;
		}
	}
	return (sap_type);
}

long
sap_select_lookup_subnet(int sid, int af, long subnet)
{
	struct sap_select *select;
	long sap_subnet;

	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_subnet = sap_item_lookup(subnet, select->ss_subnet, SAP_SUBNET_MAX);
		if (sap_subnet != sap_subnet_select(subnet)) {
			sap_subnet = SAP_SUBNET_UNKNOWN;
		}
	}
	return (sap_subnet);
}

long
sap_select_lookup_protocol(int sid, int af, long protocol)
{
	struct sap_select *select;
	long sap_protocol;

	select = sap_select_lookup(sid, af);
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
sap_select_lookup_class(int sid, int af, int clazz)
{
	struct sap_select *select;
	long sap_class;

	select = sap_select_lookup(sid, af);
	if (select != NULL) {
		sap_class = sap_item_lookup(clazz, select->ss_class, SAP_CLASS_MAX);
		if (sap_class != sap_class_select(clazz)) {
			sap_class = SAP_CLASS_UNKNOWN;
		}
	}
	return (sap_class);
}

uint32_t
sap_select_lookup_selector(struct sap_select *select, int sid)
{
	if (select != NULL) {
		if (select->ss_selector[sid] == 0) {
			select->ss_selector[sid] = sap_hashids[sid];
		}
		return (select->ss_selector[sid]);
	}
	return (0);
}

/*
 * get sap select af from sid
 */
int
sap_select_sid_to_af(int sid)
{
    int sap_af = sap_table[sid].ss_af;
    return (sap_af);
}

/*
 * get sap select sid from af
 */
int
sap_select_af_to_sid(int af)
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

static int
sap_select_compare_sid(struct sap_select *a, struct sap_select *b)
{
	if (a->ss_sid > b->ss_sid) {
		return (-1);
	} else if (a->ss_sid < b->ss_sid) {
		return (1);
	} else {
		return (0);
	}
}

static int
sap_select_compare_af(struct sap_select *a, struct sap_select *b)
{
	if (a->ss_af > b->ss_af) {
		return (-1);
	} else if (a->ss_af < b->ss_af) {
		return (1);
	} else {
		return (0);
	}
}

static int
sap_select_compare_selector(struct sap_select *a, struct sap_select *b)
{
	if (a->ss_selector > b->ss_selector) {
		return (-1);
	} else if (a->ss_selector < b->ss_selector) {
		return (1);
	} else {
		return (0);
	}
}

/*
 * sap_select_compare:
 * - compares selector, sid and af.
 * returns -1 if a, 1 if b and 0 if equal
 */
int
sap_select_compare(struct sap_select *a, struct sap_select *b)
{
	int error;

	if (a != b) {
		error = sap_select_compare_sid(a, b);
		if (error != 0) {
			return (error);
		}
		error = sap_select_compare_af(a, b);
		if (error != 0) {
			return (error);
		}
		error = sap_select_compare_selector(a, b);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}
