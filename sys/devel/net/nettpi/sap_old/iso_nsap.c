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
 * Future Implementations:
 * - May benefit from using Radix Tree for lookups combined with a list.
 * 	- Use One node per address family.
 * 	e.g. radix_node_head root[8].
 */

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iso_nsap.h"

struct nsapisotable nsapisotable;

uint32_t nsap_valid_ids[NSAP_TYPE_MAX][NSAP_SUBNET_MAX];
static long nsap_id_cnt;

#define NSAPHASH(table, type, subnet) \
	&(table)->nsi_hashtbl[nsap_id((type), (subnet))]

static void nsap_init(struct nsapisotable *);
static int nsap_type_id_check(uint32_t, long);
static int nsap_subnet_id_check(uint32_t, long);
static int nsap_id_check(struct nsap_iso *, long, long);
static int nsap_compare_type_id(struct nsap_iso *, struct nsap_iso *);
static int nsap_compare_subnet_id(struct nsap_iso *, struct nsap_iso *);
static void nsap_service(struct sap_service *, char *, u_char, int);
static void nsap_setup_snsap(struct sockaddr_nsap *, void *, long, long);
static void nsap_setup_nsapa(struct nsap_addr *, void *, long, int);
static void nsap_insert(struct nsapisotable *, struct nsap_iso *, long, long);
static void nsap_remove(struct nsapisotable *, struct nsap_iso *, long, long);
static  struct nsap_iso *nsap_hashlookup(struct nsapisotable *, long, long);
static void nsap_insert_af(struct nsapisotable *, struct nsap_iso *, int);
static void nsap_remove_af(struct nsapisotable *, struct nsap_iso *, int);
static void nsap_setup_id_table(void);

static void
nsap_init(struct nsapisotable *table)
{
	table->nsi_hashtbl = hashinit(ISOLEN, M_ISOSAP, &table->nsi_hash);
	nsap_setup_id_table();
}

struct nsap_iso *
nsap_create(struct nsapisotable *table)
{
	struct nsap_iso *nsap;

	MALLOC(nsap, struct nsap_iso *, sizeof(*nsap), M_ISOSAP, M_WAITOK);
	if (nsap == NULL) {
		return (NULL);
	}
	bzero((caddr_t)nsap, sizeof(*nsap));
	nsap_init(table);
	return (nsap);
}

void
nsap_destroy(struct nsap_iso *nsap)
{
	if (nsap != NULL) {
		FREE(nsap, M_ISOSAP);
	}
}

void
nsap_attach(struct nsap_iso *nsap, int af)
{
	struct nsap_iso *nsiiso;
	struct nsapisohead *head;

	nsiiso = nsap_create(&nsapisotable);
	if (nsiiso != NULL) {
		nsap_insert_af(&nsapisotable, nsiiso, af);
		nsap = nsiiso;
	} else {
		nsap = NULL;
	}
}

void
nsap_detach(struct nsap_iso *nsap, int af)
{
	if (nsap != NULL) {
		if (!LIST_EMPTY(&nsapisotable)) {
			nsap_remove_af(&nsapisotable, nsap, af);
		} else {
			nsap_destroy(nsap);
		}
	}
}

static int
nsap_type_id_check(uint32_t id, long type)
{
	if (id > nsap_type_id(type)) {
		return (-1);
	} else if (id < nsap_type_id(type)) {
		return (1);
	}
	return (0);
}

static int
nsap_subnet_id_check(uint32_t id, long subnet)
{
	if (id > nsap_subnet_id(subnet)) {
		return (-1);
	} else if (id < nsap_subnet_id(subnet)) {
		return (1);
	}
	return (0);
}

static int
nsap_id_check(struct nsap_iso *nsap, long type, long subnet)
{
	int error;

	error = nsap_type_id_check(nsap->nsi_type_id, type);
	if (error != 0) {
		return (error);
	}
	error = nsap_subnet_id_check(nsap->nsi_subnet_id, subnet);
	if (error != 0) {
		return (error);
	}
	return (0);
}

static int
nsap_compare_type_id(struct nsap_iso *a, struct nsap_iso *b)
{
	if (a->nsi_type_id > b->nsi_type_id) {
		return (-1);
	} else if (a->nsi_type_id < b->nsi_type_id) {
		return (1);
	} else {
		return (0);
	}
}

static int
nsap_compare_subnet_id(struct nsap_iso *a, struct nsap_iso *b)
{
	if (a->nsi_subnet_id > b->nsi_subnet_id) {
		return (-1);
	} else if (a->nsi_subnet_id < b->nsi_subnet_id) {
		return (1);
	} else {
		return (0);
	}
}

/*
 * nsap_iso_compare:
 * - compares sockaddr_nsap, nsap_addr, type_id and subnet_id
 * returns -1 if a, 1 if b and 0 if equal
 */
int
nsap_iso_compare(struct nsap_iso *a, struct nsap_iso *b)
{
	int error;

	if (a != b) {
		error = sockaddr_nsap_compare(a->nsi_snsap, b->nsi_snsap);
		if (error != 0) {
			return (error);
		}
		error = nsap_addr_compare(a->nsi_nsapa, b->nsi_nsapa);
		if (error != 0) {
			return (error);
		}
		error = nsap_compare_type_id(a, b);
		if (error != 0) {
			return (error);
		}
		error = nsap_compare_subnet_id(a, b);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}

void
nsap_iso_attach(struct nsap_iso *nsap)
{
	nsap_attach(nsap, AF_ISO);
	nsap_attach(nsap, AF_INET);
	nsap_attach(nsap, AF_INET6);
	nsap_attach(nsap, AF_NS);
}

void
nsap_iso_detach(struct nsap_iso *nsap)
{
	nsap_detach(nsap, AF_ISO);
	nsap_detach(nsap, AF_INET);
	nsap_detach(nsap, AF_INET6);
	nsap_detach(nsap, AF_NS);
}

/*
 * Only ISO/OSI based services have currently been defined.
 */
static void
nsap_service(struct sap_service *service, char *addr, u_char addrlen, int class)
{
	sap_service_init(service, addr, addrlen, class);
	sap_class(service, class);
}

static void
nsap_setup_snsap(struct sockaddr_nsap *snsap, void *arg, long type, long subnet)
{
	switch (type) {
	case NSAP_TYPE_SIN4:
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)arg;
		if (sin != NULL) {
			snsap->snsap_sin4 = *sin;
			sap_type(snsap, NSAP_TYPE_SIN4);
			switch (subnet) {
			case NSAP_SUBNET_IPV4:
				sap_subnet(snsap, NSAP_SUBNET_IPV4);
				break;
			case NSAP_SUBNET_IPV6:
				sap_subnet(snsap, NSAP_SUBNET_IPV4);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				sap_subnet(snsap, NSAP_SUBNET_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SIN6:
	{
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)arg;
		if (sin6 != NULL) {
			snsap->snsap_sin6 = *sin6;
			sap_type(snsap, NSAP_TYPE_SIN6);
			switch (subnet) {
			case NSAP_SUBNET_IPV4:
				/* FALLTHROUGH */
			case NSAP_SUBNET_IPV6:
				sap_subnet(snsap, NSAP_SUBNET_IPV6);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				sap_subnet(snsap, NSAP_SUBNET_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SNS:
	{
		struct sockaddr_ns *sns = (struct sockaddr_ns *)arg;
		if (sns != NULL) {
			snsap->snsap_sns = *sns;
			sap_type(snsap, NSAP_TYPE_SNS);
			sap_subnet(snsap, NSAP_SUBNET_IDP);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SISO:
	{
		struct sockaddr_iso *siso = (struct sockaddr_iso *)arg;
		if (siso != NULL) {
			snsap->snsap_siso = *siso;
			sap_type(snsap, NSAP_TYPE_SISO);
			switch (subnet) {
			case NSAP_SUBNET_CONS:
				sap_subnet(snsap, NSAP_SUBNET_CONS);
				break;
			case NSAP_SUBNET_CLNS:
				sap_subnet(snsap, NSAP_SUBNET_CLNS);
				break;
			case NSAP_SUBNET_CLNP:
				sap_subnet(snsap, NSAP_SUBNET_CLNP);
				break;
			case NSAP_SUBNET_ISIS:
				sap_subnet(snsap, NSAP_SUBNET_ISIS);
				break;
			case NSAP_SUBNET_ESIS:
				sap_subnet(snsap, NSAP_SUBNET_ESIS);
				break;
			case NSAP_SUBNET_UNKNOWN:
				/* FALLTHROUGH */
			default:
				sap_subnet(snsap, NSAP_SUBNET_UNKNOWN);
				break;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SX25:
	{
		struct sockaddr_x25 *sx25 = (struct sockaddr_x25 *)arg;
		if (sx25 != NULL) {
			snsap->snsap_sx25 = *sx25;
			sap_type(snsap, NSAP_TYPE_SX25);
			sap_subnet(snsap, NSAP_SUBNET_X25);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SATM:
	case NSAP_TYPE_SIPX:
	case NSAP_TYPE_SSNA:
	case NSAP_TYPE_UNKNOWN:
unknown:
		/* FALLTHROUGH */
	default:
		sap_type(snsap, NSAP_TYPE_UNKNOWN);
		sap_subnet(snsap, NSAP_SUBNET_UNKNOWN);
		break;
	}
}

static void
nsap_setup_nsapa(struct nsap_addr *nsapa, void *arg, long type, int class)
{
	switch (type) {
	case NSAP_TYPE_SIN4:
	{
		struct in_addr *in4 = (struct in_addr *)arg;
		if (in4 != NULL) {
			nsapa->nsapa_in4 = *in4;
			nsap_service(&nsapa->nsapa_service, (struct in_addr *)in4, sizeof(*in4), NSAP_CLASS_CLNS);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SIN6:
	{
		struct in6_addr *in6 = (struct in6_addr *)arg;
		if (in6 != NULL) {
			nsapa->nsapa_in6 = *in6;
			nsap_service(&nsapa->nsapa_service, (struct in6_addr *)in6, sizeof(*in6), NSAP_CLASS_CLNS);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SNS:
	{
		struct ns_addr *ns = (struct ns_addr *)arg;
		if (ns != NULL) {
			nsapa->nsapa_ns = *ns;
			nsap_service(&nsapa->nsapa_service, (struct ns_addr *)ns, sizeof(*ns), NSAP_CLASS_CLNS);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SISO:
	{
		struct iso_addr *iso = (struct iso_addr *)arg;
		if (iso != NULL) {
			nsapa->nsapa_iso = *iso;
			switch (class) {
			case NSAP_CLASS_CONS:
				nsap_service(&nsapa->nsapa_service, (struct iso_addr *)iso, sizeof(*iso), NSAP_CLASS_CONS);
				break;
			case NSAP_CLASS_CLNS:
				nsap_service(&nsapa->nsapa_service, (struct iso_addr *)iso, sizeof(*iso), NSAP_CLASS_CLNS);
				break;
			case NSAP_CLASS_UNKNOWN:
				goto unknown;
			}
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SX25:
	{
		struct x25_addr *x25 = (struct x25_addr *)arg;
		if (x25 != NULL) {
			nsapa->nsapa_x25 = *x25;
			nsap_service(&nsapa->nsapa_service, (struct x25_addr *)x25, sizeof(*x25), NSAP_CLASS_CONS);
		} else {
			goto unknown;
		}
		break;
	}
	case NSAP_TYPE_SATM:
	case NSAP_TYPE_SIPX:
	case NSAP_TYPE_SSNA:
	case NSAP_TYPE_UNKNOWN:
unknown:
		/* FALLTHROUGH */
	default:
		nsap_service(&nsapa->nsapa_service, NULL, NULL, NSAP_CLASS_UNKNOWN);
		break;
	}
}

/*
 * nsap_canconnect:
 * - checks if nsap provider can connect to nsap user.
 */
int
nsap_canconnect(struct sockaddr_nsap *snsap, void *arg, long type, long subnet, int class)
{
	struct nsap_addr *nsapa;
	int error;

	if (snsap != NULL) {
		error = nsap_acknowledge_snsap(&nsapisotable, snsap, type, subnet);
		if (error != 0) {
			return (error);
		}
		nsap_setup_snsap(snsap, arg, type, subnet);
	} else {
		return (EINVAL);
	}
	nsapa = &snsap->snsap_addr;
	if (nsapa != NULL) {
		error = nsap_acknowledge_nsapa(&nsapisotable, nsapa, type, subnet, class);
		if (error != 0) {
			return (error);
		}
		nsap_setup_nsapa(nsapa, arg, type, class);
	} else {
		return (EINVAL);
	}
	return (0);
}

/*
 * nsap_candisconnect:
 * - checks if nsap provider can disconnect from nsap user.
 */
int
nsap_candisconnect(struct sockaddr_nsap *snsap, void *arg, long type, long subnet, int class)
{
	struct nsap_addr *nsapa;
	int error;

	if (snsap != NULL) {
		error = nsap_acknowledge_snsap(&nsapisotable, snsap, type, subnet);
		if (error != 0) {
			return (error);
		}
		nsap_setup_snsap(snsap, NULL, NSAP_TYPE_UNKNOWN, NSAP_SUBNET_UNKNOWN);
	} else {
		return (EINVAL);
	}
	nsapa = &snsap->snsap_addr;
	if (nsapa != NULL) {
		error = nsap_acknowledge_nsapa(&nsapisotable, nsapa, type, subnet, class);
		if (error != 0) {
			return (error);
		}
		nsap_setup_nsapa(nsapa, NULL, NSAP_TYPE_UNKNOWN, NSAP_CLASS_UNKNOWN);
	} else {
		return (EINVAL);
	}
	return (0);
}

int
nsap_acknowledge_snsap(struct sockaddr_nsap *snsap, long type, long subnet)
{
	struct nsap_iso *nsap;
	int error;

	nsap = nsap_lookup(&nsapisotable, type, subnet);
	if (nsap != NULL) {
		/* sockaddr_nsap */
		if (nsap->nsi_snsap != NULL) {
			error = sockaddr_nsap_compare(nsap->nsi_snsap, snsap);
			if (error != 0) {
				/* validate type_id and subnet_id */
				error = nsap_id_check(nsap, snsap->snsap_type, snsap->snsap_subnet);
				if (error != 0) {
					return (error);
				}
			}
		}
	}
	return (0);
}

int
nsap_acknowledge_nsapa(struct nsap_addr *nsapa, long type, long subnet, int class)
{
	struct nsap_iso *nsap;
	int error;

	nsap = nsap_lookup(type, subnet);
	if (nsap != NULL) {
		/* nsap_addr */
		if (nsap->nsi_nsapa != NULL) {
			error = nsap_addr_compare(nsap->nsi_nsapa, nsapa);
			if (error != 0) {
				error = sap_service_check_class(type, subnet, class);
				if (error != 0) {
					return (error);
				}
			}
		}
	}
	return (0);
}

struct nsap_iso *
nsap_lookup(long type, long subnet)
{
	return (nsap_hashlookup(&nsapisotable, type, subnet));
}

static struct nsap_iso *
nsap_hashlookup(struct nsapisotable *table, long type, long subnet)
{
	struct nsapisohead *head;
	struct nsap_iso *nsap;

	head = NSAPHASH(table, type, subnet);
	LIST_FOREACH(nsap, head, nsi_hash) {
		if (nsap_id_check(nsap, type, subnet)) {
			return (nsap);
		}
	}
	return (NULL);
}

static void
nsap_insert(struct nsapisotable *table, struct nsap_iso *nsap, long type, long subnet)
{
	struct nsapisohead *head;

	head = NSAPHASH(table, type, subnet);
	if ((head == NULL) || (nsap == NULL)) {
		return;
	}

	nsap->nsi_type_id = nsap_type_id(type);
	nsap->nsi_subnet_id = nsap_subnet_id(subnet);
	nsap->nsi_id = nsap_valid_ids[type][subnet];
	LIST_INSERT_HEAD(head, nsap, nsi_hash);
}

static void
nsap_remove(struct nsapisotable *table, struct nsap_iso *nsap, long type, long subnet)
{
	nsap = nsap_hashlookup(table, type, subnet);
	if (nsap != NULL) {
		LIST_REMOVE(nsap, nsi_hash);
	}
}

/* Inserts all valid network layer protocols for that address family */
static void
nsap_insert_af(struct nsapisotable *table, struct nsap_iso *nsap, int af)
{
	switch (af) {
	case AF_INET:
		nsap_insert(table, nsap, NSAP_TYPE_SIN4,
				NSAP_SUBNET_IPV4);
		nsap_insert(table, nsap, NSAP_TYPE_SIN4,
				NSAP_SUBNET_IPV6);
		break;
	case AF_INET6:
		nsap_insert(table, nsap, NSAP_TYPE_SIN6,
				NSAP_SUBNET_IPV4);
		nsap_insert(table, nsap, NSAP_TYPE_SIN6,
				NSAP_SUBNET_IPV6);
		break;
	case AF_NS:
		nsap_insert(table, nsap, NSAP_TYPE_SNS,
				NSAP_SUBNET_IDP);
		break;
	case AF_ISO:
		nsap_insert(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_CONS);
		nsap_insert(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_CLNS);
		nsap_insert(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_CLNP);
		nsap_insert(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_ISIS);
		nsap_insert(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_ESIS);
		break;
	case AF_CCITT:
		nsap_insert(table, nsap, NSAP_TYPE_SX25,
				NSAP_SUBNET_X25);
		break;
	case AF_NATM:
		nsap_insert(table, nsap, NSAP_TYPE_SATM,
				NSAP_SUBNET_ATM);
		break;
	case AF_IPX:
		nsap_insert(table, nsap, NSAP_TYPE_SIPX,
				NSAP_SUBNET_IPX);
		break;
	case AF_SNA:
		nsap_insert(table, nsap, NSAP_TYPE_SSNA,
				NSAP_SUBNET_SNA);
		break;
	}
}

/* Removes all valid network layer protocols for that address family */
static void
nsap_remove_af(struct nsapisotable *table, struct nsap_iso *nsap, int af)
{
	switch (af) {
	case AF_INET:
		nsap_remove(table, nsap, NSAP_TYPE_SIN4,
				NSAP_SUBNET_IPV4);
		nsap_remove(table, nsap, NSAP_TYPE_SIN4,
				NSAP_SUBNET_IPV6);
		break;
	case AF_INET6:
		nsap_remove(table, nsap, NSAP_TYPE_SIN6,
				NSAP_SUBNET_IPV4);
		nsap_remove(table, nsap, NSAP_TYPE_SIN6,
				NSAP_SUBNET_IPV6);
		break;
	case AF_NS:
		nsap_remove(table, nsap, NSAP_TYPE_SNS,
				NSAP_SUBNET_IDP);
		break;
	case AF_ISO:
		nsap_remove(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_CONS);
		nsap_remove(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_CLNS);
		nsap_remove(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_CLNP);
		nsap_remove(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_ISIS);
		nsap_remove(table, nsap, NSAP_TYPE_SISO,
				NSAP_SUBNET_ESIS);
		break;
	case AF_CCITT:
		nsap_remove(table, nsap, NSAP_TYPE_SX25,
				NSAP_SUBNET_X25);
		break;
	case AF_NATM:
		nsap_remove(table, nsap, NSAP_TYPE_SATM,
				NSAP_SUBNET_ATM);
		break;
	case AF_IPX:
		nsap_remove(table, nsap, NSAP_TYPE_SIPX,
				NSAP_SUBNET_IPX);
		break;
	case AF_SNA:
		nsap_remove(table, nsap, NSAP_TYPE_SSNA,
				NSAP_SUBNET_SNA);
		break;
	}
}

static void
nsap_setup_id_table(void)
{
    long type, subnet, cnt;
    uint32_t nsap_ids[NSAP_TYPE_MAX][NSAP_SUBNET_MAX];

    type = subnet = cnt = 0;

    /* zeroed out nsap_id table */
    for (type = 1; type < NSAP_TYPE_MAX; type++) {
        for (subnet = 1; subnet < NSAP_SUBNET_MAX; subnet++) {
        	nsap_ids[type][subnet] = 0;
        }
    }

    /* add nsap_id's to table */
    for (type = 1; type < NSAP_TYPE_MAX; type++) {
        for (subnet = 1; subnet < NSAP_SUBNET_MAX; subnet++) {
            nsap_ids[NSAP_TYPE_SIN4][NSAP_SUBNET_IPV4] = nsap_id(NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4);
            nsap_ids[NSAP_TYPE_SIN4][NSAP_SUBNET_IPV6] = nsap_id(NSAP_TYPE_SIN4, NSAP_SUBNET_IPV6); /* nsap id should match sin6 with ipv4 */
            nsap_ids[NSAP_TYPE_SIN6][NSAP_SUBNET_IPV4] = nsap_id(NSAP_TYPE_SIN6, NSAP_SUBNET_IPV4); /* nsap id should match sin4 with ipv6 */
            nsap_ids[NSAP_TYPE_SIN6][NSAP_SUBNET_IPV6] = nsap_id(NSAP_TYPE_SIN6, NSAP_SUBNET_IPV6);
            nsap_ids[NSAP_TYPE_SNS][NSAP_SUBNET_IDP] = nsap_id(NSAP_TYPE_SNS, NSAP_SUBNET_IDP);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CONS] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CONS);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CLNS] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CLNS);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_CLNP] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_CLNP);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_ISIS] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_ISIS);
            nsap_ids[NSAP_TYPE_SISO][NSAP_SUBNET_ESIS] = nsap_id(NSAP_TYPE_SISO, NSAP_SUBNET_ESIS);
            nsap_ids[NSAP_TYPE_SX25][NSAP_SUBNET_X25] = nsap_id(NSAP_TYPE_SX25, NSAP_SUBNET_X25);
            nsap_ids[NSAP_TYPE_SATM][NSAP_SUBNET_ATM] = nsap_id(NSAP_TYPE_SATM, NSAP_SUBNET_ATM);
            nsap_ids[NSAP_TYPE_SIPX][NSAP_SUBNET_IPX] = nsap_id(NSAP_TYPE_SIPX, NSAP_SUBNET_IPX);
            nsap_ids[NSAP_TYPE_SSNA][NSAP_SUBNET_SNA] = nsap_id(NSAP_TYPE_SSNA, NSAP_SUBNET_SNA);
        }
    }

    /* remove invalid nsap_ids from table (i.e. any with 0) */
    for (type = 1; type < NSAP_TYPE_MAX; type++) {
        for (subnet = 1; subnet < NSAP_SUBNET_MAX; subnet++) {
            if (nsap_ids[type][subnet] != 0) {
            	nsap_valid_ids[type][subnet] = nsap_ids[type][subnet];
                cnt++;
            }
        }
    }
    nsap_id_cnt = cnt;
}

#ifdef nsap_radix

/* NSAP using Radix lookups */
/* sap select id */
enum sap_sids {
	SAP_SID_UNKNOWN,
	SAP_SID_INET4,
	SAP_SID_INET6,
	SAP_SID_NS,
	SAP_SID_ISO,
	SAP_SID_X25,
	SAP_SID_ATM,
	SAP_SID_IPX,
	SAP_SID_SNA,
	SAP_SID_MAX
};

struct nsap_iso1 {
	struct radix_node nsi_nodes[SAP_TABLE_MAX];
	LIST_ENTRY(nsap_iso) nsi_hash;

	struct sockaddr_nsap *nsi_smask[SAP_TABLE_MAX]; /* masked sockaddr */
	struct sockaddr_nsap *nsi_snsap;
	uint32_t nsi_id;			/* nsap id */
	uint32_t nsi_type_id;		/* type id (not nsap_types) */
	uint32_t nsi_subnet_id;		/* subnet id (not nsap_subnets) */
};

struct nsapisotree {
	struct radix_node_head *nsi_tree[SAP_TABLE_MAX];
	struct nsapisohead *nsi_hashtbl;
	u_long nsi_hash;
};

/* service access point */
struct sap_addr {
	char saa_addr[ISOLEN];		/* address */
	unsigned char saa_addrlen; 	/* address length */
};

struct sockaddr_sap {
	long saap_type;				/* stack type (address family) */
	long saap_subnet;			/* subnet type (network layer protocols) */
	long saap_subtran;			/* subtran type (transport layer protocols) */
	int saap_class;				/* class type (connection or connection-less) */
	struct sap_addr saap_addr;
};

struct sap_head;
LIST_HEAD(sap_head, sap_node);
struct sap_tree {
	struct radix_node_head *st_tree[9];
	struct sap_head *st_hashtbl;
	u_long st_hash;
};

struct sap_node {
	LIST_ENTRY(sap_node) st_hash;
	struct radix_node st_node[9];
	struct sockaddr_sap *st_smask[9];
	struct sockaddr_sap *st_saap;
};

/* sockaddr_nsap radix masks */
#define sap_unknown(nsap) 	((nsap)->nsi_smask[SAP_SID_UNKNOWN])
#define sap_inet4(nsap) 	((nsap)->nsi_smask[SAP_SID_INET4])
#define sap_inet6(nsap)		((nsap)->nsi_smask[SAP_SID_INET6])
#define sap_ns(nsap)		((nsap)->nsi_smask[SAP_SID_NS])
#define sap_iso(nsap)		((nsap)->nsi_smask[SAP_SID_ISO])
#define sap_x25(nsap)		((nsap)->nsi_smask[SAP_SID_X25])
#define sap_atm(nsap)		((nsap)->nsi_smask[SAP_SID_ATM])
#define sap_ipx(nsap)		((nsap)->nsi_smask[SAP_SID_IPX])
#define sap_sna(nsap)		((nsap)->nsi_smask[SAP_SID_SNA])

struct radix_node_head *
nsap_radix_node_head(struct nsapisotree *tree, int sid)
{
	return (tree->nsi_tree[sid]);
}

struct radix_node *
nsap_radix_node_add(struct nsapisotree *tree, struct nsap_iso1 *nsap, struct sockaddr_nsap *snsap, int sid)
{
	struct radix_node_head *rnh;
	struct sockaddr_nsap *smask;
	struct radix_node *rn;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}
	smask = nsap->nsi_smask[sid];
	if (smask == NULL) {
		return (NULL);
	}
	rn = rnh->rnh_addaddr(snsap, smask, rnh, nsap->nsi_nodes);
	return (rn);
}

struct radix_node *
nsap_radix_node_delete(struct nsapisotree *tree, struct nsap_iso1 *nsap, struct sockaddr_nsap *snsap, int sid)
{
	struct radix_node_head *rnh;
	struct sockaddr_nsap *smask;
	struct radix_node *rn;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}
	smask = nsap->nsi_smask[sid];
	if (smask == NULL) {
		return (NULL);
	}
	rn = rnh->rnh_deladdr(snsap, smask, rnh);
	return (rn);
}

struct radix_node *
nsap_radix_node_matchaddr(struct nsapisotree *tree, struct sockaddr_nsap *snsap, int sid)
{
	struct radix_node_head *rnh;
	struct radix_node *rn;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}
	rn = rnh->rnh_matchaddr(snsap, rnh);
	return (rn);
}

struct radix_node *
nsap_radix_node_lookup(struct nsapisotree *tree, struct nsap_iso1 *nsap, struct sockaddr_nsap *snsap, int sid)
{
	struct radix_node_head *rnh;
	struct sockaddr_nsap *smask;
	struct radix_node *rn;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}
	smask = nsap->nsi_smask[sid];
	if (smask == NULL) {
		return (NULL);
	}
	rn = rnh->rnh_lookup(snsap, smask, rnh);
	return (rn);
}

struct nsap_iso1 *
nsap_lookup(struct nsapisotree *tree, struct sockaddr_nsap *snsap, long type, long subnet, int sid)
{
	struct radix_node_head *rnh;
	struct radix_node *rn;
	struct nsap_iso1 *nsap, *match;
	int error;

	rnh = nsap_radix_node_head(tree, sid);
	if (rnh == NULL) {
		return (NULL);
	}

	rn = nsap_radix_node_matchaddr(snsap, sid);
	if (rn && (rn->rn_flags & RNF_ROOT) == 0) {
		match = (struct nsap_iso1 *)rn;
		error = nsap_id_check(match, type, subnet);
	}

	nsap = nsap_hashlookup(tree, type, subnet);
	if (nsap != NULL) {
		if (error != 0) {
			match = nsap;
		}
	}
	return (match);
}

void
nsap_insert1(struct nsapisotree *tree, struct nsap_iso1 *nsap, struct sockaddr_nsap *snsap, long type, long subnet, int sid)
{
	struct nsapisohead *head;
	struct radix_node *rn;

	head = NSAPHASH(tree, type, subnet);
	if (head == NULL || nsap == NULL) {
		return;
	}
	rn = nsap_radix_node_add(tree, nsap, snsap, sid);
	if (rn == NULL) {
		return;
	}

	nsap->nsi_snsap = snsap;
	nsap->nsi_type_id = nsap_type_id(type);
	nsap->nsi_subnet_id = nsap_subnet_id(subnet);
	nsap->nsi_id = nsap_valid_ids[type][subnet];
	LIST_INSERT_HEAD(head, nsap, nsi_hash);
}

void
nsap_insert1_af(struct nsapisotree *tree, struct nsap_iso1 *nsap, int sid, int af)
{
	switch (af) {
	case AF_INET:
		nsap_insert1(tree, nsap, sap_inet4(nsap), NSAP_TYPE_SIN4, NSAP_SUBNET_IPV4, SAP_SID_INET4);
		nsap_insert1(tree, nsap, sap_inet4(nsap), NSAP_TYPE_SIN4, NSAP_SUBNET_IPV6, SAP_SID_INET4);
		break;
	}
}
#endif
